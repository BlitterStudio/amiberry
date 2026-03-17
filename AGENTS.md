# AMIBERRY — PROJECT KNOWLEDGE BASE

**Generated:** 2026-03-15
**Commit:** e69453b2
**Branch:** master

## OVERVIEW

Amiga emulator (UAE-based) targeting Linux/macOS/Windows/Android/FreeBSD. C/C++ core (~1.3M lines), CMake build, multi-architecture JIT (ARM64/x86-64). Heterogeneous hardware emulation: M68K + PPC + x86(PCem) + DSP3210 + keyboard MCUs.

## STRUCTURE

```
amiberry/
├── src/                    # Emulation core (~700 files)
│   ├── osdep/             # Platform abstraction + GUI + renderers
│   │   ├── imgui/         # Dear ImGui GUI panels (30+ modular)
│   │   └── vkbd/          # Virtual keyboard (touch/GL)
│   ├── include/           # Core headers (119 files — API contracts)
│   ├── jit/               # JIT compiler backends
│   │   ├── arm/           # ARM64 codegen (primary target)
│   │   └── x86/           # x86-64 codegen
│   ├── pcem/              # PCem x86 CPU emulation (vendored, 193 files)
│   ├── archivers/         # Archive formats: 7z, chd, dms, lha, lzx, zip
│   ├── slirp/             # Embedded TCP/IP stack (vendored)
│   ├── ppc/               # PowerPC emulation (PearPC, vendored)
│   ├── qemuvga/           # QEMU VGA board emulation (vendored)
│   ├── softfloat/         # IEEE 754 FP emulation (vendored)
│   ├── dsp3210/           # Motorola DSP emulation
│   ├── kbmcu/             # Keyboard MCU emulation (6500/6805/8039)
│   ├── mame/              # TMS34010 from MAME (vendored)
│   ├── machdep/           # Machine-dependent definitions
│   ├── sounddep/          # Audio backend
│   ├── threaddep/         # Threading abstraction
│   ├── caps/              # IPF disk image support
│   └── cputest/           # CPU validation test suite
├── external/              # Third-party: ImGui, MT-32, FloppyBridge, CAPS, NFD
├── android/               # Kotlin/Compose Android app + Gradle build
├── libretro/              # RetroArch core wrapper + libretro-common
├── cmake/                 # CMake modules, toolchains, platform flags
├── packaging/             # DEB/RPM/DMG/Flatpak/ZIP packaging
├── data/                  # Runtime assets (fonts, icons, floppy sounds, vkbd)
├── controllers/           # SDL game controller mappings
├── whdboot/               # WHDLoad boot files
└── docs/                  # User documentation
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add GUI panel | `src/osdep/imgui/` | One file per panel, add to `cmake/SourceFiles.cmake` |
| Add config option | `src/cfgfile.cpp` + `src/include/options.h` | 10K-line parser, add GUI in imgui/ |
| Platform-specific code | `src/osdep/amiberry_*.cpp` | Use `#ifdef __ANDROID__` / `_WIN32` / `__APPLE__` |
| JIT bug | `src/jit/arm/compemu_support_arm.cpp` | Block compilation, flags, see `amiberry-arm64-jit` skill |
| Custom chip emulation | `src/custom.cpp` | 68K custom chip (Agnus, Denise, Paula) |
| CPU emulation | `src/newcpu.cpp` + `src/cpuemu_*.cpp` | Interpreter + generated opcode handlers |
| Memory mapping | `src/memory.cpp` + `src/osdep/amiberry_mem.cpp` | Natmem + bank system |
| Renderer changes | `src/osdep/irenderer.h` → `opengl_renderer.cpp` / `sdl_renderer.cpp` | Factory pattern |
| Build config | `CMakeLists.txt` + `cmake/*.cmake` | 15+ feature flags |
| Android app | `android/app/src/main/java/` | Kotlin Compose, launches SDL in `:sdl` process |
| Packaging | `packaging/CMakeLists.txt` | CPack: DEB/RPM/DMG/ZIP |
| Dependencies | `vcpkg.json` (Win), `Brewfile` (Mac), apt (Linux) | See CI workflow |

## CONVENTIONS

- **Tabs** for indentation in C/C++ (no .clang-format — convention-based)
- **`amiberry_` prefix** for platform-specific functions
- **UAE naming** in emulator core (matches WinUAE upstream)
- **ImGui patterns** in GUI code
- **Platform guards**: `#ifdef __ANDROID__`, `#ifdef _WIN32`, `#ifdef __APPLE__`, `#ifdef AMIBERRY`
- **Feature guards**: `#ifdef USE_OPENGL`, `#ifdef USE_IMGUI` (always defined), `#ifdef USE_PCEM`, etc.
- **Vendored code**: `pcem/`, `slirp/`, `ppc/`, `qemuvga/`, `softfloat/`, `mame/` — minimize modifications

