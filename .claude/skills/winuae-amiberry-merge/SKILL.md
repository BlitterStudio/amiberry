---
name: winuae-amiberry-merge
description: Assist with merging updates and synchronizing functionality from the upstream WinUAE repository (Windows) to Amiberry (Linux/macOS/Android/Windows). Includes a comprehensive guide for porting Win32 GUI dialogs to Dear ImGui, mapping Windows controls to ImGui equivalents, and analyzing platform-specific code. Use this for analyzing upstream commits, identifying Windows-specific code, adapting GUI panels, and verifying feature parity.
---

# WinUAE to Amiberry Merge Assistant

Help merge updates from WinUAE (Windows-only, MSVC) to Amiberry (multi-platform: Linux/macOS/Android/Windows with MinGW-w64).

## Overview

WinUAE is the upstream Windows-based Amiga emulator. Amiberry is a cross-platform port.

**Core emulation is identical:** Most emulation code (CPU, chipset, floppy, HDD) requires no changes when merging.

**Platform layer differs:**
- **Rendering:** WinUAE uses Direct3D; Amiberry uses OpenGL + SDL2
- **Audio:** WinUAE uses WASAPI; Amiberry uses SDL2 audio
- **Input:** WinUAE uses DirectInput; Amiberry uses SDL2
- **Threading:** WinUAE uses Win32 threads; Amiberry uses SDL2 threading
- **GUI:** WinUAE uses Win32 dialogs; Amiberry uses Dear ImGui (in `src/osdep/imgui/`)
- **Build:** WinUAE uses Visual Studio; Amiberry uses CMake (+ Gradle for Android)
- **Platforms:** Amiberry targets Linux, macOS, Android, and Windows (SDL2, migrating to SDL3 eventually)
- **CI/CD:** GitHub Actions builds all platforms on each commit

## Merge Workflow

Follow these steps when merging WinUAE updates:

### 1. Identify What Changed

Get the WinUAE commit(s) to merge. This could be:
- A specific commit hash
- A range of commits
- The latest upstream changes

Review the commit messages and changed files to understand:
- What functionality was added/changed
- Which subsystems are affected
- Whether it's bug fixes, new features, or refactoring

### 2. Analyze Platform Dependencies

Use the analysis scripts to identify Windows-specific code:

```bash
# Analyze specific files or directories for Windows API usage
python scripts/analyze_windows_code.py <path-to-changed-files>

# Identify GUI code that needs ImGui adaptation
python scripts/analyze_gui_code.py <path-to-changed-files>
```

The scripts will flag:
- **Direct3D** code needing OpenGL adaptation
- **WASAPI** audio needing SDL2 audio adaptation
- **DirectInput** needing SDL2 input
- **Win32 threading** needing SDL2 threading
- Win32 API calls needing SDL2/POSIX equivalents
- GUI code needing ImGui implementation
- File system code needing path handling fixes

**Note:** Core emulation code (CPU, chipset) typically won't trigger many warnings.

### 3. Plan the Adaptation

For each identified issue:

**Windows API → SDL2/POSIX:**
- Consult `references/platform-mappings.md` for common API translations
- Use SDL2 when available for cross-platform abstraction (rendering, audio, input, threading)
- Fall back to POSIX APIs only when SDL2 doesn't provide equivalent functionality
- Add platform-specific guards using `#ifdef` when necessary

**GUI Changes:**
- Win32 dialogs → ImGui windows in `src/osdep/imgui/`
- Maintain similar layout and functionality to WinUAE for consistency
- Use ImGui layout functions (`ImGui::SameLine()`, `ImGui::Spacing()`)
- Keep user-facing behavior as close to WinUAE as possible
- **See the "GUI Synchronization Guide" section below for detailed instructions.**

**File System:**
- Replace backslashes with forward slashes or use `std::filesystem::path`
- Handle platform-specific paths appropriately
- Ensure Android file access goes through proper APIs when needed

