# CLAUDE.md - Amiberry Project Guide

This file provides guidance for Claude Code when working with the Amiberry codebase.

## Project Overview

Amiberry is an optimized Amiga emulator based on UAE (Unix Amiga Emulator). It provides accurate Amiga emulation with a focus on performance.

**Supported Platforms:**
- **Linux**: x86_64, ARM (including Raspberry Pi and other SBCs), AArch64
- **macOS**: Intel (x86_64) and Apple Silicon (ARM64)
- **Android**: AArch64 and x86_64
- **FreeBSD**: x86_64
- **Windows**: x86_64 (MinGW-w64/GCC, dependencies via vcpkg)

- **Version**: 8.0.0 (Public Beta 22)
- **Languages**: C/C++
- **Build System**: CMake (minimum 3.16)
- **License**: GPL (see LICENSE file)
- **Homepage**: https://amiberry.com

## Build Commands

### Desktop (Linux/macOS)

```bash
# Configure (out-of-tree build required)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Install
cmake --install build
```

### macOS with Homebrew dependencies

```bash
brew bundle  # Install dependencies from Brewfile
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Android

The Android build uses Gradle and fetches dependencies via CMake FetchContent:

```bash
cd android
./gradlew assembleRelease
```

### Windows (MinGW-w64)

```powershell
# Prerequisites: MinGW-w64 (GCC), CMake, Ninja, vcpkg
# Set environment
$env:VCPKG_ROOT = "D:\Github\vcpkg"

# Configure with CMake preset
cmake --preset windows-debug   # or windows-release

# Build
cd out/build/windows-debug
ninja -j12

# Run (no GUI test)
.\amiberry.exe -G

# Run with logging
.\amiberry.exe --log
```

Or use the helper script:
```powershell
powershell -ExecutionPolicy Bypass -File "build_and_run.ps1"
```

### Build Options

Key CMake options (all default ON unless noted):

| Option | Description |
|--------|-------------|
| `USE_PCEM` | PCem x86 emulation (disabled on RISC-V) |
| `USE_IMGUI` | ImGui-based GUI (current active development) |
| `USE_OPENGL` | OpenGL rendering |
| `USE_LIBSERIALPORT` | Serial port emulation |
| `USE_LIBENET` | Network emulation |
| `USE_PORTMIDI` | MIDI support |
| `USE_LIBMPEG2` | CD32 FMV support |
| `USE_ZSTD` | CHD compressed disk image support |
| `USE_GPIOD` | GPIO LED control (OFF by default) |
| `USE_DBUS` | DBus control (OFF by default) |
| `WITH_LTO` | Link Time Optimization (OFF by default) |
| `WITH_OPTIMIZE` | Native CPU optimizations (OFF by default) |

## Project Architecture

### Directory Structure

```
amiberry/
├── src/                    # Main source code
│   ├── osdep/             # Platform-specific code (OS-dependent)
│   │   ├── imgui/         # ImGui GUI panels
│   │   ├── gui/           # Legacy GUI code
│   │   └── vkbd/          # Virtual keyboard
│   ├── include/           # Core emulator headers
│   ├── archivers/         # Archive format support (7z, chd, dms, lha, lzx, zip)
│   ├── jit/               # JIT compiler (ARM and x86)
│   ├── pcem/              # PCem x86 emulation
│   ├── slirp/             # TCP/IP stack for networking
│   ├── ppc/               # PowerPC emulation
│   └── qemuvga/           # QEMU VGA emulation
├── external/              # Third-party libraries
│   ├── imgui/             # Dear ImGui
│   ├── ImGuiFileDialog/   # File dialog for ImGui
│   ├── mt32emu/           # Roland MT-32 emulation
│   ├── floppybridge/      # Physical floppy drive support
│   ├── capsimage/         # IPF disk image support
│   └── libguisan/         # Legacy GUI library
├── cmake/                 # CMake modules and platform configs
├── data/                  # Runtime data files (icons, fonts, etc.)
├── controllers/           # Controller mapping files
├── whdboot/              # WHDLoad boot files
├── android/              # Android-specific build files
└── packaging/            # Distribution packaging files
```

### Key Source Files

- `src/main.cpp` - Application entry point
- `src/osdep/amiberry.cpp` - Main platform abstraction (~170KB, core initialization)
- `src/osdep/amiberry_gfx.cpp` - Graphics/display handling
- `src/osdep/amiberry_input.cpp` - Input device handling
- `src/osdep/amiberry_input.h` - Input device types and declarations
- `src/osdep/on_screen_joystick.cpp` - On-screen touch joystick (Android/touchscreens)
- `src/osdep/on_screen_joystick.h` - On-screen joystick public API
- `src/osdep/amiberry_gui.cpp` - GUI management
- `src/cfgfile.cpp` - Configuration file parsing
- `src/custom.cpp` - Amiga custom chip emulation
- `src/newcpu.cpp` - 68000 CPU emulation
- `src/memory.cpp` - Memory management
- `src/osdep/amiberry_mem.cpp` - Natmem shared memory mapping and gap management
- `src/osdep/sigsegv_handler.cpp` - SIGSEGV handler for JIT code faults
- `src/vm.cpp` - Virtual memory abstraction (reserve/commit/decommit)

### Natmem / JIT Memory System

The emulator maps Amiga's address space into a contiguous host virtual memory block ("natmem") for direct pointer-based access from both the JIT compiler and interpreter (`canbang` mode).

**Architecture:**
- `uae_vm_reserve()` reserves a large block (up to 2GB) with `PROT_NONE` (no access)
- Individual memory banks (Chip RAM, ROMs, Z3 RAM, RTG VRAM) are committed via `uae_vm_commit()` during `memory_reset()`
- JIT-compiled code and interpreter access emulated memory as `natmem_offset + M68k_address` — direct dereference, no bank lookup
- `shmids[]` array (MAX_SHMID=256) tracks all committed regions

**Gap handling (`commit_natmem_gaps()`):**
Gaps between committed banks (e.g., 0x00F10000-0x00F7FFFF between Boot ROM and Kickstart ROM) would cause SIGSEGV on access. After all banks are mapped, `commit_natmem_gaps()` walks the natmem space and commits any remaining gaps with the correct fill value (zero or 0xFF based on `cs_unmapped_space`). This is called at the end of `memory_reset()` behind `#ifdef NATMEM_OFFSET`.

