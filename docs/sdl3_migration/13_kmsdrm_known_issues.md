# SDL3 KMSDRM Known Issues (Raspberry Pi Console)

**Last updated:** 2026-03-13
**Applies to:** SDL 3.4.2 (current stable as of March 2026)
**Context:** Running Amiberry from the Linux console on Raspberry Pi (no X11/Wayland compositor), using the KMSDRM video backend with OpenGL ES 3.0.

---

## Current Amiberry KMSDRM Handling

Amiberry detects KMSDRM at runtime (`SDL_GetCurrentVideoDriver()`) and applies these adaptations in `src/osdep/gfx_window.cpp`:

- Forces `GFX_FULLWINDOW` mode (no exclusive fullscreen on KMSDRM)
- Reuses the GUI window/renderer for the emulation window (KMSDRM single-window constraint)
- Preserves the window during `close_hwnds()` to avoid KMSDRM resource conflicts
- Requests GLES 3.0 context (not desktop GL) when KMSDRM is detected
- Uses OpenGL ES renderer (not Vulkan — Vulkan + KMSDRM SDL renderer is broken)

Detection flag: `kmsdrm_detected` (declared in `src/osdep/amiberry_gfx.h`)

---

## 🔴 Critical — Currently Broken in SDL3

### 1. Keyboard Input Not Received Under KMSDRM