**Platform Guards:**
```cpp
#ifdef _WIN32
    // Windows-specific (both WinUAE/MSVC and Amiberry/MinGW-w64)
    // Note: Amiberry on Windows DOES define _WIN32, so many WinUAE
    // Windows code paths are active. Use #ifdef AMIBERRY to distinguish.
#elif defined(__APPLE__)
    // macOS-specific
#elif defined(ANDROID)
    // Android-specific
#else
    // Linux and other Unix-like
#endif

// To distinguish Amiberry from WinUAE on Windows:
#if defined(_WIN32) && defined(AMIBERRY)
    // Amiberry on Windows (MinGW-w64/GCC)
#elif defined(_WIN32)
    // WinUAE (MSVC)
#endif

// Platforms without reliable symlink support:
#if defined(__ANDROID__) || defined(_WIN32)
    // Use std::filesystem::copy() instead of create_symlink()
#endif
```

**Key difference: Amiberry on Windows vs WinUAE:**
- Amiberry uses MinGW-w64 (GCC) — provides POSIX headers (`unistd.h`, `dirent.h`, etc.)
- WinUAE uses MSVC — requires `_MSC_VER`-specific code
- Many `#ifdef _WIN32` blocks from WinUAE now activate in Amiberry on Windows
- `sysconfig.h` previously `#undef _WIN32` to suppress this; that was removed
- JIT is non-functional on 64-bit Windows (pointers > 32-bit); interpreter mode is used

### 4. Apply Changes

Adapt the WinUAE code for Amiberry:
- Replace Windows APIs with SDL2/POSIX equivalents
- Implement or modify ImGui UI for any GUI changes
- Add appropriate platform guards for macOS/Android specifics
- Update CMakeLists.txt if new files are added
- Ensure code follows Amiberry's existing patterns

### 5. Test

After applying changes:
- Verify code compiles on all platforms (CI will check automatically)
- Test functionality on primary development platform
- Check that GUI changes match WinUAE behavior
- Confirm no Windows-specific code leaked through
- Test with different configurations if relevant

## GUI Synchronization Guide

This section provides guidance for synchronizing Amiberry's ImGui GUI implementation with WinUAE's Windows GUI (`win32gui.cpp`). The Amiberry panels in `src/osdep/imgui/` are ports of WinUAE's Windows dialog-based GUI.

### WinUAE win32gui.cpp Structure

WinUAE's GUI code (mostly in `od-win32/win32gui.cpp`) follows a consistent pattern for each settings panel:

#### Key Function Types

1. **`values_to_XXXdlg()`** - Populates dialog controls from `workprefs`
   - Reads current settings and updates UI controls
   - Example: `values_to_memorydlg()` sets slider positions from memory sizes

2. **`values_from_XXXdlg()`** - Reads dialog controls into `workprefs` (if present)
   - Some panels handle this in the dialog proc instead

3. **`enable_for_XXXdlg()`** - Enables/disables controls based on configuration
   - Example: Z3 controls disabled when `address_space_24 == true`

4. **`fix_values_XXXdlg()`** - Validates and fixes invalid configurations
   - Example: `fix_values_memorydlg()` limits Fast RAM when Chip > 2MB

5. **`XXXDlgProc()`** - Windows dialog procedure
   - `WM_INITDIALOG` - Initialize controls, set ranges
   - `WM_COMMAND` - Handle button clicks, checkbox changes
   - `WM_HSCROLL` - Handle slider changes
   - `WM_USER` - Refresh dialog from settings

### Files Reference

| Panel | Amiberry ImGui (src/osdep/imgui/) | WinUAE Functions |
|-------|----------------|------------------|
| RAM | `ram.cpp` | `MemoryDlgProc`, `values_to_memorydlg`, `setfastram_selectmenu` |
| CPU | `cpu.cpp` | `CPUDlgProc`, `values_to_cpudlg` |
| Chipset | `chipset.cpp` | `ChipsetDlgProc`, `values_to_chipsetdlg` |
| Display | `display.cpp` | `DisplayDlgProc`, `values_to_displaydlg` |
| Sound | `sound.cpp` | `SoundDlgProc`, `values_to_sounddlg` |
| Floppy | `floppy.cpp` | `FloppyDlgProc`, `values_to_floppydlg` |
| HD | `hd.cpp` | `HarddiskDlgProc`, `values_to_harddiskdlg` |
| Expansions | `expansions.cpp` | `ExpansionDlgProc`, `values_to_expansiondlg` |

### Porting Guidelines

#### ImGui Equivalents for Windows Controls

