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
- `src/osdep/macos_bookmarks.h` - macOS security-scoped bookmark API (no-op stubs when not App Store)
- `src/osdep/macos_bookmarks.mm` - macOS security-scoped bookmark implementation (Objective-C++)
- `src/osdep/on_screen_joystick.cpp` - On-screen touch joystick (Android/touchscreens)
- `src/osdep/on_screen_joystick.h` - On-screen joystick public API
- `src/osdep/on_screen_joystick_gl.h/.cpp` - On-screen joystick GL shader infrastructure
- `src/osdep/vkbd/vkbd_gl.h/.cpp` - Virtual keyboard GL shader infrastructure
- `src/osdep/irenderer.h` - Abstract renderer interface
- `src/osdep/opengl_renderer.h/.cpp` - OpenGL renderer backend
- `src/osdep/sdl_renderer.h/.cpp` - SDL2 renderer backend
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

The emulator maps Amiga's address space into a contiguous host virtual memory block ("natmem") for direct pointer-based access from both the JIT compiler and interpreter (`canbang` mode).

**Architecture:**
- `uae_vm_reserve()` reserves a large block (up to 2GB) with `PROT_NONE` (no access)
- Individual memory banks (Chip RAM, ROMs, Z3 RAM, RTG VRAM) are committed via `uae_vm_commit()` during `memory_reset()`
- JIT-compiled code and interpreter access emulated memory as `natmem_offset + M68k_address` — direct dereference, no bank lookup
- `shmids[]` array (MAX_SHMID=256) tracks all committed regions

**Gap handling (`commit_natmem_gaps()`):**
Gaps between committed banks (e.g., 0x00F10000-0x00F7FFFF between Boot ROM and Kickstart ROM) would cause SIGSEGV on access. After all banks are mapped, `commit_natmem_gaps()` walks the natmem space and commits any remaining gaps with the correct fill value (zero or 0xFF based on `cs_unmapped_space`). This is called at the end of `memory_reset()` behind `#ifdef NATMEM_OFFSET`. On POSIX systems, `memset` is skipped when the fill byte is 0x00 because the kernel zero-fills anonymous pages on demand — this avoids physically allocating the trailing gap (which can be ~1.8GB on A4000 configs) and prevents OOM on memory-constrained platforms like Android.

**Key variables** (in `amiberry_mem.cpp`):
- `natmem_reserved` — base pointer of reserved block
- `natmem_reserved_size` — total size of reserved block
- `natmem_offset` — pointer used for M68k address translation (may differ from `natmem_reserved` when RTG VRAM is offset)
- `canbang` — flag indicating direct natmem access is active

**SIGSEGV handler** (`sigsegv_handler.cpp`):
When a fault occurs inside JIT code range, the handler decodes the ARM64 LDR/STR instruction and emulates the access via bank handlers. Faults outside JIT code range are not recovered.

### ARM64 JIT Android Support (2026-02)

The ARM64 JIT compiler works on Android (AArch64). Three issues were solved to enable it:

1. **macOS-only guards generalized**: 14 `#if defined(__APPLE__) && defined(CPU_AARCH64)` guards in `compemu_support_arm.cpp` changed to `#if defined(CPU_AARCH64)`. This gives Android the same RWX code allocation, combined popallspace+cache layout, and 64-bit pointer tolerance that macOS uses. macOS-specific APIs (`MAP_JIT`, `pthread_jit_write_protect_np`) remain behind `__APPLE__` guards.

2. **`UAE_VM_32BIT` scan loop**: `vm_acquire_code()` used the `UAE_VM_32BIT` flag on non-Apple ARM64, causing ~460K futile `mmap`/`munmap` iterations because Android's natmem sits well above 4GB (`0x7604c00000`). Fixed by setting `flags = 0` for non-Apple ARM64.

