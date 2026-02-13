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

- **Version**: 8.0.0 (Public Beta 20)
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
- `src/osdep/amiberry_gui.cpp` - GUI management
- `src/cfgfile.cpp` - Configuration file parsing
- `src/custom.cpp` - Amiga custom chip emulation
- `src/newcpu.cpp` - 68000 CPU emulation
- `src/memory.cpp` - Memory management

### GUI System

The project is transitioning to ImGui (`USE_IMGUI`). ImGui panel sources are in `src/osdep/imgui/`:

- `quickstart.cpp` - Quick start panel
- `configurations.cpp` - Configuration management
- `chipset.cpp`, `cpu.cpp`, `ram.cpp` - Hardware settings
- `floppy.cpp`, `hd.cpp` - Storage configuration
- `display.cpp`, `sound.cpp` - Output settings
- `input.cpp` - Controller configuration
- `whdload.cpp` - WHDLoad integration

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

**Features disabled on Windows:** `USE_IPC_SOCKET`, `USE_GPIOD`, `USE_DBUS`, `USE_UAENET_PCAP`, `WITH_VPAR`, physical CD-ROM IOCTL, physical hard drive enumeration.

**Runtime data directory:** The `data/` directory (fonts, icons, controller maps) must exist in the working directory at runtime. On Windows, `get_data_directory()` returns `getcwd() + "\data\"`. Missing fonts will crash debug builds due to ImGui assertions in `AddFontFromFileTTF`. The fix in `main_window.cpp` checks `std::filesystem::exists(font_path)` before loading.

**Config file I/O:** MinGW links `msvcrt.dll` which doesn't support `"ccs=UTF-8"` fopen mode. Config save/load uses plain `"w"`, `"rt"`, `"wt"` modes under `#ifdef AMIBERRY`.

**Logging:** `write_log()` in `src/osdep/writelog.cpp` requires `--log` CLI flag or `write_logfile` config option. Without either, all logging is silently suppressed.

## Dependencies

### Required

- SDL2, SDL2_image, SDL2_ttf
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
