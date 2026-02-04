//
// Unix domain socket IPC implementation for Amiberry
// Cross-platform: Linux, macOS, FreeBSD
//

#ifdef USE_IPC_SOCKET

#include "amiberry_ipc.h"
#include "sysdeps.h"
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
#include "gui/gui_handling.h"
#include "newcpu.h"
#include "blitter.h"
#ifdef DEBUGGER
#include "debug.h"
#endif
#include <dirent.h>

#include <iostream>
#include <sstream>
#include <cstring>
#include <map>
#include <algorithm>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <poll.h>

using namespace Amiberry::IPC;

// Socket state
static int server_socket = -1;
static std::string socket_path;
static bool ipc_active = false;

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

// Command handlers - reusing logic from DBus implementation
static std::string HandleQuit(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received QUIT" << std::endl;
	uae_quit();
	return make_response(true);
}

static std::string HandlePause(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received PAUSE" << std::endl;
	setpaused(3);
	activationtoggle(0, true);
	return make_response(true);
}

static std::string HandleResume(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received RESUME" << std::endl;
	resumepaused(3);
	activationtoggle(0, false);
	return make_response(true);
}

static std::string HandleReset(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received RESET" << std::endl;
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

	uae_reset(hard, keyboard);
	return make_response(true);
}

static std::string HandleScreenshot(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SCREENSHOT" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Missing filename"});
	}

	if (!create_screenshot()) {
		return make_response(false, {"Failed to create screenshot"});
	}

	int ret = save_thumb(args[0]);
	if (ret == 0) {
		return make_response(false, {"Failed to save screenshot to: " + args[0]});
	}

	return make_response(true, {args[0]});
}

static std::string HandleSavestate(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SAVESTATE" << std::endl;
	if (args.size() < 2) {
		return make_response(false, {"Usage: SAVESTATE <statefile> <configfile>"});
	}

	savestate_initsave(args[0].c_str(), 1, true, true);
	save_state(args[0].c_str(), "...");

	strncpy(changed_prefs.description, "autosave", sizeof(changed_prefs.description));
	cfgfile_save(&changed_prefs, args[1].c_str(), 0);

	return make_response(true);
}

static std::string HandleLoadState(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received LOADSTATE" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Missing state file path"});
	}

	savestate_state = STATE_DORESTORE;
	_tcscpy(savestate_fname, args[0].c_str());
	return make_response(true);
}

static std::string HandleDiskSwap(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received DISKSWAP" << std::endl;
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

	for (int i = 0; i < 4; i++) {
		if (!_tcscmp(currprefs.floppyslots[i].df, currprefs.dfxlist[disknum]))
			changed_prefs.floppyslots[i].df[0] = 0;
	}
	_tcscpy(changed_prefs.floppyslots[drivenum].df, currprefs.dfxlist[disknum]);
	set_config_changed();

	return make_response(true);
}

static std::string HandleQueryDiskSwap(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received QUERYDISKSWAP" << std::endl;
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
	std::cout << "IPC: Received INSERTFLOPPY" << std::endl;
	if (args.size() < 2) {
		return make_response(false, {"Usage: INSERTFLOPPY <path> <drivenum>"});
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

	_tcsncpy(changed_prefs.floppyslots[drivenum].df, args[0].c_str(), MAX_DPATH);
	changed_prefs.floppyslots[drivenum].df[MAX_DPATH - 1] = 0;
	set_config_changed();

	return make_response(true);
}

static std::string HandleInsertCD(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received INSERTCD" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Missing CD image path"});
	}

	_tcsncpy(changed_prefs.cdslots[0].name, args[0].c_str(), MAX_DPATH);
	changed_prefs.cdslots[0].name[MAX_DPATH - 1] = 0;
	changed_prefs.cdslots[0].inuse = true;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetStatus(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_STATUS" << std::endl;
	std::vector<std::string> responses;

	responses.push_back(std::string("Paused=") + (pause_emulation ? "true" : "false"));
	responses.push_back(std::string("Config=") + changed_prefs.description);

	for (int i = 0; i < 4; ++i) {
		if (changed_prefs.floppyslots[i].df[0])
			responses.push_back(std::string("Floppy") + std::to_string(i) + "=" + changed_prefs.floppyslots[i].df);
	}

	return make_response(true, responses);
}

