---
name: troubleshoot-amiberry
description: Autonomous troubleshooting workflow for Amiberry bugs. Use this when investigating, reproducing, or fixing bugs in Amiberry. Provides a complete edit-build-run-test-fix cycle using the Amiberry MCP server tools for process management, IPC control, screenshot analysis, log monitoring, and crash detection.
allowed-tools: Bash(cmake:*), Bash(make:*), Bash(wsl:*), Read, Write, Edit, Grep, Glob
argument-hint: [bug description or issue number]
---

# Amiberry Autonomous Troubleshooting

Debug and fix Amiberry bugs through iterative edit-build-run-test-fix cycles with minimal user interaction.

## Bug to investigate

$ARGUMENTS

## Environment

Amiberry source is at the current working directory (or a nearby `amiberry/` directory).
The MCP server (`amiberry-mcp-server`) provides tools for runtime control.

**Determine the platform** before building:
- **macOS**: Build natively with `cmake`
- **Linux / WSL2**: Build with `cmake` (use `wsl -e` prefix if on Windows host)
- **Windows (native)**: Build with MinGW-w64/GCC via CMake presets and Ninja

## Build Commands

### Linux / macOS
```bash
# Configure (first time or after CMake changes)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DUSE_IPC_SOCKET=ON

# Build (subsequent changes)
cmake --build build -j$(nproc)
```

### Windows host targeting WSL2
```bash
wsl -e bash -c "cd ~/amiberry && cmake --build build -j$(nproc)"
```

### Windows (native MinGW-w64)
```powershell
# Use the helper script (sets PATH for CLion's cmake/ninja/mingw, VCPKG_ROOT)
powershell -ExecutionPolicy Bypass -File "build_and_run.ps1"

# Or manually:
$env:VCPKG_ROOT = "D:\Github\vcpkg"
cmake --preset windows-debug
cd out/build/windows-debug
ninja -j12

# Run with logging enabled
.\amiberry.exe --log
```

**Windows build notes:**
- Build dir: `out/build/windows-debug` (or `windows-release`)
- Kill stale `amiberry.exe` before rebuild if "permission denied"
- `write_log()` is silent unless `--log` flag or `write_logfile` config is set
- JIT is non-functional on 64-bit Windows (interpreter mode works fine)
- PowerShell `$env:` variables get stripped in bash inline commands; use `.ps1` files with `-File`

## Troubleshooting Workflow

### Phase 1: Understand the Bug

1. Read the bug description / issue carefully
2. Identify which subsystem is likely involved (graphics, input, audio, CPU emulation, chipset, config, GUI, etc.)
3. Search the codebase for relevant code:
   - Use `Grep` to find related functions, variables, error messages
   - Use `Glob` to find relevant source files
   - Read the code to understand the current behavior
4. Form a hypothesis about the root cause

### Phase 2: Reproduce

1. **Launch Amiberry** with appropriate config:
   - Use `launch_and_wait_for_ipc` to start Amiberry and wait for IPC readiness
   - Specify a model (`A500`, `A1200`, etc.) or config file as needed
   - This automatically enables logging

2. **Verify it's running**:
   - Use `health_check` to confirm process + IPC + emulation status

3. **Set up the reproduction scenario**:
   - Use `runtime_load_config` to load specific configurations
   - Use `runtime_insert_floppy` to insert disk images
   - Use `send_key` to navigate menus or trigger actions
   - Use `runtime_set_config` to change specific options

4. **Observe the behavior**:
   - Use `runtime_screenshot_view` to see what's on screen
   - Use `tail_log` to monitor log output
   - Use `runtime_get_fps` to check performance
   - Use `runtime_get_cpu_regs` / `runtime_get_custom_regs` for hardware state

5. **If it crashes**:
   - Use `get_crash_info` to detect the crash and analyze signals + log patterns
   - Use `get_process_info` for exit code and signal details

### Phase 3: Fix

1. **Make code changes** using Edit tool on the Amiberry source files
2. **Keep changes minimal** - fix only what's broken, don't refactor surrounding code
3. **Kill the running instance**: Use `kill_amiberry`
4. **Rebuild**:
   ```bash
   cmake --build build -j$(nproc)
   ```
5. Check build output for errors. If build fails, fix and retry.

### Phase 4: Verify

1. **Restart with same config**: Use `restart_amiberry` or `launch_and_wait_for_ipc`
2. **Run the same reproduction steps** from Phase 2
3. **Check results**:
   - `runtime_screenshot_view` - does the screen look correct?
   - `tail_log` - are there still errors/warnings?
   - `get_crash_info` - did it crash again?
   - `health_check` - is everything still running?

