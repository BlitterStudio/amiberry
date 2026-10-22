//
// IPC control transport for Amiberry
// Unix domain socket (Linux, macOS, FreeBSD) or named pipe (Windows)
//

#ifdef USE_IPC_SOCKET

#include "amiberry_ipc.h"
#include "settings/amiberry_actions.h"
#include "settings/amiberry_media.h"
#include "settings/amiberry_settings.h"
#include "sysdeps.h"
#include "amiberry_gfx.h"
#include "options.h"
#include "target.h"
#include "uae.h"
#include "savestate.h"
#include "inputdevice.h"
#include "memory.h"
#include "disk.h"
#include "xwin.h"
#include "sounddep/sound.h"
#include "filesys.h"
#include "gui.h"
#include "custom.h"
#include "amiga_constants.h"
#include "gui/gui_handling.h"
#include "newcpu.h"
#include "blitter.h"
#ifdef DEBUGGER
#include "debug.h"
#include "gdb_server.h"
#endif
#include <dirent.h>

#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <cctype>
#include <cstring>
#include <map>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <chrono>
#include <mutex>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>

#ifdef _WIN32
#include <sddl.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <poll.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

using namespace Amiberry::IPC;

// Listener state. socket_path is the Unix socket path, or the pipe name on
// Windows.
#ifndef _WIN32
static int server_socket = -1;
#endif
static std::string socket_path;
static int socket_instance = 0;
static bool ipc_active = false;
static bool ipc_quit_requested = false;
static std::mutex ipc_handle_mutex;
static AmiberryGuiInputConfigState gui_input_config_state;

// Command handler type
typedef std::function<std::string(const std::vector<std::string>&)> CommandHandler;
static std::map<std::string, CommandHandler> command_handlers;

// Helper: Split string by delimiter
static std::vector<std::string> split(const std::string& str, char delim)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, delim)) {
		tokens.push_back(token);
	}
	return tokens;
}

// Helper: Join strings
static std::string join(const std::vector<std::string>& parts, char delim)
{
	std::string result;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) result += delim;
		result += parts[i];
	}
	return result;
}

// Helper: Make response
static std::string make_response(bool success, const std::vector<std::string>& data = {})
{
	std::string response = success ? RESPONSE_OK : RESPONSE_ERROR;
	for (const auto& d : data) {
		response += '\t';
		response += d;
	}
	response += '\n';
	return response;
}

static const char* bool_field(const bool value)
{
	return value ? "true" : "false";
}

static const char* tablet_mode_name(const int value)
{
	switch (value) {
	case TABLET_OFF: return "off";
	case TABLET_MOUSEHACK: return "mousehack";
	case TABLET_REAL: return "real";
	default: return "unknown";
	}
}

static bool parse_tablet_mode(const std::string& value, int& result)
{
	if (value == "off")
		result = TABLET_OFF;
	else if (value == "mousehack")
		result = TABLET_MOUSEHACK;
	else if (value == "real")
		result = TABLET_REAL;
	else
		return false;
	return true;
}

static const char* mouse_untrap_name(const int value)
{
	switch (value) {
	case MOUSEUNTRAP_NONE: return "off";
	case MOUSEUNTRAP_MIDDLEBUTTON: return "middle";
	case MOUSEUNTRAP_MAGIC: return "magic";
	case MOUSEUNTRAP_BOTH: return "both";
	default: return "unknown";
	}
}

static bool parse_mouse_untrap(const std::string& value, int& result)
{
	if (value == "off")
		result = MOUSEUNTRAP_NONE;
	else if (value == "middle")
		result = MOUSEUNTRAP_MIDDLEBUTTON;
	else if (value == "magic")
		result = MOUSEUNTRAP_MAGIC;
	else if (value == "both")
		result = MOUSEUNTRAP_BOTH;
	else
		return false;
	return true;
}

static AmiberryGuiInputConfigSnapshot gui_input_config_snapshot()
{
	return gui_input_config_state.observe(
		changed_prefs.input_tablet, changed_prefs.input_mouse_untrap,
		currprefs.input_tablet, currprefs.input_mouse_untrap);
}

static bool gui_input_focus_ready()
{
	const bool uncaptured_ready = currprefs.input_tablet >= TABLET_MOUSEHACK
		&& (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)
		&& mousehack_alive();
	return isfocus() > 0 && (ismouseactive() || uncaptured_ready);
}

static std::vector<std::string> gui_input_config_fields(
	const AmiberryGuiInputConfigSnapshot& input)
{
	return {
		"pending_tablet_mode=" + std::string(tablet_mode_name(
			input.pending_tablet_mode)),
		"effective_tablet_mode=" + std::string(tablet_mode_name(
			input.effective_tablet_mode)),
		"pending_mouse_untrap=" + std::string(mouse_untrap_name(
			input.pending_mouse_untrap)),
		"effective_mouse_untrap=" + std::string(mouse_untrap_name(
			input.effective_mouse_untrap)),
		"pending_effective_diverged=" + std::string(bool_field(
			input.pending_tablet_mode != input.effective_tablet_mode
			|| input.pending_mouse_untrap != input.effective_mouse_untrap)),
		"input_config_revision="
			+ std::to_string(input.input_config_revision)
	};
}

static std::string guarded_input_response(
	const bool applied, const char* reason,
	const AmiberryGuiGeometrySnapshot& geometry, const int active_monitor,
	const AmiberryGuiInputConfigSnapshot& input, const int button_mask,
	const AmiberryGuiGuardedInputRequest* request = nullptr)
{
	const AmiberryGuiGuardedInputResponse response{
		applied,
		reason,
		geometry.runtime_id,
		geometry.geometry_revision,
		amiberry_gui_guarded_response_monitor_id(
			active_monitor, geometry.monitor_id),
		input.input_config_revision,
		button_mask & AMIBERRY_GUI_SUPPORTED_BUTTON_MASK,
		applied && request != nullptr,
		request != nullptr ? request->x : 0,
		request != nullptr ? request->y : 0
	};
	return make_response(
		applied, amiberry_gui_guarded_response_fields(response));
}

// Command handlers. Runtime actions go through the shared actions API: the
// handler posts the action and runs it at once (IPC handlers already run on
// the emulator's main thread), so the reply reports what the action did.

static bool run_posted(amiberry_action_id id, std::string* error)
{
	char message[512];
	message[0] = 0;
	amiberry_actions_run_pending();
	const amiberry_action_result result =
		amiberry_actions_poll(id, message, sizeof message);
	if (error)
		*error = message;
	return result == AMIBERRY_ACTION_DONE;
}

static std::string run_posted_reply(amiberry_action_id id)
{
	std::string error;
	if (id != 0 && run_posted(id, &error))
		return make_response(true);
	return make_response(false, {error.empty() ? "Action was not executed" : error});
}

static std::string HandleQuit(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received QUIT" << std::endl;
	}
	// Posted now, executed by IPCHandle() after this pass has written the
	// reply, exactly like the old deferred uae_quit(). A full action queue
	// rejects the post: this handler runs on the main thread, so drain the
	// queue and try once more, and never acknowledge a quit that will not
	// happen.
	if (!amiberry_actions_quit()) {
		amiberry_actions_run_pending();
		if (!amiberry_actions_quit())
			return make_response(false, {"Quit could not be queued"});
	}
	ipc_quit_requested = true;
	return make_response(true);
}

static std::string HandlePause(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received PAUSE" << std::endl;
	}
	return run_posted_reply(amiberry_actions_pause());
}

static std::string HandleResume(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received RESUME" << std::endl;
	}
	return run_posted_reply(amiberry_actions_resume());
}

static std::string HandleReset(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received RESET" << std::endl;
	}
	bool hard = false;
	bool keyboard = true;

	if (!args.empty()) {
		if (args[0] == "HARD") {
			hard = true;
			keyboard = false;
		} else if (args[0] == "SOFT" || args[0] == "KEYBOARD") {
			hard = false;
			keyboard = true;
		} else {
			return make_response(false, {"Invalid reset type"});
		}
	}

	return run_posted_reply(amiberry_actions_reset(hard, keyboard));
}

static std::string HandleScreenshot(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SCREENSHOT" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Missing filename"});
	}
	if (args[0].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}
	if (args.size() > 1) {
		if (args.size() != 2 || args[1] != "ACTIONABLE") {
			return make_response(false, {"Usage: SCREENSHOT <path> [ACTIONABLE]"});
		}
		const int monid = amiberry_get_active_input_monitor();
		if (monid < 0) {
			return make_response(false, {"No active input monitor"});
		}
		if (!amiberry_actionable_screenshot_supported(monid)) {
			return make_response(false, {
				"reason=unsupported_renderer",
				"Actionable screenshots are not supported by the active renderer"
			});
		}
		AmiberryGuiGeometrySnapshot snapshot;
		if (!amiberry_capture_actionable_screenshot(monid, args[0], snapshot)) {
			return make_response(false,
				{"Failed to capture coherent actionable screenshot"});
		}
		return make_response(true,
			amiberry_gui_actionable_fields(args[0], snapshot));
	}

	std::string error;
	const amiberry_action_id id = amiberry_actions_screenshot(args[0].c_str());
	if (id == 0 || !run_posted(id, &error)) {
		return make_response(false, {error.empty() ? "Action was not executed" : error});
	}

	return make_response(true, {args[0]});
}

static std::string HandleSavestate(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SAVESTATE" << std::endl;
	}
#ifdef DEBUGGER
	if (debugger_is_stopped() || gdb_server_halted()) {
		return make_response(false, {"Cannot save state while the debugger has the CPU stopped; resume or PAUSE first"});
	}
#endif
	if (args.size() < 2) {
		return make_response(false, {"Usage: SAVESTATE <statefile> <configfile>"});
	}
	if (args[0].size() >= MAX_DPATH || args[1].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}

	std::string error;
	const amiberry_action_id id = amiberry_actions_save_state(args[0].c_str());
	if (id == 0 || !run_posted(id, &error)) {
		return make_response(false, {error.empty() ? "Action was not executed" : error});
	}

	strncpy(changed_prefs.description, "autosave", sizeof(changed_prefs.description));
	cfgfile_save(&changed_prefs, args[1].c_str(), 0);

	return make_response(true);
}

static std::string HandleLoadState(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received LOADSTATE" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Missing state file path"});
	}
	if (args[0].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}

	return run_posted_reply(amiberry_actions_load_state(args[0].c_str()));
}

static std::string HandleDiskSwap(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DISKSWAP" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: DISKSWAP <disknum> <drivenum>"});
	}

	int disknum, drivenum;
	try {
		disknum = std::stoi(args[0]);
		drivenum = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid disk or drive number"});
	}

	if (disknum < 0 || disknum >= MAX_SPARE_DRIVES || drivenum < 0 || drivenum > 3) {
		return make_response(false, {"Invalid disk or drive number"});
	}

	return run_posted_reply(amiberry_actions_insert_swapper_entry(disknum, drivenum));
}

static std::string HandleQueryDiskSwap(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received QUERYDISKSWAP" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: QUERYDISKSWAP <drivenum>"});
	}

	int drivenum;
	try {
		drivenum = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid drive number: " + args[0]});
	}

	if (drivenum < 0 || drivenum > 3) {
		return make_response(false, {"Drive number must be 0-3"});
	}

	int disknum = -1;
	for (int i = 0; i < MAX_SPARE_DRIVES; i++) {
		if (!_tcscmp(currprefs.floppyslots[drivenum].df, currprefs.dfxlist[i])) {
			disknum = i;
			break;
		}
	}

	return make_response(true, {std::to_string(disknum)});
}

static std::string HandleInsertFloppy(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received INSERTFLOPPY" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: INSERTFLOPPY <path> <drivenum>"});
	}
	if (args[0].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}

	int drivenum;
	try {
		drivenum = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid drive number: " + args[1]});
	}

	if (drivenum < 0 || drivenum > 3) {
		return make_response(false, {"Drive number must be 0-3"});
	}

	return run_posted_reply(amiberry_actions_insert_floppy(drivenum, args[0].c_str()));
}

static std::string HandleInsertCD(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received INSERTCD" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Missing CD image path"});
	}
	if (args[0].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}

	return run_posted_reply(amiberry_actions_insert_cd(args[0].c_str()));
}

static std::string HandleGetStatus(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_STATUS" << std::endl;
	}
	// Handlers run on the emulator's main thread; refresh first so the
	// snapshot reflects earlier commands in this same pass.
	amiberry_actions_run_pending();
	amiberry_actions_status status;
	amiberry_actions_get_status(&status);
	std::vector<std::string> responses;

	responses.push_back(std::string("Paused=") + (status.paused ? "true" : "false"));
	responses.push_back(std::string("Config=") + status.config_description);

	for (int i = 0; i < AMIBERRY_ACTIONS_FLOPPY_DRIVES; ++i) {
		if (status.floppy[i].media_path[0])
			responses.push_back(std::string("Floppy") + std::to_string(i) + "=" + status.floppy[i].media_path);
	}

	return make_response(true, responses);
}

static std::string HandleGetConfig(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_CONFIG" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Missing option name"});
	}

	const std::string& optname = args[0];
	std::string value;

	// Memory options
	if (optname == "chipmem_size") {
		value = std::to_string(changed_prefs.chipmem.size);
	} else if (optname == "fastmem_size") {
		value = std::to_string(changed_prefs.fastmem[0].size);
	} else if (optname == "bogomem_size") {
		value = std::to_string(changed_prefs.bogomem.size);
	} else if (optname == "z3fastmem_size") {
		value = std::to_string(changed_prefs.z3fastmem[0].size);
	}
	// CPU options
	else if (optname == "cpu_model") {
		value = std::to_string(changed_prefs.cpu_model);
	} else if (optname == "cpu_speed") {
		value = std::to_string(changed_prefs.m68k_speed);
	} else if (optname == "cpu_compatible") {
		value = changed_prefs.cpu_compatible ? "true" : "false";
	} else if (optname == "cpu_24bit_addressing") {
		value = changed_prefs.address_space_24 ? "true" : "false";
	}
	// Chipset options
	else if (optname == "chipset") {
		value = std::to_string(changed_prefs.chipset_mask);
	} else if (optname == "ntsc") {
		value = changed_prefs.ntscmode ? "true" : "false";
	}
	// Floppy options
	else if (optname == "floppy_speed") {
		value = std::to_string(changed_prefs.floppy_speed);
	} else if (optname == "nr_floppies") {
		value = std::to_string(changed_prefs.nr_floppies);
	}
	// Display options
	else if (optname == "gfx_width") {
		value = std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.width);
	} else if (optname == "gfx_height") {
		value = std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.height);
	} else if (optname == "gfx_fullscreen") {
		value = changed_prefs.gfx_apmode[0].gfx_fullscreen ? "true" : "false";
	} else if (optname == "gfx_auto_crop") {
		value = changed_prefs.gfx_auto_crop ? "true" : "false";
	} else if (optname == "gfx_manual_crop") {
		value = changed_prefs.gfx_manual_crop ? "true" : "false";
	} else if (optname == "gfx_manual_crop_width") {
		value = std::to_string(changed_prefs.gfx_manual_crop_width);
	} else if (optname == "gfx_manual_crop_height") {
		value = std::to_string(changed_prefs.gfx_manual_crop_height);
	} else if (optname == "gfx_horizontal_offset") {
		value = std::to_string(changed_prefs.gfx_horizontal_offset);
	} else if (optname == "gfx_vertical_offset") {
		value = std::to_string(changed_prefs.gfx_vertical_offset);
	} else if (optname == "gfx_overscanmode") {
		value = std::to_string(changed_prefs.gfx_overscanmode);
	}
	// Sound options
	else if (optname == "sound_output") {
		value = std::to_string(changed_prefs.produce_sound);
	} else if (optname == "sound_stereo") {
		value = std::to_string(changed_prefs.sound_stereo);
	}
	// Joystick options
	else if (optname == "joyport0") {
		value = std::to_string(changed_prefs.jports[0].id);
	} else if (optname == "joyport1") {
		value = std::to_string(changed_prefs.jports[1].id);
	}
	// Description
	else if (optname == "description") {
		value = changed_prefs.description;
	}
	// Input options
	else if (optname == "tablet_mode") {
		switch (changed_prefs.input_tablet) {
		case TABLET_OFF:
			value = "off";
			break;
		case TABLET_MOUSEHACK:
			value = "mousehack";
			break;
		case TABLET_REAL:
			value = "real";
			break;
		default:
			return make_response(false, {"Invalid tablet_mode state"});
		}
	}
	else if (optname == "mouse_untrap") {
		switch (changed_prefs.input_mouse_untrap) {
		case MOUSEUNTRAP_NONE:
			value = "off";
			break;
		case MOUSEUNTRAP_MIDDLEBUTTON:
			value = "middle";
			break;
		case MOUSEUNTRAP_MAGIC:
			value = "magic";
			break;
		case MOUSEUNTRAP_BOTH:
			value = "both";
			break;
		default:
			return make_response(false, {"Invalid mouse_untrap state"});
		}
	}
	else {
		return make_response(false, {"Unknown option: " + optname});
	}

	return make_response(true, {value});
}

