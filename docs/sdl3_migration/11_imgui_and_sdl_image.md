# Step 11: ImGui Backend Swap and SDL_image Migration

## Objective

Complete the ImGui backend migration from SDL2 to SDL3 backends, and migrate all SDL_image usage from SDL2_image to SDL3_image. This step connects the ImGui GUI system to the new SDL3 infrastructure.

## Prerequisites

- Step 10 complete (OpenGL and hints migrated)
- Step 01 already updated `external/imgui/CMakeLists.txt` to compile SDL3 backends

## Files to Modify

| File | Changes |
|------|---------|
| `src/osdep/gui/main_window.cpp` | ImGui init/shutdown/frame/render calls, SDL_image usage |
| `src/osdep/imgui/imgui_panels.h` | ImGui backend header include |
| `src/osdep/imgui/about.cpp` | `IMG_Load()` for logo image |
| `src/osdep/imgui/controller_map.cpp` | `IMG_Load()` for controller images |
| `src/osdep/imgui/savestates.cpp` | `IMG_Load()` for savestate screenshots |
| `src/osdep/vkbd/vkbd.cpp` | `IMG_Load()` for keyboard skin |
| `src/osdep/shader_preset.cpp` | `IMG_Load()` for LUT textures |
| `src/osdep/gfx_platform_internal_host.h` | `IMG_Load()` for window icon, `IMG_SavePNG` for screenshots |
| `src/osdep/amiberry.cpp` | `IMG_Load()` for icon |

## Detailed Changes

### 1. ImGui Backend Headers

```cpp
// Before:
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_impl_opengl3.h"  // Windows only

// After:
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_impl_opengl3.h"  // Windows only (unchanged — SDL-version-independent)
```

### 2. ImGui Initialization (`main_window.cpp`)

```cpp
// Before (SDL2):
ImGui_ImplSDL2_InitForSDLRenderer(mon->gui_window, mon->gui_renderer);
ImGui_ImplSDLRenderer2_Init(mon->gui_renderer);

// After (SDL3):
ImGui_ImplSDL3_InitForSDLRenderer(mon->gui_window, mon->gui_renderer);
ImGui_ImplSDLRenderer3_Init(mon->gui_renderer);
```

For OpenGL path (Windows):
```cpp
// Before:
ImGui_ImplSDL2_InitForOpenGL(mon->gui_window, gl_context);

// After:
ImGui_ImplSDL3_InitForOpenGL(mon->gui_window, gl_context);
```

### 3. ImGui Per-Frame Calls

```cpp
// Before:
ImGui_ImplSDLRenderer2_NewFrame();
ImGui_ImplSDL2_NewFrame();

// After:
ImGui_ImplSDLRenderer3_NewFrame();
ImGui_ImplSDL3_NewFrame();
```

For OpenGL path:
```cpp
// Before:
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplSDL2_NewFrame();

// After:
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplSDL3_NewFrame();
```

### 4. ImGui Render

```cpp
// Before:
ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);

// After:
ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);
```

For OpenGL path:
```cpp
// Before:
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

// After (unchanged — SDL-version-independent):
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
```

### 5. ImGui Event Processing

```cpp
// Before:
ImGui_ImplSDL2_ProcessEvent(&event);

// After:
ImGui_ImplSDL3_ProcessEvent(&event);
```

### 6. ImGui Shutdown

```cpp
// Before:
ImGui_ImplSDLRenderer2_Shutdown();
ImGui_ImplSDL2_Shutdown();

// After:
ImGui_ImplSDLRenderer3_Shutdown();
ImGui_ImplSDL3_Shutdown();
```

For OpenGL path:
```cpp
// Before:
ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplSDL2_Shutdown();

// After:
ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplSDL3_Shutdown();
```

### 7. ImGui Texture Creation Helper

The `gui_create_texture()` function in `main_window.cpp` creates ImGui textures from SDL surfaces:

```cpp
// Before:
ImTextureID gui_create_texture(SDL_Surface* surface, ...) {
    // SDL Renderer path:
    return (ImTextureID)SDL_CreateTextureFromSurface(mon->gui_renderer, surface);
    // OpenGL path:
    return (ImTextureID)(intptr_t)gl_tex;
}

// After:
ImTextureID gui_create_texture(SDL_Surface* surface, ...) {
    // SDL Renderer path (unchanged — SDL_CreateTextureFromSurface exists in SDL3):
    return (ImTextureID)(intptr_t)SDL_CreateTextureFromSurface(mon->gui_renderer, surface);
    // OpenGL path:
    return (ImTextureID)(intptr_t)gl_tex;
}
```

**Note**: In SDL3's ImGui renderer backend, `ImTextureID` may need to be cast differently. Check `imgui_impl_sdlrenderer3.h` for the expected type. SDL3's `SDL_Texture*` should still work as `ImTextureID`.

### 8. SDL_image Migration (SDL2_image → SDL3_image)

**Include change**:
```cpp
// Before:
#include <SDL_image.h>

// After:
#include <SDL3_image/SDL_image.h>
```

**IMG_Init / IMG_Quit**:
```cpp
// Before (SDL2):
IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
// ... use IMG_Load ...
IMG_Quit();

// After (SDL3):
// IMG_Init/IMG_Quit are REMOVED in SDL3_image — auto-initialized
// Simply remove these calls
```

**IMG_Load**:
```cpp
// Before:
SDL_Surface* surface = IMG_Load(filename);

// After:
SDL_Surface* surface = IMG_Load(filename);  // Unchanged
```

The function name is the same, but the returned surface uses SDL3 surface format.

**IMG_SavePNG**:
```cpp
// Before:
IMG_SavePNG(surface, filename);

// After:
IMG_SavePNG(surface, filename);  // Unchanged
```

**IMG_GetError**:
```cpp
// Before:
const char* err = IMG_GetError();

// After:
const char* err = SDL_GetError();  // SDL3_image uses SDL's error system
```

### 9. Touch-Specific ImGui Handling

The touch-to-scroll conversion in `main_window.cpp` uses SDL touch events:
```cpp
// Before:
case SDL_FINGERDOWN:
case SDL_FINGERUP:
case SDL_FINGERMOTION:

// After (already handled in Step 07):
case SDL_EVENT_FINGER_DOWN:
case SDL_EVENT_FINGER_UP:
case SDL_EVENT_FINGER_MOTION:
```

Also, the `ImGuiConfigFlags_IsTouchScreen` flag should still work.

### 10. Android Text Input

```cpp
// Before:
if (want_text && !SDL_IsTextInputActive())
    SDL_StartTextInput();
else if (!want_text && SDL_IsTextInputActive())
    SDL_StopTextInput();

// After (already handled in Step 07):
if (want_text && !SDL_TextInputActive(mon->gui_window))
    SDL_StartTextInput(mon->gui_window);
else if (!want_text && SDL_TextInputActive(mon->gui_window))
    SDL_StopTextInput(mon->gui_window);
```

## Search Patterns

```bash
# ImGui SDL2 backend references
grep -rn 'ImGui_ImplSDL2_\|ImGui_ImplSDLRenderer2_' src/
grep -rn 'imgui_impl_sdl2\|imgui_impl_sdlrenderer2' src/

# SDL_image
grep -rn 'IMG_Load\|IMG_Save\|IMG_Init\|IMG_Quit\|IMG_GetError' src/
grep -rn '#include.*SDL_image' src/

# ImTextureID casts
grep -rn 'ImTextureID' src/
```

## Acceptance Criteria

1. ImGui GUI renders correctly with SDL3 backends
2. All GUI panels functional (quickstart, configurations, chipset, CPU, RAM, display, sound, input, floppy, HD, etc.)
3. ImGui file dialogs work (open/save configuration, select ROM, select floppy)
4. ImGui event processing works (keyboard, mouse, gamepad navigation)
5. Touch scrolling works in ImGui panels (Android)
6. Images load correctly via SDL3_image:
   - Window icon
   - About screen logo
   - Controller images in mapping GUI
   - Savestate screenshots
   - Virtual keyboard skins
   - Shader LUT textures
7. Screenshot saving works (IMG_SavePNG)
8. OpenGL ImGui path works on Windows
9. No visual artifacts or incorrect textures in the GUI
10. Theme colors and DPI scaling work correctly
