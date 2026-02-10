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

// Joystick, Quicksave, LED status, etc.
constexpr char const* CMD_QUICKSAVE = "QUICKSAVE";
constexpr char const* CMD_QUICKLOAD = "QUICKLOAD";
constexpr char const* CMD_GET_JOYPORT_MODE = "GET_JOYPORT_MODE";
constexpr char const* CMD_SET_JOYPORT_MODE = "SET_JOYPORT_MODE";
constexpr char const* CMD_GET_AUTOFIRE = "GET_AUTOFIRE";
constexpr char const* CMD_SET_AUTOFIRE = "SET_AUTOFIRE";
constexpr char const* CMD_GET_LED_STATUS = "GET_LED_STATUS";
constexpr char const* CMD_LIST_HARDDRIVES = "LIST_HARDDRIVES";
constexpr char const* CMD_SET_DISPLAY_MODE = "SET_DISPLAY_MODE";
constexpr char const* CMD_GET_DISPLAY_MODE = "GET_DISPLAY_MODE";
constexpr char const* CMD_SET_NTSC = "SET_NTSC";
constexpr char const* CMD_GET_NTSC = "GET_NTSC";
constexpr char const* CMD_SET_SOUND_MODE = "SET_SOUND_MODE";
constexpr char const* CMD_GET_SOUND_MODE = "GET_SOUND_MODE";

// Mouse, CPU, RTG, Floppy, Chipset, Memory
constexpr char const* CMD_TOGGLE_MOUSE_GRAB = "TOGGLE_MOUSE_GRAB";
constexpr char const* CMD_GET_MOUSE_SPEED = "GET_MOUSE_SPEED";
constexpr char const* CMD_SET_CPU_SPEED = "SET_CPU_SPEED";
constexpr char const* CMD_GET_CPU_SPEED = "GET_CPU_SPEED";
constexpr char const* CMD_TOGGLE_RTG = "TOGGLE_RTG";
constexpr char const* CMD_SET_FLOPPY_SPEED = "SET_FLOPPY_SPEED";
constexpr char const* CMD_GET_FLOPPY_SPEED = "GET_FLOPPY_SPEED";
constexpr char const* CMD_DISK_WRITE_PROTECT = "DISK_WRITE_PROTECT";
constexpr char const* CMD_GET_DISK_WRITE_PROTECT = "GET_DISK_WRITE_PROTECT";
constexpr char const* CMD_TOGGLE_STATUS_LINE = "TOGGLE_STATUS_LINE";
constexpr char const* CMD_SET_CHIPSET = "SET_CHIPSET";
constexpr char const* CMD_GET_CHIPSET = "GET_CHIPSET";
constexpr char const* CMD_GET_MEMORY_CONFIG = "GET_MEMORY_CONFIG";
constexpr char const* CMD_GET_FPS = "GET_FPS";

// Memory configuration and Window control
constexpr char const* CMD_SET_CHIP_MEM = "SET_CHIP_MEM";
constexpr char const* CMD_SET_FAST_MEM = "SET_FAST_MEM";
constexpr char const* CMD_SET_SLOW_MEM = "SET_SLOW_MEM";
constexpr char const* CMD_SET_Z3_MEM = "SET_Z3_MEM";
constexpr char const* CMD_GET_CPU_MODEL = "GET_CPU_MODEL";
constexpr char const* CMD_SET_CPU_MODEL = "SET_CPU_MODEL";
constexpr char const* CMD_SET_WINDOW_SIZE = "SET_WINDOW_SIZE";
constexpr char const* CMD_GET_WINDOW_SIZE = "GET_WINDOW_SIZE";
constexpr char const* CMD_SET_SCALING = "SET_SCALING";
constexpr char const* CMD_GET_SCALING = "GET_SCALING";
constexpr char const* CMD_SET_LINE_MODE = "SET_LINE_MODE";
constexpr char const* CMD_GET_LINE_MODE = "GET_LINE_MODE";
constexpr char const* CMD_SET_RESOLUTION = "SET_RESOLUTION";
constexpr char const* CMD_GET_RESOLUTION = "GET_RESOLUTION";

// Autocrop and WHDLoad
constexpr char const* CMD_SET_AUTOCROP = "SET_AUTOCROP";
constexpr char const* CMD_GET_AUTOCROP = "GET_AUTOCROP";
constexpr char const* CMD_INSERT_WHDLOAD = "INSERT_WHDLOAD";
constexpr char const* CMD_EJECT_WHDLOAD = "EJECT_WHDLOAD";
constexpr char const* CMD_GET_WHDLOAD = "GET_WHDLOAD";

// Debugging and diagnostics
constexpr char const* CMD_DEBUG_ACTIVATE = "DEBUG_ACTIVATE";
constexpr char const* CMD_DEBUG_DEACTIVATE = "DEBUG_DEACTIVATE";
constexpr char const* CMD_DEBUG_STATUS = "DEBUG_STATUS";
constexpr char const* CMD_DEBUG_STEP = "DEBUG_STEP";
constexpr char const* CMD_DEBUG_STEP_OVER = "DEBUG_STEP_OVER";
constexpr char const* CMD_DEBUG_CONTINUE = "DEBUG_CONTINUE";
constexpr char const* CMD_GET_CPU_REGS = "GET_CPU_REGS";
constexpr char const* CMD_GET_CUSTOM_REGS = "GET_CUSTOM_REGS";
constexpr char const* CMD_DISASSEMBLE = "DISASSEMBLE";
constexpr char const* CMD_SET_BREAKPOINT = "SET_BREAKPOINT";
constexpr char const* CMD_CLEAR_BREAKPOINT = "CLEAR_BREAKPOINT";
constexpr char const* CMD_LIST_BREAKPOINTS = "LIST_BREAKPOINTS";
constexpr char const* CMD_GET_COPPER_STATE = "GET_COPPER_STATE";
constexpr char const* CMD_GET_BLITTER_STATE = "GET_BLITTER_STATE";
constexpr char const* CMD_GET_DRIVE_STATE = "GET_DRIVE_STATE";
constexpr char const* CMD_GET_AUDIO_STATE = "GET_AUDIO_STATE";
constexpr char const* CMD_GET_DMA_STATE = "GET_DMA_STATE";

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