static std::string HandleSetConfig(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_CONFIG" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: SET_CONFIG <option> <value>"});
	}

	const std::string& optname = args[0];
	const std::string& optval = args[1];

	try {
		// Floppy options
		if (optname == "floppy_speed") {
			changed_prefs.floppy_speed = std::stol(optval);
			set_config_changed();
		}
		// CPU options
		else if (optname == "cpu_speed") {
			changed_prefs.m68k_speed = std::stol(optval);
			set_config_changed();
		} else if (optname == "turbo_emulation") {
			changed_prefs.turbo_emulation = (optval == "true" || optval == "1");
			set_config_changed();
		}
		// Display options
		else if (optname == "gfx_fullscreen") {
			bool fullscreen = (optval == "true" || optval == "1");
			changed_prefs.gfx_apmode[0].gfx_fullscreen = fullscreen ? GFX_FULLWINDOW : GFX_WINDOW;
			set_config_changed();
		}
		// Crop options — enabling one mode disables the other
		else if (optname == "gfx_auto_crop") {
			const bool enable = (optval == "true" || optval == "1");
			changed_prefs.gfx_auto_crop = enable;
			if (enable) {
				changed_prefs.gfx_manual_crop = false;
			}
			set_config_changed();
		}
		else if (optname == "gfx_manual_crop") {
			const bool enable = (optval == "true" || optval == "1");
			changed_prefs.gfx_manual_crop = enable;
			if (enable) {
				changed_prefs.gfx_auto_crop = false;
			}
			set_config_changed();
		}
		else if (optname == "gfx_manual_crop_width") {
			// Clamp on the parsed long before narrowing and to the same
			// native bounds the Filter panel uses, so a headless client
			// cannot set a crop larger than the surface.
			const long max_w = AMIGA_WIDTH_MAX << changed_prefs.gfx_resolution;
			changed_prefs.gfx_manual_crop_width = static_cast<int>(
				std::clamp(std::stol(optval), 0L, max_w));
			set_config_changed();
		}
		else if (optname == "gfx_manual_crop_height") {
			const long max_h = AMIGA_HEIGHT_MAX << changed_prefs.gfx_vresolution;
			changed_prefs.gfx_manual_crop_height = static_cast<int>(
				std::clamp(std::stol(optval), 0L, max_h));
			set_config_changed();
		}
		else if (optname == "gfx_horizontal_offset") {
			const long max_w = AMIGA_WIDTH_MAX << changed_prefs.gfx_resolution;
			changed_prefs.gfx_horizontal_offset = static_cast<int>(
				std::clamp(std::stol(optval), -max_w, max_w));
			set_config_changed();
		}
		else if (optname == "gfx_vertical_offset") {
			const long max_h = AMIGA_HEIGHT_MAX << changed_prefs.gfx_vresolution;
			changed_prefs.gfx_vertical_offset = static_cast<int>(
				std::clamp(std::stol(optval), -max_h, max_h));
			set_config_changed();
		}
		else if (optname == "gfx_overscanmode") {
			// Validate on the parsed long before narrowing; on LP64 an
			// out-of-range value would otherwise wrap into a valid mode.
			const long mode = std::stol(optval);
			// Keep in sync with the overscan_items table in the Display panel.
			if (mode < 0 || mode > 8) {
				return make_response(false, {"Invalid gfx_overscanmode (use 0-8)"});
			}
			changed_prefs.gfx_overscanmode = static_cast<int>(mode);
			set_config_changed();
		} else if (optname == "sound_output") {
			changed_prefs.produce_sound = std::stol(optval);
			set_config_changed();
		} else if (optname == "sound_stereo") {
			changed_prefs.sound_stereo = std::stol(optval);
			set_config_changed();
		} else if (optname == "sound_volume") {
			changed_prefs.sound_volume_master = std::stol(optval);
			set_config_changed();
		}
		// Chipset options
		else if (optname == "ntsc") {
			changed_prefs.ntscmode = (optval == "true" || optval == "1");
			set_config_changed();
		}
		// Input options
		else if (optname == "tablet_mode") {
			int tablet_mode;
			if (optval == "off")
				tablet_mode = TABLET_OFF;
			else if (optval == "mousehack")
				tablet_mode = TABLET_MOUSEHACK;
			else if (optval == "real")
				tablet_mode = TABLET_REAL;
			else
				return make_response(false, {"Invalid tablet_mode value (use off, mousehack, or real)"});
			if (changed_prefs.input_tablet != tablet_mode) {
				changed_prefs.input_tablet = tablet_mode;
				set_config_changed();
			}
		}
		else if (optname == "mouse_untrap") {
			int mouse_untrap;
			if (optval == "off")
				mouse_untrap = MOUSEUNTRAP_NONE;
			else if (optval == "middle")
				mouse_untrap = MOUSEUNTRAP_MIDDLEBUTTON;
			else if (optval == "magic")
				mouse_untrap = MOUSEUNTRAP_MAGIC;
			else if (optval == "both")
				mouse_untrap = MOUSEUNTRAP_BOTH;
			else
				return make_response(false, {"Invalid mouse_untrap value (use off, middle, magic, or both)"});
			if (changed_prefs.input_mouse_untrap != mouse_untrap) {
				changed_prefs.input_mouse_untrap = mouse_untrap;
				set_config_changed();
			}
		}
		else {
			return make_response(false, {"Unknown option: " + optname});
		}
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid value for " + optname + ": " + optval});
	}

	return make_response(true);
}

static std::string HandleLoadConfig(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received LOAD_CONFIG" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Missing config file path"});
	}
	if (args[0].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}

	int result = target_cfgfile_load(&changed_prefs, args[0].c_str(), CONFIG_TYPE_DEFAULT, 0);
	if (result) {
		set_config_changed();
		return make_response(true);
	} else {
		return make_response(false, {"Failed to load config"});
	}
}

static std::string HandleSendKey(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SEND_KEY" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: SEND_KEY <keycode> <state>"});
	}

	int keycode, state;
	try {
		keycode = std::stoi(args[0]);
		state = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid keycode or state"});
	}

	inputdevice_do_keyboard(keycode, state);

	return make_response(true);
}

static std::string HandleReadMem(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received READ_MEM " << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: READ_MEM <address> <width(1,2,4)>"});
	}

	uint32_t addr;
	int width;
	try {
		addr = std::stoul(args[0], nullptr, 0);
		width = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid address or width"});
	}

	uint32_t value;
	if (width == 1) {
		value = get_byte(addr);
	} else if (width == 2) {
		value = get_word(addr);
	} else if (width == 4) {
		value = get_long(addr);
	} else {
		return make_response(false, {"Invalid width (use 1, 2, or 4)"});
	}

	return make_response(true, {std::to_string(value)});
}

static std::string HandleWriteMem(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received WRITE_MEM " << std::endl;
	}
	if (args.size() < 3) {
		return make_response(false, {"Usage: WRITE_MEM <address> <width(1,2,4)> <value>"});
	}

	uint32_t addr;
	int width;
	uint32_t value;
	try {
		addr = std::stoul(args[0], nullptr, 0);
		width = std::stoi(args[1]);
		value = std::stoul(args[2], nullptr, 0);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid address, width, or value"});
	}

	if (width == 1) {
		put_byte(addr, value);
	} else if (width == 2) {
		put_word(addr, value);
	} else if (width == 4) {
		put_long(addr, value);
	} else {
		return make_response(false, {"Invalid width (use 1, 2, or 4)"});
	}

	return make_response(true);
}

// === NEW COMMAND HANDLERS ===

static std::string HandleEjectFloppy(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received EJECT_FLOPPY" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: EJECT_FLOPPY <drivenum>"});
	}

	int drivenum;
	try {
		drivenum = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid drive number: " + args[0]});
	}

	if (drivenum < 0 || drivenum > 3) {
		return make_response(false, {"Drive number must be 0-3"});
	}

	return run_posted_reply(amiberry_actions_eject_floppy(drivenum));
}

static std::string HandleEjectCD(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received EJECT_CD" << std::endl;
	}

	return run_posted_reply(amiberry_actions_eject_cd());
}

static std::string HandleSetVolume(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_VOLUME" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_VOLUME <0-100>"});
	}

	int volume;
	try {
		volume = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid volume: " + args[0]});
	}

	if (volume < 0 || volume > 100) {
		return make_response(false, {"Volume must be 0-100"});
	}

	changed_prefs.sound_volume_master = volume;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetVolume(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_VOLUME" << std::endl;
	}
	return make_response(true, {std::to_string(currprefs.sound_volume_master)});
}

static std::string HandleMute(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received MUTE" << std::endl;
	}
	set_volume(0, 1);
	return make_response(true);
}

static std::string HandleUnmute(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received UNMUTE" << std::endl;
	}
	set_volume(currprefs.sound_volume_master, 0);
	return make_response(true);
}

static std::string HandleToggleFullscreen(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received TOGGLE_FULLSCREEN" << std::endl;
	}
	toggle_fullscreen(0, -1);
	return make_response(true);
}

static std::string HandleSetWarp(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_WARP" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_WARP <0|1>"});
	}

	int mode;
	try {
		mode = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid warp mode: " + args[0]});
	}

	warpmode(mode ? 1 : 0);
	return make_response(true);
}

static std::string HandleGetWarp(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_WARP" << std::endl;
	}
	return make_response(true, {std::to_string(currprefs.turbo_emulation ? 1 : 0)});
}

static std::string HandleGetVersion(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_VERSION" << std::endl;
	}
	std::vector<std::string> info;
	info.push_back("version=" + get_version_string());
	info.push_back("sdl=" + get_sdl_version_string());
	return make_response(true, info);
}

static std::string HandleListFloppies(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received LIST_FLOPPIES" << std::endl;
	}
	std::vector<std::string> responses;

	for (int i = 0; i < 4; ++i) {
		std::string entry = "DF" + std::to_string(i) + "=";
		if (currprefs.floppyslots[i].df[0]) {
			entry += currprefs.floppyslots[i].df;
		} else {
			entry += "<empty>";
		}
		responses.push_back(entry);
	}

	return make_response(true, responses);
}

// The media lists and folders come from the media API (amiberry_media.h),
// which native front ends use too.
struct media_name {
	const char* name;
	int value;
};

static const media_name media_lists[] = {
	{"RECENT_FLOPPIES", AMIBERRY_MEDIA_RECENT_FLOPPIES},
	{"RECENT_CDS", AMIBERRY_MEDIA_RECENT_CDS},
	{"RECENT_WHDLOAD", AMIBERRY_MEDIA_RECENT_WHDLOAD},
	{"CD_DRIVES", AMIBERRY_MEDIA_CD_DRIVES},
	{"CONFIGURATIONS", AMIBERRY_MEDIA_CONFIGURATIONS},
	{"SAVESTATES", AMIBERRY_MEDIA_SAVESTATES},
};

static const media_name media_folders[] = {
	{"ROMS", AMIBERRY_FOLDER_ROMS},
	{"FLOPPIES", AMIBERRY_FOLDER_FLOPPIES},
	{"CDS", AMIBERRY_FOLDER_CDS},
	{"HARD_DRIVES", AMIBERRY_FOLDER_HARD_DRIVES},
	{"CONFIGURATIONS", AMIBERRY_FOLDER_CONFIGURATIONS},
	{"SAVESTATES", AMIBERRY_FOLDER_SAVESTATES},
	{"SCREENSHOTS", AMIBERRY_FOLDER_SCREENSHOTS},
	{"WHDLOAD_ARCHIVES", AMIBERRY_FOLDER_WHDLOAD_ARCHIVES},
};

template <size_t N>
static int find_media_name(const media_name (&table)[N], const std::string& name)
{
	for (const media_name& entry : table) {
		const size_t length = std::strlen(entry.name);
		bool same = name.size() == length;
		for (size_t i = 0; same && i < length; i++)
			same = std::toupper(static_cast<unsigned char>(name[i])) == entry.name[i];
		if (same)
			return entry.value;
	}
	return -1;
}

// Copies a string the media API reports through get, growing the buffer
// while it is too small (a folder path gets a trailing separator, so it can
// exceed the host's path limit by one).
template <typename Get>
static bool media_string(Get get, std::string& out)
{
	std::vector<char> buffer(MAX_DPATH + 16);
	for (;;) {
		const amiberry_settings_status status = get(buffer.data(), buffer.size());
		if (status == AMIBERRY_SETTINGS_OK) {
			out = buffer.data();
			return true;
		}
		if (status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL || buffer.size() >= 65536)
			return false;
		buffer.resize(buffer.size() * 2);
	}
}

// The entries of a media list: paths, or names when names is set.
static std::vector<std::string> media_entries(amiberry_media_list list, bool names)
{
	std::vector<std::string> entries;
	const size_t count = amiberry_media_refresh(list);
	for (size_t i = 0; i < count; i++) {
		std::string entry;
		const bool ok = media_string([&](char* buffer, size_t size) {
			return names ? amiberry_media_entry(list, i, nullptr, 0, buffer, size)
				: amiberry_media_entry(list, i, buffer, size, nullptr, 0);
		}, entry);
		// A tab or line break would split the reply's fields or lines; such
		// a name cannot be represented in this protocol and is left out.
		if (ok && entry.find_first_of("\t\r\n") == std::string::npos)
			entries.push_back(std::move(entry));
	}
	return entries;
}

static std::string HandleListConfigs(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received LIST_CONFIGS" << std::endl;
	}
	const std::vector<std::string> configs = media_entries(AMIBERRY_MEDIA_CONFIGURATIONS, true);
	if (configs.empty()) {
		return make_response(true, {"<no configs found>"});
	}
	return make_response(true, configs);
}

static std::string HandleListMedia(const std::vector<std::string>& args)
{
	if (args.size() != 1) {
		return make_response(false, {"Usage: LIST_MEDIA <RECENT_FLOPPIES|RECENT_CDS|RECENT_WHDLOAD|CD_DRIVES|CONFIGURATIONS|SAVESTATES>"});
	}
	const int list = find_media_name(media_lists, args[0]);
	if (list < 0) {
		return make_response(false, {"Unknown media list: " + args[0]});
	}
	return make_response(true, media_entries(static_cast<amiberry_media_list>(list), false));
}

static std::string HandleGetFolder(const std::vector<std::string>& args)
{
	const int folder = args.size() == 1 ? find_media_name(media_folders, args[0]) : -1;
	if (folder < 0) {
		return make_response(false, {"Usage: GET_FOLDER <ROMS|FLOPPIES|CDS|HARD_DRIVES|CONFIGURATIONS|SAVESTATES|SCREENSHOTS|WHDLOAD_ARCHIVES>"});
	}
	std::string path;
	if (!media_string([&](char* buffer, size_t size) {
			return amiberry_folder_get(static_cast<amiberry_folder>(folder), buffer, size);
		}, path)) {
		return make_response(false, {"Folder path could not be read"});
	}
	// A tab or line break would split the reply; such a path cannot be
	// represented in this protocol.
	if (path.find_first_of("\t\r\n") != std::string::npos) {
		return make_response(false, {"Folder path contains a tab or line break"});
	}
	return make_response(true, {path});
}

static std::string HandleSetFolder(const std::vector<std::string>& args)
{
	const int folder = args.size() == 2 ? find_media_name(media_folders, args[0]) : -1;
	if (folder < 0) {
		return make_response(false, {"Usage: SET_FOLDER <folder> <path>"});
	}
	switch (amiberry_folder_set(static_cast<amiberry_folder>(folder), args[1].c_str())) {
	case AMIBERRY_SETTINGS_OK:
		return make_response(true);
	case AMIBERRY_SETTINGS_UNAVAILABLE:
		return make_response(false, {"Folder could not be stored (access or settings file): " + args[1]});
	default:
		return make_response(false, {"Invalid path: " + args[1]});
	}
}

static std::string HandleFrameAdvance(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received FRAME_ADVANCE" << std::endl;
	}
	if (!pause_emulation) {
		return make_response(false, {"Emulation must be paused first"});
	}

	int frames = 1;
	if (!args.empty()) {
		try {
			frames = std::stoi(args[0]);
		} catch (const std::exception& e) {
			return make_response(false, {"Invalid frame count: " + args[0]});
		}
		if (frames < 1) frames = 1;
		if (frames > 100) frames = 100;
	}

	// Advance one frame at a time
	for (int i = 0; i < frames; ++i) {
		resumepaused(3);
		// Brief resume then pause again
		setpaused(3);
	}

	return make_response(true, {std::to_string(frames) + " frame(s) advanced"});
}

static std::string HandleSetMouseSpeed(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_MOUSE_SPEED" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_MOUSE_SPEED <10-200>"});
	}

	int speed;
	try {
		speed = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid speed: " + args[0]});
	}

	if (speed < 10 || speed > 200) {
		return make_response(false, {"Mouse speed must be 10-200"});
	}

	changed_prefs.input_mouse_speed = speed;
	set_config_changed();

	return make_response(true);
}

static std::string HandleSendMouse(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SEND_MOUSE" << std::endl;
	}
	if (args.size() < 3) {
		return make_response(false, {"Usage: SEND_MOUSE <dx> <dy> <buttons>"});
	}

	int dx, dy, buttons;
	try {
		dx = std::stoi(args[0]);
		dy = std::stoi(args[1]);
		buttons = std::stoi(args[2]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid mouse parameters"});
	}

	// Send mouse movement (relative)
	setmousestate(0, 0, dx, 0);  // X axis
	setmousestate(0, 1, dy, 0);  // Y axis

	// Send button states (bit 0 = left, bit 1 = right, bit 2 = middle)
	setmousebuttonstate(0, 0, (buttons & 1) ? 1 : 0);  // Left button
	setmousebuttonstate(0, 1, (buttons & 2) ? 1 : 0);  // Right button
	setmousebuttonstate(0, 2, (buttons & 4) ? 1 : 0);  // Middle button

	return make_response(true);
}

static std::string HandleSendMouseAbs(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SEND_MOUSE_ABS" << std::endl;
	}
	if (args.size() < 3) {
		return make_response(false, {"Usage: SEND_MOUSE_ABS <x> <y> <buttons>"});
	}

	int x, y, buttons;
	try {
		x = std::stoi(args[0]);
		y = std::stoi(args[1]);
		buttons = std::stoi(args[2]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid mouse parameters"});
	}

	// Coordinates are raw Amiberry window coordinates. The shared host mouse
	// path applies SDL logical presentation or OpenGL HiDPI scaling exactly once.
	if (!amiberry_send_mouse_abs(x, y)) {
		return make_response(false,
			{"Absolute mouse input rejected (check coordinates, focus, and active tablet_mode)"});
	}

	// Send button states (bit 0 = left, bit 1 = right, bit 2 = middle)
	setmousebuttonstate(0, 0, (buttons & 1) ? 1 : 0);
	setmousebuttonstate(0, 1, (buttons & 2) ? 1 : 0);
	setmousebuttonstate(0, 2, (buttons & 4) ? 1 : 0);

	return make_response(true);
}

static std::string HandleGetGuiAutomationState(
	const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_GUI_AUTOMATION_STATE" << std::endl;
	}
	if (!args.empty())
		return make_response(false, {"Usage: GET_GUI_AUTOMATION_STATE"});

	const int active_monitor = amiberry_get_active_input_monitor();
	const auto geometry = amiberry_gui_geometry_snapshot();
	const auto input = gui_input_config_snapshot();
	const bool geometry_matches_active = geometry.valid
		&& geometry.monitor_id == active_monitor;
	std::vector<std::string> fields{
		"schema_version=1",
		"runtime_id=" + geometry.runtime_id,
		"geometry_revision=" + std::to_string(geometry.geometry_revision),
		"monitor_id=" + std::to_string(active_monitor),
		"geometry_valid=" + std::string(bool_field(geometry_matches_active)),
	};
	const auto input_fields = gui_input_config_fields(input);
	fields.insert(fields.end(), input_fields.begin(), input_fields.end());
	fields.emplace_back("focus_ready="
		+ std::string(bool_field(gui_input_focus_ready())));
	fields.emplace_back("supported_button_mask="
		+ std::to_string(AMIBERRY_GUI_SUPPORTED_BUTTON_MASK));
	fields.emplace_back("button_mask=" + std::to_string(
		getmousebuttonstate(0) & AMIBERRY_GUI_SUPPORTED_BUTTON_MASK));
	return make_response(true, fields);
}

static std::string HandleSetGuiAutomationConfig(
	const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_GUI_AUTOMATION_CONFIG" << std::endl;
	}
	if (args.size() != 5) {
		return make_response(false, {
			"schema_version=1", "reason=malformed_request",
			"usage=SET_GUI_AUTOMATION_CONFIG <expected_tablet_mode> "
				"<expected_mouse_untrap> <expected_input_config_revision> "
				"<tablet_mode> <mouse_untrap>"
		});
	}

	int expected_tablet = 0;
	int expected_untrap = 0;
	int new_tablet = 0;
	int new_untrap = 0;
	std::uint64_t expected_revision = 0;
	if (!parse_tablet_mode(args[0], expected_tablet)
		|| !parse_mouse_untrap(args[1], expected_untrap)
		|| !amiberry_gui_parse_u64(args[2], expected_revision)
		|| !parse_tablet_mode(args[3], new_tablet)
		|| !parse_mouse_untrap(args[4], new_untrap)) {
		return make_response(false,
			{"schema_version=1", "reason=malformed_request"});
	}

	// IPC and GUI preference mutations run on the main event thread. Feed the
	// live pending values into the locked CAS so changes made by an earlier event
	// are synchronized before ownership is checked.
	const AmiberryGuiPendingInputConfig expected{
		expected_tablet, expected_untrap
	};
	const AmiberryGuiInputConfigValues current{
		{changed_prefs.input_tablet, changed_prefs.input_mouse_untrap},
		{currprefs.input_tablet, currprefs.input_mouse_untrap}
	};
	const AmiberryGuiPendingInputConfig desired{new_tablet, new_untrap};
	const auto result = gui_input_config_state.compare_exchange(
		expected_revision, expected, current, desired,
		[](const int tablet_mode, const int mouse_untrap) {
			const bool changed = changed_prefs.input_tablet != tablet_mode
				|| changed_prefs.input_mouse_untrap != mouse_untrap;
			changed_prefs.input_tablet = tablet_mode;
			changed_prefs.input_mouse_untrap = mouse_untrap;
			if (changed)
				set_config_changed();
		});
	auto fields = gui_input_config_fields(result.snapshot);
	fields.insert(fields.begin(), result.applied
		? "reason=none" : "reason=input_config_conflict");
	fields.insert(fields.begin(), "schema_version=1");
	return make_response(result.applied, fields);
}