static std::string HandleGetConfig(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_CONFIG" << std::endl;
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
	else {
		return make_response(false, {"Unknown option: " + optname});
	}

	return make_response(true, {value});
}

static std::string HandleSetConfig(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_CONFIG" << std::endl;
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
			changed_prefs.gfx_apmode[0].gfx_fullscreen = fullscreen ? GFX_FULLSCREEN : GFX_WINDOW;
			set_config_changed();
		}
		// Sound options
		else if (optname == "sound_output") {
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
	std::cout << "IPC: Received LOAD_CONFIG" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Missing config file path"});
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
	std::cout << "IPC: Received SEND_KEY" << std::endl;
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

	inputdevice_add_inputcode(keycode, state, nullptr);

	return make_response(true);
}

static std::string HandleReadMem(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received READ_MEM" << std::endl;
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
	std::cout << "IPC: Received WRITE_MEM" << std::endl;
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
	std::cout << "IPC: Received EJECT_FLOPPY" << std::endl;
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

	disk_eject(drivenum);
	return make_response(true);
}

static std::string HandleEjectCD(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received EJECT_CD" << std::endl;

	changed_prefs.cdslots[0].name[0] = 0;
	changed_prefs.cdslots[0].inuse = false;
	set_config_changed();

	return make_response(true);
}

static std::string HandleSetVolume(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_VOLUME" << std::endl;
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
	std::cout << "IPC: Received GET_VOLUME" << std::endl;
	return make_response(true, {std::to_string(currprefs.sound_volume_master)});
}

static std::string HandleMute(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received MUTE" << std::endl;
	set_volume(0, 1);
	return make_response(true);
}

static std::string HandleUnmute(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received UNMUTE" << std::endl;
	set_volume(currprefs.sound_volume_master, 0);
	return make_response(true);
}

static std::string HandleToggleFullscreen(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received TOGGLE_FULLSCREEN" << std::endl;
	toggle_fullscreen(0, -1);
	return make_response(true);
}

static std::string HandleSetWarp(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_WARP" << std::endl;
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
	std::cout << "IPC: Received GET_WARP" << std::endl;
	return make_response(true, {std::to_string(currprefs.turbo_emulation ? 1 : 0)});
}

static std::string HandleGetVersion(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_VERSION" << std::endl;
	std::vector<std::string> info;
	info.push_back("version=" + get_version_string());
	info.push_back("sdl=" + get_sdl2_version_string());
	return make_response(true, info);
}

static std::string HandleListFloppies(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received LIST_FLOPPIES" << std::endl;
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

static std::string HandleListConfigs(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received LIST_CONFIGS" << std::endl;
	std::vector<std::string> configs;

	std::string config_path = get_configuration_path();
	DIR* dir = opendir(config_path.c_str());
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr) {
			std::string name = entry->d_name;
			// Only list .uae files
			if (name.length() > 4 && name.substr(name.length() - 4) == ".uae") {
				configs.push_back(name);
			}
		}
		closedir(dir);
	}

	if (configs.empty()) {
		return make_response(true, {"<no configs found>"});
	}

	return make_response(true, configs);
}