**Key variables** (in `amiberry_mem.cpp`):
- `natmem_reserved` — base pointer of reserved block
- `natmem_reserved_size` — total size of reserved block
- `natmem_offset` — pointer used for M68k address translation (may differ from `natmem_reserved` when RTG VRAM is offset)
- `canbang` — flag indicating direct natmem access is active

**SIGSEGV handler** (`sigsegv_handler.cpp`):
When a fault occurs inside JIT code range, the handler decodes the ARM64 LDR/STR instruction and emulates the access via bank handlers. Faults outside JIT code range are not recovered.

### ARM64 JIT Stability Notes (2026-02)

- ARM64 now defaults to direct JIT address mode (`jit_n_addr_unsafe=0` at reset). Unsafe banks still flip to indirect mode via normal `S_N_ADDR` handling in `map_banks()`.
- ARM64 keeps ROM and UAE Boot ROM (`rtarea`) blocks in interpreter mode to avoid known unstable codegen paths.
- A fixed safety fallback remains for the known Lightwave startup hotspot (`PC` range `0x4003df00`-`0x4003e1ff`), even if optional hotspot guard toggles are disabled.
- ARM64 dynamically quarantines unstable JIT blocks learned at runtime (SIGSEGV/JIT fault recovery, autoconfig warning paths, and selected illegal-op startup probes), then runs those blocks interpreted.
- Dynamic unstable-key lookup uses a bitmap (O(1) lookup), reset on `compemu_reset()`.
- Guard logging is quiet by default; set `AMIBERRY_ARM64_GUARD_VERBOSE=1` for per-key/per-window quarantine logs during investigation.
- `AMIBERRY_ARM64_DISABLE_HOTSPOT_GUARD=1` now only disables the optional guard path and is retained for A/B diagnostics; it does not bypass the fixed safety hotspot guard.

### GUI System

The project is transitioning to ImGui (`USE_IMGUI`). ImGui panel sources are in `src/osdep/imgui/`:

- `quickstart.cpp` - Quick start panel
- `configurations.cpp` - Configuration management
- `chipset.cpp`, `cpu.cpp`, `ram.cpp` - Hardware settings
- `floppy.cpp`, `hd.cpp` - Storage configuration
- `display.cpp`, `sound.cpp` - Output settings
- `input.cpp` - Controller configuration
- `whdload.cpp` - WHDLoad integration

### On-Screen Joystick (Touch Controls)

The on-screen joystick provides touch-based D-pad and fire buttons for Android and other touchscreen devices. It is implemented in `src/osdep/on_screen_joystick.cpp` with rendering support for both SDL2 renderer and OpenGL.