4. **If NOT fixed**: Go back to Phase 3 with new hypothesis
5. **If fixed**: Clean up and report findings

### Phase 5: Report

Summarize:
- What the bug was (root cause)
- What was changed (files and line numbers)
- How to verify the fix
- Any side effects or concerns

## Available MCP Tools Reference

### Process Lifecycle
| Tool | Purpose |
|------|---------|
| `check_process_alive` | Is Amiberry running? Returns PID + exit code |
| `get_process_info` | Detailed info: PID, status, signal, crash detection |
| `kill_amiberry` | Force kill (SIGTERM then SIGKILL) |
| `wait_for_exit` | Block until process exits (with timeout) |
| `restart_amiberry` | Kill + relaunch with same command |

### Workflow
| Tool | Purpose |
|------|---------|
| `launch_and_wait_for_ipc` | Launch with logging, wait for IPC ready |
| `health_check` | Combined: process + IPC ping + status + FPS |

### Observation
| Tool | Purpose |
|------|---------|
| `runtime_screenshot_view` | Screenshot returned as image data |
| `tail_log` | New log lines since last read |
| `wait_for_log_pattern` | Wait for regex in log (with timeout) |
| `get_crash_info` | Process signals + log crash pattern scanning |

### Emulation Control
| Tool | Purpose |
|------|---------|
| `pause_emulation` / `resume_emulation` | Pause/resume |
| `reset_emulation` | Soft or hard reset |
| `frame_advance` | Step N frames when paused |
| `runtime_set_config` / `runtime_get_config` | Change/query config at runtime |
| `runtime_load_config` | Load a .uae config file |

### Debugging
| Tool | Purpose |
|------|---------|
| `runtime_get_cpu_regs` | All CPU registers (D0-D7, A0-A7, PC, SR) |
| `runtime_get_custom_regs` | Custom chip registers (DMACON, INTENA, etc.) |
| `runtime_read_memory` | Read Amiga memory (1/2/4 bytes) |
| `runtime_write_memory` | Write Amiga memory |
| `runtime_disassemble` | Disassemble at address |
| `runtime_set_breakpoint` / `runtime_clear_breakpoint` | Breakpoints |
| `runtime_debug_activate` / `runtime_debug_step` | Debugger control |
| `runtime_debug_step_over` | Step over JSR/BSR |
| `runtime_get_copper_state` / `runtime_get_blitter_state` | Hardware state |

### Media & Display
| Tool | Purpose |
|------|---------|
| `runtime_insert_floppy` / `runtime_eject_floppy` | Floppy management |
| `runtime_insert_cd` / `runtime_eject_cd` | CD management |
| `send_key` | Send keyboard input (Amiga keycodes) |
| `runtime_set_display_mode` | Window/fullscreen/fullwindow |
| `runtime_set_ntsc` | PAL/NTSC switching |

## Amiga Keycodes (common)

| Key | Code | Key | Code |
|-----|------|-----|------|
| Return | 0x44 | Space | 0x40 |
| Escape | 0x45 | Backspace | 0x41 |
| Up | 0x4C | Down | 0x4D |
| Left | 0x4F | Right | 0x4E |
| F1-F10 | 0x50-0x59 | Help | 0x5F |
| Left Amiga | 0x66 | Right Amiga | 0x67 |
| A-Z | 0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31 | | |

To send a keypress, call `send_key` twice: once with state=1 (press), then state=0 (release).

## Key Source Directories

| Directory | Contents |
|-----------|----------|
| `src/` | Core emulation (UAE-derived) |
| `src/osdep/` | Platform abstraction (Linux/macOS/Android/Windows) |
| `src/osdep/imgui/` | GUI panels (Dear ImGui) |
| `src/include/` | Headers and interfaces |
| `src/osdep/amiberry_ipc_socket.cpp` | IPC socket implementation |
| `src/osdep/amiberry_gfx.cpp` | Graphics/display |
| `src/osdep/amiberry_input.cpp` | Input handling |
| `src/osdep/amiberry.cpp` | Core platform layer |

## Tips

- Always use `--log` (handled automatically by `launch_and_wait_for_ipc`) for log output
- Use `tail_log` frequently to catch errors early
- If the screen looks wrong, `pause_emulation` + `runtime_screenshot_view` gives a stable frame
- For timing-sensitive bugs, use `runtime_set_config` to change CPU speed or floppy speed
- For crashes, the signal name in `get_crash_info` tells you a lot: SIGSEGV = null pointer/bad memory, SIGABRT = assertion/abort, SIGBUS = alignment
- Build with Debug type (`-DCMAKE_BUILD_TYPE=Debug`) for better crash info
- If you need to test multiple configs, use `kill_amiberry` + `launch_and_wait_for_ipc` between each