static std::string HandleFrameAdvance(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received FRAME_ADVANCE" << std::endl;
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
	std::cout << "IPC: Received SET_MOUSE_SPEED" << std::endl;
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
	std::cout << "IPC: Received SEND_MOUSE" << std::endl;
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

static std::string HandlePing(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received PING" << std::endl;
	return make_response(true, {"PONG"});
}

static std::string HandleQuickSave(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received QUICKSAVE" << std::endl;
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

	savestate_quick(slot, 1);  // 1 = save
	return make_response(true, {"Saved to slot " + std::to_string(slot)});
}

static std::string HandleQuickLoad(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received QUICKLOAD" << std::endl;
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

	savestate_quick(slot, 0);  // 0 = load
	return make_response(true, {"Loading from slot " + std::to_string(slot)});
}

static std::string HandleGetJoyportMode(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_JOYPORT_MODE" << std::endl;
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
	std::cout << "IPC: Received SET_JOYPORT_MODE" << std::endl;
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
	std::cout << "IPC: Received GET_AUTOFIRE" << std::endl;
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
	std::cout << "IPC: Received SET_AUTOFIRE" << std::endl;
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
	std::cout << "IPC: Received GET_LED_STATUS" << std::endl;
	std::vector<std::string> responses;

	// Power LED
	responses.push_back("power=" + std::to_string(gui_data.powerled));

	// Floppy drive LEDs (DF0-DF3)
	for (int i = 0; i < 4; ++i) {
		bool motor_on = gui_data.drives[i].drive_motor;
		bool writing = gui_data.drives[i].drive_writing;
		std::string status = motor_on ? (writing ? "writing" : "reading") : "off";
		responses.push_back("df" + std::to_string(i) + "=" + status);
	}

	// HD LED
	responses.push_back("hd=" + std::to_string(gui_data.hd));

	// CD LED
	responses.push_back("cd=" + std::to_string(gui_data.cd));

	// Caps Lock
	responses.push_back("caps=" + std::to_string(gui_data.capslock ? 1 : 0));

	return make_response(true, responses);
}

static std::string HandleListHarddrives(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received LIST_HARDDRIVES" << std::endl;
	std::vector<std::string> responses;

	// List mounted hardfiles from prefs
	for (int i = 0; i < MAX_FILESYSTEM_UNITS; ++i) {
		struct uaedev_config_data *uci = &currprefs.mountconfig[i];
		if (uci->ci.rootdir[0]) {
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
	std::cout << "IPC: Received SET_DISPLAY_MODE" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Usage: SET_DISPLAY_MODE <mode> (0=window, 1=fullscreen, 2=fullwindow)"});
	}

	int mode;
	try {
		mode = std::stoi(args[0]);
	} catch (const std::exception& e) {
		return make_response(false, {"Invalid mode: " + args[0]});
	}

	if (mode < 0 || mode > 2) {
		return make_response(false, {"Mode must be 0-2 (0=window, 1=fullscreen, 2=fullwindow)"});
	}

	changed_prefs.gfx_apmode[0].gfx_fullscreen = mode;
	set_config_changed();

	return make_response(true);
}

static std::string HandleGetDisplayMode(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_DISPLAY_MODE" << std::endl;
	const int mode = currprefs.gfx_apmode[0].gfx_fullscreen;
	const char* mode_names[] = {"window", "fullscreen", "fullwindow"};
	std::string mode_name = (mode >= 0 && mode <= 2) ? mode_names[mode] : "unknown";

	return make_response(true, {std::to_string(mode), mode_name});
}

static std::string HandleSetNTSC(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_NTSC" << std::endl;
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
	std::cout << "IPC: Received GET_NTSC" << std::endl;
	return make_response(true, {std::to_string(currprefs.ntscmode ? 1 : 0), currprefs.ntscmode ? "NTSC" : "PAL"});
}

static std::string HandleSetSoundMode(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_SOUND_MODE" << std::endl;
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
	std::cout << "IPC: Received GET_SOUND_MODE" << std::endl;
	const int mode = currprefs.produce_sound;
	const char* mode_names[] = {"off", "normal", "stereo", "best"};
	std::string mode_name = (mode >= 0 && mode <= 3) ? mode_names[mode] : "unknown";

	return make_response(true, {std::to_string(mode), mode_name});
}

static std::string HandleToggleMouseGrab(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received TOGGLE_MOUSE_GRAB" << std::endl;
	toggle_mousegrab();
	return make_response(true);
}

static std::string HandleGetMouseSpeed(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_MOUSE_SPEED" << std::endl;
	return make_response(true, {std::to_string(currprefs.input_mouse_speed)});
}

static std::string HandleSetCPUSpeed(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_CPU_SPEED" << std::endl;
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
	std::cout << "IPC: Received GET_CPU_SPEED" << std::endl;
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
	std::cout << "IPC: Received TOGGLE_RTG" << std::endl;
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
	std::cout << "IPC: Received SET_FLOPPY_SPEED" << std::endl;
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
	std::cout << "IPC: Received GET_FLOPPY_SPEED" << std::endl;
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
	std::cout << "IPC: Received DISK_WRITE_PROTECT" << std::endl;
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
	std::cout << "IPC: Received GET_DISK_WRITE_PROTECT" << std::endl;
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
	std::cout << "IPC: Received TOGGLE_STATUS_LINE" << std::endl;
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
	std::cout << "IPC: Received SET_CHIPSET" << std::endl;
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
	std::cout << "IPC: Received GET_CHIPSET" << std::endl;
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
	std::cout << "IPC: Received GET_MEMORY_CONFIG" << std::endl;
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
	std::cout << "IPC: Received GET_FPS" << std::endl;
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
	std::cout << "IPC: Received SET_CHIP_MEM" << std::endl;
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
	std::cout << "IPC: Received SET_FAST_MEM" << std::endl;
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
	std::cout << "IPC: Received SET_SLOW_MEM" << std::endl;
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
	std::cout << "IPC: Received SET_Z3_MEM" << std::endl;
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
	std::cout << "IPC: Received GET_CPU_MODEL" << std::endl;
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
	std::cout << "IPC: Received SET_CPU_MODEL" << std::endl;
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
	std::cout << "IPC: Received SET_WINDOW_SIZE" << std::endl;
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
	std::cout << "IPC: Received GET_WINDOW_SIZE" << std::endl;
	std::vector<std::string> responses;

	int width = currprefs.gfx_monitor[0].gfx_size_win.width;
	int height = currprefs.gfx_monitor[0].gfx_size_win.height;

	responses.push_back("width=" + std::to_string(width));
	responses.push_back("height=" + std::to_string(height));

	return make_response(true, responses);
}

static std::string HandleSetScaling(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_SCALING" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Usage: SET_SCALING <method> (-1=auto, 0=nearest, 1=linear, 2=integer)"});
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
		else {
			return make_response(false, {"Invalid scaling method: " + args[0]});
		}
	}

	if (method < -1 || method > 2) {
		return make_response(false, {"Scaling method must be -1..2 (-1=auto, 0=nearest, 1=linear, 2=integer)"});
	}

	// Set scaling method
	changed_prefs.scaling_method = method;
	set_config_changed();

	const char* method_names[] = {"auto", "nearest", "linear", "integer"};
	const int method_index = method + 1; // -1..2 -> 0..3
	const char* method_name = (method_index >= 0 && method_index <= 3) ? method_names[method_index] : "unknown";
	return make_response(true, {std::to_string(method), method_name});
}