3. **OOM from natmem gap `memset`**: Enabling JIT sets `canbang=true` (via `jit_direct_compatible_memory` when `cachesize > 0`), which activates `commit_natmem_gaps()`. The trailing gap (~1.84GB on A4000) `memset` forced physical page allocation, OOM-killing on Android. Fixed in `amiberry_mem.cpp`: skip `memset` when `fill_byte == 0` on POSIX (the kernel zero-fills anonymous pages).

**Key insight**: Before JIT is enabled, `cachesize == 0` → `jit_direct_compatible_memory = false` → `canbang = false` → `commit_natmem_gaps()` returns early. The 1.84GB trailing gap was never an issue until JIT activated it.

**Android JIT memory model**: Code memory is allocated RWX (`PROT_READ|PROT_WRITE|PROT_EXEC`) from the start via `uae_vm_alloc(..., UAE_VM_READ_WRITE_EXECUTE)`. Unlike macOS (which uses MAP_JIT + W^X toggling via `pthread_jit_write_protect_np`), Android keeps memory permanently RWX. The `jit_begin_write_window()` / `jit_end_write_window()` calls are no-ops on Android. `flush_cpu_icache()` uses `__builtin___clear_cache()` (not macOS's `sys_icache_invalidate`). An SELinux error hint is logged in `vm.cpp` if RWX allocation fails.

### ARM64 JIT Stability Notes (2026-02)

- ARM64 defaults to direct JIT address mode (`jit_n_addr_unsafe=0` at reset). Unsafe banks flip to indirect mode via `S_N_ADDR` handling in `map_banks()`.
- ARM64 keeps ROM and UAE Boot ROM (`rtarea`) blocks in interpreter mode.
- Fixed safety fallback for the Lightwave startup hotspot (`PC` range `0x4003df00`-`0x4003e1ff`).
- ARM64 dynamically quarantines unstable JIT blocks (SIGSEGV recovery, autoconfig paths, illegal-op probes) using an O(1) bitmap lookup, reset on `compemu_reset()`.
- Fixed KS 3.1 quarantine guards PC range `0x0803ae00`-`0x0803b100` (RAMSEY memory CRC kernel).

**Debugging env vars** (all optional, zero overhead when unset):
- `AMIBERRY_ARM64_GUARD_VERBOSE=1` — log quarantine events
- `AMIBERRY_ARM64_ENABLE_HOTSPOT_GUARD=1` — re-enable optional hotspot guard (defaults OFF)

### ARM64 JIT 64-bit Pointer Fix (2026-02, macos-jit branch)

**Problem**: On platforms where `natmem_offset` lives above 4GB (e.g. macOS ARM64 at `0x300000000`), the JIT's virtual register `PC_P` (register 16) holds a 64-bit host pointer, but 18 code paths truncated it to 32 bits. All fixes are `CPU_AARCH64`-guarded (not macOS-only).

**Design principle**: PC_P is the ONLY virtual register holding a 64-bit host pointer. All others (D0-D7, A0-A7, FLAGX, etc.) hold 32-bit M68k values. Fixes add PC_P-specific 64-bit paths while keeping 32-bit ops for everything else (W-register ops zero upper 32 bits for clean `[Xn, X27]` indexing).

**Fix points** (files: `compemu_arm.h`, `compemu_support_arm.cpp`, `compemu_arm.cpp`, `codegen_arm64.cpp`, `compemu_midfunc_arm64.cpp`, `gencomp_arm.c`):
1-3. `reg_status::val`, `set_const()`, `get_const()` — `uae_u32` → `uintptr`
4-5. `register_branch()`, `compemu_host_pc_from_const()` — `uae_u32` → `uintptr`
6-7. `arm_ADD_l_ri()`, `arm_ADD_l_ri8()`, `arm_SUB_l_ri8()` — 64-bit ADD/SUB when `d == PC_P`
8-9. `mov_l_ri()`, `mov_l_mi()` — `IM32` → `IMPTR`; `LOAD_U64` for pc_p/pc_oldp
10. `compemu_raw_mov_l_rr()` — `MOV_ww` → `MOV_xx` for PC_P
11-12. `writeback_const()`, `alloc_reg_hinted()` — `LOAD_U64` path for PC_P
13-14. Generated `compemu_arm.cpp` + `gencomp_arm.c` — `uae_u32 v1/v2` → `uintptr`
15. `JITPTR` macro — `(uae_u32)(uintptr)` → `(uintptr)`
16-17. `current_block_pc_p`, `create_jmpdep()` — `uae_u32` → `uintptr`
18. `arm_ADD_l_ri()` parameter — `IM32` → `IMPTR` (was truncating `comp_pc_p` in all Bcc/DBcc branch target computations — the root cause of runtime crashes)
19. **BSR.L/Bcc.L displacement sign extension** — `comp_get_ilong()` returns `uae_u32` which zero-extends to `uintptr` on 64-bit. Backward branch displacements (negative signed values) must be sign-extended via `(uae_s32)` cast before use in pointer arithmetic. Without this, adding e.g. `0x00000000FFFFF000` (should be `-4096`) to PC_P at `0x3...` overflows to `0x4...`, corrupting `comp_pc_p`. Fixed in 32 generated handlers in `compemu_arm.cpp` (BSR.L + BRA.L + 14 Bcc.L × `_ff`/`_nf`) and the generator `gencomp_arm.c` (both `i_BSR` and `i_Bcc` cases). BSR.W/BSR.B were already correct (`(uae_s32)(uae_s16)` / `(uae_s32)(uae_s8)` casts sign-extend properly).

**Natmem reservation**: On ARM64, `UAE_VM_32BIT` is dropped from the natmem reservation call since the JIT is 64-bit-clean. This avoids ~25 wasted mmap/munmap cycles on platforms where the kernel ignores low-address hints.

**x86_64 JIT note**: The x86_64 JIT (`src/jit/x86/`) has been made 64-bit pointer-clean. Key changes:
- `JITPTR` passes full pointer width (`(uintptr)` cast, no truncation)
- `reg_status::val` widened to `uintptr` (was `uae_u32`) — `set_const()`/`get_const()` carry 64-bit values
- PC_P uses 64-bit loads/stores via `raw_mov_q_ri`/`raw_mov_q_mi` in `mov_l_ri`, `writeback_const`, `tomem`, `alloc_reg_hinted`
- R_MEMSTART (R15) holds `natmem_offset` for all JIT memory access: `[R_MEMSTART + m68k_addr]`
- ADDR32 prefix removed from all `raw_*` functions (eliminates 5-cycle Intel Core penalty)
- `readmem_real`/`writemem_real` bank handler paths rewritten for 64-bit pointer chase (function pointer and bank base address loaded via 64-bit MOV through scratch register, not RIP-relative)
- `isconst` shortcuts disabled (`#if !X86_TARGET_64BIT`) in 15 mid-level functions that would otherwise fold natmem addresses into RIP-relative memory operands (overflows ±2GB displacement on x86-64): `mov_l_rR`, `mov_w_rR`, `mov_b_rR`, `mov_l_brR`, `mov_w_brR`, `mov_b_brR`, `mov_l_Ri`, `mov_w_Ri`, `mov_b_Ri`, `mov_l_Rr`, `mov_w_Rr`, `mov_b_Rr`, `mov_l_bRr`, `mov_w_bRr`, `mov_b_bRr`
- Windows ASLR-disabling linker flags removed
- Windows blockinfo pool allocation (`vm_acquire` in `compemu_support_x86.cpp`) uses VirtualQuery walk to find free regions within ±1.75GB of the JIT cache, replacing the blind 16MB-step scan that exhausted in congested ASLR address spaces
- PIE is a compile-time error (`#error` guard in `compemu_support_x86.cpp` line 106-108)

**Natmem reservation on x86-64**: `UAE_VM_32BIT` is kept (NOT stripped) for x86-64, unlike ARM64. This ensures `natmem_offset` lands near 0x80000000 on all platforms, keeping `comp_pc_p` (= `natmem_offset + M68k_addr`) within 32 bits. This is necessary because the x86-64 JIT still has 32-bit arithmetic paths for PC_P (`add_l_ri` → `ADDLir`, `adjust_nreg` → 32-bit LEA) that would truncate if `natmem_offset > 4GB`. The ARM64 JIT doesn't have this limitation because `arm_ADD_l_ri()` uses 64-bit ADD when `d == PC_P`.

### GUI System

The project is transitioning to ImGui (`USE_IMGUI`). ImGui panel sources are in `src/osdep/imgui/`:

- `quickstart.cpp` - Quick start panel
- `configurations.cpp` - Configuration management
- `chipset.cpp`, `cpu.cpp`, `ram.cpp` - Hardware settings
- `floppy.cpp`, `hd.cpp` - Storage configuration
- `display.cpp`, `sound.cpp` - Output settings
- `input.cpp` - Controller configuration
- `whdload.cpp` - WHDLoad integration

### Rendering Architecture

Amiberry supports two rendering backends, selected at compile time via `USE_OPENGL`:
- **OpenGL** (`USE_OPENGL` defined): Hardware-accelerated rendering with shader support (CRT emulation, external GLSL, GLSLP presets, custom bezels)
- **SDL2 Renderer** (`USE_OPENGL` not defined): Software/basic rendering via `SDL_Renderer`

**IRenderer abstraction** (`src/osdep/irenderer.h`):
Abstract base class decoupling all rendering operations from `amiberry_gfx.cpp`. Virtual methods cover context lifecycle, texture allocation, frame rendering, shader management, bezel overlays, OSD/vkbd/on-screen joystick rendering, and input coordinate translation. Global `g_renderer` pointer (created by `renderer_factory.cpp`) dispatches to the active backend.

**Implementations:**
- `OpenGLRenderer` (`src/osdep/opengl_renderer.h/.cpp`) — owns GL context, `ShaderState` (crtemu, external shader, preset), `GLOverlayState` (OSD, bezel, cursor textures/shaders). Accessed via `get_opengl_renderer()`.
- `SDLRenderer` (`src/osdep/sdl_renderer.h/.cpp`) — owns `SDL_Texture*` for Amiga frame and cursor overlay. Accessed via `get_sdl_renderer()`.
- `renderer_factory.h/.cpp` — `create_renderer()` returns the appropriate implementation.

**GL infrastructure extraction:**
GL shader compilation, texture upload, and quad rendering have been extracted from overlay files into dedicated `_gl` files to reduce `#ifdef USE_OPENGL` clutter:

| Component | Common logic + state | GL infrastructure |
|-----------|---------------------|-------------------|
| Virtual keyboard | `src/osdep/vkbd/vkbd.cpp` | `src/osdep/vkbd/vkbd_gl.h/.cpp` |
| On-screen joystick | `src/osdep/on_screen_joystick.cpp` | `src/osdep/on_screen_joystick_gl.h/.cpp` |
| OSD overlays | `src/osdep/amiberry_gfx.cpp` | `src/osdep/gl_overlays.cpp` |

Each `_gl` file is wrapped in `#ifdef USE_OPENGL` / `#endif` internally (not conditionally compiled via CMake). The GL render entry points (`vkbd_redraw_gl`, `on_screen_joystick_redraw_gl`) remain in the original files because they access 15+ file-local static variables. The extracted files expose:
- Shader init: `vkbd_init_gl_shader()` / `osj_init_gl_shader()`
- Texture upload: `vkbd_upload_surface_to_gl()` / `osj_upload_surface_to_gl()`
- Quad rendering: `vkbd_render_gl_quad()` / `osj_render_gl_quad()` / `vkbd_render_gl_filled_rect()`
- Cleanup: `vkbd_cleanup_gl_shader()` / `osj_cleanup_gl_shader()`
- Handle accessors: `vkbd_get_gl_program()` / `vkbd_get_gl_vao()` / `osj_get_gl_program()` / `osj_get_gl_vao()`

**Key rendering files:**

| File | Purpose |
|------|---------|
| `src/osdep/irenderer.h` | Abstract renderer interface |
| `src/osdep/opengl_renderer.h/.cpp` | OpenGL backend |
| `src/osdep/sdl_renderer.h/.cpp` | SDL2 backend |
| `src/osdep/renderer_factory.h/.cpp` | Backend selection |
| `src/osdep/amiberry_gfx.cpp` | Graphics orchestration (window, frame timing, `show_screen()`) |
| `src/osdep/gfx_window.cpp` | Window creation and management |
| `src/osdep/gl_platform.h/.cpp` | GL function pointer loading (desktop) / GLES3 includes (Android) |
| `src/osdep/gl_overlays.cpp` | GL OSD overlay shader and rendering |
| `src/osdep/crtemu_impl.cpp` | CRT shader implementation |
| `src/osdep/external_shader.cpp` | External GLSL shader loading |
| `src/osdep/shader_preset.cpp` | GLSLP multi-pass shader presets |
| `src/osdep/gfx_state.h` | VSyncState and shared graphics state |
| `src/osdep/display_modes.cpp` | Display mode enumeration |
| `src/osdep/gfx_colors.cpp` | Pixel format detection |
| `src/osdep/gfx_prefs_check.cpp` | Graphics preference validation |

### Custom Bezel Overlay System

Custom bezels are PNG/JPEG images (e.g., CRT monitor frames) with a transparent "screen hole" where the emulator output shows through. They work with **all** shader types (crtemu built-in, external GLSL, GLSLP presets).

**Rendering order in `show_screen()`:**
1. Clear to black
2. Render emulator output via active shader (constrained to screen hole viewport)
3. Render software cursor
4. **Render custom bezel overlay** (full window, alpha blended)
5. Render OSD, virtual keyboard, on-screen joystick (on top of bezel)
6. SwapWindow

**Architecture:**
- Bezel texture loaded via `IMG_Load()` → `SDL_ConvertSurfaceFormat(ABGR8888)` → GL texture upload
- Alpha channel scanned during load to detect transparent screen hole bounding box (threshold: alpha < 128)
- Screen hole stored as normalized coordinates (`bezel_hole_x/y/w/h` in 0.0-1.0 range)
- `renderAreaX/Y/W/H` variables in `show_screen()` define the effective render area for all shader paths
- `crtemu_t::skip_aspect_correction` flag bypasses crtemu's internal 4:3 letterboxing when bezel controls the viewport
- Lazy texture loading: `render_bezel_overlay()` loads on first render frame (GL context safe), not from GUI
- `update_custom_bezel()` (called from GUI) only clears `loaded_bezel_name` to trigger reload — no GL calls
- Built-in CRT bezel and custom bezels are mutually exclusive

**Key files:**
- `src/osdep/amiberry_gfx.cpp` — `load_bezel_texture()`, `destroy_bezel_overlay()`, `update_custom_bezel()`, `render_bezel_overlay()`, renderArea logic in `show_screen()`
- `src/osdep/crtemu.h` — `skip_aspect_correction` field and modified aspect ratio logic in `crtemu_present()`
- `src/osdep/imgui/filter.cpp` — GUI controls, bezel scanning (`scan_bezels()`), mutual exclusion logic
- `src/osdep/amiberry.cpp` — `bezels_path` variable, `get_bezels_path()`/`set_bezels_path()`, config save/load
- `src/include/options.h` — `use_custom_bezel`, `custom_bezel[256]` fields in `amiberry_options`

**Config options** (in `amiberry.conf`):
- `use_custom_bezel` — enable custom bezel overlay (boolean)
- `custom_bezel` — filename relative to bezels directory (string, "none" to disable)
- `bezels_path` — path to bezels directory

### On-Screen Joystick (Touch Controls)

The on-screen joystick provides touch-based D-pad and fire buttons for Android and other touchscreen devices. Common logic and state are in `src/osdep/on_screen_joystick.cpp`; GL shader infrastructure (compilation, texture upload, quad rendering) is in `src/osdep/on_screen_joystick_gl.h/.cpp`.

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

### Android App Architecture

The Android app (`android/app/src/main/java/com/blitterstudio/amiberry/`) is a Kotlin/Jetpack Compose front-end that launches the native SDL emulator process via Intent args. There is no JNI control layer for runtime settings; communication is CLI-style through `SDL_ARGS`.

**Process and Activity model**
- `ui/MainActivity.kt` is the launcher activity in the main app process.
- `AmiberryActivity.java` extends `SDLActivity` and runs in a dedicated `:sdl` process (`AndroidManifest.xml`).
- Native launch args are passed via `Intent.putExtra("SDL_ARGS", String[])`; `AmiberryActivity.getArguments()` returns them to SDL/native `main()`.
- `AmiberryActivity` registers `OnBackInvokedCallback` (API 33+) and forwards back to SDL when SDL's trap hint is enabled (`SDL_ANDROID_TRAP_BACK_BUTTON`), otherwise finishes the activity.
- `AmiberryActivity.onDestroy()` kills the `:sdl` process when finishing, so each emulator launch starts from a clean process state.
- SDL activity is started with `android:enableOnBackInvokedCallback="true"` and fullscreen theme; immersive bars are reapplied on focus changes.

**Startup/bootstrap behavior (`MainActivity`)**
- Calls `enableEdgeToEdge()`.
- Extracts packaged assets to `getExternalFilesDir(null)` on first run per app version.
- Uses `.extracted_version` marker file to skip re-extraction when version unchanged.
- Ensures user directories exist: `roms`, `floppies`, `harddrives`, `cdroms`, `lha`, `conf`.
- Shows a loading screen ("Preparing Amiberry...") until extraction/dir prep completes.

**Navigation/UI shell**
- `ui/AmiberryApp.kt` hosts 4 root screens:
  - Quick Start
  - Settings
  - File Manager
  - Configurations
- Uses `NavigationRail` in landscape and bottom `NavigationBar` in portrait.
- Uses `Screen` sealed class with string resources for nav labels.

**State ownership and sharing**
- `SettingsViewModel` (activity-scoped in screens) is the single source of truth for current emulation settings.
- Quick Start and Settings both operate on the same `SettingsViewModel`, so model/media changes are shared across these tabs.
- `QuickStartViewModel` holds WHDLoad selection and media lists.
- `FileManagerViewModel` handles import + per-category rescans.
- `ConfigurationsViewModel` handles saved `.uae` list/load/duplicate/delete/rename operations.

**Core Android data layer**
- `data/EmulatorLauncher.kt`
  - Quick Start launch: `--rescan-roms --model <model> [-0 <floppy>] [-s cdimage0=<cd>] -G`
  - Config launch: `--rescan-roms --config <path> [-G]`
  - WHDLoad launch: `--rescan-roms --autoload <lha> -G`
  - Advanced launch: `--rescan-roms [--config <path>] -s use_gui=yes` (no `-G`)
- `data/ConfigGenerator.kt`
  - Generates `.uae` from `EmulatorSettings`.
  - Writes `use_gui=no` for native-UI-driven launches.
  - Omits `joyport1` when using `onscreen_joy` so runtime auto-assignment can apply.
- `data/ConfigParser.kt`
  - Parses known keys into `EmulatorSettings`.
  - Preserves unknown lines for round-trip safety.
  - Supports config description and heuristic `baseModel` guess when loading arbitrary configs.
- `data/ConfigRepository.kt`
  - Lists non-hidden `.uae` files from `conf/`.
  - Safe filename validation (blocks separators, `..`, and path escapes via canonical checks).
  - Save/load/delete/rename/duplicate helpers.
- `data/FileManager.kt`
  - SAF import with duplicate-safe filename suffixing.
  - Scans category dir + app root; ROM scan also includes `whdboot/game-data/Kickstarts`.
  - Computes CRC32 for ROM files (`AmigaFile.crc32`) to support deterministic ROM-ID matching.
- `data/FileRepository.kt`
  - Singleton with `StateFlow<List<AmigaFile>>` per category and rescan APIs.

**Key UI behavior details**
- Quick Start:
  - No top-right "advanced gear"; advanced access is in Settings/Configurations.
  - Supports model selection, floppy/CD media pickers, and WHDLoad launch.
  - Start button is blocked with snackbar when no ROM is available/selected.
- Settings:
  - Top app bar overflow actions: Save Configuration, Advanced (ImGui).
  - Advanced opens ImGui with a generated temp config and preserved unknown lines.
  - Start uses generated config args (`--model + --config + -G`) and same ROM presence guard.
  - Tabs: CPU, Chipset, Memory, Display, Sound, Input, Storage.
- File Manager:
  - Import via SAF (`OpenDocument`) into selected category.
  - Shows app storage path and copy-to-clipboard action.
- Configurations:
  - Single-tap loads config into `SettingsViewModel` and navigates to Settings.
  - Per-item actions: launch, edit, advanced edit in ImGui, duplicate, delete.

**Android model preset parity (`--model` vs Settings)**
- Requirement: Settings model selection must match CLI `--model` ROM behavior from `main.cpp` wrappers and `bip_*` logic.
- Do not use filename/version heuristics for default ROM assignment.
- `SettingsViewModel.applyModel()` uses ROM IDs derived from CRC32, with explicit per-model priority lists (`MODEL_ROM_PROFILE`).
- It sets both `romFile` and `romExtFile` as needed (CDTV/CD32 split-ROM cases).

**Exact ROM ID priority (matches current `--model` wrappers)**
- `A500` -> `6, 5, 4` (`bip_a500(...,130)`)
- `A500P` -> `7, 6, 5` (`bip_a500plus(...,-1)`)
- `A600` -> `10, 9, 8`
- `A1000` -> `24`
- `A2000` -> `6, 5, 4` (`bip_a2000(...,130)`)
- `A3000` -> `59`
- `A1200` -> `11, 15, 276, 281, 286, 291, 304`
- `A4000` -> `16, 31, 13, 12, 46, 278, 283, 288, 293, 306`
- `CD32` -> kick `64, 18`; ext `19` required when kick is `18` (kick `64` is combined KS+ext)
- `CDTV` -> kick `6, 32`; ext `20, 21, 22` required

**ROM-selection invariants**
- Quick Start (`--model`) and Settings model selection should result in equivalent default ROM choices.
- For CDTV/CD32 split-ROM setups, selection is invalid unless required ext ROM is present.
- If no ROM ID from profile exists, leave auto-selection empty (no heuristic fallback).
- If C++ `--model` wrappers / `bip_*` defaults change, update Android `MODEL_ROM_PROFILE` and `ROM_CRC_TO_ID`.

**Settings hardware constraints (`SettingsViewModel.applyConstraints`)**
- 68000/68010 forces 24-bit addressing; disables Z3 RAM and JIT.
- `cycle_exact=true` forces `cpu_speed=real` and disables JIT.
- JIT (`jitCacheSize > 0`) requires 68020+; when enabled it forces `cpu_speed=max` and `jitFpu=true`.
- 24-bit addressing disables JIT.
- Z3 RAM requires 32-bit addressing.
- Internal FPU model (68040) only valid for CPU >= 68040.
- On-screen joystick (`joyport1=onscreen_joy`) is disabled on non-touch devices.

### Platform Support

Platform-specific code is in `src/osdep/`:

- Common POSIX code works across Linux, macOS, FreeBSD
- Android has additional handling in `android/` directory
- Architecture-specific JIT in `src/jit/arm/` and `src/jit/x86/`
- Windows uses MinGW-w64 which provides POSIX-compatible headers (`unistd.h`, `dirent.h`, etc.)

### macOS Port Notes

**Home directory (since Beta 23):**
- macOS now uses `~/Library/Application Support/Amiberry` as the HOME directory (standard macOS convention), instead of the previous `~/Amiberry`. This path contains spaces, so all shell commands operating on paths must quote arguments.
- `get_home_directory()` creates this directory on first run.
- `get_config_directory()` returns `~/Library/Application Support/Amiberry/Configurations`.
- The `amiberry.conf` file is stored at `~/Library/Application Support/Amiberry/Configurations/amiberry.conf`.
- All user data subdirectories (floppies, roms, savestates, etc.) are created under `~/Library/Application Support/Amiberry/`.

**Security-scoped bookmarks (App Store preparation):**
- `src/osdep/macos_bookmarks.h` / `macos_bookmarks.mm` — bookmark system for macOS App Sandbox.
- Active only when `MACOS_APP_STORE` is defined; no-op inline stubs otherwise.
- Every `set_*_path()` function calls `macos_bookmark_store()` to persist access rights.
- `macos_bookmarks_init()` is called at startup (in `amiberry_main()`) to restore bookmarks.
- Bookmarks are stored as a binary plist in `~/Library/Application Support/Amiberry/bookmarks.plist`.
- Entitlements: `packaging/macos/Amiberry-AppStore.entitlements` adds `app-sandbox`, `files.user-selected.read-write`, `files.bookmarks.app-scope`, `network.client`, `device.usb`.

**Initial folder setup:**
- `create_missing_amiberry_folders()` uses `std::filesystem::copy()` (via a `copy_dir_contents` lambda) instead of `system("cp -R ...")` to handle spaces in paths correctly. This applies to all platforms, not just macOS.

**Shell quoting in downloads:**
- `download_file()` in `amiberry.cpp` wraps both the destination path and source URL in double quotes when constructing curl/wget shell commands via `popen()`. This fixes failures when paths contain spaces.

**DiskArbitration & CD-ROMs:**
- macOS mounts Audio CDs via `cddafs`, which inherently puts an exclusive lock on the standard block device (e.g., `/dev/disk4`) returning `EBUSY` when `open()` is attempted.
- `src/osdep/blkdev_ioctl.cpp` handles this by intentionally falling back to the raw character device (`/dev/rdisk4`).
- Interfacing with Apple's `DiskArbitration` framework (to retrieve CD TOCs) *requires* the block device string. Therefore `ioctl_command_toc2` temporarily reverts `/dev/rdiskX` back to `/dev/diskX` before passing it to `DADiskCreateFromBSDName`.
- Fetching audio sectors requires grouping `CDDA_BUFFERS` into a single `ioctl` call from the character device, and unpacking the contiguous stream with a 96-byte padding gap matching Amiberry's 2448-byte CDDA stride.

### Windows Port Notes

**Key Differences from POSIX platforms:**
- `sysconfig.h` previously `#undef _WIN32` — this was removed to unlock WinUAE Windows code paths
- Windows LLP64: `long` is 4 bytes (not 8); `SIZEOF_LONG` must be 4 in `sysconfig.h`
- JIT compiler works on 64-bit Windows. The x86_64 JIT is 64-bit pointer-clean: `JITPTR` passes full pointer width, `PC_P` uses 64-bit loads/stores, ADDR32 prefix removed, `isconst` shortcuts disabled for natmem-translated addresses, and blockinfo pools allocated near JIT cache via VirtualQuery walk. ASLR-disabling linker flags are no longer needed. `UAE_VM_32BIT` is kept for x86-64 natmem to ensure `comp_pc_p` fits in 32 bits (see "Natmem reservation on x86-64" in the ARM64 JIT section).
- `src/jit/x86/compemu_x86.h`: `uae_p32()` is now a simple `(uintptr)` cast (no truncation)
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