| Windows Control | ImGui Equivalent |
|-----------------|------------------|
| Trackbar (`TBM_*`) | `ImGui::SliderInt()` |
| Combo Box (`CB_*`) | `ImGui::BeginCombo()` / `ImGui::Selectable()` |
| Check Box | `ImGui::Checkbox()` or custom `AmigaCheckbox()` |
| Edit Control | `ImGui::InputText()` |
| Static Text | `ImGui::Text()` |
| Group Box | `BeginGroupBox()` / `EndGroupBox()` |
| Enable/Disable | `ImGui::BeginDisabled()` / `ImGui::EndDisabled()` |

#### Common Patterns

**1. Reading slider values:**

*WinUAE:*
```cpp
v = memsizes[msi_chip[SendMessage(GetDlgItem(hDlg, IDC_CHIPMEM), TBM_GETPOS, 0, 0)]];
```

*Amiberry ImGui:*
```cpp
int chip_idx = get_mem_index(changed_prefs.chipmem.size, msi_chip, 7);
if (ImGui::SliderInt("##slider", &chip_idx, 0, 6, "")) {
    changed_prefs.chipmem.size = memsizes[msi_chip[chip_idx]];
}
```

**2. Populating dropdowns:**

*WinUAE:*
```cpp
xSendDlgItemMessage(hDlg, IDC_COMBO, CB_ADDSTRING, 0, (LPARAM)text);
```

*Amiberry ImGui:*
```cpp
if (ImGui::BeginCombo("##combo", current_text)) {
    if (ImGui::Selectable(text, is_selected)) { /* handle selection */ }
    ImGui::EndCombo();
}
```

#### Validation Checklist

When porting or updating a panel, verify:

- [ ] All controls present in WinUAE are implemented
- [ ] Enable/disable logic matches WinUAE's `enable_for_XXXdlg()`
- [ ] Value validation matches WinUAE's `fix_values_XXXdlg()` (if exists)
- [ ] Side effects (e.g., auto-enable chipset features) are implemented
- [ ] Multiple board support where applicable
- [ ] Dropdown items match WinUAE format (especially names with prefixes)
- [ ] Size limits and range checks match WinUAE constants

## Common Scenarios

### Scenario: Core Emulation Changes

When WinUAE updates core emulation (CPU, chipset, floppy, HDD, memory):
- **Usually no platform dependencies** - code is identical between projects
- Check for timing functions if present (`QueryPerformanceCounter` → `SDL_GetPerformanceCounter`)
- Check for file I/O if disk/ROM handling changed
- Check for CD-ROM physical IOCTL handling. **macOS specifically mounts physical audio CDs with exclusive `cddafs` locks**, requiring physical CD access `open()` to target character devices (`/dev/rdiskX`) instead of block devices (`/dev/diskX`). Apple's DiskArbitration framework TOC retrieval requires the block device string counterpart instead.
- Check for threading if multi-threaded emulation changed
- **Most core emulation merges cleanly with minimal or no changes**

### Scenario: Rendering/Graphics Changes

When WinUAE updates Direct3D rendering:
- Requires significant adaptation to OpenGL
- Check shader code (HLSL → GLSL)
- Verify texture/buffer management
- Test visual output carefully
- May need expertise in both Direct3D and OpenGL

### Scenario: Input System Changes

When WinUAE updates input handling:
- DirectInput → SDL2 input system
- May need updates to both keyboard and joystick handling
- Check `GetAsyncKeyState` usage → `SDL_GetKeyboardState`
- Test on actual hardware if possible

### Scenario: Audio System Changes

When WinUAE updates WASAPI audio code:
- Adapt to SDL2 audio subsystem
- Check buffer sizes and latency settings
- Verify sample rate handling
- Test audio quality and synchronization

## Common Pitfalls & Troubleshooting

### 0. Missing GUI Logic When Porting Panels
**Symptom:** Feature works in WinUAE but not in Amiberry's ImGui GUI.
**Cause:** WinUAE's `win32gui.cpp` dialog procedures contain logic (validation, auto-detection, side effects) that may not have been ported to the corresponding ImGui panel.
**Example:** `hd.cpp` was missing `hardfile_testrdb()` after HDF file selection — WinUAE calls this in `HarddiskDlgProc` to auto-detect RDB and reset geometry. The ImGui port omitted it.
**Fix:** When debugging GUI issues, always compare the ImGui panel code against the corresponding WinUAE dialog procedure for missing function calls, especially `values_from_*`, `fix_values_*`, and helper functions called on control changes.