## ANTI-PATTERNS (THIS PROJECT)

- **NEVER** suppress type errors — no `as any` equivalent hacks
- **NEVER** modify vendored code in `pcem/`, `slirp/`, `ppc/`, `softfloat/` without documenting upstream delta
- **NEVER** use `send_input_event()` for new input code — use `setjoystickstate()`/`setjoybuttonstate()`
- **NEVER** assume symlinks work — Windows/Android need `std::filesystem::copy()`
- **NEVER** use in-tree builds — CMake enforces out-of-tree
- **DO NOT** enable `#if 0` blocks without understanding context (546 disabled blocks, many intentionally off)
- **DO NOT** run `memset` on POSIX zero-fill pages (causes OOM on Android natmem gaps)
- **DO NOT** modify ROM scanning in `parse_cmdline()` — must be in early arg loop (before `initialize_ini()`)
- FPU SNAN detection is **broken** — cannot reliably detect signaling NaN via host FPU
- Filesystem emulation **will crash** on concurrent host filesystem modifications
- Threading in CHD/osdsync has **known data races**

## UNIQUE STYLES

- **Dual entry point**: `src/main.cpp` → `src/osdep/main.cpp` → `amiberry_main()` (restart loop)
- **Generated code**: `cpuemu_*.cpp` files are **generated by `gencpu.cpp`** — do not edit directly
- **64-bit pointer discipline** in JIT: PC_P is the ONLY 64-bit virtual register; all others 32-bit M68k values
- **Inter-block flag optimization** currently **disabled** (`#if 0` in `compile_block()`) on ARM64
- **Android version** derived from root `CMakeLists.txt` `project(VERSION)` — single source of truth

## COMMANDS

```bash
# Configure + build (Linux/macOS)
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# Windows (MinGW-w64 + vcpkg)
cmake --preset windows-release && cmake --build out/build/windows-release

# Android
cd android && ./gradlew assembleRelease

# Run (no GUI test)
./build/amiberry -G

# Run with logging
./build/amiberry --log

# Cross-compile ARM64
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-aarch64-linux-gnu.cmake -B build
```

## SDL3 KMSDRM STATUS (RPi Console)

**As of SDL 3.4.2 (March 2026):** KMSDRM has critical open bugs. See `docs/sdl3_migration/13_kmsdrm_known_issues.md` for full details.

- 🔴 **Keyboard input broken** ([SDL #15166](https://github.com/libsdl-org/SDL/issues/15166)) — TTY grabs all keyboard; SDL app receives nothing. Milestoned SDL 3.6.0.
- 🔴 **VT switching broken** ([SDL #13145](https://github.com/libsdl-org/SDL/issues/13145)) — Ctrl+Alt+Fn non-functional. Milestoned SDL 3.6.0.
- 🟠 **RPi 5 requires SDL ≥ 3.4.0** — atomic modesetting restored in [PR #11511](https://github.com/libsdl-org/SDL/pull/11511).
- 🟠 **RPi 2/3/Zero not supported** — build failures and black screen with SDL3 KMSDRM.
- 🟡 **`SDL_UNIX_CONSOLE_BUILD=ON` disables KMSDRM** — never use this CMake flag.
- 🟡 **Vulkan + KMSDRM SDL renderer broken** — use OpenGL ES (Amiberry already does this).
- ✅ **Amiberry KMSDRM handling** is correct: forces fullwindow, reuses GUI window, requests GLES 3.0, preserves window on close.
- **Minimum viable SDL3 for KMSDRM:** SDL ≥ 3.6.0 (unreleased). Until then, recommend users run under a compositor (sway/cage).

## NOTES

- **No automated tests** — manual testing with Amiga software is the methodology
- **CLAUDE.md** at `.claude/CLAUDE.md` has comprehensive architecture docs (387 lines) — read it first
- **CI** builds 15+ platform variants (see `.github/workflows/c-cpp.yml`)
- **WinUAE upstream**: Core files (`custom.cpp`, `newcpu.cpp`, `memory.cpp`, etc.) track WinUAE — use `winuae-amiberry-merge` skill for porting
- **JIT debugging**: Use `amiberry-arm64-jit` skill — covers 12 root-cause bug fixes
- **Troubleshooting**: Use `troubleshoot-amiberry` skill — full edit-build-run-test cycle with MCP tools

## CHILD AGENTS.md

```
./AGENTS.md (this file)
├── src/AGENTS.md              — Core emulation modules
│   ├── src/include/AGENTS.md  — Header API contracts
│   ├── src/osdep/AGENTS.md    — Platform abstraction + renderers
│   │   └── src/osdep/imgui/AGENTS.md — GUI panels
│   ├── src/jit/AGENTS.md      — JIT compiler architecture
│   └── src/pcem/AGENTS.md     — PCem x86 emulation
├── libretro/AGENTS.md         — RetroArch integration
└── android/AGENTS.md          — Android app architecture
```
