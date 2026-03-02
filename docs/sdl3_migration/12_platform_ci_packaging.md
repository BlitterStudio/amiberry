# Step 12: Platform-Specific Fixes, CI, Packaging, and Cleanup

## Objective

Handle all remaining platform-specific changes, update CI/CD workflows and packaging configurations, remove the compatibility header, update project documentation, and document the SDL_GPU opportunity for future Vulkan work.

## Prerequisites

- Steps 00-11 all complete

## Files to Modify

### Platform/Init
| File | Changes |
|------|---------|
| `src/osdep/amiberry_platform_internal_host.h` | `SDL_Init` flags, remaining platform-specific SDL calls |
| `src/osdep/amiberry.cpp` | SDL_Init, platform event handling, version string |
| `src/main.cpp` | SDL_main handling |

### CI/CD
| File | Changes |
|------|---------|
| `.github/workflows/c-cpp.yml` | Package names for all platforms |

### Packaging
| File | Changes |
|------|---------|
| `packaging/CMakeLists.txt` | DEB/RPM dependency names |
| `packaging/flatpak/com.blitterstudio.amiberry.yml` | SDL3 module if needed |

### Documentation
| File | Changes |
|------|---------|
| `.claude/CLAUDE.md` | Update SDL2 references to SDL3 |
| `docs/vulkan_port/00_vulkan_architecture_overview.md` | Add SDL_GPU opportunity note |

### Cleanup
| File | Changes |
|------|---------|
| `src/osdep/sdl_compat.h` | Remove (if created during transition) |
| `cmake/FindSDL2.cmake` | Remove |
| `cmake/FindSDL2_image.cmake` | Remove |

## Detailed Changes

### 1. SDL_Init Flags

```cpp
// Before (SDL2):
SDL_Init(SDL_INIT_EVERYTHING);

// After (SDL3):
SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);
```

**Key changes**:
- `SDL_INIT_EVERYTHING` → removed in SDL3. Must specify individual subsystems.
- `SDL_INIT_GAMECONTROLLER` → `SDL_INIT_GAMEPAD`
- `SDL_INIT_TIMER` → removed (timers are always available)
- `SDL_INIT_HAPTIC` → still available if used

**Location**: `amiberry_platform_internal_host.h` line ~23.

### 2. SDL_main Handling

SDL3 changes how `SDL_main` works. Check `src/main.cpp`:

```cpp
// SDL2:
#include <SDL.h>  // Defines main → SDL_main on some platforms

// SDL3 options:
// Option A: Define SDL_MAIN_HANDLED before including SDL.h
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
// Then call SDL_SetMainReady() before SDL_Init()

// Option B: Use SDL3's callback model (for platforms that need it)
// Not recommended for Amiberry — stick with traditional main()
```

**Android-specific**: SDL3 on Android may require different Activity integration. Check SDL3's Android documentation. The `SDL_main` → `SDL3_main` rename may affect the JNI bridge.

### 3. SDL Version String

```cpp
// Before:
SDL_version compiled, linked;
SDL_VERSION(&compiled);
SDL_GetVersion(&linked);
snprintf(buf, ..., "SDL %d.%d.%d (compiled) / %d.%d.%d (linked)",
    compiled.major, compiled.minor, compiled.patch,
    linked.major, linked.minor, linked.patch);

// After (SDL3):
int version = SDL_GetVersion();
int major = SDL_VERSIONNUM_MAJOR(version);
int minor = SDL_VERSIONNUM_MINOR(version);
int micro = SDL_VERSIONNUM_MICRO(version);
snprintf(buf, ..., "SDL %d.%d.%d", major, minor, micro);
```

**Location**: `amiberry.cpp` function `get_sdl2_version_string()` — rename to `get_sdl_version_string()`.

### 4. CI Workflow (`.github/workflows/c-cpp.yml`)

**Ubuntu/Debian builds**:
```yaml
# Before:
apt-get install libsdl2-dev libsdl2-image-dev

# After:
apt-get install libsdl3-dev libsdl3-image-dev
```

**Note**: SDL3 packages may not be available in all Ubuntu/Debian versions yet (as of early 2026). Options:
- Use PPAs or build SDL3 from source in CI
- Use FetchContent for all platforms (not just Android)
- Check package availability in Ubuntu 24.04, 25.10, Debian Trixie

**macOS builds**:
```yaml
# Before:
brew install sdl2 sdl2_image

# After:
brew install sdl3 sdl3_image
```

**Windows builds** (MSYS2):
- vcpkg.json already updated in Step 01

**Android builds**:
- FetchContent already updated in Step 01
- Verify NDK compatibility

**Fedora/Debian Docker images**:
- Update package names in Docker configurations

### 5. Packaging

**DEB dependencies** (`packaging/CMakeLists.txt` line ~45):
```cmake
# Before:
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libsdl2-image-2.0-0, ...")

# After:
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl3-0, libsdl3-image-0, ...")
```

**RPM dependencies** (line ~82):
```cmake
# Before:
set(CPACK_RPM_PACKAGE_REQUIRES "SDL2, SDL2_image, ...")

# After:
set(CPACK_RPM_PACKAGE_REQUIRES "SDL3, SDL3_image, ...")
```