static std::string HandleSendMouseAbsGuarded(
	const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SEND_MOUSE_ABS_GUARDED" << std::endl;
	}
	const int active_monitor = amiberry_get_active_input_monitor();
	const auto geometry = amiberry_gui_geometry_snapshot();
	const auto input = gui_input_config_snapshot();
	const auto current_button_mask = static_cast<int>(getmousebuttonstate(0));
	if (args.size() != 7) {
		return guarded_input_response(false, "malformed_request",
			geometry, active_monitor, input, current_button_mask);
	}

	AmiberryGuiGuardedInputRequest request;
	if (!amiberry_gui_parse_int(args[0], request.x)
		|| !amiberry_gui_parse_int(args[1], request.y)
		|| !amiberry_gui_parse_int(args[2], request.button_mask)
		|| !amiberry_gui_parse_u64(args[4], request.geometry_revision)
		|| !amiberry_gui_parse_int(args[5], request.monitor_id)
		|| !amiberry_gui_parse_u64(args[6], request.input_config_revision)) {
		return guarded_input_response(false, "malformed_request",
			geometry, active_monitor, input, current_button_mask);
	}
	request.runtime_id = args[3];
	if (request.runtime_id.empty()) {
		return guarded_input_response(false, "malformed_request",
			geometry, active_monitor, input, current_button_mask);
	}

	const AmiberryGuiGuardedInputEnvironment environment{
		gui_input_focus_ready(),
		currprefs.input_tablet >= TABLET_MOUSEHACK,
		active_monitor,
		input.input_config_revision
	};
	const auto result = amiberry_gui_guarded_input_apply(
		request, environment,
		[](const int monitor_id, const int x, const int y,
			const int button_mask) {
			if (!amiberry_send_mouse_abs_to_monitor(monitor_id, x, y))
				return false;
			setmousebuttonstateall(0, button_mask,
				AMIBERRY_GUI_SUPPORTED_BUTTON_MASK);
			return true;
		});
	return guarded_input_response(result.applied,
		amiberry_gui_guard_reason_name(result.reason), result.geometry,
		active_monitor, input, static_cast<int>(getmousebuttonstate(0)),
		&request);
}

static std::string HandleReleaseMouseButtons(
	const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received RELEASE_MOUSE_BUTTONS" << std::endl;
	}
	if (!args.empty())
		return make_response(false, {"Usage: RELEASE_MOUSE_BUTTONS"});
	amiberry_gui_release_mouse_buttons(
		[](const int button) { setmousebuttonstate(0, button, 0); });
	const int button_mask = static_cast<int>(getmousebuttonstate(0))
		& AMIBERRY_GUI_SUPPORTED_BUTTON_MASK;
	return make_response(button_mask == 0, {
		"schema_version=1",
		"button_mask=" + std::to_string(button_mask)
	});
}

static std::string HandlePing(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received PING" << std::endl;
	}
	return make_response(true, {"PONG"});
}

static std::string HandleQuickSave(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received QUICKSAVE" << std::endl;
	}
#ifdef DEBUGGER
	if (debugger_is_stopped() || gdb_server_halted()) {
		return make_response(false, {"Cannot save state while the debugger has the CPU stopped; resume or PAUSE first"});
	}
#endif
	int slot = 0;
	if (!args.empty()) {
		try {
			slot = std::stoi(args[0]);
		} catch (const std::exception& e) {
			return make_response(false, {"Invalid slot number: " + args[0]});
		}
		if (slot < 0 || slot > 9) {
			return make_response(false, {"Slot must be 0-9"});
		}
	}

	std::string error;
	const amiberry_action_id id = amiberry_actions_save_state_slot(slot);
	if (id == 0 || !run_posted(id, &error)) {
		return make_response(false, {error.empty() ? "Action was not executed" : error});
	}
	return make_response(true, {"Saved to slot " + std::to_string(slot)});
}

static std::string HandleQuickLoad(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received QUICKLOAD" << std::endl;
	}
	int slot = 0;
	if (!args.empty()) {
		try {
			slot = std::stoi(args[0]);
		} catch (const std::exception& e) {
			return make_response(false, {"Invalid slot number: " + args[0]});
		}
		if (slot < 0 || slot > 9) {
			return make_response(false, {"Slot must be 0-9"});
		}
	}

	std::string error;
	const amiberry_action_id id = amiberry_actions_load_state_slot(slot);
	if (id == 0 || !run_posted(id, &error)) {
		return make_response(false, {error.empty() ? "Action was not executed" : error});
	}
	return make_response(true, {"Loading from slot " + std::to_string(slot)});
}

static std::string HandleGetJoyportMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_JOYPORT_MODE" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: GET_JOYPORT_MODE <port>"});
	}

	int port;
	try {
		port = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid port number: " + args[0]});
	}

	if (port < 0 || port > 3) {
		return make_response(false, {"Port must be 0-3"});
	}

	// Mode names matching JSEM_MODE_* constants
	const char* mode_names[] = {
		"default", "wheelmouse", "mouse", "joystick",
		"gamepad", "analog_joystick", "cdtv_remote", "cd32_pad", "lightpen"
	};

	const int mode = currprefs.jports[port].mode;
	std::string mode_name = (mode >= 0 && mode <= 8) ? mode_names[mode] : "unknown";

	return make_response(true, {std::to_string(mode), mode_name});
}

static std::string HandleSetJoyportMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_JOYPORT_MODE" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: SET_JOYPORT_MODE <port> <mode>"});
	}

	int port, mode;
	try {
		port = std::stoi(args[0]);
		mode = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid argument"});
	}

	if (port < 0 || port > 3) {
		return make_response(false, {"Port must be 0-3"});
	}

	if (mode < 0 || mode > 8) {
		return make_response(false, {"Mode must be 0-8 (0=default, 2=mouse, 3=joystick, 4=gamepad, 7=cd32)"});
	}

	changed_prefs.jports[port].mode = mode;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetAutofire(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_AUTOFIRE" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: GET_AUTOFIRE <port>"});
	}

	int port;
	try {
		port = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid port number: " + args[0]});
	}

	if (port < 0 || port > 3) {
		return make_response(false, {"Port must be 0-3"});
	}

	int autofire = currprefs.jports[port].autofire;
	return make_response(true, {std::to_string(autofire)});
}

static std::string HandleSetAutofire(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_AUTOFIRE" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: SET_AUTOFIRE <port> <mode>"});
	}

	int port, mode;
	try {
		port = std::stoi(args[0]);
		mode = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid argument"});
	}

	if (port < 0 || port > 3) {
		return make_response(false, {"Port must be 0-3"});
	}

	if (mode < 0 || mode > 4) {
		return make_response(false, {"Autofire mode must be 0-4 (0=off, 1=normal, 2=toggle, 3=always)"});
	}

	changed_prefs.jports[port].autofire = mode;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetLEDStatus(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_LED_STATUS" << std::endl;
	}
	amiberry_actions_run_pending();
	amiberry_actions_status snapshot;
	amiberry_actions_get_status(&snapshot);
	std::vector<std::string> responses;

	// Power LED
	responses.push_back("power=" + std::to_string(snapshot.power_led ? 1 : 0));

	// Floppy drive LEDs (DF0-DF3)
	for (int i = 0; i < AMIBERRY_ACTIONS_FLOPPY_DRIVES; ++i) {
		const auto& drive = snapshot.floppy[i];
		std::string status = drive.motor_on ? (drive.writing ? "writing" : "reading") : "off";
		responses.push_back("df" + std::to_string(i) + "=" + status);
	}

	// HD LED
	responses.push_back("hd=" + std::to_string(snapshot.hd_activity));

	// CD LED
	responses.push_back("cd=" + std::to_string(snapshot.cd_activity));

	// Caps Lock
	responses.push_back("caps=" + std::to_string(snapshot.caps_lock ? 1 : 0));

	return make_response(true, responses);
}

static std::string HandleListHarddrives(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received LIST_HARDDRIVES" << std::endl;
	}
	std::vector<std::string> responses;

	// List mounted hardfiles from prefs: the entries in use, not what a
	// removed entry left behind past them.
	const int count = std::min(currprefs.mountitems, MOUNT_CONFIG_SIZE);
	for (int i = 0; i < count; ++i) {
		struct uaedev_config_data *uci = &currprefs.mountconfig[i];
		// A tab or line break would split the reply.
		if (uci->ci.rootdir[0] && !std::strpbrk(uci->ci.rootdir, "\t\r\n")) {
			std::string entry = "unit" + std::to_string(i) + "=";
			if (uci->ci.type == UAEDEV_HDF) {
				entry += "hdf:";
			} else if (uci->ci.type == UAEDEV_CD) {
				entry += "cd:";
			} else if (uci->ci.type == UAEDEV_DIR) {
				entry += "dir:";
			} else {
				entry += "other:";
			}
			entry += uci->ci.rootdir;
			if (uci->ci.readonly) {
				entry += " (readonly)";
			}
			responses.push_back(entry);
		}
	}

	if (responses.empty()) {
		return make_response(true, {"<no harddrives mounted>"});
	}

	return make_response(true, responses);
}

static std::string HandleSetDisplayMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_DISPLAY_MODE" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_DISPLAY_MODE <mode> (0=window, 1=legacy fullscreen alias, 2=fullwindow)"});
	}

	int mode;
	try {
		mode = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid mode: " + args[0]});
	}

	if (mode < 0 || mode > 2) {
		return make_response(false, {"Mode must be 0-2 (0=window, 1=legacy fullscreen alias, 2=fullwindow)"});
	}

	changed_prefs.gfx_apmode[0].gfx_fullscreen = amiberry_normalize_gfx_fullscreen_mode(mode);
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetDisplayMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_DISPLAY_MODE" << std::endl;
	}
	const int mode = amiberry_normalize_gfx_fullscreen_mode(
		currprefs.gfx_apmode[0].gfx_fullscreen);
	const char* mode_names[] = {"window", "fullscreen", "fullwindow"};
	std::string mode_name = (mode >= 0 && mode <= 2) ? mode_names[mode] : "unknown";

	return make_response(true, {std::to_string(mode), mode_name});
}

static std::string HandleSetNTSC(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_NTSC" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_NTSC <0|1> (0=PAL, 1=NTSC)"});
	}

	int ntsc;
	try {
		ntsc = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid value: " + args[0]});
	}

	changed_prefs.ntscmode = ntsc ? true : false;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetNTSC(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_NTSC" << std::endl;
	}
	return make_response(true, {std::to_string(currprefs.ntscmode ? 1 : 0), currprefs.ntscmode ? "NTSC" : "PAL"});
}

static std::string HandleSetSoundMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_SOUND_MODE" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_SOUND_MODE <mode> (0=off, 1=normal, 2=stereo, 3=best)"});
	}

	int mode;
	try {
		mode = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid mode: " + args[0]});
	}

	if (mode < 0 || mode > 3) {
		return make_response(false, {"Mode must be 0-3 (0=off, 1=normal, 2=stereo, 3=best)"});
	}

	changed_prefs.produce_sound = mode;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetSoundMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_SOUND_MODE" << std::endl;
	}
	const int mode = currprefs.produce_sound;
	const char* mode_names[] = {"off", "normal", "stereo", "best"};
	std::string mode_name = (mode >= 0 && mode <= 3) ? mode_names[mode] : "unknown";

	return make_response(true, {std::to_string(mode), mode_name});
}

static std::string HandleToggleMouseGrab(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received TOGGLE_MOUSE_GRAB" << std::endl;
	}
	toggle_mousegrab();
	return make_response(true);
}

static std::string HandleGetMouseSpeed(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_MOUSE_SPEED" << std::endl;
	}
	return make_response(true, {std::to_string(currprefs.input_mouse_speed)});
}

static std::string HandleSetCPUSpeed(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_CPU_SPEED" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_CPU_SPEED <speed> (-1=max, 0=cycle-exact, >0=percentage)"});
	}

	int speed;
	try {
		speed = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid speed: " + args[0]});
	}

	changed_prefs.m68k_speed = speed;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetCPUSpeed(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_CPU_SPEED" << std::endl;
	}
	const int speed = currprefs.m68k_speed;
	std::string desc;
	if (speed == -1) {
		desc = "max";
	} else if (speed == 0) {
		desc = "cycle-exact";
	} else {
		desc = std::to_string(speed) + "%";
	}
	return make_response(true, {std::to_string(speed), desc});
}

static std::string HandleToggleRTG(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received TOGGLE_RTG" << std::endl;
	}
	int monid = 0;
	if (!args.empty()) {
		try {
			monid = std::stoi(args[0]);
		} catch (const std::exception& e) {
			return make_response(false, {"Invalid monitor ID: " + args[0]});
		}
	}

	bool result = toggle_rtg(monid, -1);
	return make_response(true, {result ? "RTG" : "Chipset"});
}

static std::string HandleSetFloppySpeed(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_FLOPPY_SPEED" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_FLOPPY_SPEED <speed> (0=turbo, 100=1x, 200=2x, 400=4x, 800=8x)"});
	}

	int speed;
	try {
		speed = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid speed: " + args[0]});
	}

	// Valid values: 0 (turbo), 100 (1x), 200 (2x), 400 (4x), 800 (8x)
	if (speed != 0 && speed != 100 && speed != 200 && speed != 400 && speed != 800) {
		return make_response(false, {"Speed must be 0 (turbo), 100 (1x), 200 (2x), 400 (4x), or 800 (8x)"});
	}

	changed_prefs.floppy_speed = speed;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetFloppySpeed(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_FLOPPY_SPEED" << std::endl;
	}
	const int speed = currprefs.floppy_speed;
	std::string desc;
	switch (speed) {
		case 0: desc = "turbo"; break;
		case 100: desc = "1x"; break;
		case 200: desc = "2x"; break;
		case 400: desc = "4x"; break;
		case 800: desc = "8x"; break;
		default: desc = std::to_string(speed); break;
	}
	return make_response(true, {std::to_string(speed), desc});
}

static std::string HandleDiskWriteProtect(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DISK_WRITE_PROTECT" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: DISK_WRITE_PROTECT <drive> <0|1>"});
	}

	int drive, protect;
	try {
		drive = std::stoi(args[0]);
		protect = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid argument"});
	}

	if (drive < 0 || drive > 3) {
		return make_response(false, {"Drive must be 0-3"});
	}

	if (currprefs.floppyslots[drive].df[0] == 0) {
		return make_response(false, {"No disk in drive " + std::to_string(drive)});
	}

	disk_setwriteprotect(&changed_prefs, drive, currprefs.floppyslots[drive].df, protect != 0);
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetDiskWriteProtect(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_DISK_WRITE_PROTECT" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: GET_DISK_WRITE_PROTECT <drive>"});
	}

	int drive;
	try {
		drive = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid drive number: " + args[0]});
	}

	if (drive < 0 || drive > 3) {
		return make_response(false, {"Drive must be 0-3"});
	}

	if (currprefs.floppyslots[drive].df[0] == 0) {
		return make_response(false, {"No disk in drive " + std::to_string(drive)});
	}

	bool wp = disk_getwriteprotect(&currprefs, currprefs.floppyslots[drive].df, drive) != 0;
	return make_response(true, {std::to_string(wp ? 1 : 0), wp ? "protected" : "writable"});
}

static std::string HandleToggleStatusLine(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received TOGGLE_STATUS_LINE" << std::endl;
	}
	// Cycle through: off -> chipset -> RTG -> both -> off
	int current = changed_prefs.leds_on_screen;
	int next;
	switch (current) {
		case 0: next = 1; break;  // off -> chipset
		case 1: next = 2; break;  // chipset -> RTG
		case 2: next = 3; break;  // RTG -> both
		default: next = 0; break; // both/other -> off
	}

	changed_prefs.leds_on_screen = next;
	set_config_changed();

	const char* mode_names[] = {"off", "chipset", "rtg", "both"};
	return make_response(true, {std::to_string(next), mode_names[next]});
}

static std::string HandleSetChipset(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_CHIPSET" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_CHIPSET <chipset> (OCS, ECS_AGNUS, ECS_DENISE, ECS, AGA)"});
	}

	std::string chipset = args[0];
	// Convert to uppercase
	std::transform(chipset.begin(), chipset.end(), chipset.begin(), ::toupper);

	unsigned int mask;
	if (chipset == "OCS" || chipset == "0") {
		mask = 0;
	} else if (chipset == "ECS_AGNUS" || chipset == "1") {
		mask = CSMASK_ECS_AGNUS;
	} else if (chipset == "ECS_DENISE" || chipset == "2") {
		mask = CSMASK_ECS_DENISE;
	} else if (chipset == "ECS" || chipset == "3") {
		mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	} else if (chipset == "AGA" || chipset == "4") {
		mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	} else {
		return make_response(false, {"Invalid chipset: " + args[0]});
	}

	changed_prefs.chipset_mask = mask;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetChipset(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_CHIPSET" << std::endl;
	}
	unsigned int mask = currprefs.chipset_mask;
	std::string name;

	if (mask & CSMASK_AGA) {
		name = "AGA";
	} else if ((mask & (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE)) == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE)) {
		name = "ECS";
	} else if (mask & CSMASK_ECS_AGNUS) {
		name = "ECS_AGNUS";
	} else if (mask & CSMASK_ECS_DENISE) {
		name = "ECS_DENISE";
	} else {
		name = "OCS";
	}

	return make_response(true, {std::to_string(mask), name});
}

static std::string HandleGetMemoryConfig(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_MEMORY_CONFIG" << std::endl;
	}
	std::vector<std::string> responses;

	// All memory sizes in KB
	responses.push_back("chip=" + std::to_string(currprefs.chipmem.size / 1024) + "KB");
	responses.push_back("fast=" + std::to_string(currprefs.fastmem[0].size / 1024) + "KB");
	responses.push_back("bogo=" + std::to_string(currprefs.bogomem.size / 1024) + "KB");
	responses.push_back("z3=" + std::to_string(currprefs.z3fastmem[0].size / 1024) + "KB");
	responses.push_back("rtg=" + std::to_string(currprefs.rtgboards[0].rtgmem_size / 1024) + "KB");

	return make_response(true, responses);
}

static std::string HandleGetFPS(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_FPS" << std::endl;
	}
	std::vector<std::string> responses;

	// FPS is stored in tenths (e.g., 500 = 50.0 fps)
	int fps = (gui_data.fps + 5) / 10;
	int idle = (gui_data.idle + 5) / 10;

	responses.push_back("fps=" + std::to_string(fps));
	responses.push_back("idle=" + std::to_string(idle) + "%");
	responses.push_back("lines=" + std::to_string(gui_data.lines));
	responses.push_back("lace=" + std::string(gui_data.lace ? "true" : "false"));

	return make_response(true, responses);
}

