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

	auto it = command_handlers.find(cmd);
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
	struct stat st;
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