**Flatpak** (`packaging/flatpak/com.blitterstudio.amiberry.yml`):
- Freedesktop SDK 24.08 includes SDL2. Check if a newer SDK version includes SDL3.
- If SDL3 is not in the SDK, add it as a module built from source:
```yaml
- name: SDL3
  buildsystem: cmake-ninja
  config-opts:
    - -DCMAKE_BUILD_TYPE=Release
    - -DSDL_TESTS=OFF
  sources:
    - type: archive
      url: https://github.com/libsdl-org/SDL/releases/download/release-3.2.8/SDL3-3.2.8.tar.gz
      sha256: <sha256>

- name: SDL3_image
  buildsystem: cmake-ninja
  config-opts:
    - -DCMAKE_BUILD_TYPE=Release
  sources:
    - type: archive
      url: https://github.com/libsdl-org/SDL_image/releases/download/release-3.2.4/SDL3_image-3.2.4.tar.gz
      sha256: <sha256>
```

### 6. Android-Specific

**Activity integration**: SDL3 on Android may change how the Activity class works. Check:
- `android/app/src/main/java/` for any SDL Activity references
- SDL3's `SDL_main` / activity integration for Android NDK
- Touch event handling differences

**Hints**:
- `SDL_HINT_ANDROID_TRAP_BACK_BUTTON` → verify still works
- `SDL_HINT_ORIENTATIONS` → verify still works

### 7. iOS Preparation

SDL3 has improved iOS support. While Amiberry doesn't currently support iOS, document the opportunity:
- SDL3 supports iOS natively
- SDL_GPU works with Metal on iOS
- CMake can target iOS with proper toolchain
- No immediate action needed, but note compatibility in documentation

### 8. Windows-Specific

- Verify `SDL_MAIN_HANDLED` works correctly with MinGW-w64
- Verify Winsock integration in `slirp/` still works (SDL3 doesn't change Winsock usage)
- Verify `bsdsocket_host.cpp` Windows path (uses WinUAE's native model, independent of SDL)

### 9. Remove Compatibility Header

If `src/osdep/sdl_compat.h` was created during the transition:
1. Remove the file
2. Ensure all files include `<SDL3/SDL.h>` directly
3. Remove any remaining compatibility macros

### 10. Remove Old CMake Find Modules

Delete if they still exist:
- `cmake/FindSDL2.cmake`
- `cmake/FindSDL2_image.cmake`

### 11. Update CLAUDE.md

Update all SDL2 references in `.claude/CLAUDE.md`:
- Build commands (package names)
- API references (function names)
- Architecture documentation
- Dependencies section

### 12. Update Vulkan Port Docs

Add a note to `docs/vulkan_port/00_vulkan_architecture_overview.md`:

```markdown
## SDL_GPU Alternative (Post-SDL3 Migration)

With the SDL3 migration complete, Amiberry now has access to SDL3's `SDL_GPU` API,
which provides a cross-platform GPU abstraction supporting:
- Vulkan (Linux, Windows, Android)
- Metal (macOS, iOS)
- Direct3D 12 (Windows)

This could serve as an alternative to raw Vulkan for the `IRenderer` abstraction
described in Phase 1. Advantages:
- Single API covers all platforms (no need for separate Vulkan/Metal/D3D12 backends)
- ImGui already has `imgui_impl_sdlgpu3.cpp` for rendering
- Shader cross-compilation handled by SDL_GPU's shader system

The `IRenderer` interface should accommodate both approaches:
- `SDLRenderer3` — current SDL3 renderer (software/GPU accelerated)
- `SDLGPURenderer` — SDL_GPU backend (Vulkan/Metal/D3D12)
- `VulkanRenderer` — direct Vulkan (if needed for maximum control)
```

### 13. Final Verification Checklist

Run the following on each platform:

**All platforms**:
- [ ] CMake configure succeeds
- [ ] Build completes without errors or warnings
- [ ] Emulator launches
- [ ] A500 Kickstart 1.3 boots
- [ ] A1200 Kickstart 3.1 boots
- [ ] Floppy disk loading works
- [ ] GUI opens and all panels are functional
- [ ] Sound output works
- [ ] Keyboard input works
- [ ] Mouse input works
- [ ] Joystick/gamepad input works
- [ ] Fullscreen toggle works
- [ ] Window resize works
- [ ] Clipboard copy/paste works
- [ ] Screenshot saving works
- [ ] Configuration save/load works

**Platform-specific**:
- [ ] Linux: Flatpak builds and runs
- [ ] macOS: DMG package creates and installs
- [ ] Windows: ZIP package creates, exe runs
- [ ] Android: APK builds and installs, touch controls work

## Search Patterns

```bash
# Remaining SDL2 references
grep -rn 'SDL2\|sdl2\|SDL_INIT_EVERYTHING\|SDL_INIT_GAMECONTROLLER' src/
grep -rn 'SDL2\|sdl2' cmake/ packaging/ .github/ vcpkg.json Brewfile
grep -rn 'SDL_VERSION\|SDL_GetVersion' src/
grep -rn 'sdl_compat\.h' src/

# Verify no SDL2 includes remain
grep -rn '#include.*<SDL\.h>' src/
grep -rn '#include.*<SDL_image\.h>' src/
```

## Acceptance Criteria

1. **Build**: Full build succeeds on all platforms (Linux x86_64, Linux aarch64, macOS Intel, macOS ARM64, Windows x64, Android aarch64, Android x86_64)
2. **CI**: CI pipeline passes on all platforms
3. **Packages**: DEB, RPM, Flatpak, DMG, ZIP, APK/AAB all build correctly
4. **Runtime**: Emulator boots and runs Amiga software without regressions
5. **GUI**: All ImGui panels functional
6. **Audio**: Sound output works correctly
7. **Input**: Keyboard, mouse, joystick, gamepad, and touch input all work
8. **Display**: Windowed, fullscreen (both modes), resize all work
9. **Cleanup**: No SDL2 references remain in source code, build files, or packaging
10. **Documentation**: CLAUDE.md and Vulkan port docs updated
11. **Compatibility header removed** (if it was created)
