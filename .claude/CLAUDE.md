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

- **Version**: 8.0.0 (Public Beta 26)
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

#### Android Versioning (Play Store)

Android now derives app version metadata from the top-level `CMakeLists.txt`:
- `project(... VERSION x.y.z)` -> Android `versionName` base
- `set(VERSION_PRE_RELEASE "N")` -> Android beta suffix (`x.y.z-betaN`)

`versionCode` is generated to stay monotonic for Play uploads:
- Formula: `major * 1_000_000 + minor * 10_000 + patch * 100 + slot`
- `slot = N` for pre-release (`VERSION_PRE_RELEASE` 1..98)
- `slot = 99` for final release (no pre-release suffix / `0`)

Examples:
- `8.0.0-beta26` -> `versionCode 8000026`
- `8.0.0` (final) -> `versionCode 8000099`

This ensures final is always greater than any beta for the same `x.y.z`.
If needed, override only the code at build time with:

```bash
cd android
./gradlew bundleRelease -PANDROID_VERSION_CODE=8000100
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
| `USE_IMGUI` | ImGui-based GUI (always enabled; compile definition kept for `#ifdef` guards) |
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
- `src/osdep/macos_bookmarks.h` - macOS security-scoped bookmark API (no-op stubs when not App Store)
- `src/osdep/macos_bookmarks.mm` - macOS security-scoped bookmark implementation (Objective-C++)
- `src/osdep/on_screen_joystick.cpp` - On-screen touch joystick (Android/touchscreens)
- `src/osdep/on_screen_joystick.h` - On-screen joystick public API
- `src/osdep/on_screen_joystick_gl.h/.cpp` - On-screen joystick GL shader infrastructure
- `src/osdep/vkbd/vkbd_gl.h/.cpp` - Virtual keyboard GL shader infrastructure
- `src/osdep/irenderer.h` - Abstract renderer interface
- `src/osdep/opengl_renderer.h/.cpp` - OpenGL renderer backend
- `src/osdep/sdl_renderer.h/.cpp` - SDL3 renderer backend
- `src/osdep/renderer_factory.h/.cpp` - Renderer creation factory
- `src/osdep/gl_platform.h/.cpp` - GL function pointer loading / GLES3 includes
- `src/osdep/gl_overlays.cpp` - GL OSD overlay rendering
- `src/osdep/amiberry_gui.cpp` - GUI management
- `src/cfgfile.cpp` - Configuration file parsing
- `src/custom.cpp` - Amiga custom chip emulation
- `src/newcpu.cpp` - 68000 CPU emulation
- `src/memory.cpp` - Memory management
- `src/osdep/amiberry_mem.cpp` - Natmem shared memory mapping and gap management
- `src/osdep/sigsegv_handler.cpp` - SIGSEGV handler for JIT code faults
- `src/vm.cpp` - Virtual memory abstraction (reserve/commit/decommit)

### ROM Detection & Registry System

**Registry:** Amiberry uses an INI-file-based "registry" (`src/osdep/registry.cpp`, always `inimode=1`) stored in `amiberry.ini`. Sections act as registry "trees" (e.g., `[DetectedROMs]`). Persists across launches.

**ROM scanning flow:**
1. `amiberry_main()` early arg loop processes `--rescan-roms` → sets `forceroms = 1`
2. `initialize_ini()` → `read_rom_list(true)` checks `!exists || forceroms`
3. If true: `scan_roms()` runs — walks ROM path, CRC32-matches against `rommgr.cpp` database (340+ entries), writes to `[DetectedROMs]` in INI, adds built-in AROS ROMs (crc32==0xffffffff)
4. `read_rom_list()` loads entries from INI into global `rl[]` array
5. Model selection (`bip_a500()`, etc.) calls `configure_rom(p, roms, romcheck)` which searches `rl[]` for matching ROM IDs in priority order

**Critical timing:** `initialize_ini()` runs BEFORE `parse_cmdline_and_init_file()`. CLI flags affecting ROM scanning must be processed in the early arg loop in `amiberry_main()` (alongside `--log`, `--jit-selftest`), not in `parse_cmdline()`.