### ARM64 JIT toggles (current)

- `AMIBERRY_ARM64_GUARD_VERBOSE=1`: enables detailed dynamic guard learning logs (per key/window). Keep this off for normal runs.
- Optional ARM64 hotspot guard is OFF by default for performance.
- `AMIBERRY_ARM64_ENABLE_HOTSPOT_GUARD=1`: re-enables optional hotspot logic for A/B diagnostics.
- `AMIBERRY_ARM64_DISABLE_HOTSPOT_GUARD=1`: forces optional hotspot logic off; fixed ARM64 safety fallback for the known Lightwave startup hotspot remains active.
- ARM64 JIT stability checks should be validated with at least 3 configs: SysInfo, A4000, and Lightwave.

## Input & On-Screen Joystick Troubleshooting

- **On-screen D-pad acts like a mouse instead of joystick**: Two possible causes:
  1. **SDL touch-to-mouse synthesis**: SDL2 synthesizes `SDL_MOUSEBUTTONDOWN`/`SDL_MOUSEMOTION` from touch events by default. Fix: filter `event.button.which == SDL_TOUCH_MOUSEID` / `event.motion.which == SDL_TOUCH_MOUSEID` in mouse event handlers when on-screen joystick is active (done in `amiberry.cpp`).
  2. **Wrong port assignment**: Another device (e.g., Android accelerometer) may be assigned to Port 1, overriding the on-screen joystick. The on-screen joystick auto-assigns to Port 1 via `on_screen_joystick_set_enabled(true)`, but check `changed_prefs.jports[1].id` to verify.

- **On-screen joystick not appearing in Input dropdown**: The virtual device must be registered in `init_joystick()` (`amiberry_input.cpp`). Check that `num_joystick < MAX_INPUT_DEVICES` and `osj_device_index` is set. The device appears as "On-Screen Joystick" in the joystick device list.

- **Input injection debugging**: Use `--log` flag. The on-screen joystick logs `"On-Screen Joystick registered as JOY%d"` at startup. Use `setjoystickstate()`/`setjoybuttonstate()` (proper device API); avoid `send_input_event()` which bypasses port mode configuration.

- **Port mode mismatch**: If port 1 mode is not `JSEM_MODE_JOYSTICK`, D-pad input may behave as mouse. The auto-assignment sets `changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK` and calls `inputdevice_config_change()`.

## Windows-Specific Troubleshooting

- **Black screen with USE_OPENGL**: Usually caused by JIT `jit_abort()` → `uae_reset()` permanently setting `quit_program`, which blocks pixel drawing in `waitqueue()`. Fixed in `src/jit/x86/compemu_x86.h`.
- **GUI crash only with debugger**: Debug builds (`-DDEBUG`) enable `assert()` in tinyxml2. This can cause crashes with debugger attached but not in normal execution. Not a real bug.
- **Symlink failures in WHDBooter**: Windows requires admin or Developer Mode for symlinks. Directory symlinks use `std::filesystem::copy()` directly. File symlinks have try-catch fallback.
- **Winsock differences**: `close()` → `closesocket()`, `ioctl()` → `ioctlsocket()`, `errno` → `WSAGetLastError()`. Check `src/slirp/` for patterns.
- **No log output**: `write_log()` returns early if neither `--log` nor `write_logfile` is enabled. Always pass `--log` when debugging.
- **GUI crash on startup (missing data dir)**: If the `data/` directory (fonts, icons) is missing from the runtime working directory, ImGui's `AddFontFromFileTTF` asserts and crashes in debug builds (exit code 3). Fix: `main_window.cpp` checks `std::filesystem::exists(font_path)` before loading. Deployment fix: copy `data/` from source tree to working directory.
- **Config save does nothing**: MinGW links `msvcrt.dll` which doesn't support `"ccs=UTF-8"` fopen mode (`errno=22`). Fixed with plain `"w"`/`"rt"`/`"wt"` modes under `#ifdef AMIBERRY` in `cfgfile.cpp` and `ini.cpp`.
- **Hardfile RDB not detected in GUI**: ImGui `hd.cpp` was missing `hardfile_testrdb()` call after HDF file selection, causing geometry to stay at 32,1,2 instead of auto-detecting RDB (0,0,0). Fixed in commit `c2b5c053`.
