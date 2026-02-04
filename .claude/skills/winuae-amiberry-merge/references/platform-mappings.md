# Platform API Mappings for WinUAE to Amiberry

## Common Emulation Code

**Important:** Most of the core emulation code is **identical** between WinUAE and Amiberry:
- CPU emulation (68k, PPC if applicable)
- Chipset emulation (Paula, Denise, Agnus, etc.)
- Floppy disk controller
- Hard disk emulation
- Memory management
- Configuration parsing (much of it)

These areas typically require minimal or no changes when merging, unless they interact with platform-specific systems (file I/O, timing, threading).

## Platform-Specific Areas

Changes in these WinUAE subsystems **always require adaptation**:

1. **Rendering Pipeline** (Direct3D → OpenGL + SDL2)
2. **Audio Output** (WASAPI → SDL2 audio)
3. **Input Handling** (DirectInput → SDL2)
4. **Threading** (Win32 threads → SDL2 threading)
5. **GUI** (Win32 dialogs → ImGui)
6. **File System** (Windows APIs → POSIX/C++ filesystem)

## Windows to SDL2 Mappings

**Note:** Amiberry currently uses SDL2, with plans to migrate to SDL3 eventually. For now, all mappings target SDL2 APIs.

### Window/Display Management
- `CreateWindow()` → `SDL_CreateWindow()`
- `ShowWindow()` → `SDL_ShowWindow()`
- `UpdateWindow()` → `SDL_UpdateWindowSurface()`
- `GetDC()` / `ReleaseDC()` → SDL surface/renderer access
- `SetWindowPos()` → `SDL_SetWindowPosition()` / `SDL_SetWindowSize()`

### Input Handling
- `GetAsyncKeyState()` → `SDL_GetKeyboardState()`
- `GetKeyState()` → `SDL_GetKeyboardState()`
- DirectInput → `SDL_Joystick*()` / `SDL_GameController*()`
- Raw Input → SDL event system
- `SetCursor()` → `SDL_SetCursor()`
- `ShowCursor()` → `SDL_ShowCursor()`

### Graphics/Rendering
- Direct3D → OpenGL rendering (Amiberry uses OpenGL for GPU acceleration)
- DirectDraw → SDL2 + OpenGL
- GDI operations → SDL2 surface operations
- `BitBlt()` → `SDL_BlitSurface()` / OpenGL texture uploads
- `StretchBlt()` → `SDL_BlitScaled()` / OpenGL scaled rendering
- D3D shaders → OpenGL shaders (GLSL)
- D3D resources → OpenGL textures/buffers

### Audio
- WASAPI → SDL audio subsystem (`SDL_OpenAudioDevice()`, `SDL_QueueAudio()`)
- DirectSound → SDL audio subsystem (if older WinUAE code still uses it)
- `waveOut*` → SDL audio (if legacy code present)

### Threading
- `CreateThread()` → `SDL_CreateThread()`
- `WaitForSingleObject()` → `SDL_WaitThread()`
- `CRITICAL_SECTION` → `SDL_mutex` / `SDL_LockMutex()` / `SDL_UnlockMutex()`
- `EnterCriticalSection()` / `LeaveCriticalSection()` → `SDL_LockMutex()` / `SDL_UnlockMutex()`
- `CreateEvent()` → `SDL_CreateSemaphore()` or `SDL_CreateCond()`
- `SetEvent()` / `ResetEvent()` → `SDL_SemPost()` / `SDL_CondSignal()`

**Note:** Amiberry uses SDL2 for all threading. Only use pthreads directly if SDL2 doesn't provide the needed functionality.

### File System
- `CreateFile()` → `fopen()` or C++ filesystem
- `ReadFile()` / `WriteFile()` → `fread()` / `fwrite()`
- `FindFirstFile()` / `FindNextFile()` → `opendir()` / `readdir()` on Linux/macOS, or C++17 filesystem
- `GetFileAttributes()` → `stat()` or `std::filesystem::status()`
- Backslashes `\` → Forward slashes `/` or use `std::filesystem::path`
- `MAX_PATH` → `PATH_MAX` (Linux) or remove limit with dynamic allocation

### Timing
- `QueryPerformanceCounter()` → `SDL_GetPerformanceCounter()`
- `QueryPerformanceFrequency()` → `SDL_GetPerformanceFrequency()`
- `GetTickCount()` → `SDL_GetTicks()`
- `Sleep()` → `SDL_Delay()` or `std::this_thread::sleep_for()`

### System Info
- `GetSystemMetrics()` → `SDL_GetDisplayBounds()` / `SDL_GetDisplayMode()`
- `GetVersionEx()` → Platform-specific or remove
- Registry access → Config files (libconfig, INI, JSON)

## Platform-Specific Guards

### Standard Pattern
```cpp
#ifdef _WIN32
    // Windows-specific code
#elif defined(__APPLE__)
    // macOS-specific code
#elif defined(ANDROID)
    // Android-specific code
#else
    // Linux or other Unix-like systems
#endif
```

### Common Guard Scenarios

**File paths:**
```cpp
#ifdef _WIN32
    const char PATH_SEP = '\\';
#else
    const char PATH_SEP = '/';