**`--rescan-roms` flag:** Forces `scan_roms()` on every launch. Android always passes this to ensure newly added ROMs are detected. Added in the early arg scan at `amiberry.cpp:5441`.

### CLI Argument Processing Order

Startup in `amiberry_main()` processes arguments in phases:
1. **Early arg scan** (`amiberry_main()` loop) — `--log`, `--jit-selftest`, `--rescan-roms`
2. `parse_amiberry_cmd_line()` — Amiberry-specific args (custom data dir, etc.)
3. **`initialize_ini()`** — loads `amiberry.ini`, calls `read_rom_list()` (ROM scanning happens here)
4. `parse_cmdline_and_init_file()`:
   a. `parse_cmdline_2()` — `-cfgparam` only
   b. `target_cfgfile_load(optionsfile)` — loads default config if exists
   c. `parse_cmdline()` — `--model`, `--config`, `-0`, `--autoload`, `-s`, etc. (processed in argv order)

**Key implication:** `--model` and `--config` in `parse_cmdline()` execute in command-line order. When Android passes `--model A500 --config file.uae`, the config file loads AFTER `bip_a500()` and can override ROM selection via `kickstart_rom_file`.

### Natmem / JIT Memory System

Amiga address space mapped into contiguous host virtual memory ("natmem") for direct `natmem_offset + M68k_addr` access. `uae_vm_reserve()` reserves up to 2GB with `PROT_NONE`; banks committed via `uae_vm_commit()` during `memory_reset()`. Key vars (`amiberry_mem.cpp`): `natmem_reserved`, `natmem_offset`, `canbang`.

**Gap handling**: `commit_natmem_gaps()` fills unmapped regions between banks. Skips `memset` when fill=0 on POSIX (kernel zero-fills pages, avoids OOM on Android with ~1.8GB trailing gap). `canbang` is false when JIT disabled (`cachesize==0`), so gaps are only committed with JIT active.

**SIGSEGV handler** (`sigsegv_handler.cpp`): Decodes ARM64 LDR/STR in JIT code range and emulates via bank handlers.

### JIT Compiler

**ARM64** (`src/jit/arm/`): Works on macOS, Linux, and Android. Key design principle: PC_P (register 16) is the ONLY 64-bit virtual register; all others hold 32-bit M68k values. 19 fix points ensure 64-bit correctness (type widening to `uintptr`, `LOAD_U64` paths, BSR.L/Bcc.L sign extension via `(uae_s32)` cast). Inter-block flag optimization disabled (`#if 0` in `compile_block()`). `UAE_VM_32BIT` dropped from natmem reservation (JIT is 64-bit-clean). Dynamic block quarantine for unstable JIT blocks (O(1) bitmap). The `amiberry-jit-fix` skill contains full debugging history and fix details.

**Android specifics**: Code memory allocated RWX permanently (no W^X toggling). `flush_cpu_icache()` uses `__builtin___clear_cache()`. SELinux error hint logged if RWX fails.

**x86_64** (`src/jit/x86/`): 64-bit pointer-clean, PIE-compatible. `JITPTR` = `(uintptr)`, PC_P uses 64-bit loads/stores, ADDR32 prefix removed, `isconst` shortcuts disabled for natmem addresses. `UAE_VM_32BIT` kept for x86-64 natmem (ensures `comp_pc_p` fits 32 bits — x86-64 still has 32-bit PC_P arithmetic paths). Anchor-based JIT allocator (`jit_vm_acquire()`) places code within ±2GB of `.data` globals for RIP-relative addressing; blockinfo pools and veccode allocated near the JIT cache. If no suitable VA is found, JIT is gracefully disabled at runtime.

**Debugging env vars**: `AMIBERRY_ARM64_GUARD_VERBOSE=1`, `AMIBERRY_ARM64_ENABLE_HOTSPOT_GUARD=1`.

### GUI System

ImGui is the GUI framework (`USE_IMGUI` compile definition is always set). ImGui panel sources are in `src/osdep/imgui/`:

- `quickstart.cpp` - Quick start panel
- `configurations.cpp` - Configuration management
- `chipset.cpp`, `cpu.cpp`, `ram.cpp` - Hardware settings
- `floppy.cpp`, `hd.cpp` - Storage configuration
- `display.cpp`, `sound.cpp` - Output settings
- `input.cpp` - Controller configuration
- `whdload.cpp` - WHDLoad integration

### Rendering Architecture

Two backends selected at compile time via `USE_OPENGL`:
- **OpenGL** (`OpenGLRenderer`): Hardware rendering with shaders (CRT, external GLSL, GLSLP presets, bezels)
- **SDL3** (`SDLRenderer`): Software/basic rendering via `SDL_Renderer`

**IRenderer abstraction** (`irenderer.h`): Abstract base class for all rendering ops. Global `g_renderer` (from `renderer_factory.cpp`) dispatches to active backend. `OpenGLRenderer` owns GL context + `ShaderState` + `GLOverlayState`. `SDLRenderer` owns `SDL_Texture*`. Accessed via `get_opengl_renderer()` / `get_sdl_renderer()`.

**GL infrastructure extraction**: Shader compilation, texture upload, and quad rendering extracted into `_gl` files (wrapped internally in `#ifdef USE_OPENGL`). GL render entry points (`vkbd_redraw_gl`, `on_screen_joystick_redraw_gl`) stay in original files (access file-local statics). Extracted files expose init/upload/render/cleanup/accessor functions.

| Component | Common logic | GL infrastructure |
|-----------|-------------|-------------------|
| Virtual keyboard | `vkbd/vkbd.cpp` | `vkbd/vkbd_gl.h/.cpp` |
| On-screen joystick | `on_screen_joystick.cpp` | `on_screen_joystick_gl.h/.cpp` |
| OSD overlays | `amiberry_gfx.cpp` | `gl_overlays.cpp` |

### Custom Bezel Overlay System

PNG/JPEG overlays with transparent screen hole, works with all shader types. Alpha channel scanned at load to detect hole (threshold < 128), stored as normalized coordinates.

**Rendering order** in `show_screen()`: clear → shader output (constrained to hole via `renderAreaX/Y/W/H`) → cursor → bezel (full window, alpha blended) → OSD/vkbd/joystick → SwapWindow.

**Key design**: GL calls only in render context (lazy load on first frame). `update_custom_bezel()` from GUI only clears `loaded_bezel_name`. `crtemu_t::skip_aspect_correction` bypasses 4:3 letterboxing. Built-in CRT and custom bezels are mutually exclusive. Config: `use_custom_bezel`, `custom_bezel`, `bezels_path` in `amiberry.conf`.

### On-Screen Joystick (Touch Controls)

Touch D-pad and fire buttons for Android/touchscreens. Common logic in `on_screen_joystick.cpp`; GL infrastructure extracted to `on_screen_joystick_gl.h/.cpp`.

Registered as a virtual joystick device (last in `di_joystick[]`, 2 axes, 2 buttons). Input via `setjoystickstate()`/`setjoybuttonstate()`. Auto-assigns to Port 1 when enabled. Key functions: `on_screen_joystick_set_enabled()`, `on_screen_joystick_update_layout()`, `on_screen_joystick_handle_finger_down/up/motion()`.

**Touch filtering**: When active, synthetic mouse events (`SDL_TOUCH_MOUSEID`) are filtered in `amiberry.cpp` to prevent D-pad touches becoming mouse input. Config: `onscreen_joystick` (bool), `input_default_onscreen_keyboard` (bool) in `struct uae_prefs`.

### Input Device System Overview

UAE input uses `inputdevice_functions` structs per device type. Joysticks stored in `di_joystick[]` (count: `num_joystick`). Port assignment: `JSEM_JOYS + index` / `JSEM_MICE + index` / `JSEM_KBDLAYOUT + index` → `changed_prefs.jports[port].id`.

**Input injection**: Use `setjoystickstate()`/`setjoybuttonstate()` (proper device API with port mode/mapping). Avoid `send_input_event()` (bypasses port mode) for new code. **Device registration**: append to `di_joystick[]`, increment `num_joystick`, call `cleardid()` + `fixthings()`.

### Android App Architecture