**Architecture:**
- Registered as a **virtual joystick device** ("On-Screen Joystick") in the UAE input device system via `init_joystick()` in `amiberry_input.cpp`
- Always the **last device** in `di_joystick[]` (appended after physical SDL joysticks)
- Has 2 axes (X, Y) and 2 buttons (Fire, 2nd Fire), configured with proper `axismappings`, `buttonmappings`, and metadata
- Input injected via `setjoystickstate()` / `setjoybuttonstate()` — the proper UAE input device API that routes through the full input mapping and port mode system
- **Auto-assigns to Port 1** in joystick mode when enabled via `on_screen_joystick_set_enabled(true)`, using `JSEM_JOYS + dev` as the port assignment ID and calling `inputdevice_config_change()` to reconfigure

**Key functions:**
- `get_onscreen_joystick_device_index()` — returns the `di_joystick[]` index, or -1 if not registered (declared in `amiberry_input.h`)
- `on_screen_joystick_set_enabled(bool)` — enables/disables and handles auto-assignment
- `on_screen_joystick_update_layout()` — recalculates positions when screen geometry changes
- `on_screen_joystick_handle_finger_down/up/motion()` — touch event handlers; return true if consumed

**SDL touch-to-mouse filtering:**
SDL2 synthesizes mouse events from touch events by default (`SDL_HINT_TOUCH_MOUSE_EVENTS = "1"`). When the on-screen joystick is active, `amiberry.cpp` filters out these synthetic events by checking `event.button.which == SDL_TOUCH_MOUSEID` / `event.motion.which == SDL_TOUCH_MOUSEID` to prevent touch D-pad input from being interpreted as Amiga mouse movement.

**Layout sizing constants** (in `on_screen_joystick.cpp`):
- `DPAD_SIZE_FRACTION = 0.38` — D-pad diameter as fraction of min(screen_w, screen_h)
- `BUTTON_SIZE_FRACTION = 0.22` — fire button diameter
- `BUTTON_GAP_FRACTION = 0.05` — gap between fire buttons
- `KB_BUTTON_SIZE_FRACTION = 0.14` — keyboard toggle button size

**Config options** (in `struct uae_prefs`):
- `onscreen_joystick` — enable on-screen joystick (boolean)
- `input_default_onscreen_keyboard` — show keyboard button on the on-screen overlay

### Input Device System Overview

The UAE input device system uses `inputdevice_functions` structs for each device type (`IDTYPE_JOYSTICK`, `IDTYPE_MOUSE`, `IDTYPE_KEYBOARD`). Joystick devices are stored in the `di_joystick[]` array with count `num_joystick`.

**Port assignment IDs:** `JSEM_JOYS + device_index` for joysticks, `JSEM_MICE + index` for mice, `JSEM_KBDLAYOUT + index` for keyboard layouts. These are stored in `changed_prefs.jports[port].id`.

**Input injection APIs:**
- `setjoystickstate(dev, axis, state, max)` / `setjoybuttonstate(dev, button, state)` — proper device API, respects port mode (joystick/mouse/analog) and full input mapping
- `send_input_event(INPUTEVENT_JOY*_*, state, max, autofire)` — direct event injection, bypasses port mode. Avoid for new code.

**Device registration pattern:** Append to `di_joystick[]`, increment `num_joystick`, call `cleardid()` + `fixthings()` to initialize metadata, set axes/buttons/names/mappings. The device then appears in the Input configuration GUI dropdown.

### Platform Support

Platform-specific code is in `src/osdep/`:

- Common POSIX code works across Linux, macOS, FreeBSD
- Android has additional handling in `android/` directory
- Architecture-specific JIT in `src/jit/arm/` and `src/jit/x86/`
- Windows uses MinGW-w64 which provides POSIX-compatible headers (`unistd.h`, `dirent.h`, etc.)

### Windows Port Notes