static std::string HandleGetScaling(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_SCALING" << std::endl;
	std::vector<std::string> responses;

	int method = currprefs.scaling_method;
	const char* method_names[] = {"auto", "nearest", "linear", "integer"};
	const int method_index = method + 1; // -1..2 -> 0..3
	std::string method_name = (method_index >= 0 && method_index <= 3) ? method_names[method_index] : "unknown";

	responses.push_back("method=" + std::to_string(method));
	responses.push_back("method_name=" + method_name);

	return make_response(true, responses);
}

static std::string HandleSetLineMode(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_LINE_MODE" << std::endl;
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
	std::cout << "IPC: Received GET_LINE_MODE" << std::endl;
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
	std::cout << "IPC: Received SET_RESOLUTION" << std::endl;
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
	std::cout << "IPC: Received GET_RESOLUTION" << std::endl;

	int res = currprefs.gfx_resolution;
	const char* res_names[] = {"lores", "hires", "superhires"};
	std::string res_name = (res >= 0 && res <= 2) ? res_names[res] : "unknown";

	return make_response(true, {std::to_string(res), res_name});
}

// Autocrop and WHDLoad ===

static std::string HandleSetAutocrop(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received SET_AUTOCROP" << std::endl;
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
	std::cout << "IPC: Received GET_AUTOCROP" << std::endl;

	bool enabled = currprefs.gfx_auto_crop;
	return make_response(true, {std::to_string(enabled ? 1 : 0), enabled ? "enabled" : "disabled"});
}