**SDL Issue:** [#15166](https://github.com/libsdl-org/SDL/issues/15166) — opened March 5, 2026
**Status:** Open, milestoned for SDL 3.6.0
**Affects:** SDL 3.4.2 (current stable)

The background TTY console grabs all keyboard input instead of the SDL3 application. The app sees gamepad input but not keyboard. `SDL_SetWindowKeyboardGrab(window, true)` has no effect.

**Impact:** Showstopper for keyboard-driven emulation. Users launching Amiberry from the console without a compositor will have no keyboard input.

**Workaround:** Run under a minimal Wayland compositor (e.g., sway, cage) so SDL3 uses the Wayland backend instead of KMSDRM.

### 2. VT Switching Broken

**SDL Issue:** [#13145](https://github.com/libsdl-org/SDL/issues/13145) — opened May 2025
**Status:** Open, milestoned for SDL 3.6.0

Ctrl+Alt+Fn to switch virtual terminals does not work when SDL3 is using KMSDRM. Something in SDL3's keyboard initialization disables console switching.

**Impact:** Users cannot switch away from Amiberry to a terminal without killing the process.

---

## 🟠 High — Hardware/Version Dependent

### 3. Atomic Modesetting Required for RPi 5

**SDL PR:** [#11511](https://github.com/libsdl-org/SDL/pull/11511) — merged October 19, 2025
**Fixed in:** SDL ≥ 3.4.0

The KMSDRM atomic modesetting path (required by RPi 5's DRM driver) was absent from SDL3 until October 2025. The non-atomic legacy path produces garbage output on RPi 5.

**Impact:** SDL3 versions below 3.4.0 are **unusable** on Raspberry Pi 5 with KMSDRM. Amiberry must depend on SDL ≥ 3.4.0.

### 4. Pi Zero W / Pi 2-3 — Not Working

- **[#12418](https://github.com/libsdl-org/SDL/issues/12418):** Pi Zero W shows only a black screen with SDL3 KMSDRM (open, milestone 3.x)
- **[#14749](https://github.com/libsdl-org/SDL/issues/14749):** SDL3 fails to link on RPi 2/3 buildroot (missing `rpi-userland` libs, open since Jan 2026)
- **[#11444](https://github.com/libsdl-org/SDL/issues/11444):** The legacy `rpi`/`brcmegl`/`dispmanx` backend was removed from SDL3 entirely. Pre-Pi 4 hardware must use KMSDRM, which has unresolved issues on those platforms.

**Impact:** SDL3 effectively supports **RPi 4 and RPi 5 only**. RPi 2/3/Zero support is broken.

### 5. `SDL_UNIX_CONSOLE_BUILD=ON` Silently Disables KMSDRM

**SDL Issue:** [#13869](https://github.com/libsdl-org/SDL/issues/13869) — opened September 2025

Building SDL3 with `-DSDL_UNIX_CONSOLE_BUILD=ON` excludes KMSDRM from the build. At runtime, `SDL_CreateWindow` fails with "kmsdrm is not available". No build-time warning.

**Impact:** Never use this CMake flag. Build SDL3 with default options and set `SDL_VIDEO_DRIVER=kmsdrm` at runtime.

---

## 🟡 Medium — Behavioral Differences

### 6. Adaptive VSync May Not Work

Amiberry calls `SDL_GL_SetSwapInterval(interval)` in `opengl_renderer.cpp`. The KMSDRM EGL backend may not support adaptive VSync (interval `-1`). The current code logs a failure but does not fall back to interval `1`.

**Recommendation:** Add explicit fallback: if `SDL_GL_SetSwapInterval(-1)` fails, retry with `1`.

### 7. `SDL_VIDEODRIVER` Environment Variable Renamed

The environment variable was renamed to `SDL_VIDEO_DRIVER` in SDL3. The old name has a fallback since SDL ≥ 3.2.0 ([PR #11120](https://github.com/libsdl-org/SDL/pull/11120)), but launch scripts and distro configs should be updated.

### 8. KMSDRM Not Autodetected When Wayland Is Running

**SDL Issue:** [#8218](https://github.com/libsdl-org/SDL/issues/8218)

If a Wayland compositor is running on another VT, SDL3 tries Wayland first and fails instead of falling back to KMSDRM. Users must explicitly set `SDL_VIDEO_DRIVER=kmsdrm`.

### 9. Vulkan + KMSDRM SDL Renderer Is Broken

**Source:** [PR #11511 discussion](https://github.com/libsdl-org/SDL/pull/11511)

The SDL 2D renderer's Vulkan backend fails on KMSDRM because `vkGetPhysicalDeviceDisplayPropertiesKHR` finds no displays. Direct Vulkan rendering (bypassing the SDL renderer) works. OpenGL ES is the correct path.

**Impact:** Amiberry already uses GLES — no action needed. Do not attempt to switch to a Vulkan-based SDL renderer on KMSDRM.

---

## Minimum Viable SDL3 Version for KMSDRM

| Target | Minimum SDL3 | Notes |
|--------|-------------|-------|
| RPi 5 | ≥ 3.4.0 | Atomic modesetting restored |
| RPi 4 | ≥ 3.4.0 | Atomic also needed for correct behavior |
| RPi 2/3/Zero | ❌ Not supported | Build and runtime failures |
| Keyboard input | ≥ 3.6.0 (unreleased) | [#15166](https://github.com/libsdl-org/SDL/issues/15166) |
| VT switching | ≥ 3.6.0 (unreleased) | [#13145](https://github.com/libsdl-org/SDL/issues/13145) |

**Recommendation:** Wait for SDL 3.6.0 before targeting KMSDRM console use. Until then, recommend users run under a minimal compositor (sway/cage) for the Wayland backend.

---

## Other Emulator Projects — SDL3 KMSDRM Status

| Project | SDL3 Status | KMSDRM Notes |
|---------|-------------|-------------|
| DOSBox-Staging | [PR #4714](https://github.com/dosbox-staging/dosbox-staging/pull/4714) — draft, Jan 2026 | HiDPI coordinate confusion was the biggest issue |
| Dolphin | [PR #13694](https://github.com/dolphin-emu/dolphin/pull/13694) — merged Jun 2025 | SDL3 only for input, not rendering |
| ScummVM | [PR #6312](https://github.com/scummvm/scummvm/pull/6312) — merged Feb 2025 | Parallel SDL2/SDL3 backends |
| PPSSPP | Planned for v1.21 (Apr 2026) | Explicitly reverted sdl2-compat due to Vulkan fullscreen regression |
| RetroArch | [#18441](https://github.com/libretro/RetroArch/issues/18441) — not started | Feature request only |
| DOSBox-X | [#6148](https://github.com/joncampbell123/dosbox-x/issues/6148) — not started | Feature request only |

---

## References

- [SDL3 Migration Guide](https://wiki.libsdl.org/SDL3/README-migration)
- [SDL3 KMSDRM atomic restore PR](https://github.com/libsdl-org/SDL/pull/11511)
- [SDL3 KMSDRM keyboard bug](https://github.com/libsdl-org/SDL/issues/15166)
- [SDL3 KMSDRM VT switching bug](https://github.com/libsdl-org/SDL/issues/13145)
- [SDL3 Pi Zero black screen](https://github.com/libsdl-org/SDL/issues/12418)
- [SDL3 `SDL_UNIX_CONSOLE_BUILD` conflict](https://github.com/libsdl-org/SDL/issues/13869)