static std::string HandleSetChipMem(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_CHIP_MEM" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_CHIP_MEM <size_kb> (0, 256, 512, 1024, 2048, 4096, 8192)"});
	}

	int size_kb;
	try {
		size_kb = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid size: " + args[0]});
	}

	// Chip RAM valid sizes: 256KB, 512KB, 1MB, 2MB, 4MB, 8MB
	static const int valid_chip_sizes[] = {256, 512, 1024, 2048, 4096, 8192};
	bool valid = false;
	for (const int vs : valid_chip_sizes) {
		if (size_kb == vs) {
			valid = true;
			break;
		}
	}

	if (!valid) {
		return make_response(false, {"Chip RAM must be 256, 512, 1024, 2048, 4096, or 8192 KB"});
	}

	const uae_u32 size_bytes = static_cast<uae_u32>(size_kb) * 1024;

	// ECS/OCS chipsets require special handling
	if (!(changed_prefs.chipset_mask & CSMASK_ECS_AGNUS) && size_bytes > 0x80000) {
		return make_response(false, {"OCS chipset limited to 512KB chip RAM"});
	}

	// If chip RAM > 2MB, fast RAM must be 0
	if (size_bytes > 0x200000 && changed_prefs.fastmem[0].size > 0) {
		changed_prefs.fastmem[0].size = 0;
	}

	changed_prefs.chipmem.size = size_bytes;
	set_config_changed();

	return make_response(true, {"Chip RAM set to " + std::to_string(size_kb) + " KB"});
}

static std::string HandleSetFastMem(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_FAST_MEM" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_FAST_MEM <size_kb> (0, 64, 128, 256, 512, 1024, 2048, 4096, 8192)"});
	}

	int size_kb;
	try {
		size_kb = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid size: " + args[0]});
	}

	// Fast RAM valid sizes: 0, 64KB, 128KB, 256KB, 512KB, 1MB, 2MB, 4MB, 8MB
	static const int valid_fast_sizes[] = {0, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
	bool valid = false;
	for (const int vs : valid_fast_sizes) {
		if (size_kb == vs) {
			valid = true;
			break;
		}
	}

	if (!valid) {
		return make_response(false, {"Fast RAM must be 0, 64, 128, 256, 512, 1024, 2048, 4096, or 8192 KB"});
	}

	uae_u32 size_bytes = static_cast<uae_u32>(size_kb) * 1024;

	// If fast RAM > 0, chip RAM must be <= 2MB
	if (size_bytes > 0 && changed_prefs.chipmem.size > 0x200000) {
		changed_prefs.chipmem.size = 0x200000;
	}

	changed_prefs.fastmem[0].size = size_bytes;
	set_config_changed();

	return make_response(true, {"Fast RAM set to " + std::to_string(size_kb) + " KB"});
}

static std::string HandleSetSlowMem(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_SLOW_MEM" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_SLOW_MEM <size_kb> (0, 256, 512, 1024, 1536, 1792)"});
	}

	int size_kb;
	try {
		size_kb = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid size: " + args[0]});
	}

	// Slow/Bogo RAM valid sizes: 0, 256KB, 512KB, 1MB, 1.5MB, 1.8MB
	static const int valid_slow_sizes[] = {0, 256, 512, 1024, 1536, 1792};
	bool valid = false;
	for (int vs : valid_slow_sizes) {
		if (size_kb == vs) {
			valid = true;
			break;
		}
	}

	if (!valid) {
		return make_response(false, {"Slow RAM must be 0, 256, 512, 1024, 1536, or 1792 KB"});
	}

	uae_u32 size_bytes = static_cast<uae_u32>(size_kb) * 1024;
	changed_prefs.bogomem.size = size_bytes;
	set_config_changed();

	return make_response(true, {"Slow RAM set to " + std::to_string(size_kb) + " KB"});
}

static std::string HandleSetZ3Mem(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_Z3_MEM" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_Z3_MEM <size_mb> (0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024)"});
	}

	int size_mb;
	try {
		size_mb = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid size: " + args[0]});
	}

	// Z3 RAM valid sizes in MB: 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
	static const int valid_z3_sizes[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
	bool valid = false;
	for (int vs : valid_z3_sizes) {
		if (size_mb == vs) {
			valid = true;
			break;
		}
	}

	if (!valid) {
		return make_response(false, {"Z3 Fast RAM must be 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, or 1024 MB"});
	}

	uae_u32 size_bytes = static_cast<uae_u32>(size_mb) * 1024 * 1024;
	changed_prefs.z3fastmem[0].size = size_bytes;
	set_config_changed();

	return make_response(true, {"Z3 Fast RAM set to " + std::to_string(size_mb) + " MB"});
}

static std::string HandleGetCPUModel(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_CPU_MODEL" << std::endl;
	}
	std::vector<std::string> responses;

	int model = currprefs.cpu_model;
	std::string name = "68" + std::to_string(model / 10);

	responses.push_back("model=" + std::to_string(model));
	responses.push_back("name=" + name);
	responses.push_back("fpu=" + std::to_string(currprefs.fpu_model));
	responses.push_back("24bit=" + std::string(currprefs.address_space_24 ? "true" : "false"));
	responses.push_back("compatible=" + std::string(currprefs.cpu_compatible ? "true" : "false"));
	responses.push_back("cycle_exact=" + std::string(currprefs.cpu_cycle_exact ? "true" : "false"));

	return make_response(true, responses);
}

static std::string HandleSetCPUModel(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_CPU_MODEL" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_CPU_MODEL <model> (68000, 68010, 68020, 68030, 68040, 68060)"});
	}

	std::string model_str = args[0];
	// Convert to uppercase and strip "68" prefix if present
	std::transform(model_str.begin(), model_str.end(), model_str.begin(), ::toupper);
	if (model_str.substr(0, 2) == "68") {
		model_str = model_str.substr(2);
	}

	int model;
	try {
		model = std::stoi(model_str);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid CPU model: " + args[0]});
	}

	// Valid CPU models (in internal format)
	static const int valid_models[] = {0, 10, 20, 30, 40, 60};  // 68000, 68010, 68020, 68030, 68040, 68060
	bool valid = false;
	for (int vm : valid_models) {
		if (model == vm) {
			valid = true;
			break;
		}
	}

	if (!valid) {
		return make_response(false, {"CPU model must be 68000, 68010, 68020, 68030, 68040, or 68060"});
	}

	// Store as full model number
	changed_prefs.cpu_model = model == 0 ? 68000 : 68000 + model;

	// Adjust address space for 68000/68010
	if (model <= 10) {
		changed_prefs.address_space_24 = true;
	}

	set_config_changed();

	return make_response(true, {"CPU model set to 68" + std::to_string(model == 0 ? 0 : model)});
}

static std::string HandleSetWindowSize(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_WINDOW_SIZE" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: SET_WINDOW_SIZE <width> <height>"});
	}

	int width, height;
	try {
		width = std::stoi(args[0]);
		height = std::stoi(args[1]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid width or height"});
	}

	if (width < 320 || width > 3840 || height < 200 || height > 2160) {
		return make_response(false, {"Window size must be between 320x200 and 3840x2160"});
	}

	changed_prefs.gfx_monitor[0].gfx_size_win.width = width;
	changed_prefs.gfx_monitor[0].gfx_size_win.height = height;
	set_config_changed();

	return make_response(true, {"Window size set to " + std::to_string(width) + "x" + std::to_string(height)});
}

static std::string HandleGetWindowSize(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_WINDOW_SIZE" << std::endl;
	}
	std::vector<std::string> responses;

	int width = currprefs.gfx_monitor[0].gfx_size_win.width;
	int height = currprefs.gfx_monitor[0].gfx_size_win.height;

	responses.push_back("width=" + std::to_string(width));
	responses.push_back("height=" + std::to_string(height));

	return make_response(true, responses);
}

static std::string HandleSetScaling(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_SCALING" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_SCALING <method> (-1=auto, 0=nearest, 1=linear, 2=integer, 3=stretch)"});
	}

	int method;
	try {
		method = std::stoi(args[0]);
	} catch (const std::exception& e) {
		// Try string values
		std::string str = args[0];
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		if (str == "auto") method = -1;
		else if (str == "nearest") method = 0;
		else if (str == "linear") method = 1;
		else if (str == "integer") method = 2;
		else if (str == "stretch") method = 3;
		else {
			return make_response(false, {"Invalid scaling method: " + args[0]});
		}
	}

	if (method < -1 || method > 3) {
		return make_response(false, {"Scaling method must be -1..3 (-1=auto, 0=nearest, 1=linear, 2=integer, 3=stretch)"});
	}

	// Set scaling method
	changed_prefs.scaling_method = method;
	set_config_changed();

	const char* method_names[] = {"auto", "nearest", "linear", "integer", "stretch"};
	const int method_index = method + 1; // -1..3 -> 0..4
	const char* method_name = (method_index >= 0 && method_index <= 4) ? method_names[method_index] : "unknown";
	return make_response(true, {std::to_string(method), method_name});
}

static std::string HandleGetScaling(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_SCALING" << std::endl;
	}
	std::vector<std::string> responses;

	int method = currprefs.scaling_method;
	const char* method_names[] = {"auto", "nearest", "linear", "integer", "stretch"};
	const int method_index = method + 1; // -1..3 -> 0..4
	std::string method_name = (method_index >= 0 && method_index <= 4) ? method_names[method_index] : "unknown";

	responses.push_back("method=" + std::to_string(method));
	responses.push_back("method_name=" + method_name);

	return make_response(true, responses);
}

static std::string HandleSetLineMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_LINE_MODE" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_LINE_MODE <mode> (0=none, 1=double, 2=scanlines)"});
	}

	int mode;
	try {
		mode = std::stoi(args[0]);
	} catch (const std::exception& e) {
		// Try string values
		std::string str = args[0];
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		if (str == "none" || str == "single") mode = 0;
		else if (str == "double" || str == "doubled") mode = 1;
		else if (str == "scanlines") mode = 2;
		else {
			return make_response(false, {"Invalid line mode: " + args[0]});
		}
	}

	if (mode < 0 || mode > 2) {
		return make_response(false, {"Line mode must be 0-2 (0=none/single, 1=double, 2=scanlines)"});
	}

	if (mode == 0) {
		changed_prefs.gfx_vresolution = 0;  // VRES_NONDOUBLE
		changed_prefs.gfx_pscanlines = 0;
		changed_prefs.gfx_iscanlines = 0;
	} else if (mode == 1) {
		changed_prefs.gfx_vresolution = 1;  // VRES_DOUBLE
		changed_prefs.gfx_pscanlines = 0;
		changed_prefs.gfx_iscanlines = 0;
	} else {
		changed_prefs.gfx_vresolution = 1;  // VRES_DOUBLE
		changed_prefs.gfx_pscanlines = 1;   // Enable scanlines
		changed_prefs.gfx_iscanlines = 0;
	}

	set_config_changed();

	const char* mode_names[] = {"single", "double", "scanlines"};
	return make_response(true, {std::to_string(mode), mode_names[mode]});
}

static std::string HandleGetLineMode(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_LINE_MODE" << std::endl;
	}
	std::vector<std::string> responses;

	int vres = currprefs.gfx_vresolution;
	int pscan = currprefs.gfx_pscanlines;
	int iscan = currprefs.gfx_iscanlines;

	std::string mode_name;
	int mode;
	if (vres == 0) {
		mode = 0;
		mode_name = "single";
	} else if (pscan == 0 && iscan == 0) {
		mode = 1;
		mode_name = "double";
	} else {
		mode = 2;
		mode_name = "scanlines";
	}

	responses.push_back("mode=" + std::to_string(mode));
	responses.push_back("name=" + mode_name);
	responses.push_back("vresolution=" + std::to_string(vres));
	responses.push_back("pscanlines=" + std::to_string(pscan));
	responses.push_back("iscanlines=" + std::to_string(iscan));

	return make_response(true, responses);
}

static std::string HandleSetResolution(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_RESOLUTION" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_RESOLUTION <mode> (0=lores, 1=hires, 2=superhires)"});
	}

	int res;
	try {
		res = std::stoi(args[0]);
	} catch (const std::exception& e) {
		// Try string values
		std::string str = args[0];
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		if (str == "lores" || str == "low") res = 0;
		else if (str == "hires" || str == "high") res = 1;
		else if (str == "superhires" || str == "super") res = 2;
		else {
			return make_response(false, {"Invalid resolution: " + args[0]});
		}
	}

	if (res < 0 || res > 2) {
		return make_response(false, {"Resolution must be 0-2 (0=lores, 1=hires, 2=superhires)"});
	}

	changed_prefs.gfx_resolution = res;
	set_config_changed();

	const char* res_names[] = {"lores", "hires", "superhires"};
	return make_response(true, {std::to_string(res), res_names[res]});
}

static std::string HandleGetResolution(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_RESOLUTION" << std::endl;
	}

	int res = currprefs.gfx_resolution;
	const char* res_names[] = {"lores", "hires", "superhires"};
	std::string res_name = (res >= 0 && res <= 2) ? res_names[res] : "unknown";

	return make_response(true, {std::to_string(res), res_name});
}

// Autocrop and WHDLoad ===

static std::string HandleSetAutocrop(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_AUTOCROP" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: SET_AUTOCROP <0|1>"});
	}

	int enable;
	try {
		enable = std::stoi(args[0]);
	} catch (const std::exception& e) {
		std::string str = args[0];
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		if (str == "on" || str == "true" || str == "yes") enable = 1;
		else if (str == "off" || str == "false" || str == "no") enable = 0;
		else {
			return make_response(false, {"Invalid value: " + args[0]});
		}
	}

	changed_prefs.gfx_auto_crop = (enable != 0);
	set_config_changed();

	return make_response(true, {std::to_string(changed_prefs.gfx_auto_crop ? 1 : 0),
		changed_prefs.gfx_auto_crop ? "enabled" : "disabled"});
}

static std::string HandleGetAutocrop(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_AUTOCROP" << std::endl;
	}

	bool enabled = currprefs.gfx_auto_crop;
	return make_response(true, {std::to_string(enabled ? 1 : 0), enabled ? "enabled" : "disabled"});
}

static std::string HandleInsertWHDLoad(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received INSERT_WHDLOAD" << std::endl;
	}
	if (args.empty()) {
		return make_response(false, {"Usage: INSERT_WHDLOAD <path_to_lha_or_directory>"});
	}
	if (args[0].size() >= MAX_DPATH) {
		return make_response(false, {"Path exceeds maximum length"});
	}

	std::string path = args[0];

	// Check if file/directory exists
	struct stat st{};
	if (stat(path.c_str(), &st) != 0) {
		return make_response(false, {"File or directory not found: " + path});
	}

	// Verify it's either an LHA file or a directory
	bool is_dir = S_ISDIR(st.st_mode);
	bool is_lha = false;
	if (!is_dir) {
		std::string lower_path = path;
		std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
		is_lha = (lower_path.length() > 4 &&
			(lower_path.substr(lower_path.length() - 4) == ".lha" ||
			 lower_path.substr(lower_path.length() - 4) == ".lzh" ||
			 lower_path.substr(lower_path.length() - 4) == ".lzx"));
	}

	if (!is_dir && !is_lha) {
		return make_response(false, {"Path must be an LHA/LZH/LZX archive or a WHDLoad game directory"});
	}

	// Set the WHDLoad filename and trigger auto configuration
	whdload_prefs.whdload_filename = path;
	whdload_auto_prefs(&changed_prefs, path.c_str());
	set_config_changed();

	return make_response(true, {"WHDLoad game loaded: " + path});
}

static std::string HandleEjectWHDLoad(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received EJECT_WHDLOAD" << std::endl;
	}

	if (whdload_prefs.whdload_filename.empty()) {
		return make_response(false, {"No WHDLoad game is currently loaded"});
	}

	std::string previous = whdload_prefs.whdload_filename;
	whdload_prefs.whdload_filename.clear();
	clear_whdload_prefs();

	return make_response(true, {"WHDLoad game ejected: " + previous});
}

static std::string HandleGetWHDLoad(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_WHDLOAD" << std::endl;
	}

	std::vector<std::string> responses;

	if (whdload_prefs.whdload_filename.empty()) {
		responses.emplace_back("loaded=0");
		responses.emplace_back("filename=");
	} else {
		responses.emplace_back("loaded=1");
		responses.push_back("filename=" + whdload_prefs.whdload_filename);

		if (!whdload_prefs.game_name.empty())
			responses.push_back("game_name=" + whdload_prefs.game_name);
		if (!whdload_prefs.sub_path.empty())
			responses.push_back("sub_path=" + whdload_prefs.sub_path);
		if (!whdload_prefs.selected_slave.filename.empty())
			responses.push_back("slave=" + whdload_prefs.selected_slave.filename);
		if (whdload_prefs.slave_count > 0)
			responses.push_back("slave_count=" + std::to_string(whdload_prefs.slave_count));
	}

	return make_response(true, responses);
}

// Debugging and Diagnostics ===

#ifdef DEBUGGER
static std::string HandleDebugActivate(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DEBUG_ACTIVATE" << std::endl;
	}

	if (debugger_active) {
		return make_response(true, {"Debugger already active"});
	}

	activate_debugger();
	return make_response(true, {"Debugger activated"});
}

static std::string HandleDebugDeactivate(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DEBUG_DEACTIVATE" << std::endl;
	}

	if (!debugger_active) {
		return make_response(true, {"Debugger not active"});
	}

	deactivate_debugger();
	return make_response(true, {"Debugger deactivated"});
}

static std::string HandleDebugStatus(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DEBUG_STATUS" << std::endl;
	}

	std::vector<std::string> responses;
	responses.push_back("active=" + std::to_string(debugger_active ? 1 : 0));
	const bool stopped = debugger_is_stopped();
	responses.push_back("stopped=" + std::to_string(stopped ? 1 : 0));
	responses.push_back("debugging=" + std::to_string(debugging ? 1 : 0));
	responses.push_back("exception_debugging=" + std::to_string(exception_debugging ? 1 : 0));
	// The saved text belongs to the last stop; any exit from the debugger
	// (continue, step, DEBUG_DEACTIVATE, the console) makes it historical.
	responses.push_back(std::string("reason=") + (stopped ? debugger_last_stop_reason() : ""));

	return make_response(true, responses);
}

static std::string HandleDebugStep(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DEBUG_STEP" << std::endl;
	}

	int count = 1;
	if (!args.empty()) {
		try {
			count = std::stoi(args[0]);
		} catch (...) {
			return make_response(false, {"Invalid instruction count"});
		}
	}
	if (count < 1 || count > 10000) {
		return make_response(false, {"Instruction count must be 1-10000"});
	}
	if (!debugger_request_step(count)) {
		return make_response(false, {"Debugger is not stopped"});
	}

	return make_response(true, {"Stepping " + std::to_string(count) + " instruction(s)"});
}

static std::string HandleDebugStepOver(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DEBUG_STEP_OVER" << std::endl;
	}

	if (!debugger_request_step_over()) {
		return make_response(false, {"Debugger is not stopped"});
	}

	return make_response(true, {"Stepping over current instruction"});
}

static std::string HandleDebugContinue(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DEBUG_CONTINUE" << std::endl;
	}

	if (!debugger_request_continue()) {
		return make_response(false, {"Debugger is not stopped"});
	}

	return make_response(true, {"Execution continued"});
}

#ifdef GDBSERVER
static bool monitor_prefix(const std::string& text, const char* prefix)
{
	const size_t n = strlen(prefix);
	return text.compare(0, n, prefix) == 0 && (text.size() == n || text[n] == ' ');
}

