//
// Unified IPC interface for Amiberry
// Supports Unix domain sockets (cross-platform: Linux, macOS, FreeBSD)
//

#pragma once

#ifdef USE_IPC_SOCKET

#include <string>
#include <vector>
#include <functional>

namespace Amiberry::IPC {

// Socket path - use XDG_RUNTIME_DIR on Linux if available, else /tmp
#ifdef __linux__
    #define DEFAULT_SOCKET_PATH "/tmp/amiberry.sock"
#else
    // macOS and other Unix systems
    constexpr char const* DEFAULT_SOCKET_PATH = "/tmp/amiberry.sock";
#endif

// Command names (same as DBus for consistency)
constexpr char const* CMD_QUIT = "QUIT";
constexpr char const* CMD_PAUSE = "PAUSE";
constexpr char const* CMD_RESUME = "RESUME";
constexpr char const* CMD_RESET = "RESET";
constexpr char const* CMD_SCREENSHOT = "SCREENSHOT";
constexpr char const* CMD_SAVESTATE = "SAVESTATE";
constexpr char const* CMD_LOADSTATE = "LOADSTATE";
constexpr char const* CMD_DISKSWAP = "DISKSWAP";
constexpr char const* CMD_QUERYDISKSWAP = "QUERYDISKSWAP";
constexpr char const* CMD_INSERTFLOPPY = "INSERTFLOPPY";
constexpr char const* CMD_INSERTCD = "INSERTCD";
constexpr char const* CMD_GET_STATUS = "GET_STATUS";
constexpr char const* CMD_GET_CONFIG = "GET_CONFIG";
constexpr char const* CMD_SET_CONFIG = "SET_CONFIG";
constexpr char const* CMD_LOAD_CONFIG = "LOAD_CONFIG";
constexpr char const* CMD_SEND_KEY = "SEND_KEY";
constexpr char const* CMD_READ_MEM = "READ_MEM";
constexpr char const* CMD_WRITE_MEM = "WRITE_MEM";

// New commands
constexpr char const* CMD_EJECT_FLOPPY = "EJECT_FLOPPY";
constexpr char const* CMD_EJECT_CD = "EJECT_CD";
constexpr char const* CMD_SET_VOLUME = "SET_VOLUME";
constexpr char const* CMD_GET_VOLUME = "GET_VOLUME";
constexpr char const* CMD_MUTE = "MUTE";
constexpr char const* CMD_UNMUTE = "UNMUTE";
constexpr char const* CMD_TOGGLE_FULLSCREEN = "TOGGLE_FULLSCREEN";
constexpr char const* CMD_SET_WARP = "SET_WARP";
constexpr char const* CMD_GET_WARP = "GET_WARP";
constexpr char const* CMD_GET_VERSION = "GET_VERSION";
constexpr char const* CMD_LIST_FLOPPIES = "LIST_FLOPPIES";
constexpr char const* CMD_LIST_CONFIGS = "LIST_CONFIGS";
constexpr char const* CMD_FRAME_ADVANCE = "FRAME_ADVANCE";
constexpr char const* CMD_SET_MOUSE_SPEED = "SET_MOUSE_SPEED";
constexpr char const* CMD_SEND_MOUSE = "SEND_MOUSE";
constexpr char const* CMD_PING = "PING";
constexpr char const* CMD_HELP = "HELP";

// Response status
constexpr char const* RESPONSE_OK = "OK";
constexpr char const* RESPONSE_ERROR = "ERROR";

// IPC Setup and cleanup
void IPCSetup();
void IPCCleanUp();

// Call from main loop to handle incoming connections
void IPCHandle();

// Check if IPC is active
bool IPCIsActive();

} // namespace Amiberry::IPC

#endif // USE_IPC_SOCKET