Kotlin/Jetpack Compose front-end (`android/`) launches native SDL emulator in a separate `:sdl` process. No JNI runtime control — communication is CLI-style via `Intent.putExtra("SDL_ARGS", String[])`. `AmiberryActivity` extends `SDLActivity`; `onDestroy()` kills the `:sdl` process. `MainActivity` extracts assets on first run per version (`.extracted_version` marker).

**Launch patterns** (`data/EmulatorLauncher.kt`):
- Quick Start: `--rescan-roms --model <model> [-0 <floppy>] [-s cdimage0=<cd>] -G`
- Config: `--rescan-roms --config <path> [-G]`
- WHDLoad: `--rescan-roms --autoload <lha> -G`
- Advanced (ImGui): `--rescan-roms [--config <path>] -s use_gui=yes`

**ROM ID priority** (must match C++ `--model` / `bip_*`; update `MODEL_ROM_PROFILE` if C++ changes):
- A500: 6,5,4 | A500P: 7,6,5 | A600: 10,9,8 | A1000: 24 | A2000: 6,5,4 | A3000: 59
- A1200: 11,15,276,281,286,291,304 | A4000: 16,31,13,12,46,278,283,288,293,306
- CD32: kick 64,18; ext 19 (required when kick=18) | CDTV: kick 6,32; ext 20,21,22

**Hardware constraints** (`SettingsViewModel.applyConstraints`): 68000/68010 forces 24-bit (disables Z3/JIT). `cycle_exact` forces `cpu_speed=real`, disables JIT. JIT requires 68020+, forces `cpu_speed=max`. Z3 requires 32-bit. On-screen joystick disabled on non-touch devices.

### Platform Support

Platform-specific code is in `src/osdep/`:

- Common POSIX code works across Linux, macOS, FreeBSD
- Android has additional handling in `android/` directory
- Architecture-specific JIT in `src/jit/arm/` and `src/jit/x86/`
- Windows uses MinGW-w64 which provides POSIX-compatible headers (`unistd.h`, `dirent.h`, etc.)

### macOS Port Notes

- **Home directory**: `~/Library/Application Support/Amiberry` (has spaces — quote all paths in shell commands). Config at `Configurations/amiberry.conf`.
- **Security-scoped bookmarks** (`macos_bookmarks.h/.mm`): For App Sandbox. Active only when `MACOS_APP_STORE` defined; no-op stubs otherwise. Every `set_*_path()` calls `macos_bookmark_store()`.
- **Folder setup**: `create_missing_amiberry_folders()` uses `std::filesystem::copy()` (not `system("cp -R ...")`) for space-safe paths. `download_file()` quotes curl/wget args.
- **CD-ROMs**: `blkdev_ioctl.cpp` uses raw `/dev/rdiskX` for audio sectors; temporarily reverts to `/dev/diskX` for DiskArbitration TOC queries.

### Windows Port Notes

- `sysconfig.h` previously `#undef _WIN32` — removed to unlock WinUAE code paths. LLP64: `SIZEOF_LONG` must be 4.
- `sysconfig.h` includes `<winsock2.h>` at the very top to prevent `std::byte` vs `rpcndr.h byte` ambiguity.
- `crtemu.h` uses `CRTEMU_SDL` path on Windows+Amiberry (not WinUAE's LoadLibrary path).
- **Symlinks**: WHDBooter uses `std::filesystem::copy()` on Windows (symlinks need admin/DevMode).
- **Winsock**: `src/slirp/` uses `closesocket()`, `ioctlsocket()`, `WSAGetLastError()`, cast to `(char*)`.
- **bsdsocket.library**: Windows uses WSAAsyncSelect + hidden HWND model (`bsdsocket_host.cpp`). POSIX uses select() + worker threads.
- **Disabled features**: `USE_IPC_SOCKET`, `USE_GPIOD`, `USE_DBUS`, `USE_UAENET_PCAP`, `WITH_VPAR`, physical CD/HD.
- **Runtime**: `data/` directory must exist in working directory. Missing fonts crash debug builds. `--log` required for logging output.

## Dependencies

### Required

- SDL3, SDL3_image

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

ImGui is always enabled. Main branch is `master`.

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
