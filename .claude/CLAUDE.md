# CLAUDE.md - Amiberry Project Guide

This file provides guidance for Claude Code when working with the Amiberry codebase.

## Project Overview

Amiberry is an optimized Amiga emulator based on UAE (Unix Amiga Emulator). It provides accurate Amiga emulation with a focus on performance.

**Supported Platforms:**
- **Linux**: x86_64, ARM (including Raspberry Pi and other SBCs), AArch64
- **macOS**: Intel (x86_64) and Apple Silicon (ARM64)
- **Android**: AArch64 and x86_64
- **FreeBSD**: x86_64

- **Version**: 8.0.0 (pre-release 18)
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

Keep platform code in `src/osdep/` and use preprocessor guards for platform-specific sections (`#ifdef __ANDROID__`, `#ifdef __APPLE__`, etc.).