static bool monitor_changes_state(const std::string& text)
{
	return monitor_prefix(text, "watch add") || monitor_prefix(text, "watch range") ||
		monitor_prefix(text, "watch remove") || text == "watch clear-events" ||
		monitor_prefix(text, "dma-watch add") || monitor_prefix(text, "dma-watch remove") ||
		monitor_prefix(text, "condition add") || monitor_prefix(text, "condition remove") ||
		monitor_prefix(text, "exception-mask") ||
		text == "history on" || text == "history off" ||
		text == "guest-output on" || text == "guest-output off" || text == "guest-output clear" ||
		monitor_prefix(text, "process-break name") || monitor_prefix(text, "process-break address") ||
		text == "process-break clear";
}

static bool monitor_reads_guest(const std::string& text)
{
	return monitor_prefix(text, "disasm") || text == "tasks" ||
		text == "segments" || monitor_prefix(text, "segments");
}

static std::string monitor_refusal(const std::string& text)
{
	// gdb_server.cpp refuses the threaded CPU because debug() and bank remaps
	// can run while the other thread is still executing. The same constraint
	// applies to MONITOR mutations and guest-memory reads.
	if (monitor_prefix(text, "input sequence"))
		return "input sequence is advanced only while a GDB client is connected; use input event";
	if (currprefs.cpu_thread && (monitor_changes_state(text) || monitor_reads_guest(text)))
		return "not available with the threaded CPU (cpu_threaded=true)";
	// A connected GDB client owns the shared monitor object, and its detach
	// clears all of it, held input included.
	if (gdb_server_connected() && (monitor_changes_state(text) ||
		monitor_prefix(text, "input event") || text == "input release"))
		return "a GDB client is connected; monitor state changes are refused";
	if (monitor_changes_state(text) && !pause_emulation && !debugger_is_stopped())
		return "monitor state changes require PAUSE or a stopped debugger";
	return {};
}

static std::vector<std::string> monitor_result_fields(const std::string& result)
{
	std::vector<std::string> fields;
	size_t start = 0;
	while (start < result.size()) {
		auto end = result.find('\n', start);
		if (end == std::string::npos)
			end = result.size();
		std::string line = result.substr(start, end - start);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (!line.empty()) {
			for (char& c : line) {
				if (c == '\t')
					c = ' ';
			}
			fields.push_back(std::move(line));
		}
		if (end == result.size())
			break;
		start = end + 1;
	}
	return fields;
}

static std::string HandleMonitor(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received MONITOR" << std::endl;
	}
	if (args.empty())
		return make_response(false, {"Usage: MONITOR <command> [args]"});

	std::string text;
	for (size_t i = 0; i < args.size(); i++) {
		if (i)
			text += ' ';
		text += args[i];
	}
	for (unsigned char c : text) {
		if (c < 0x20)
			return make_response(false, {"Invalid control character in monitor command"});
	}
	std::string refusal = monitor_refusal(text);
	if (!refusal.empty())
		return make_response(false, {refusal});

	std::string result, error;
	if (!gdb_monitor_command(text, result, error))
		return make_response(false, {error.empty() ? std::string("monitor command failed") : error});
	// Conditions, history and program-entry breaks all depend on
	// per-instruction trace checks; recompute them from every source.
	if (monitor_changes_state(text))
		debugger_breakpoints_changed();
	return make_response(true, monitor_result_fields(result));
}
#endif
#endif

static std::string HandleGetCPURegs(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_CPU_REGS" << std::endl;
	}

	std::vector<std::string> responses;

	// Data registers
	for (int i = 0; i < 8; i++) {
		char buf[32];
		snprintf(buf, sizeof(buf), "D%d=%08X", i, regs.regs[i]);
		responses.emplace_back(buf);
	}

	// Address registers
	for (int i = 0; i < 8; i++) {
		char buf[32];
		snprintf(buf, sizeof(buf), "A%d=%08X", i, regs.regs[8 + i]);
		responses.emplace_back(buf);
	}

	// Program counter and status
	char buf[64];
	snprintf(buf, sizeof(buf), "PC=%08X", M68K_GETPC);
	responses.emplace_back(buf);

	uae_u16 sr = regs.sr;
	snprintf(buf, sizeof(buf), "SR=%04X", sr);
	responses.emplace_back(buf);

	// Individual flags
	responses.push_back(std::string("T=") + std::to_string((sr >> 15) & 1));
	responses.push_back(std::string("S=") + std::to_string((sr >> 13) & 1));
	responses.push_back(std::string("X=") + std::to_string((sr >> 4) & 1));
	responses.push_back(std::string("N=") + std::to_string((sr >> 3) & 1));
	responses.push_back(std::string("Z=") + std::to_string((sr >> 2) & 1));
	responses.push_back(std::string("V=") + std::to_string((sr >> 1) & 1));
	responses.push_back(std::string("C=") + std::to_string(sr & 1));

	// Stack pointers
	snprintf(buf, sizeof(buf), "USP=%08X", regs.usp);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "ISP=%08X", regs.isp);
	responses.emplace_back(buf);

	return make_response(true, responses);
}

static std::string HandleGetCustomRegs(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_CUSTOM_REGS" << std::endl;
	}

	std::vector<std::string> responses;
	char buf[64];

	// Key custom chip registers
	snprintf(buf, sizeof(buf), "DMACON=%04X", dmacon);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "INTENA=%04X", intena);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "INTREQ=%04X", intreq);
	responses.emplace_back(buf);

	// DMA status breakdown
	responses.push_back(std::string("dma_master=") + ((dmacon & 0x200) ? "1" : "0"));
	responses.push_back(std::string("dma_blitter=") + ((dmacon & 0x40) ? "1" : "0"));
	responses.push_back(std::string("dma_copper=") + ((dmacon & 0x80) ? "1" : "0"));
	responses.push_back(std::string("dma_bitplane=") + ((dmacon & 0x100) ? "1" : "0"));
	responses.push_back(std::string("dma_disk=") + ((dmacon & 0x10) ? "1" : "0"));
	responses.push_back(std::string("dma_audio=") + std::to_string(dmacon & 0x0f));

	// Copper position
	snprintf(buf, sizeof(buf), "COP1LC=%08X", get_copper_address(1));
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "COP2LC=%08X", get_copper_address(2));
	responses.emplace_back(buf);

	return make_response(true, responses);
}

static std::string HandleDisassemble(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received DISASSEMBLE" << std::endl;
	}

	uaecptr addr = M68K_GETPC;  // Default to current PC
	int lines = 10;             // Default lines

	if (!args.empty()) {
		try {
			// Handle hex with or without 0x prefix
			const std::string& addr_str = args[0];
			if (addr_str.substr(0, 2) == "0x" || addr_str.substr(0, 2) == "0X") {
				addr = std::stoul(addr_str, nullptr, 16);
			} else {
				addr = std::stoul(addr_str, nullptr, 16);
			}
		} catch (...) {
			return make_response(false, {"Invalid address: " + args[0]});
		}
	}
	if (args.size() > 1) {
		try {
			lines = std::stoi(args[1]);
			if (lines < 1) lines = 1;
			if (lines > 50) lines = 50;  // Limit output
		} catch (...) {
			lines = 10;
		}
	}

	std::vector<std::string> responses;

#ifdef DEBUGGER
	// Use the disassembler
	for (int i = 0; i < lines; i++) {
		TCHAR buf[256];
		uaecptr next_addr = addr;
		m68k_disasm_2(buf, sizeof(buf) / sizeof(TCHAR), addr, nullptr, 0, &next_addr, 1,
			nullptr, nullptr, 0xffffffff, 0);
		responses.emplace_back(buf);
		addr = next_addr;
	}
#else
	// Simple fallback - just show hex bytes
	char buf[64];
	snprintf(buf, sizeof(buf), "Disassembly at %08X (DEBUGGER not compiled in)", addr);
	responses.emplace_back(buf);
#endif

	return make_response(true, responses);
}

#ifdef DEBUGGER
static std::string HandleSetBreakpoint(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SET_BREAKPOINT" << std::endl;
	}

	if (args.empty()) {
		return make_response(false, {"Usage: SET_BREAKPOINT <address> [slot]"});
	}

	uaecptr addr;
	try {
		const std::string& addr_str = args[0];
		if (addr_str.substr(0, 2) == "0x" || addr_str.substr(0, 2) == "0X") {
			addr = std::stoul(addr_str, nullptr, 16);
		} else {
			addr = std::stoul(addr_str, nullptr, 16);
		}
	} catch (...) {
		return make_response(false, {"Invalid address: " + args[0]});
	}

	int slot = -1;  // Find free slot
	if (args.size() > 1) {
		try {
			slot = std::stoi(args[1]);
			if (slot < 0 || slot >= BREAKPOINT_TOTAL) {
				return make_response(false, {"Slot must be 0-" + std::to_string(BREAKPOINT_TOTAL - 1)});
			}
		} catch (...) {
			slot = -1;
		}
		if (slot >= 0 && gdb_monitor_has_condition(slot)) {
			return make_response(false, {"Slot " + std::to_string(slot) + " is reserved for a monitor condition"});
		}
	}

	// Find a free slot if not specified
	if (slot < 0) {
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (!bpnodes[i].enabled && !gdb_monitor_has_condition(i)) {
				slot = i;
				break;
			}
		}
		if (slot < 0) {
			return make_response(false, {"No free breakpoint slots"});
		}
	}

	// Set the breakpoint
	bpnodes[slot].value1 = addr;
	bpnodes[slot].value2 = 0;
	bpnodes[slot].mask = 0xffffffff;
	bpnodes[slot].type = BREAKPOINT_REG_PC;
	bpnodes[slot].oper = BREAKPOINT_CMP_EQUAL;
	bpnodes[slot].opersigned = false;
	bpnodes[slot].cnt = 0;
	bpnodes[slot].chain = -1;
	bpnodes[slot].enabled = 1;
	debugger_breakpoints_changed();

	char buf[64];
	snprintf(buf, sizeof(buf), "Breakpoint set at %08X in slot %d", addr, slot);
	return make_response(true, {buf});
}

static std::string HandleClearBreakpoint(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received CLEAR_BREAKPOINT" << std::endl;
	}

	std::string selector = args.empty() ? "ALL" : args[0];
	std::transform(selector.begin(), selector.end(), selector.begin(), ::toupper);
	if (selector == "ALL") {
		// Clear all breakpoints
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (!gdb_monitor_has_condition(i)) {
				bpnodes[i].enabled = 0;
			}
		}
		debugger_breakpoints_changed();
		return make_response(true, {"All breakpoints cleared"});
	}

	uaecptr addr;
	try {
		addr = std::stoul(args[0], nullptr, 16);
	} catch (...) {
		return make_response(false, {"Invalid address: " + args[0]});
	}

	int cleared = 0;
	for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (bpnodes[i].enabled && bpnodes[i].type == BREAKPOINT_REG_PC && bpnodes[i].value1 == addr && !gdb_monitor_has_condition(i)) {
			bpnodes[i].enabled = 0;
			cleared++;
		}
	}
	if (cleared == 0) {
		return make_response(false, {"No breakpoint at address " + args[0]});
	}
	debugger_breakpoints_changed();

	char buf[64];
	snprintf(buf, sizeof(buf), "%d breakpoint(s) cleared at %08X", cleared, addr);
	return make_response(true, {buf});
}

static std::string HandleListBreakpoints(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received LIST_BREAKPOINTS" << std::endl;
	}

	std::vector<std::string> responses;
	int count = 0;

	for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (bpnodes[i].enabled && !gdb_monitor_has_condition(i)) {
			char buf[128];
			snprintf(buf, sizeof(buf), "slot=%d address=%08X type=%d enabled=1",
				i, bpnodes[i].value1, bpnodes[i].type);
			responses.emplace_back(buf);
			count++;
		}
	}

	if (count == 0) {
		responses.emplace_back("No breakpoints set");
	} else {
		responses.insert(responses.begin(), "count=" + std::to_string(count));
	}

	return make_response(true, responses);
}
#endif

static std::string HandleGetCopperState(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_COPPER_STATE" << std::endl;
	}

	std::vector<std::string> responses;
	char buf[64];

	snprintf(buf, sizeof(buf), "COP1LC=%08X", get_copper_address(1));
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "COP2LC=%08X", get_copper_address(2));
	responses.emplace_back(buf);

	// Copper DMA enabled?
	responses.push_back(std::string("enabled=") + ((dmacon & 0x80) && (dmacon & 0x200) ? "1" : "0"));

	return make_response(true, responses);
}

static std::string HandleGetBlitterState(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_BLITTER_STATE" << std::endl;
	}

	std::vector<std::string> responses;
	char buf[64];

	// Blitter control registers
	snprintf(buf, sizeof(buf), "BLTCON0=%04X", blt_info.bltcon0);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "BLTCON1=%04X", blt_info.bltcon1);
	responses.emplace_back(buf);

	// Blitter size
	snprintf(buf, sizeof(buf), "BLTSIZE_H=%d", blt_info.hblitsize);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "BLTSIZE_V=%d", blt_info.vblitsize);
	responses.emplace_back(buf);

	// Channel pointers
	snprintf(buf, sizeof(buf), "BLTAPT=%08X", blt_info.bltapt);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "BLTBPT=%08X", blt_info.bltbpt);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "BLTCPT=%08X", blt_info.bltcpt);
	responses.emplace_back(buf);
	snprintf(buf, sizeof(buf), "BLTDPT=%08X", blt_info.bltdpt);
	responses.emplace_back(buf);

	// Status
	responses.push_back(std::string("busy=") + (blt_info.blit_main ? "1" : "0"));
	responses.push_back(std::string("dma_enabled=") + ((dmacon & 0x40) && (dmacon & 0x200) ? "1" : "0"));

	return make_response(true, responses);
}

static std::string HandleGetDriveState(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_DRIVE_STATE" << std::endl;
	}

	int drive = -1;  // All drives by default
	if (!args.empty()) {
		try {
			drive = std::stoi(args[0]);
			if (drive < 0 || drive > 3) {
				return make_response(false, {"Drive must be 0-3"});
			}
		} catch (...) {
			return make_response(false, {"Invalid drive number"});
		}
	}

	std::vector<std::string> responses;

	int start = (drive < 0) ? 0 : drive;
	int end = (drive < 0) ? 3 : drive;

	for (int i = start; i <= end; i++) {
		char buf[256];
		snprintf(buf, sizeof(buf), "DF%d_motor=%d", i, gui_data.drives[i].drive_motor ? 1 : 0);
		responses.emplace_back(buf);
		snprintf(buf, sizeof(buf), "DF%d_track=%d", i, gui_data.drives[i].drive_track);
		responses.emplace_back(buf);
		snprintf(buf, sizeof(buf), "DF%d_writing=%d", i, gui_data.drives[i].drive_writing ? 1 : 0);
		responses.emplace_back(buf);
		snprintf(buf, sizeof(buf), "DF%d_disabled=%d", i, gui_data.drives[i].drive_disabled ? 1 : 0);
		responses.emplace_back(buf);
		snprintf(buf, sizeof(buf), "DF%d_protected=%d", i, gui_data.drives[i].floppy_protected ? 1 : 0);
		responses.emplace_back(buf);
		snprintf(buf, sizeof(buf), "DF%d_inserted=%d", i, gui_data.drives[i].floppy_inserted ? 1 : 0);
		responses.emplace_back(buf);

		std::string df = "DF" + std::to_string(i) + "_disk=";
		if (gui_data.drives[i].df[0]) {
			df += gui_data.drives[i].df;
		} else {
			df += "<empty>";
		}
		responses.push_back(df);
	}

	return make_response(true, responses);
}

static std::string HandleGetAudioState(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_AUDIO_STATE" << std::endl;
	}

	std::vector<std::string> responses;

	// Audio DMA enabled per channel
	responses.push_back(std::string("ch0_dma=") + ((dmacon & 0x01) ? "1" : "0"));
	responses.push_back(std::string("ch1_dma=") + ((dmacon & 0x02) ? "1" : "0"));
	responses.push_back(std::string("ch2_dma=") + ((dmacon & 0x04) ? "1" : "0"));
	responses.push_back(std::string("ch3_dma=") + ((dmacon & 0x08) ? "1" : "0"));
	responses.push_back(std::string("master=") + ((dmacon & 0x200) ? "1" : "0"));

	// Sndbuf status from GUI
	responses.push_back("sndbuf=" + std::to_string(gui_data.sndbuf));
	responses.push_back("sndbuf_status=" + std::to_string(gui_data.sndbuf_status));

	return make_response(true, responses);
}

static std::string HandleGetDMAState(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received GET_DMA_STATE" << std::endl;
	}

	std::vector<std::string> responses;
	char buf[64];

	snprintf(buf, sizeof(buf), "DMACON=%04X", dmacon);
	responses.emplace_back(buf);

	// Individual DMA channels
	responses.push_back(std::string("master=") + ((dmacon & 0x200) ? "1" : "0"));
	responses.push_back(std::string("blitter_priority=") + ((dmacon & 0x400) ? "1" : "0"));
	responses.push_back(std::string("blitter=") + ((dmacon & 0x40) ? "1" : "0"));
	responses.push_back(std::string("copper=") + ((dmacon & 0x80) ? "1" : "0"));
	responses.push_back(std::string("bitplane=") + ((dmacon & 0x100) ? "1" : "0"));
	responses.push_back(std::string("sprite=") + ((dmacon & 0x20) ? "1" : "0"));
	responses.push_back(std::string("disk=") + ((dmacon & 0x10) ? "1" : "0"));
	responses.push_back(std::string("audio0=") + ((dmacon & 0x01) ? "1" : "0"));
	responses.push_back(std::string("audio1=") + ((dmacon & 0x02) ? "1" : "0"));
	responses.push_back(std::string("audio2=") + ((dmacon & 0x04) ? "1" : "0"));
	responses.push_back(std::string("audio3=") + ((dmacon & 0x08) ? "1" : "0"));

	return make_response(true, responses);
}

