# Step 01: Build System Migration

## Objective

Update all build configurations to find, link, and fetch SDL3 instead of SDL2, and SDL3_image instead of SDL2_image. After this step, `cmake -B build` configures successfully on all platforms. Source code will not yet compile (SDL2 APIs still in use).

## Prerequisites

- Step 00 complete (overview document exists)

## Files to Modify

| File | Changes |
|------|---------|
| `cmake/Dependencies.cmake` | Primary: find_package, FetchContent, target names, link libraries |
| `cmake/FindSDL2.cmake` | Rename to `cmake/FindSDL3.cmake` or remove (SDL3 ships proper CMake config) |
| `cmake/FindSDL2_image.cmake` | Rename to `cmake/FindSDL3_image.cmake` or remove |
| `external/imgui/CMakeLists.txt` | SDL3 find_package, backend file swap |
| `vcpkg.json` | Package names |
| `Brewfile` | Package names |
| `CMakeLists.txt` | Verify no direct SDL2 references (delegates to Dependencies.cmake) |

## Detailed Changes

### 1. `cmake/Dependencies.cmake`

**Desktop SDL3 discovery** (replace lines ~337-341):
```cmake
# Before (SDL2):
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image MODULE)

# After (SDL3):
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
```

**Android FetchContent** (replace lines ~47-56):
```cmake
# Before (SDL2):
FetchContent_Declare(sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.10
)
FetchContent_Declare(sdl2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.2
)

# After (SDL3):
FetchContent_Declare(sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
)
FetchContent_Declare(sdl3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
)
```

**Target names** (throughout file):
- `SDL2::SDL2` → `SDL3::SDL3`
- `SDL2-static` → `SDL3-static`
- `SDL2_image` / `SDL2_image::SDL2_image` → `SDL3_image::SDL3_image`
- `${AMIBERRY_SDL2_TARGET}` → `${AMIBERRY_SDL3_TARGET}` (or simplify to `SDL3::SDL3`)

**Android target sanitization** (`amiberry_android_sanitize_target` function, lines ~166-199):
- Update all `SDL2` references to `SDL3`
- The `-lSDL2` raw library name → `-lSDL3`
- Verify the sanitization logic still applies to SDL3's CMake targets on Android

**Link libraries** (line ~205 and ~484):
```cmake
# Before:
target_link_libraries(${PROJECT_NAME} PRIVATE ${AMIBERRY_SDL2_TARGET} SDL2_image)

# After:
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
```

**Include directory extraction** (lines ~450-464):
- Update to extract includes from `SDL3::SDL3` and `SDL3_image::SDL3_image` targets
- SDL3 uses `<SDL3/SDL.h>` include path (the target should set this automatically)

### 2. `cmake/FindSDL2.cmake` and `cmake/FindSDL2_image.cmake`

SDL3 ships proper CMake config files (`SDL3Config.cmake`), so custom find modules may not be needed. Options:
- **Option A**: Delete both files (SDL3's CONFIG mode should work everywhere)
- **Option B**: Create `FindSDL3.cmake` / `FindSDL3_image.cmake` as thin wrappers that just call `find_package(SDL3 CONFIG)` for backward compatibility

Recommended: **Option A** — delete and rely on SDL3's native CMake support. If FreeBSD pkg-config is needed, add a fallback in Dependencies.cmake.

### 3. `external/imgui/CMakeLists.txt`

```cmake
# Before:
add_library(imgui STATIC
    ...
    backends/imgui_impl_sdl2.cpp
    backends/imgui_impl_sdlrenderer2.cpp
)
find_package(SDL2 REQUIRED)
target_link_libraries(imgui PRIVATE SDL2::SDL2)

# After:
add_library(imgui STATIC
    ...
    backends/imgui_impl_sdl3.cpp
    backends/imgui_impl_sdlrenderer3.cpp
)
find_package(SDL3 REQUIRED)
target_link_libraries(imgui PRIVATE SDL3::SDL3)
```

If `IMGUI_USE_OPENGL` is set, `imgui_impl_opengl3.cpp` stays unchanged (it's SDL-version-independent).

### 4. `vcpkg.json`

```json
{
  "dependencies": [
    "sdl3",
    "sdl3-image",
    ...
  ]
}
```

**Note**: Verify the exact vcpkg port names. SDL3 may be registered as `sdl3` in vcpkg. Check with `vcpkg search sdl3`.

### 5. `Brewfile`

```ruby
brew "sdl3"
brew "sdl3_image"
```

**Note**: Verify Homebrew formula names. SDL3 may be `sdl3` or `sdl` (since SDL3 is now the primary version).

### 6. FreeBSD (CMakeLists.txt lines 88-108)

Update pkg-config module names:
- `sdl2` → `sdl3`
- `SDL2_image` → `SDL3_image`
- `SDL2_INCLUDE_DIRS` → `SDL3_INCLUDE_DIRS`

### 7. `CMakePresets.json`

No SDL-specific content expected, but verify the Windows preset's vcpkg integration still works with the updated `vcpkg.json`.

### 8. `android/app/build.gradle`

No direct SDL references (uses CMake). Verify:
- CMake version ≥ 3.22.1 still works with SDL3
- NDK version compatibility with SDL3
- ABI targets (arm64-v8a, x86_64) unchanged

## Acceptance Criteria

1. `cmake -B build -DCMAKE_BUILD_TYPE=Release` configures successfully on at least one desktop platform (Linux or macOS), finding SDL3
2. Android: `cmake` configuration in Gradle context downloads SDL3 via FetchContent
3. Windows: `cmake --preset windows-debug` configures with vcpkg finding SDL3
4. The build will NOT compile yet (source still uses SDL2 APIs) — this is expected
5. No SDL2 references remain in any CMake, vcpkg, or Brewfile configuration