### 0b. fopen Mode Strings on MinGW
**Symptom:** `fopen()` returns NULL with `errno=22 (EINVAL)` on Windows.
**Cause:** MinGW GCC links `msvcrt.dll` (not `ucrtbase.dll`). The `"ccs=UTF-8"` fopen mode extension is UCRT-only. WinUAE (MSVC) links `ucrtbase.dll` where this works.
**Fix:** Under `#ifdef AMIBERRY`, use plain modes (`"w"`, `"rt"`, `"wt"`) without `ccs=UTF-8`. Also note: the `'e'` flag (close-on-exec) is not valid on Windows — strip it.

### 1. The "Black Screen" on Standard VSync
**Symptom:** Amiga emulation runs (Audio works) but screen is black when `VSync Standard` is active.
**Cause:** Amiberry manages frame timing differently than WinUAE. WinUAE's `drawing.cpp` often contains blanking limit checks that conflict with Amiberry's `amiberry_gfx.cpp`.
**Fix:**
- Ensure `set_custom_limits(-1, -1, -1, -1, false)` is called in `lockscr()` (see `amiberry_gfx.cpp`).
- Do NOT port WinUAE's `vbcopy()` changes if they enforce alpha channels incorrectly for SDL2.
- Check `show_screen_maybe()` logic; Amiberry handles presentation in `SDL2_renderframe`.

### 1b. Black Screen on Windows with USE_OPENGL (64-bit)
**Symptom:** Emulation starts but screen stays black. Queue type 0/1/2 entries never processed.
**Cause:** JIT `check_uae_p32()` detects 64-bit pointers → `jit_abort()` → `uae_reset(1,0)` → `quit_program=4` permanently. `waitqueue()` in `drawing.cpp` blocks all pixel-drawing queue entries when `quit_program != 0`.
**Fix:** In `src/jit/x86/compemu_x86.h`, `check_uae_p32()` logs instead of calling `jit_abort()` under `#ifdef AMIBERRY`. JIT is non-functional on 64-bit Windows; interpreter mode handles everything.

### 2. Mouse Coordinate Drift
**Symptom:** Mouse clicks are offset from the cursor, especially on macOS or High-DPI screens.
**Cause:** SDL2 Events report "Screen Coordinates" (Points), but OpenGL renders in "Pixels".
**Fix:**
- Do NOT use raw event coordinates directly for Amiga input.
- You must apply a scaling factor using `SDL_GetWindowSize` vs `SDL_GL_GetDrawableSize`.
- See `handle_mouse_motion_event` in `amiberry.cpp` for the correct implementation.

### 3. ImGui Scaling on Android/Touch
**Symptom:** Buttons are too small to touch on Android, or dialogs are huge on Desktop.
**Cause:** Hardcoding pixel sizes (e.g., `Width(100)`).
**Fix:**
- ALWAYS use `BUTTON_WIDTH` constant for sizing interactive elements.
- Use `ImGui::GetContentRegionAvail().x` for dynamic widths.
- Avoid absolute pixel values for layout.

## Performance Pitfalls

### RTG: Dirty Rectangles
WinUAE's `getwritewatch` returns a list of pages. Amiberry tracks `min_dirty_page_index` and `max_dirty_page_index`.
- **Optimization:** When processing RTG updates, ensure you iterate ONLY from `min` to `max` pages. Scanning the entire VRAM (e.g. 8MB+) every frame will kill performance on ARM devices.

### Zero Copy Hazards
If `currprefs.rtg_zerocopy` is true, `gfx_lock_picasso()` may return `nullptr`.
- This means "No copy needed, surface IS the VRAM".
- **Crash Risk:** If you blindly access the pointer returned by `lockvars` without checking for nullptr, you will crash.

## Quick Reference

**Key Amiberry directories:**
- `src/osdep/imgui/` - ImGui GUI implementation
- `src/osdep/` - Platform-specific code
- Root CMakeLists.txt - Build configuration

**Analysis tools:**
- `scripts/analyze_windows_code.py` - Find Windows API usage
- `scripts/analyze_gui_code.py` - Find GUI code needing ImGui

**References:**
- `references/platform-mappings.md` - API translation guide

## Tips

- Start with non-GUI changes (they're usually easier)
- Keep commits focused on logical units
- Preserve WinUAE's commit messages for traceability
- When uncertain about an API translation, check how similar code is handled elsewhere in Amiberry
- Test incrementally rather than merging everything at once