static std::string HandleHelp(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received HELP" << std::endl;
	}
	std::vector<std::string> commands;
	commands.emplace_back("Available commands:");
	commands.emplace_back("QUIT, PAUSE, RESUME, RESET [HARD|SOFT]");
	commands.emplace_back("SCREENSHOT <path>, SAVESTATE <state> <cfg>, LOADSTATE <state>");
	commands.emplace_back("QUICKSAVE [slot], QUICKLOAD [slot] (slots 0-9)");
	commands.emplace_back("INSERTFLOPPY <path> <drive>, EJECT_FLOPPY <drive>, LIST_FLOPPIES");
	commands.emplace_back("SET_FLOPPY_SPEED <speed>, GET_FLOPPY_SPEED");
	commands.emplace_back("DISK_WRITE_PROTECT <drive> <0|1>, GET_DISK_WRITE_PROTECT <drive>");
	commands.emplace_back("INSERTCD <path>, EJECT_CD");
	commands.emplace_back("LIST_HARDDRIVES");
	commands.emplace_back("DISKSWAP <disknum> <drive>, QUERYDISKSWAP <drive>");
	commands.emplace_back("GET_STATUS, GET_CONFIG <opt>, SET_CONFIG <opt> <val>");
	commands.emplace_back("LOAD_CONFIG <path>, LIST_CONFIGS");
	commands.emplace_back("LIST_MEDIA <list>, GET_FOLDER <folder>, SET_FOLDER <folder> <path>");
	commands.emplace_back("SET_VOLUME <0-100>, GET_VOLUME, MUTE, UNMUTE");
	commands.emplace_back("SET_WARP <0|1>, GET_WARP, TOGGLE_FULLSCREEN");
	commands.emplace_back("SET_DISPLAY_MODE <0-2>, GET_DISPLAY_MODE, TOGGLE_RTG");
	commands.emplace_back("SET_NTSC <0|1>, GET_NTSC, TOGGLE_STATUS_LINE");
	commands.emplace_back("SET_SOUND_MODE <0-3>, GET_SOUND_MODE");
	commands.emplace_back("SET_CHIPSET <OCS|ECS|AGA>, GET_CHIPSET");
	commands.emplace_back("SET_CPU_SPEED <speed>, GET_CPU_SPEED");
	commands.emplace_back("GET_CPU_MODEL, SET_CPU_MODEL <68000-68060>");
	commands.emplace_back("GET_MEMORY_CONFIG, GET_FPS");
	commands.emplace_back("SET_CHIP_MEM <kb>, SET_FAST_MEM <kb>, SET_SLOW_MEM <kb>, SET_Z3_MEM <mb>");
	commands.emplace_back("SET_WINDOW_SIZE <w> <h>, GET_WINDOW_SIZE");
	commands.emplace_back("SET_SCALING <-1..2>, GET_SCALING");
	commands.emplace_back("SET_LINE_MODE <0-2>, GET_LINE_MODE");
	commands.emplace_back("SET_RESOLUTION <0-2>, GET_RESOLUTION");
	commands.emplace_back("GET_JOYPORT_MODE <port>, SET_JOYPORT_MODE <port> <mode>");
	commands.emplace_back("GET_AUTOFIRE <port>, SET_AUTOFIRE <port> <mode>");
	commands.emplace_back("GET_LED_STATUS");
	commands.emplace_back("FRAME_ADVANCE [n], SET_MOUSE_SPEED <10-200>, GET_MOUSE_SPEED");
	commands.emplace_back("TOGGLE_MOUSE_GRAB");
	commands.emplace_back("SEND_KEY <code> <state>, SEND_MOUSE <dx> <dy> <buttons>");
	commands.emplace_back("SEND_MOUSE_ABS <x> <y> <buttons> (requires tablet_mode mousehack or real)");
	commands.emplace_back("--- GUI automation controller commands (prefer MCP/HTTP) ---");
	commands.emplace_back("SCREENSHOT <path> ACTIONABLE (SDL/OpenGL; unavailable on Vulkan)");
	commands.emplace_back("GET_GUI_AUTOMATION_STATE");
	commands.emplace_back("SET_GUI_AUTOMATION_CONFIG <expected_tablet_mode> <expected_mouse_untrap> <expected_input_config_revision> <tablet_mode> <mouse_untrap>");
	commands.emplace_back("SEND_MOUSE_ABS_GUARDED <x> <y> <button_mask> <runtime_id> <geometry_revision> <monitor_id> <input_config_revision>");
	commands.emplace_back("RELEASE_MOUSE_BUTTONS");
	commands.emplace_back("READ_MEM <addr> <width>, WRITE_MEM <addr> <width> <val>");
	commands.emplace_back("SET_AUTOCROP <0|1>, GET_AUTOCROP");
	commands.emplace_back("INSERT_WHDLOAD <path>, EJECT_WHDLOAD, GET_WHDLOAD");
#ifdef DEBUGGER
	commands.emplace_back("--- Debugging Commands ---");
	commands.emplace_back("DEBUG_ACTIVATE, DEBUG_DEACTIVATE, DEBUG_STATUS");
	commands.emplace_back("DEBUG_STEP [count], DEBUG_STEP_OVER, DEBUG_CONTINUE");
#ifdef GDBSERVER
	commands.emplace_back("MONITOR <text> - GDB monitor command; lines returned as fields");
#endif
	commands.emplace_back("GET_CPU_REGS, GET_CUSTOM_REGS");
	commands.emplace_back("DISASSEMBLE <addr> [count] - disassemble instructions");
	commands.emplace_back("SET_BREAKPOINT <addr>, CLEAR_BREAKPOINT <addr|ALL>, LIST_BREAKPOINTS");
	commands.emplace_back("GET_COPPER_STATE, GET_BLITTER_STATE");
	commands.emplace_back("GET_DRIVE_STATE [drive], GET_AUDIO_STATE, GET_DMA_STATE");
#endif
	commands.emplace_back("GET_VERSION, PING, HELP");
	commands.emplace_back("SETTINGS_GET <id>");
	commands.emplace_back("SETTINGS_SET <id> <value>");
	commands.emplace_back("SETTINGS_VALIDATE");
	commands.emplace_back("SETTINGS_COMMIT");
	commands.emplace_back("SETTINGS_RESET_SESSION");
	commands.emplace_back("SETTINGS_GET_ACTIVE <id>");
	commands.emplace_back("SETTINGS_LIST");
	commands.emplace_back("SETTINGS_DESCRIBE <id>");
	commands.emplace_back("SETTINGS_APPLY <live|reset|restart>");
	commands.emplace_back("SETTINGS_MOUNTS");
	commands.emplace_back("SETTINGS_MOUNT_ADD <uaehf value>");
	commands.emplace_back("SETTINGS_MOUNT_REPLACE <index> <uaehf value>");
	commands.emplace_back("SETTINGS_MOUNT_REMOVE <index>");
	return make_response(true, commands);
}

// Initialize command handlers
// === Settings edit session commands ===
// One session owned by the IPC server, executed on the main thread like
// every other handler. Lazily opened; SETTINGS_RESET_SESSION discards it.
// One session for the whole IPC server (all clients share it). It is
// reopened whenever it holds no uncommitted SETTINGS_SET edits, so a
// configuration loaded or changed through any other command (LOAD_CONFIG,
// SET_CONFIG, the GUI) is what the next SETTINGS_* command sees, and it is
// closed after SETTINGS_COMMIT and SETTINGS_RESET_SESSION.
static amiberry_settings_session* ipc_settings_session;
static bool ipc_settings_dirty;

static void close_settings_session()
{
	if (ipc_settings_session) {
		amiberry_settings_session_close(ipc_settings_session);
		ipc_settings_session = nullptr;
	}
	ipc_settings_dirty = false;
}

static amiberry_settings_session* settings_session()
{
	if (ipc_settings_session && !ipc_settings_dirty)
		close_settings_session();
	if (!ipc_settings_session) {
		ipc_settings_session = amiberry_settings_session_open();
		ipc_settings_dirty = false;
	}
	return ipc_settings_session;
}

static std::string settings_status_message(amiberry_settings_status status,
	const std::string& id, const std::string& value)
{
	switch (status) {
	case AMIBERRY_SETTINGS_OK:
		return "OK";
	case AMIBERRY_SETTINGS_UNKNOWN_ID:
		return "Unknown option: " + id;
	case AMIBERRY_SETTINGS_INVALID_VALUE:
		return "Invalid value for " + id + ": " + value;
	case AMIBERRY_SETTINGS_UNAVAILABLE:
		return "Option unavailable now: " + id;
	case AMIBERRY_SETTINGS_BUFFER_TOO_SMALL:
		return "Value too long";
	}
	return "Settings error";
}

// Reads a setting into value with a buffer that grows until it fits: PATH
// and STRING values can be longer than AMIBERRY_SETTINGS_VALUE_MAX.
template <typename Get>
static amiberry_settings_status read_setting(std::string& value, Get&& get)
{
	std::vector<char> buffer(AMIBERRY_SETTINGS_VALUE_MAX);
	for (;;) {
		const amiberry_settings_status status = get(buffer.data(), buffer.size());
		if (status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL || buffer.size() >= 65536) {
			if (status == AMIBERRY_SETTINGS_OK)
				value = buffer.data();
			return status;
		}
		buffer.resize(buffer.size() * 4);
	}
}

static std::string HandleSettingsGet(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_GET" << std::endl;
	}
	if (args.size() < 1) {
		return make_response(false, {"Usage: SETTINGS_GET <id>"});
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	std::string value;
	const amiberry_settings_status status = read_setting(value, [&](char* buffer, size_t size) {
		return amiberry_settings_get(session, args[0].c_str(), buffer, size);
	});
	if (status != AMIBERRY_SETTINGS_OK) {
		return make_response(false, {settings_status_message(status, args[0], "")});
	}
	return make_response(true, {args[0] + "=" + value});
}

static std::string HandleSettingsSet(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_SET" << std::endl;
	}
	if (args.size() < 2) {
		return make_response(false, {"Usage: SETTINGS_SET <id> <value>"});
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	const amiberry_settings_status status =
		amiberry_settings_set(session, args[0].c_str(), args[1].c_str());
	if (status != AMIBERRY_SETTINGS_OK) {
		return make_response(false, {settings_status_message(status, args[0], args[1])});
	}
	ipc_settings_dirty = true;
	return make_response(true);
}

static std::string HandleSettingsValidate(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_VALIDATE" << std::endl;
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	const size_t option_count = amiberry_settings_count();
	std::vector<amiberry_settings_adjustment> adjustments(option_count);
	const size_t found = amiberry_settings_validate(session,
		adjustments.empty() ? nullptr : adjustments.data(), adjustments.size());
	if (found == 0) {
		return make_response(true, {"none"});
	}
	std::vector<std::string> fields;
	for (size_t i = 0; i < found && i < adjustments.size(); ++i) {
		fields.push_back(std::string(adjustments[i].id)
			+ "=" + adjustments[i].requested
			+ "->" + adjustments[i].adjusted
			+ " (" + (adjustments[i].reason ? adjustments[i].reason : "") + ")");
	}
	return make_response(true, fields);
}

static const char* apply_class_name(amiberry_apply_class apply)
{
	switch (apply) {
	case AMIBERRY_APPLY_LIVE: return "live";
	case AMIBERRY_APPLY_RESET: return "reset";
	case AMIBERRY_APPLY_RESTART: return "restart";
	}
	return "restart";
}

static std::string HandleSettingsCommit(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_COMMIT" << std::endl;
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	bool changed = false;
	const amiberry_apply_class apply = amiberry_settings_commit(session, &changed);
	close_settings_session();
	if (changed) {
		// Like SET_CONFIG: make the running emulation pick up what changed.
		set_config_changed();
	}
	return make_response(true, {
		std::string("changed=") + (changed ? "true" : "false"),
		std::string("apply=") + apply_class_name(apply)
	});
}

// A mount list index as SETTINGS_MOUNT_REPLACE and SETTINGS_MOUNT_REMOVE
// take it: decimal digits only.
static bool parse_mount_index(const std::string& text, size_t& index)
{
	if (text.empty() || text.size() > 4
		|| !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
		return false;
	index = static_cast<size_t>(std::stoul(text));
	return true;
}

static std::string mount_status_message(amiberry_settings_status status, const std::string& subject)
{
	switch (status) {
	case AMIBERRY_SETTINGS_OK:
		return "OK";
	case AMIBERRY_SETTINGS_UNKNOWN_ID:
		return "No mount entry " + subject;
	case AMIBERRY_SETTINGS_INVALID_VALUE:
		return "Invalid mount entry: " + subject;
	case AMIBERRY_SETTINGS_UNAVAILABLE:
		return "The mount list is full";
	case AMIBERRY_SETTINGS_BUFFER_TOO_SMALL:
		return "Mount entry too long";
	}
	return "Settings error";
}

static std::string HandleSettingsMounts(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_MOUNTS" << std::endl;
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	std::vector<std::string> entries;
	const size_t count = amiberry_settings_mount_count(session);
	for (size_t i = 0; i < count; ++i) {
		char value[CONFIG_BLEN];
		if (amiberry_settings_mount_get(session, i, value, sizeof value) != AMIBERRY_SETTINGS_OK) {
			return make_response(false, {"Mount entry " + std::to_string(i) + " could not be read"});
		}
		const std::string entry = value;
		// A tab or line break would split the reply; such an entry cannot
		// be represented in this protocol.
		if (entry.find_first_of("\t\r\n") != std::string::npos) {
			return make_response(false, {"Mount entry " + std::to_string(i) + " contains a tab or line break"});
		}
		entries.push_back(entry);
	}
	if (entries.empty()) {
		return make_response(true, {"none"});
	}
	return make_response(true, entries);
}

static std::string HandleSettingsMountAdd(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_MOUNT_ADD" << std::endl;
	}
	if (args.size() != 1) {
		return make_response(false, {"Usage: SETTINGS_MOUNT_ADD <uaehf value>"});
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	const amiberry_settings_status status = amiberry_settings_mount_add(session, args[0].c_str());
	if (status != AMIBERRY_SETTINGS_OK) {
		return make_response(false, {mount_status_message(status, args[0])});
	}
	ipc_settings_dirty = true;
	return make_response(true);
}

static std::string HandleSettingsMountReplace(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_MOUNT_REPLACE" << std::endl;
	}
	size_t index = 0;
	if (args.size() != 2 || !parse_mount_index(args[0], index)) {
		return make_response(false, {"Usage: SETTINGS_MOUNT_REPLACE <index> <uaehf value>"});
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	const amiberry_settings_status status = amiberry_settings_mount_replace(session, index, args[1].c_str());
	if (status != AMIBERRY_SETTINGS_OK) {
		return make_response(false, {mount_status_message(status,
			status == AMIBERRY_SETTINGS_UNKNOWN_ID ? args[0] : args[1])});
	}
	ipc_settings_dirty = true;
	return make_response(true);
}

static std::string HandleSettingsMountRemove(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_MOUNT_REMOVE" << std::endl;
	}
	size_t index = 0;
	if (args.size() != 1 || !parse_mount_index(args[0], index)) {
		return make_response(false, {"Usage: SETTINGS_MOUNT_REMOVE <index>"});
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	const amiberry_settings_status status = amiberry_settings_mount_remove(session, index);
	if (status != AMIBERRY_SETTINGS_OK) {
		return make_response(false, {mount_status_message(status, args[0])});
	}
	ipc_settings_dirty = true;
	return make_response(true);
}

static std::string HandleSettingsList(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_LIST" << std::endl;
	}
	std::vector<std::string> ids;
	for (size_t i = 0; i < amiberry_settings_count(); ++i) {
		ids.emplace_back(amiberry_settings_at(i)->id);
	}
	return make_response(true, ids);
}

static const char* setting_type_name(amiberry_setting_type type)
{
	switch (type) {
	case AMIBERRY_SETTING_BOOL: return "bool";
	case AMIBERRY_SETTING_INT: return "int";
	case AMIBERRY_SETTING_REAL: return "real";
	case AMIBERRY_SETTING_CHOICE: return "choice";
	case AMIBERRY_SETTING_PATH: return "path";
	case AMIBERRY_SETTING_STRING: return "string";
	}
	return "string";
}

// The shortest spelling that reads back as v, whole numbers without an
// exponent (60000, 0.1, 1e-05).
static std::string number_text(double v)
{
	char text[64];
	if (v == std::floor(v) && std::fabs(v) < 1e15) {
		std::snprintf(text, sizeof text, "%.0f", v);
		return text;
	}
	for (int precision = 1; precision <= 17; ++precision) {
		std::snprintf(text, sizeof text, "%.*g", precision, v);
		if (std::strtod(text, nullptr) == v)
			break;
	}
	return text;
}

static std::string HandleSettingsDescribe(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_DESCRIBE" << std::endl;
	}
	if (args.size() != 1) {
		return make_response(false, {"Usage: SETTINGS_DESCRIBE <id>"});
	}
	const amiberry_setting_descriptor* d = amiberry_settings_find(args[0].c_str());
	if (!d) {
		return make_response(false, {"Unknown option: " + args[0]});
	}
	amiberry_settings_session* session = settings_session();
	if (!session) {
		return make_response(false, {"Out of memory"});
	}
	std::vector<std::string> fields;
	fields.push_back(std::string("type=") + setting_type_name(d->type));
	fields.push_back(std::string("apply=") + apply_class_name(d->apply));
	fields.push_back(std::string("group=") + d->group);
	fields.push_back(std::string("default=") + (d->default_value ? d->default_value : ""));
	fields.push_back(std::string("available=")
		+ (amiberry_settings_is_available(session, d->id) ? "true" : "false"));
	double min = 0, max = 0;
	if (amiberry_settings_get_range(session, d->id, &min, &max)) {
		fields.push_back("min=" + number_text(min));
		fields.push_back("max=" + number_text(max));
		fields.push_back("step=" + number_text(d->step));
	}
	// The choices the option offers now, with whether each is available.
	// Copied first: the list may be rebuilt by the next layer call (host
	// device lists are).
	const amiberry_setting_choice* choices = nullptr;
	const size_t count = amiberry_settings_choices(session, d->id, &choices);
	std::vector<std::string> values;
	for (size_t i = 0; i < count; ++i) {
		values.emplace_back(choices[i].value);
	}
	for (const std::string& value : values) {
		const bool available = amiberry_settings_choice_is_available(session, d->id, value.c_str());
		fields.push_back(std::string(available ? "choice=" : "unavailable_choice=") + value);
	}
	// A tab or line break in a field would split the reply.
	for (const std::string& field : fields) {
		if (field.find_first_of("\t\r\n") != std::string::npos) {
			return make_response(false, {"Description of " + args[0] + " contains a tab or line break"});
		}
	}
	return make_response(true, fields);
}

static std::string HandleSettingsGetActive(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_GET_ACTIVE" << std::endl;
	}
	if (args.size() < 1) {
		return make_response(false, {"Usage: SETTINGS_GET_ACTIVE <id>"});
	}
	std::string value;
	const amiberry_settings_status status = read_setting(value, [&](char* buffer, size_t size) {
		return amiberry_settings_get_active(args[0].c_str(), buffer, size);
	});
	if (status != AMIBERRY_SETTINGS_OK) {
		return make_response(false, {settings_status_message(status, args[0], "")});
	}
	return make_response(true, {args[0] + "=" + value});
}

static std::string HandleSettingsApply(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_APPLY" << std::endl;
	}
	if (args.size() != 1) {
		return make_response(false, {"Usage: SETTINGS_APPLY <live|reset|restart>"});
	}
	std::string name = args[0];
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	amiberry_apply_class apply;
	if (name == "live") {
		apply = AMIBERRY_APPLY_LIVE;
	} else if (name == "reset") {
		apply = AMIBERRY_APPLY_RESET;
	} else if (name == "restart") {
		apply = AMIBERRY_APPLY_RESTART;
	} else {
		return make_response(false, {"Usage: SETTINGS_APPLY <live|reset|restart>"});
	}
	return run_posted_reply(amiberry_actions_apply_settings(apply));
}

static std::string HandleSettingsResetSession(const std::vector<std::string>& args)
{
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Received SETTINGS_RESET_SESSION" << std::endl;
	}
	close_settings_session();
	return make_response(true);
}