static std::string HandleInsertWHDLoad(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received INSERT_WHDLOAD" << std::endl;
	if (args.empty()) {
		return make_response(false, {"Usage: INSERT_WHDLOAD <path_to_lha_or_directory>"});
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
	std::cout << "IPC: Received EJECT_WHDLOAD" << std::endl;

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
	std::cout << "IPC: Received GET_WHDLOAD" << std::endl;

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
	std::cout << "IPC: Received DEBUG_ACTIVATE" << std::endl;

	if (debugger_active) {
		return make_response(true, {"Debugger already active"});
	}

	activate_debugger();
	return make_response(true, {"Debugger activated"});
}

static std::string HandleDebugDeactivate(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received DEBUG_DEACTIVATE" << std::endl;

	if (!debugger_active) {
		return make_response(true, {"Debugger not active"});
	}

	deactivate_debugger();
	return make_response(true, {"Debugger deactivated"});
}

static std::string HandleDebugStatus(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received DEBUG_STATUS" << std::endl;

	std::vector<std::string> responses;
	responses.push_back("active=" + std::to_string(debugger_active ? 1 : 0));
	responses.push_back("debugging=" + std::to_string(debugging ? 1 : 0));
	responses.push_back("exception_debugging=" + std::to_string(exception_debugging ? 1 : 0));

	return make_response(true, responses);
}

static std::string HandleDebugStep(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received DEBUG_STEP" << std::endl;

	if (!debugger_active) {
		return make_response(false, {"Debugger not active - use DEBUG_ACTIVATE first"});
	}

	// Single step mode - execute one instruction
	set_special(SPCFLAG_BRK);
	return make_response(true, {"Single step executed"});
}

static std::string HandleDebugContinue(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received DEBUG_CONTINUE" << std::endl;

	if (!debugger_active) {
		return make_response(false, {"Debugger not active"});
	}

	deactivate_debugger();
	return make_response(true, {"Execution continued"});
}
#endif

static std::string HandleGetCPURegs(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received GET_CPU_REGS" << std::endl;

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
	std::cout << "IPC: Received GET_CUSTOM_REGS" << std::endl;

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
	std::cout << "IPC: Received DISASSEMBLE" << std::endl;

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
		addr = m68k_disasm_2(buf, sizeof(buf) / sizeof(TCHAR), addr, nullptr, 0xffffffff, nullptr, 1, nullptr, nullptr, 0, 1);
		responses.emplace_back(buf);
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
	std::cout << "IPC: Received SET_BREAKPOINT" << std::endl;

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
	}

	// Find a free slot if not specified
	if (slot < 0) {
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (!bpnodes[i].enabled) {
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
	bpnodes[slot].type = BREAKPOINT_REG_PC;
	bpnodes[slot].oper = BREAKPOINT_CMP_EQUAL;
	bpnodes[slot].enabled = 1;

	char buf[64];
	snprintf(buf, sizeof(buf), "Breakpoint set at %08X in slot %d", addr, slot);
	return make_response(true, {buf});
}

static std::string HandleClearBreakpoint(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received CLEAR_BREAKPOINT" << std::endl;

	if (args.empty()) {
		// Clear all breakpoints
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			bpnodes[i].enabled = 0;
		}
		return make_response(true, {"All breakpoints cleared"});
	}

	int slot;
	try {
		slot = std::stoi(args[0]);
		if (slot < 0 || slot >= BREAKPOINT_TOTAL) {
			return make_response(false, {"Slot must be 0-" + std::to_string(BREAKPOINT_TOTAL - 1)});
		}
	} catch (...) {
		return make_response(false, {"Invalid slot number"});
	}

	bpnodes[slot].enabled = 0;

	char buf[64];
	snprintf(buf, sizeof(buf), "Breakpoint %d cleared", slot);
	return make_response(true, {buf});
}

static std::string HandleListBreakpoints(const std::vector<std::string>& args)
{
	std::cout << "IPC: Received LIST_BREAKPOINTS" << std::endl;

	std::vector<std::string> responses;
	int count = 0;

	for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (bpnodes[i].enabled) {
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
	std::cout << "IPC: Received GET_COPPER_STATE" << std::endl;

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
	std::cout << "IPC: Received GET_BLITTER_STATE" << std::endl;

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
	std::cout << "IPC: Received GET_DRIVE_STATE" << std::endl;

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
	std::cout << "IPC: Received GET_AUDIO_STATE" << std::endl;

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
	std::cout << "IPC: Received GET_DMA_STATE" << std::endl;

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
	std::cout << "IPC: Received HELP" << std::endl;
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
	commands.emplace_back("READ_MEM <addr> <width>, WRITE_MEM <addr> <width> <val>");
	commands.emplace_back("SET_AUTOCROP <0|1>, GET_AUTOCROP");
	commands.emplace_back("INSERT_WHDLOAD <path>, EJECT_WHDLOAD, GET_WHDLOAD");
#ifdef DEBUGGER
	commands.emplace_back("--- Debugging Commands ---");
	commands.emplace_back("DEBUG_ACTIVATE, DEBUG_DEACTIVATE, DEBUG_STATUS");
	commands.emplace_back("DEBUG_STEP [count], DEBUG_STEP_OVER, DEBUG_CONTINUE");
	commands.emplace_back("GET_CPU_REGS, GET_CUSTOM_REGS");
	commands.emplace_back("DISASSEMBLE <addr> [count] - disassemble instructions");
	commands.emplace_back("SET_BREAKPOINT <addr>, CLEAR_BREAKPOINT <addr|ALL>, LIST_BREAKPOINTS");
	commands.emplace_back("GET_COPPER_STATE, GET_BLITTER_STATE");
	commands.emplace_back("GET_DRIVE_STATE [drive], GET_AUDIO_STATE, GET_DMA_STATE");
#endif
	commands.emplace_back("GET_VERSION, PING, HELP");
	return make_response(true, commands);
}

// Initialize command handlers
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
	command_handlers[CMD_PING] = HandlePing;
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
	command_handlers[CMD_DEBUG_STEP_OVER] = HandleDebugStep;  // Same handler, step over logic handled internally
	command_handlers[CMD_DEBUG_CONTINUE] = HandleDebugContinue;
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

	const auto it = command_handlers.find(cmd);
	if (it != command_handlers.end()) {
		return it->second(args);
	}

	return make_response(false, {"Unknown command: " + cmd});
}

// Handle a single client connection
static void HandleClient(int client_socket)
{
	char buffer[4096];
	std::string accumulated;

	// Set non-blocking
	int flags = fcntl(client_socket, F_GETFL, 0);
	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

	// Read with timeout
	struct pollfd pfd{};
	pfd.fd = client_socket;
	pfd.events = POLLIN;

	while (true) {
		int ret = poll(&pfd, 1, 100); // 100ms timeout
		if (ret <= 0) break;

		ssize_t n = read(client_socket, buffer, sizeof(buffer) - 1);
		if (n <= 0) break;

		buffer[n] = '\0';
		accumulated += buffer;

		// Process complete lines
		size_t pos;
		while ((pos = accumulated.find('\n')) != std::string::npos) {
			std::string line = accumulated.substr(0, pos);
			accumulated.erase(0, pos + 1);

			// Remove trailing \r if present
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			if (!line.empty()) {
				std::string response = ProcessCommand(line);
				write(client_socket, response.c_str(), response.length());
			}
		}
	}

	close(client_socket);
}

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

// Helper: Generate socket path with optional instance suffix
static std::string get_socket_path(int instance = 0)
{
	std::string base_path;
	const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
	if (xdg_runtime) {
		base_path = std::string(xdg_runtime) + "/amiberry";
	} else {
		base_path = "/tmp/amiberry";
	}

	if (instance == 0) {
		return base_path + ".sock";
	} else {
		return base_path + "_" + std::to_string(instance) + ".sock";
	}
}

void Amiberry::IPC::IPCSetup()
{
	// Try to find an available socket path
	// First, try the default path
	socket_path = get_socket_path(0);

	// Check if existing socket is stale (from a crashed instance)
	if (is_socket_stale(socket_path)) {
		std::cout << "IPC: Removing stale socket at " << socket_path << std::endl;
		unlink(socket_path.c_str());
	}

	// Create socket
	server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_socket < 0) {
		std::cerr << "IPC: Failed to create socket: " << strerror(errno) << std::endl;
		return;
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
				std::cout << "IPC: Removing stale socket at " << socket_path << std::endl;
				unlink(socket_path.c_str());
			}
		}

		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

		if (::bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
			bound = true;
			if (instance > 0) {
				std::cout << "IPC: Default socket in use, using instance " << instance << std::endl;
			}
			break;
		}

		// EADDRINUSE means another instance is using this socket
		if (errno != EADDRINUSE) {
			std::cerr << "IPC: Failed to bind socket: " << strerror(errno) << std::endl;
			close(server_socket);
			server_socket = -1;
			return;
		}
	}

	if (!bound) {
		std::cerr << "IPC: Could not find available socket path after 10 attempts" << std::endl;
		close(server_socket);
		server_socket = -1;
		return;
	}

	// Listen
	if (listen(server_socket, 5) < 0) {
		std::cerr << "IPC: Failed to listen on socket: " << strerror(errno) << std::endl;
		close(server_socket);
		server_socket = -1;
		unlink(socket_path.c_str());
		return;
	}

	// Initialize handlers
	InitHandlers();

	ipc_active = true;
	std::cout << "IPC: Listening on " << socket_path << std::endl;

	// Register cleanup
	atexit(Amiberry::IPC::IPCCleanUp);
}

void Amiberry::IPC::IPCCleanUp()
{
	if (server_socket >= 0) {
		close(server_socket);
		server_socket = -1;
	}

	if (!socket_path.empty()) {
		unlink(socket_path.c_str());
		socket_path.clear();
	}

	ipc_active = false;
}

void Amiberry::IPC::IPCHandle()
{
	if (server_socket < 0) return;

	// Check for incoming connections (non-blocking)
	struct pollfd pfd{};
	pfd.fd = server_socket;
	pfd.events = POLLIN;

	int ret = poll(&pfd, 1, 0); // Non-blocking
	if (ret > 0 && (pfd.revents & POLLIN)) {
		struct sockaddr_un client_addr{};
		socklen_t client_len = sizeof(client_addr);

		int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
		if (client_socket >= 0) {
			HandleClient(client_socket);
		}
	}
}

bool Amiberry::IPC::IPCIsActive()
{
	return ipc_active;
}

#endif // USE_IPC_SOCKET