**Key Differences from POSIX platforms:**
- `sysconfig.h` previously `#undef _WIN32` — this was removed to unlock WinUAE Windows code paths
- Windows LLP64: `long` is 4 bytes (not 8); `SIZEOF_LONG` must be 4 in `sysconfig.h`
- JIT compiler is non-functional on 64-bit Windows (pointers exceed 32-bit); interpreter mode works fine
- `src/jit/x86/compemu_x86.h`: `check_uae_p32()` logs instead of calling `jit_abort()` under `AMIBERRY`
- `src/osdep/crtemu.h` line 80: uses `CRTEMU_SDL` path on Windows+Amiberry (not WinUAE's direct LoadLibrary path)

**Symlinks:** Windows requires admin/DevMode for symlinks and uses different APIs for file vs directory symlinks. The WHDBooter (`src/osdep/amiberry_whdbooter.cpp`) uses `std::filesystem::copy()` directly on Windows for directory links, with try-catch fallback for file links.

**Winsock:** `src/slirp/` has full Winsock2 support. Key pattern differences: `closesocket()` not `close()`, `ioctlsocket()` not `ioctl()`/`fcntl()`, `WSAGetLastError()` not `errno`, cast socket options to `(char*)`.

**bsdsocket.library emulation:** On Windows, uses WinUAE's native WSAAsyncSelect + hidden HWND model (`src/osdep/bsdsocket_host.cpp`, `#ifdef _WIN32` block). Architecture: hidden window receives async socket events, dedicated `sock_thread` with `MsgWaitForMultipleObjects` loop, thread pools for WaitSelect (64 threads) and DNS lookups (64 threads), `CRITICAL_SECTION` synchronization. On POSIX platforms, uses the original Amiberry implementation (select() + worker threads). Key MinGW adaptation: no SEH (`__try/__except` removed), `GetModuleHandle(NULL)` replaces WinUAE's `hInst`.

**std::byte ambiguity:** `sysconfig.h` includes `<winsock2.h>` at the very top on Windows, before any C++ standard headers. This prevents the `std::byte` vs `rpcndr.h byte` typedef ambiguity that occurs when `sysdeps.h`'s `using namespace std;` pulls `std::byte` into the global namespace before Windows headers define their `byte` typedef.

**Features disabled on Windows:** `USE_IPC_SOCKET`, `USE_GPIOD`, `USE_DBUS`, `USE_UAENET_PCAP`, `WITH_VPAR`, physical CD-ROM IOCTL, physical hard drive enumeration.

**Runtime data directory:** The `data/` directory (fonts, icons, controller maps) must exist in the working directory at runtime. On Windows, `get_data_directory()` returns `getcwd() + "\data\"`. Missing fonts will crash debug builds due to ImGui assertions in `AddFontFromFileTTF`. The fix in `main_window.cpp` checks `std::filesystem::exists(font_path)` before loading.

**Config file I/O:** MinGW links `msvcrt.dll` which doesn't support `"ccs=UTF-8"` fopen mode. Config save/load uses plain `"w"`, `"rt"`, `"wt"` modes under `#ifdef AMIBERRY`.

**Logging:** `write_log()` in `src/osdep/writelog.cpp` requires `--log` CLI flag or `write_logfile` config option. Without either, all logging is silently suppressed.

## Dependencies

### Required

- SDL2, SDL2_image
- ZLIB
- libpng
- FLAC
- mpg123 (desktop only)

### Optional

- zstd (CHD support)
- libserialport (serial port emulation)
- libenet (network emulation)
- PortMidi (MIDI support)
- libmpeg2 (CD32 FMV)
- libpcap (uaenet)

### Bundled in external/

- Dear ImGui
- ImGuiFileDialog
- MT-32 emulator (mt32emu)
- FloppyBridge
- CAPSImage (IPF support)

## Code Style Guidelines

- Use tabs for indentation in C/C++ files
- Follow existing naming conventions in each subsystem
- Emulator core code follows UAE conventions
- GUI code follows ImGui patterns
- Platform code uses `amiberry_` prefix

## Current Development Branch

The `imgui` branch is the active development branch focusing on the ImGui GUI implementation. Main branch is `master`.

## Testing

No automated test suite. Manual testing with various Amiga software and configurations is the primary testing method.

## Common Tasks

### Adding a new ImGui panel

1. Create new `.cpp` file in `src/osdep/imgui/`
2. Add to `IMGUI_GUI_FILES` in `cmake/SourceFiles.cmake`
3. Include panel in the main GUI rendering loop

### Adding a new configuration option

1. Update `src/cfgfile.cpp` for parsing/saving
2. Update relevant GUI panel in `src/osdep/imgui/`
3. Add to `struct uae_prefs` in include files if needed

### Platform-specific changes

Keep platform code in `src/osdep/` and use preprocessor guards for platform-specific sections:
- `#ifdef __ANDROID__` — Android-specific
- `#ifdef __APPLE__` — macOS-specific
- `#ifdef _WIN32` — Windows-specific
- `#ifdef AMIBERRY` — Amiberry-specific (distinguishes from WinUAE)
- `#if defined(__ANDROID__) || defined(_WIN32)` — platforms without reliable symlink support