static void InitHandlers()
{
	if (!command_handlers.empty()) return;

	command_handlers[CMD_QUIT] = HandleQuit;
	command_handlers[CMD_PAUSE] = HandlePause;
	command_handlers[CMD_RESUME] = HandleResume;
	command_handlers[CMD_RESET] = HandleReset;
	command_handlers[CMD_SCREENSHOT] = HandleScreenshot;
	command_handlers[CMD_SAVESTATE] = HandleSavestate;
	command_handlers[CMD_LOADSTATE] = HandleLoadState;
	command_handlers[CMD_DISKSWAP] = HandleDiskSwap;
	command_handlers[CMD_QUERYDISKSWAP] = HandleQueryDiskSwap;
	command_handlers[CMD_INSERTFLOPPY] = HandleInsertFloppy;
	command_handlers[CMD_INSERTCD] = HandleInsertCD;
	command_handlers[CMD_GET_STATUS] = HandleGetStatus;
	command_handlers[CMD_GET_CONFIG] = HandleGetConfig;
	command_handlers[CMD_SET_CONFIG] = HandleSetConfig;
	command_handlers[CMD_LOAD_CONFIG] = HandleLoadConfig;
	command_handlers[CMD_SEND_KEY] = HandleSendKey;
	command_handlers[CMD_READ_MEM] = HandleReadMem;
	command_handlers[CMD_WRITE_MEM] = HandleWriteMem;

	command_handlers[CMD_EJECT_FLOPPY] = HandleEjectFloppy;
	command_handlers[CMD_EJECT_CD] = HandleEjectCD;
	command_handlers[CMD_SET_VOLUME] = HandleSetVolume;
	command_handlers[CMD_GET_VOLUME] = HandleGetVolume;
	command_handlers[CMD_MUTE] = HandleMute;
	command_handlers[CMD_UNMUTE] = HandleUnmute;
	command_handlers[CMD_TOGGLE_FULLSCREEN] = HandleToggleFullscreen;
	command_handlers[CMD_SET_WARP] = HandleSetWarp;
	command_handlers[CMD_GET_WARP] = HandleGetWarp;
	command_handlers[CMD_GET_VERSION] = HandleGetVersion;
	command_handlers[CMD_LIST_FLOPPIES] = HandleListFloppies;
	command_handlers[CMD_LIST_CONFIGS] = HandleListConfigs;
	command_handlers[CMD_FRAME_ADVANCE] = HandleFrameAdvance;
	command_handlers[CMD_SET_MOUSE_SPEED] = HandleSetMouseSpeed;
	command_handlers[CMD_SEND_MOUSE] = HandleSendMouse;
	command_handlers[CMD_SEND_MOUSE_ABS] = HandleSendMouseAbs;
	command_handlers[CMD_GET_GUI_AUTOMATION_STATE] = HandleGetGuiAutomationState;
	command_handlers[CMD_SET_GUI_AUTOMATION_CONFIG] = HandleSetGuiAutomationConfig;
	command_handlers[CMD_SEND_MOUSE_ABS_GUARDED] = HandleSendMouseAbsGuarded;
	command_handlers[CMD_RELEASE_MOUSE_BUTTONS] = HandleReleaseMouseButtons;
	command_handlers[CMD_PING] = HandlePing;
	command_handlers[CMD_SETTINGS_GET] = HandleSettingsGet;
	command_handlers[CMD_SETTINGS_SET] = HandleSettingsSet;
	command_handlers[CMD_SETTINGS_VALIDATE] = HandleSettingsValidate;
	command_handlers[CMD_SETTINGS_COMMIT] = HandleSettingsCommit;
	command_handlers[CMD_SETTINGS_RESET_SESSION] = HandleSettingsResetSession;
	command_handlers[CMD_SETTINGS_GET_ACTIVE] = HandleSettingsGetActive;
	command_handlers[CMD_SETTINGS_LIST] = HandleSettingsList;
	command_handlers[CMD_SETTINGS_DESCRIBE] = HandleSettingsDescribe;
	command_handlers[CMD_SETTINGS_APPLY] = HandleSettingsApply;
	command_handlers[CMD_LIST_MEDIA] = HandleListMedia;
	command_handlers[CMD_GET_FOLDER] = HandleGetFolder;
	command_handlers[CMD_SET_FOLDER] = HandleSetFolder;
	command_handlers[CMD_SETTINGS_MOUNTS] = HandleSettingsMounts;
	command_handlers[CMD_SETTINGS_MOUNT_ADD] = HandleSettingsMountAdd;
	command_handlers[CMD_SETTINGS_MOUNT_REPLACE] = HandleSettingsMountReplace;
	command_handlers[CMD_SETTINGS_MOUNT_REMOVE] = HandleSettingsMountRemove;
	command_handlers[CMD_HELP] = HandleHelp;

	command_handlers[CMD_QUICKSAVE] = HandleQuickSave;
	command_handlers[CMD_QUICKLOAD] = HandleQuickLoad;
	command_handlers[CMD_GET_JOYPORT_MODE] = HandleGetJoyportMode;
	command_handlers[CMD_SET_JOYPORT_MODE] = HandleSetJoyportMode;
	command_handlers[CMD_GET_AUTOFIRE] = HandleGetAutofire;
	command_handlers[CMD_SET_AUTOFIRE] = HandleSetAutofire;
	command_handlers[CMD_GET_LED_STATUS] = HandleGetLEDStatus;
	command_handlers[CMD_LIST_HARDDRIVES] = HandleListHarddrives;
	command_handlers[CMD_SET_DISPLAY_MODE] = HandleSetDisplayMode;
	command_handlers[CMD_GET_DISPLAY_MODE] = HandleGetDisplayMode;
	command_handlers[CMD_SET_NTSC] = HandleSetNTSC;
	command_handlers[CMD_GET_NTSC] = HandleGetNTSC;
	command_handlers[CMD_SET_SOUND_MODE] = HandleSetSoundMode;
	command_handlers[CMD_GET_SOUND_MODE] = HandleGetSoundMode;

	command_handlers[CMD_TOGGLE_MOUSE_GRAB] = HandleToggleMouseGrab;
	command_handlers[CMD_GET_MOUSE_SPEED] = HandleGetMouseSpeed;
	command_handlers[CMD_SET_CPU_SPEED] = HandleSetCPUSpeed;
	command_handlers[CMD_GET_CPU_SPEED] = HandleGetCPUSpeed;
	command_handlers[CMD_TOGGLE_RTG] = HandleToggleRTG;
	command_handlers[CMD_SET_FLOPPY_SPEED] = HandleSetFloppySpeed;
	command_handlers[CMD_GET_FLOPPY_SPEED] = HandleGetFloppySpeed;
	command_handlers[CMD_DISK_WRITE_PROTECT] = HandleDiskWriteProtect;
	command_handlers[CMD_GET_DISK_WRITE_PROTECT] = HandleGetDiskWriteProtect;
	command_handlers[CMD_TOGGLE_STATUS_LINE] = HandleToggleStatusLine;
	command_handlers[CMD_SET_CHIPSET] = HandleSetChipset;
	command_handlers[CMD_GET_CHIPSET] = HandleGetChipset;
	command_handlers[CMD_GET_MEMORY_CONFIG] = HandleGetMemoryConfig;
	command_handlers[CMD_GET_FPS] = HandleGetFPS;

	// Memory configuration and Window control
	command_handlers[CMD_SET_CHIP_MEM] = HandleSetChipMem;
	command_handlers[CMD_SET_FAST_MEM] = HandleSetFastMem;
	command_handlers[CMD_SET_SLOW_MEM] = HandleSetSlowMem;
	command_handlers[CMD_SET_Z3_MEM] = HandleSetZ3Mem;
	command_handlers[CMD_GET_CPU_MODEL] = HandleGetCPUModel;
	command_handlers[CMD_SET_CPU_MODEL] = HandleSetCPUModel;
	command_handlers[CMD_SET_WINDOW_SIZE] = HandleSetWindowSize;
	command_handlers[CMD_GET_WINDOW_SIZE] = HandleGetWindowSize;
	command_handlers[CMD_SET_SCALING] = HandleSetScaling;
	command_handlers[CMD_GET_SCALING] = HandleGetScaling;
	command_handlers[CMD_SET_LINE_MODE] = HandleSetLineMode;
	command_handlers[CMD_GET_LINE_MODE] = HandleGetLineMode;
	command_handlers[CMD_SET_RESOLUTION] = HandleSetResolution;
	command_handlers[CMD_GET_RESOLUTION] = HandleGetResolution;

	// Autocrop and WHDLoad
	command_handlers[CMD_SET_AUTOCROP] = HandleSetAutocrop;
	command_handlers[CMD_GET_AUTOCROP] = HandleGetAutocrop;
	command_handlers[CMD_INSERT_WHDLOAD] = HandleInsertWHDLoad;
	command_handlers[CMD_EJECT_WHDLOAD] = HandleEjectWHDLoad;
	command_handlers[CMD_GET_WHDLOAD] = HandleGetWHDLoad;

	// Debugging and diagnostics
#ifdef DEBUGGER
	command_handlers[CMD_DEBUG_ACTIVATE] = HandleDebugActivate;
	command_handlers[CMD_DEBUG_DEACTIVATE] = HandleDebugDeactivate;
	command_handlers[CMD_DEBUG_STATUS] = HandleDebugStatus;
	command_handlers[CMD_DEBUG_STEP] = HandleDebugStep;
	command_handlers[CMD_DEBUG_STEP_OVER] = HandleDebugStepOver;
	command_handlers[CMD_DEBUG_CONTINUE] = HandleDebugContinue;
#ifdef GDBSERVER
	command_handlers[CMD_MONITOR] = HandleMonitor;
#endif
	command_handlers[CMD_GET_CPU_REGS] = HandleGetCPURegs;
	command_handlers[CMD_GET_CUSTOM_REGS] = HandleGetCustomRegs;
	command_handlers[CMD_DISASSEMBLE] = HandleDisassemble;
	command_handlers[CMD_SET_BREAKPOINT] = HandleSetBreakpoint;
	command_handlers[CMD_CLEAR_BREAKPOINT] = HandleClearBreakpoint;
	command_handlers[CMD_LIST_BREAKPOINTS] = HandleListBreakpoints;
	command_handlers[CMD_GET_COPPER_STATE] = HandleGetCopperState;
	command_handlers[CMD_GET_BLITTER_STATE] = HandleGetBlitterState;
	command_handlers[CMD_GET_DRIVE_STATE] = HandleGetDriveState;
	command_handlers[CMD_GET_AUDIO_STATE] = HandleGetAudioState;
	command_handlers[CMD_GET_DMA_STATE] = HandleGetDMAState;
#endif
}

// Process a single command line
static std::string ProcessCommand(const std::string& line)
{
	// Parse: COMMAND\tARG1\tARG2...
	std::vector<std::string> parts = split(line, '\t');
	if (parts.empty()) {
		return make_response(false, {"Empty command"});
	}

	std::string cmd = parts[0];
	// Convert to uppercase
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

	std::vector<std::string> args(parts.begin() + 1, parts.end());
	// split() drops a trailing empty field, so `SETTINGS_SET\tppc_model\t`
	// would lose its value. An empty string is a real settings value (for
	// example ppc_model= turns PPC emulation off), so keep it for that
	// command only; every other command parses exactly as before.
	if (cmd == "SETTINGS_SET" && !line.empty() && line.back() == '\t')
		args.emplace_back();

	const auto it = command_handlers.find(cmd);
	if (it != command_handlers.end()) {
		return it->second(args);
	}

	return make_response(false, {"Unknown command: " + cmd});
}

// Transport. POSIX builds listen on a Unix-domain socket. Windows builds listen
// on a named pipe: CPython, Node and Windows PowerShell can all open a named
// pipe, but none of them can connect to an AF_UNIX socket on Windows. Both are
// driven from the emulation thread without blocking, through the Conn*,
// AcceptClients and WaitForActivity functions; request handling is shared.

enum class IoStatus {
	Ok,         // data moved
	WouldBlock, // nothing more can move until the peer acts
	Closed,     // end of stream or connection error
};

#ifdef _WIN32

static constexpr DWORD IPC_PIPE_BUFFER_SIZE = 64 * 1024;

// One pipe instance, first waiting for a client and then connected to it.
// Overlapped operations keep pointers into this object until they complete,
// so it is heap allocated and never moves. The pending connect uses read_ov.
struct PipeConnection {
	HANDLE pipe = INVALID_HANDLE_VALUE;
	OVERLAPPED read_ov{};
	OVERLAPPED write_ov{};
	bool read_pending = false;
	bool write_pending = false;
	char read_buffer[4096]{};
	std::string write_buffer;

	PipeConnection()
	{
		read_ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		write_ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	}
	PipeConnection(const PipeConnection&) = delete;
	PipeConnection& operator=(const PipeConnection&) = delete;
	~PipeConnection()
	{
		if (pipe != INVALID_HANDLE_VALUE) {
			// Bytes the pipe has accepted stay readable by the client after the
			// handle is closed; only operations still in flight are dropped.
			if (read_pending || write_pending) {
				CancelIoEx(pipe, nullptr);
				DWORD ignored;
				if (read_pending) {
					GetOverlappedResult(pipe, &read_ov, &ignored, TRUE);
				}
				if (write_pending) {
					GetOverlappedResult(pipe, &write_ov, &ignored, TRUE);
				}
			}
			CloseHandle(pipe);
		}
		if (read_ov.hEvent) {
			CloseHandle(read_ov.hEvent);
		}
		if (write_ov.hEvent) {
			CloseHandle(write_ov.hEvent);
		}
	}
};

using IPCConnection = std::unique_ptr<PipeConnection>;
#else
using IPCConnection = int;
#endif

// A connected controller. Connections stay open across IPCHandle() calls
// until the client closes them, so a controller can keep one connection for
// many requests or open one connection per request.
struct IPCClient {
	IPCConnection conn;
	std::string pending; // received bytes not handled yet
	std::string output;  // reply bytes the connection has not accepted yet
	bool eof;            // the client has stopped sending
};

static std::vector<IPCClient> ipc_clients;
static constexpr size_t IPC_MAX_CLIENTS = 16;
using IPCReadyFlags = std::array<bool, IPC_MAX_CLIENTS>;

// Commands run on the emulation thread, which calls IPCHandle() once per
// frame. After handling a request, keep listening briefly for the next one so
// a controller that sends requests back to back is not limited to one request
// per frame. No new command starts after IPC_MAX_SERVICE_TIME, except that
// each client with a request waiting gets at least one per pass.
static constexpr std::chrono::milliseconds IPC_FOLLOWUP_WAIT{2};
static constexpr std::chrono::milliseconds IPC_MAX_SERVICE_TIME{5};

// Upper bound on reads (4 KiB each) from one client per service pass, so a
// client that writes continuously cannot hold the emulation thread.
static constexpr int IPC_MAX_READS_PER_PASS = 16;

// Longest request line accepted. A client that sends more than this without a
// newline is disconnected (after a best-effort error reply), so a malformed
// controller cannot grow the receive buffer without limit.
static constexpr size_t IPC_MAX_REQUEST_BYTES = 64 * 1024;

static bool OutputBusy(const IPCClient& client);

#ifdef _WIN32

// The instance waiting for the next client.
static IPCConnection pipe_listener;

// Grants pipe access to the account Amiberry runs as and nobody else, like the
// per-user XDG_RUNTIME_DIR that holds the Unix socket. The default pipe DACL
// would let every local account open read-only connections and use up the
// client slots.
static PSECURITY_DESCRIPTOR pipe_security = nullptr;

static bool CreatePipeSecurity()
{
	HANDLE token;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		return false;
	}
	DWORD size = 0;
	GetTokenInformation(token, TokenUser, nullptr, 0, &size);
	std::vector<BYTE> user(size);
	const bool have_user = size != 0
		&& GetTokenInformation(token, TokenUser, user.data(), size, &size);
	CloseHandle(token);
	LPSTR sid = nullptr;
	if (!have_user
		|| !ConvertSidToStringSidA(reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid, &sid)) {
		return false;
	}
	const std::string sddl = std::string("D:P(A;;GA;;;") + sid + ")";
	LocalFree(sid);
	return ConvertStringSecurityDescriptorToSecurityDescriptorA(
		sddl.c_str(), SDDL_REVISION_1, &pipe_security, nullptr);
}

static bool ConnIsOpen(const IPCClient& client)
{
	return client.conn != nullptr;
}

static void CloseClient(IPCClient& client)
{
	client.conn.reset();
}

// Creates a pipe instance and starts waiting for a client on it. With
// first_instance set, creation fails with ERROR_ACCESS_DENIED when another
// process already owns the name.
static IPCConnection CreatePipeListener(const std::string& name, bool first_instance, DWORD& error)
{
	auto conn = std::make_unique<PipeConnection>();
	if (!conn->read_ov.hEvent || !conn->write_ov.hEvent) {
		error = GetLastError();
		return nullptr;
	}
	DWORD open_mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
	if (first_instance) {
		open_mode |= FILE_FLAG_FIRST_PIPE_INSTANCE;
	}
	SECURITY_ATTRIBUTES security{sizeof(security), pipe_security, FALSE};
	conn->pipe = CreateNamedPipeA(name.c_str(), open_mode,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES, IPC_PIPE_BUFFER_SIZE, IPC_PIPE_BUFFER_SIZE, 0, &security);
	if (conn->pipe == INVALID_HANDLE_VALUE) {
		error = GetLastError();
		return nullptr;
	}
	if (!ConnectNamedPipe(conn->pipe, &conn->read_ov)) {
		error = GetLastError();
		if (error == ERROR_IO_PENDING) {
			conn->read_pending = true;
		} else if (error != ERROR_PIPE_CONNECTED && error != ERROR_NO_DATA) {
			return nullptr;
		}
	}
	// Connected already (a client that has also gone again is accepted and
	// then closed on its first read): let the accept path see it.
	if (!conn->read_pending) {
		SetEvent(conn->read_ov.hEvent);
	}
	error = ERROR_SUCCESS;
	return conn;
}

// Replaces the listener after a connection. IPCHandle retries every frame
// while this fails, so each distinct error is reported once.
static void RestartPipeListener()
{
	static DWORD reported_error = ERROR_SUCCESS;
	DWORD error = ERROR_SUCCESS;
	pipe_listener = CreatePipeListener(socket_path, false, error);
	if (!pipe_listener && error != reported_error && !currprefs.ipc_disable_logging) {
		std::cerr << "IPC: Failed to create pipe instance: error " << error << std::endl;
	}
	reported_error = error;
}

// Starts a read so the read event signals when data or end of stream arrives.
static bool StartRead(PipeConnection& c)
{
	if (c.read_pending) {
		return true;
	}
	if (!ReadFile(c.pipe, c.read_buffer, sizeof(c.read_buffer), nullptr, &c.read_ov)
		&& GetLastError() != ERROR_IO_PENDING) {
		return false;
	}
	c.read_pending = true;
	return true;
}

// Appends whatever one read delivers to out.
static IoStatus ConnRead(IPCClient& client, std::string& out)
{
	PipeConnection& c = *client.conn;
	if (!StartRead(c)) {
		return IoStatus::Closed;
	}
	DWORD received = 0;
	if (!GetOverlappedResult(c.pipe, &c.read_ov, &received, FALSE)) {
		if (GetLastError() == ERROR_IO_INCOMPLETE) {
			return IoStatus::WouldBlock;
		}
		c.read_pending = false;
		return IoStatus::Closed;
	}
	c.read_pending = false;
	out.append(c.read_buffer, received);
	return IoStatus::Ok;
}

// A write handed to the pipe has not been accepted into its buffer yet.
static bool ConnWriteBusy(const IPCClient& client)
{
	return client.conn && client.conn->write_pending
		&& WaitForSingleObject(client.conn->write_ov.hEvent, 0) != WAIT_OBJECT_0;
}

// Hands data to the pipe. The pipe owns the bytes from then on, but the next
// write waits until it has accepted them (ConnWriteBusy).
static IoStatus ConnWrite(IPCClient& client, const char* data, size_t size, size_t& written)
{
	PipeConnection& c = *client.conn;
	written = 0;
	if (c.write_pending) {
		DWORD done = 0;
		if (!GetOverlappedResult(c.pipe, &c.write_ov, &done, FALSE)) {
			if (GetLastError() == ERROR_IO_INCOMPLETE) {
				return IoStatus::WouldBlock;
			}
			c.write_pending = false;
			return IoStatus::Closed;
		}
		c.write_pending = false;
	}
	c.write_buffer.assign(data, size);
	if (!WriteFile(c.pipe, c.write_buffer.data(), static_cast<DWORD>(c.write_buffer.size()), nullptr, &c.write_ov)
		&& GetLastError() != ERROR_IO_PENDING) {
		return IoStatus::Closed;
	}
	c.write_pending = true;
	written = size;
	return IoStatus::Ok;
}

