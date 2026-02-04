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