#endif
```

**Platform-specific includes:**
```cpp
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <SDL.h>
#endif
```

**Android-specific UI considerations:**
```cpp
#ifdef ANDROID
    // Touch-optimized UI scaling
    // Virtual keyboard handling
    // Specific file access via Android APIs
#endif
```

**macOS-specific:**
```cpp
#ifdef __APPLE__
    // macOS-specific file system access
    // Retina display handling
    // macOS menu integration
#endif
```

## GUI Translation (WinUAE → Amiberry ImGui)

### Dialog to ImGui Window
WinUAE uses Win32 dialogs, Amiberry uses ImGui windows in `src/osdep/imgui/`.

**Pattern:**
- Dialog resource → `ImGui::Begin()` / `ImGui::End()` in appropriate file
- Static text → `ImGui::Text()`
- Edit boxes → `ImGui::InputText()`
- Checkboxes → `ImGui::Checkbox()`
- Radio buttons → `ImGui::RadioButton()`
- Combo boxes → `ImGui::Combo()`
- Buttons → `ImGui::Button()`
- List boxes → `ImGui::ListBox()` or `ImGui::Selectable()`

### Layout Considerations
- Win32 uses absolute positioning
- ImGui uses automatic layout with manual spacing
- Use `ImGui::SameLine()` for horizontal layouts
- Use `ImGui::Spacing()`, `ImGui::Separator()` for visual structure
- Keep similar visual grouping as WinUAE for user familiarity

## Build System Notes

### CMake Integration
- Add new source files to appropriate `CMakeLists.txt`
- Platform-specific sources can be conditionally included:
  ```cmake
  if(ANDROID)
      list(APPEND SOURCES src/android_specific.cpp)
  endif()
  ```

### Android Gradle
- Android-specific Java/Kotlin code goes in Android project
- JNI calls for native interaction

## Testing Checklist After Merge

- [ ] Build succeeds on all platforms (CI will check)
- [ ] Run-time testing on primary platform
- [ ] Check GUI functionality matches WinUAE behavior
- [ ] Verify no Windows API calls leaked through
- [ ] Confirm proper platform guards are in place
- [ ] Test file I/O with platform-appropriate paths
- [ ] Verify thread safety if threading code changed

## Display Synchronization (VSync)

WinUAE and Amiberry handle VSync differently due to the underlying graphics APIs.

### The "Black Screen" Pitfall
In Amiberry, `amiberry_gfx.cpp` manages the frame timing.
- **Critical Function:** `set_custom_limits(maxvpos, maxhpos, min_vpos, min_hpos, check_only)`
- **Usage:** In `lockscr()`, Amiberry calls `set_custom_limits(-1, -1, -1, -1, false)` to ensure blanking limits are open before the frame is drawn.
- **Sync Point:** Frame limits are enforced in `SDL2_renderframe`.
- **Difference:** WinUAE often handles this in `drawing.cpp`. When merging `drawing.cpp` updates, ensure you **do not** accidentally re-enable WinUAE's `vbcopy` or limit checks that conflict with Amiberry's custom limits.

## Input Mapping & Coordinates

### Mouse Coordinates
Amiberry manually handles HiDPI/Retina scaling because SDL2 events report in "Points" (screen coordinates) while OpenGL renders in "Pixels" (drawable size).

**WinUAE Pattern:**
Often assumes 1:1 mapping or uses Windows DWM scaling.

**Amiberry Implementation (`handle_mouse_motion_event`):**
```cpp
SDL_GetWindowSize(win, &win_w, &win_h);       // Logical size (Points)
SDL_GL_GetDrawableSize(win, &draw_w, &draw_h); // Pixel size (Pixels)
float scale_x = (float)draw_w / (float)win_w;
x = (Sint32)(x * scale_x);
```
**Guidance:** If merging input code, ensure raw mouse coordinates are NOT used directly for Amiga positioning without this scaling, otherwise mouse movement will desync on macOS/High-DPI displays.

## RTG / Picasso96 Optimizations

### Dirty Region Tracking
Amiberry implements a specific optimization for RTG updates to avoid uploading full textures.

**WinUAE Pattern:**
Often uses `getwritewatch` (Windows API) which returns a list of modified pages.

**Amiberry Pattern (`picasso96.cpp`):**
Uses a manual `mark_dirty(addr, size)` function.
- **Optimization:** Maintains `min_dirty_page_index` and `max_dirty_page_index`.
- **Looping:** When processing updates (e.g., inside `picasso_getwritewatch` or equivalent loops), it iterates only from `min` to `max` instead of scanning the entire VRAM.
- **Merge Note:** When merging RTG code, ensure this optimization loop is preserved. Do not replace it with a full linear scan.

### Zero Copy
Amiberry supports "Zero Copy" where `amiga_surface->pixels` points directly to the allocated RTG VRAM.
- **Check:** `currprefs.rtg_zerocopy`
- **Constraint:** If active, `gfx_lock_picasso` may return `nullptr` to signal that no copy is needed (source and dest are identical).
- **Crash Risk:** Ensure code checking `lock` result handles `nullptr` gracefully if it intends to just "read" the buffer.