static void ConnWaitWritable(IPCClient& client, int timeout_ms)
{
	WaitForSingleObject(client.conn->write_ov.hEvent, static_cast<DWORD>(timeout_ms));
}

// Closing the handle ends the client's stream after the bytes already sent.
static void ConnFinishWrite(IPCClient&)
{
}

// Accept every connected pipe instance, up to the client limit.
// Returns true if at least one connection was accepted.
static bool AcceptClients()
{
	bool accepted = false;
	while (ipc_clients.size() < IPC_MAX_CLIENTS && pipe_listener) {
		if (pipe_listener->read_pending) {
			DWORD ignored;
			if (!GetOverlappedResult(pipe_listener->pipe, &pipe_listener->read_ov, &ignored, FALSE)) {
				if (GetLastError() == ERROR_IO_INCOMPLETE) {
					break;
				}
				// The connect failed; wait on a fresh instance instead.
				pipe_listener->read_pending = false;
				RestartPipeListener();
				continue;
			}
			pipe_listener->read_pending = false;
		}
		ipc_clients.push_back({std::move(pipe_listener), {}, {}, false});
		accepted = true;
		RestartPipeListener();
	}
	return accepted;
}

// Waits up to timeout_ms until the listener or a client can make progress.
// Sets accept_ready and ready[i] for each such client in ipc_clients.
// Returns the number of ready objects, 0 on timeout, or -1 on failure.
static int WaitForActivity(bool can_accept, int timeout_ms, bool& accept_ready, IPCReadyFlags& ready)
{
	constexpr size_t listener_owner = IPC_MAX_CLIENTS;
	std::array<HANDLE, IPC_MAX_CLIENTS + 1> handles{};
	std::array<size_t, IPC_MAX_CLIENTS + 1> owners{};
	DWORD count = 0;
	int immediate = 0;
	accept_ready = false;
	ready.fill(false);

	if (can_accept && pipe_listener) {
		owners[count] = listener_owner;
		handles[count++] = pipe_listener->read_ov.hEvent;
	}
	for (size_t i = 0; i < ipc_clients.size(); ++i) {
		IPCClient& client = ipc_clients[i];
		HANDLE event;
		if (OutputBusy(client)) {
			event = client.conn->write_ov.hEvent;
		} else if (!client.eof && StartRead(*client.conn)) {
			event = client.conn->read_ov.hEvent;
		} else {
			// The client has stopped sending, or its read failed at once:
			// servicing it answers what is left and closes it.
			ready[i] = true;
			++immediate;
			continue;
		}
		owners[count] = i;
		handles[count++] = event;
	}

	if (count == 0) {
		return immediate;
	}
	const DWORD result = WaitForMultipleObjects(count, handles.data(), FALSE,
		immediate > 0 ? 0 : static_cast<DWORD>(timeout_ms));
	if (result == WAIT_FAILED) {
		return -1;
	}
	if (result == WAIT_TIMEOUT) {
		return immediate;
	}
	int signalled = immediate;
	for (DWORD h = 0; h < count; ++h) {
		if (WaitForSingleObject(handles[h], 0) != WAIT_OBJECT_0) {
			continue;
		}
		++signalled;
		if (owners[h] == listener_owner) {
			accept_ready = true;
		} else {
			ready[owners[h]] = true;
		}
	}
	return signalled;
}

#else

static bool ConnIsOpen(const IPCClient& client)
{
	return client.conn >= 0;
}

static void CloseClient(IPCClient& client)
{
	if (client.conn >= 0) {
		close(client.conn);
		client.conn = -1;
	}
}

static void ConfigureClientSocket(int client_socket)
{
#ifdef SO_NOSIGPIPE
	const int set = 1;
	setsockopt(client_socket, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
#endif
	const int flags = fcntl(client_socket, F_GETFL, 0);
	if (flags >= 0) {
		fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
	}
}

// Appends whatever one read delivers to out.
static IoStatus ConnRead(IPCClient& client, std::string& out)
{
	char buffer[4096];
	for (;;) {
		const ssize_t n = read(client.conn, buffer, sizeof(buffer));
		if (n > 0) {
			out.append(buffer, static_cast<size_t>(n));
			return IoStatus::Ok;
		}
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return IoStatus::WouldBlock;
		}
		return IoStatus::Closed;
	}
}

static bool ConnWriteBusy(const IPCClient&)
{
	return false;
}

static IoStatus ConnWrite(IPCClient& client, const char* data, size_t size, size_t& written)
{
	written = 0;
	for (;;) {
		const ssize_t n = send(client.conn, data, size, MSG_NOSIGNAL);
		if (n > 0) {
			written = static_cast<size_t>(n);
			return IoStatus::Ok;
		}
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return IoStatus::WouldBlock;
		}
		return IoStatus::Closed;
	}
}

static void ConnWaitWritable(IPCClient& client, int timeout_ms)
{
	pollfd pfd{client.conn, POLLOUT, 0};
	poll(&pfd, 1, timeout_ms);
}

static void ConnFinishWrite(IPCClient& client)
{
	shutdown(client.conn, SHUT_WR);
}

// Accept every queued connection, up to the client limit.
// Returns true if at least one connection was accepted.
static bool AcceptClients()
{
	bool accepted = false;
	while (ipc_clients.size() < IPC_MAX_CLIENTS) {
		const int client_socket = accept(server_socket, nullptr, nullptr);
		if (client_socket < 0) {
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		ConfigureClientSocket(client_socket);
		ipc_clients.push_back({client_socket, {}, {}, false});
		accepted = true;
	}
	return accepted;
}

// Waits up to timeout_ms until the listener or a client can make progress.
// Sets accept_ready and ready[i] for each such client in ipc_clients.
// Returns the number of ready objects, 0 on timeout, or -1 on failure.
static int WaitForActivity(bool can_accept, int timeout_ms, bool& accept_ready, IPCReadyFlags& ready)
{
	std::array<pollfd, IPC_MAX_CLIENTS + 1> fds{};
	nfds_t count = 0;
	if (can_accept) {
		fds[count++] = {server_socket, POLLIN, 0};
	}
	const nfds_t first_client = count;
	for (const auto& client : ipc_clients) {
		fds[count++] = {client.conn, static_cast<short>(OutputBusy(client) ? POLLOUT : POLLIN), 0};
	}

	int ret;
	do {
		ret = poll(fds.data(), count, timeout_ms);
	} while (ret < 0 && errno == EINTR);

	accept_ready = ret > 0 && can_accept && (fds[0].revents & POLLIN);
	ready.fill(false);
	for (size_t i = 0; ret > 0 && i < ipc_clients.size(); ++i) {
		ready[i] = fds[first_client + i].revents != 0;
	}
	return ret;
}

#endif

// A reply is waiting to go out, so the client's next request must wait.
static bool OutputBusy(const IPCClient& client)
{
	return !client.output.empty() || ConnWriteBusy(client);
}

// Write as much queued output as the connection accepts without blocking.
// Returns false if the connection failed.
static bool FlushOutput(IPCClient& client)
{
	size_t sent = 0;
	while (sent < client.output.size()) {
		size_t written = 0;
		const IoStatus status = ConnWrite(client, client.output.data() + sent,
			client.output.size() - sent, written);
		if (status == IoStatus::Closed) {
			return false;
		}
		if (status == IoStatus::WouldBlock) {
			break;
		}
		sent += written;
	}
	client.output.erase(0, sent);
	return true;
}

// A complete request is waiting and nothing blocks answering it.
static bool HasBacklog(const IPCClient& client)
{
	return !OutputBusy(client) && client.pending.find('\n') != std::string::npos;
}

// QUIT ends the process, so this is the one place where waiting for a client
// is acceptable: give the reply a bounded chance to go out before closing.
static void FlushBeforeQuit(IPCClient& client)
{
	const auto give_up = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
	while (FlushOutput(client) && OutputBusy(client)
		&& std::chrono::steady_clock::now() < give_up) {
		ConnWaitWritable(client, 50);
	}
}

// Send queued replies, then read what the client has sent and answer complete
// lines. Nothing ever waits for the connection: while a reply is still queued,
// the client's further requests stay unread, so a controller that stops
// reading replies is held back by its own connection instead of stalling
// emulation.
// Requests are answered until the deadline passes, but at least one per call,
// so every client makes progress; the rest wait for the next call, and nothing
// more is read from the client until they are answered.
// Closes the client once it has stopped sending and every reply is out, and
// on error, oversized request, or QUIT.
// Returns true if a queued reply finished sending or a command ran, so the
// caller keeps listening for the client's next request.
static bool ServiceClient(IPCClient& client, std::chrono::steady_clock::time_point deadline)
{
	const bool drained = OutputBusy(client);
	if (!FlushOutput(client)) {
		CloseClient(client);
		return false;
	}
	if (OutputBusy(client)) {
		return false;
	}

	const bool backlog = client.pending.find('\n') != std::string::npos;
	for (int reads = 0; !backlog && !client.eof && reads < IPC_MAX_READS_PER_PASS; ++reads) {
		const IoStatus status = ConnRead(client, client.pending);
		if (status == IoStatus::WouldBlock) {
			break;
		}
		if (status == IoStatus::Closed) {
			// Lines already received are still answered; a client that
			// half-closes after sending can read the replies.
			client.eof = true;
		}
	}

	int processed = 0;
	size_t start = 0;
	size_t pos;
	while (!OutputBusy(client)
		&& (processed == 0 || std::chrono::steady_clock::now() < deadline)
		&& (pos = client.pending.find('\n', start)) != std::string::npos) {
		std::string line = client.pending.substr(start, pos - start);
		start = pos + 1;

		// Remove trailing \r if present
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (line.empty()) {
			continue;
		}

		client.output += ProcessCommand(line);
		++processed;
		if (ipc_quit_requested) {
			FlushBeforeQuit(client);
			ConnFinishWrite(client);
			CloseClient(client);
			return true;
		}
		if (!FlushOutput(client)) {
			CloseClient(client);
			return true;
		}
	}
	client.pending.erase(0, start);

	const size_t last_newline = client.pending.rfind('\n');
	const size_t unterminated = last_newline == std::string::npos
		? client.pending.size()
		: client.pending.size() - last_newline - 1;
	if (unterminated > IPC_MAX_REQUEST_BYTES) {
		client.output += make_response(false, {"Request too long"});
		FlushOutput(client);
		CloseClient(client);
	} else if (client.eof && !OutputBusy(client) && last_newline == std::string::npos) {
		CloseClient(client);
	}
	return drained || processed > 0;
}

#ifndef _WIN32
// Helper: Check if a socket is stale (no process listening)
static bool is_socket_stale(const std::string& path)
{
	// If socket file doesn't exist, it's not stale
	struct stat st{};
	if (stat(path.c_str(), &st) != 0) {
		return false;
	}

	// Try to connect to see if anyone is listening
	int test_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (test_socket < 0) {
		return true; // Can't create socket, assume stale
	}

	struct sockaddr_un addr{};
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

	// Set non-blocking for connect attempt
	int flags = fcntl(test_socket, F_GETFL, 0);
	fcntl(test_socket, F_SETFL, flags | O_NONBLOCK);

	int result = connect(test_socket, (struct sockaddr*)&addr, sizeof(addr));
	close(test_socket);

	if (result == 0) {
		// Connected successfully - socket is in use by another instance
		return false;
	}

	// Check error code
	if (errno == ECONNREFUSED || errno == ENOENT) {
		// No one listening or socket file gone - it's stale
		return true;
	}

	if (errno == EINPROGRESS || errno == EAGAIN) {
		// Connection in progress - socket is in use
		return false;
	}

	// Other errors - assume stale to allow cleanup
	return true;
}
#endif

// Helper: Generate socket path (pipe name on Windows) with optional instance suffix
static std::string get_socket_path(int instance = 0)
{
#ifdef _WIN32
	const std::string base_path = "\\\\.\\pipe\\amiberry";
	const char* suffix = "";
#else
	std::string base_path;
	const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
	if (xdg_runtime) {
		base_path = std::string(xdg_runtime) + "/amiberry";
	} else {
		base_path = "/tmp/amiberry";
	}
	const char* suffix = ".sock";
#endif

	if (instance == 0) {
		return base_path + suffix;
	} else {
		return base_path + "_" + std::to_string(instance) + suffix;
	}
}

#ifdef _WIN32
// Claims the first free pipe name. A pipe disappears with the process that
// created it, so there are no stale names to clean up.
static bool OpenListener()
{
	if (!CreatePipeSecurity()) {
		std::cerr << "IPC: Failed to build pipe security descriptor: error " << GetLastError() << std::endl;
		return false;
	}
	for (int instance = 0; instance < 10; ++instance) {
		socket_path = get_socket_path(instance);
		DWORD error = ERROR_SUCCESS;
		pipe_listener = CreatePipeListener(socket_path, true, error);
		if (pipe_listener) {
			socket_instance = instance;
			if (instance > 0 && !currprefs.ipc_disable_logging) {
				std::cout << "IPC: Default pipe in use, using instance " << instance << std::endl;
			}
			return true;
		}
		// ERROR_ACCESS_DENIED: another process owns this name.
		if (error != ERROR_ACCESS_DENIED && error != ERROR_PIPE_BUSY) {
			std::cerr << "IPC: Failed to create pipe " << socket_path << ": error " << error << std::endl;
			return false;
		}
	}
	std::cerr << "IPC: Could not find available pipe name after 10 attempts" << std::endl;
	return false;
}
#else
static bool OpenListener()
{
	// Try to find an available socket path
	// First, try the default path
	socket_path = get_socket_path(0);

	// Check if existing socket is stale (from a crashed instance)
	if (is_socket_stale(socket_path)) {
		if (!currprefs.ipc_disable_logging) {
			std::cout << "IPC: Removing stale socket at " << socket_path << std::endl;
		}
		unlink(socket_path.c_str());
	}

	// Create socket
	server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_socket < 0) {
		std::cerr << "IPC: Failed to create socket: " << strerror(errno) << std::endl;
		return false;
	}

	// Set non-blocking
	int flags = fcntl(server_socket, F_GETFL, 0);
	fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

	// Try to bind - if default path is in use, try alternative paths for multiple instances
	struct sockaddr_un addr{};
	bool bound = false;

	for (int instance = 0; instance < 10; ++instance) {
		if (instance > 0) {
			// Try an alternative socket path
			socket_path = get_socket_path(instance);

			// Check for stale socket at this path too
			if (is_socket_stale(socket_path)) {
				if (!currprefs.ipc_disable_logging) {
					std::cout << "IPC: Removing stale socket at " << socket_path << std::endl;
				}
				unlink(socket_path.c_str());
			}
		}

		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

		if (::bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
			bound = true;
			socket_instance = instance;
			if (instance > 0 && !currprefs.ipc_disable_logging) {
				std::cout << "IPC: Default socket in use, using instance " << instance << std::endl;
			}
			break;
		}

		// EADDRINUSE means another instance is using this socket
		if (errno != EADDRINUSE) {
			std::cerr << "IPC: Failed to bind socket: " << strerror(errno) << std::endl;
			close(server_socket);
			server_socket = -1;
			return false;
		}
	}

	if (!bound) {
		std::cerr << "IPC: Could not find available socket path after 10 attempts" << std::endl;
		close(server_socket);
		server_socket = -1;
		return false;
	}

	// Listen
	if (listen(server_socket, SOMAXCONN) < 0) {
		std::cerr << "IPC: Failed to listen on socket: " << strerror(errno) << std::endl;
		close(server_socket);
		server_socket = -1;
		unlink(socket_path.c_str());
		return false;
	}
	return true;
}
#endif

void Amiberry::IPC::IPCSetup()
{
	if (!OpenListener()) {
		return;
	}

	// Initialize handlers
	InitHandlers();

	ipc_active = true;
	if (!currprefs.ipc_disable_logging) {
		std::cout << "IPC: Listening on " << socket_path << std::endl;
	}

	// Register cleanup
	atexit(Amiberry::IPC::IPCCleanUp);
}

void Amiberry::IPC::IPCCleanUp()
{
	for (auto& client : ipc_clients) {
		CloseClient(client);
	}
	ipc_clients.clear();

#ifdef _WIN32
	pipe_listener.reset();
	LocalFree(pipe_security);
	pipe_security = nullptr;
	socket_path.clear();
#else
	if (server_socket >= 0) {
		close(server_socket);
		server_socket = -1;
	}

	if (!socket_path.empty()) {
		unlink(socket_path.c_str());
		socket_path.clear();
	}
#endif

	close_settings_session();
	ipc_active = false;
}

void Amiberry::IPC::IPCHandle()
{
	std::unique_lock<std::mutex> lock(ipc_handle_mutex, std::try_to_lock);
	if (!lock.owns_lock()) return;

#ifdef _WIN32
	if (!ipc_active) return;
	if (!pipe_listener) {
		// Creating the next instance failed earlier; try again.
		RestartPipeListener();
	}
#else
	if (server_socket < 0) return;
#endif

	using clock = std::chrono::steady_clock;
	const auto started = clock::now();
	auto last_activity = started;
	bool active = false;
	IPCReadyFlags ready{};

	while (!ipc_quit_requested) {
		// Without activity, only take what is already waiting. After a request
		// or a new connection, wait a little for the next request.
		const auto hard_deadline = started + IPC_MAX_SERVICE_TIME;
		auto deadline = hard_deadline;
		if (active) {
			deadline = std::min(deadline, last_activity + IPC_FOLLOWUP_WAIT);
		}
		const auto now = clock::now();
		if (now >= deadline) {
			break;
		}

		const bool can_accept = ipc_clients.size() < IPC_MAX_CLIENTS;
		bool backlog = false;
		for (const auto& client : ipc_clients) {
			backlog = backlog || HasBacklog(client);
		}

		// Requests left over from an earlier pass are answered without waiting.
		const int timeout_ms = active && !backlog
			? static_cast<int>(std::chrono::ceil<std::chrono::milliseconds>(deadline - now).count())
			: 0;
		bool accept_ready = false;
		const int ret = WaitForActivity(can_accept, timeout_ms, accept_ready, ready);
		if (ret < 0 || (ret == 0 && !backlog)) {
			break;
		}

		bool progressed = false;
		const size_t waited_on = ipc_clients.size();
		for (size_t i = 0; i < waited_on && !ipc_quit_requested; ++i) {
			IPCClient& client = ipc_clients[i];
			if ((ready[i] || HasBacklog(client))
				&& ServiceClient(client, hard_deadline)) {
				progressed = true;
			}
		}
		ipc_clients.erase(
			std::remove_if(ipc_clients.begin(), ipc_clients.end(),
				[](const IPCClient& client) { return !ConnIsOpen(client); }),
			ipc_clients.end());

		if (can_accept && accept_ready && !ipc_quit_requested && AcceptClients()) {
			progressed = true;
		}

		if (progressed) {
			active = true;
			last_activity = clock::now();
		}
	}

	if (ipc_quit_requested) {
		ipc_quit_requested = false;
		// Executes the quit action HandleQuit posted (and anything queued
		// before it), after the replies have been written.
		amiberry_actions_run_pending();
	}
}

int Amiberry::IPC::IPCInstance()
{
	return ipc_active ? socket_instance : 0;
}

bool Amiberry::IPC::IPCIsActive()
{
	return ipc_active;
}

#endif // USE_IPC_SOCKET
