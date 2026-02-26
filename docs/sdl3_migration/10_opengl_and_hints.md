# Step 10: OpenGL Context and Hints Migration

## Objective

Migrate OpenGL context creation/management and all remaining SDL hint usage from SDL2 to SDL3 equivalents. Also handle clipboard API changes.

## Prerequisites

- Step 09 complete (joystick and gamepad migrated)

## Files to Modify

| File | Operations |
|------|-----------|
| `src/osdep/amiberry_gfx.cpp` | GL context creation, GL_SwapWindow, GL_DeleteContext, GL_MakeCurrent, GL_GetDrawableSize |
| `src/osdep/amiberry.cpp` | GL_GetDrawableSize, hints |
| `src/osdep/gl_platform.cpp` | SDL_GL_GetProcAddress (46 calls) |
| `src/osdep/gl_platform.h` | GL context type |
| `src/osdep/gui/main_window.cpp` | GL context for ImGui, GL_SwapWindow |
| `src/osdep/clipboard.cpp` | SDL_GetClipboardText, SDL_SetClipboardText |
| `src/osdep/amiberry_platform_internal_host.h` | Various hints |

## Detailed Changes

### 1. OpenGL Context Functions

| SDL2 | SDL3 | Notes |
|------|------|-------|
| `SDL_GL_CreateContext(win)` | `SDL_GL_CreateContext(win)` | Unchanged |
| `SDL_GL_DeleteContext(ctx)` | `SDL_GL_DestroyContext(ctx)` | Renamed |
| `SDL_GL_MakeCurrent(win, ctx)` | `SDL_GL_MakeCurrent(win, ctx)` | Returns `bool` instead of `int` |
| `SDL_GL_SwapWindow(win)` | `SDL_GL_SwapWindow(win)` | Returns `bool` instead of `int` |
| `SDL_GL_SetAttribute(attr, val)` | `SDL_GL_SetAttribute(attr, val)` | Returns `bool` instead of `int` |
| `SDL_GL_GetAttribute(attr, &val)` | `SDL_GL_GetAttribute(attr, &val)` | Returns `bool` instead of `int` |
| `SDL_GL_SetSwapInterval(n)` | `SDL_GL_SetSwapInterval(n)` | Returns `bool` instead of `int` |
| `SDL_GL_GetProcAddress(name)` | `SDL_GL_GetProcAddress(name)` | Unchanged |
| `SDL_GL_GetDrawableSize(win, &w, &h)` | `SDL_GetWindowSizeInPixels(win, &w, &h)` | **Renamed** and moved out of GL namespace |

**Return type changes**: Most GL functions change from returning `int` (0 = success) to returning `bool` (true = success). Update any error checking:
```cpp
// Before:
if (SDL_GL_MakeCurrent(window, context) != 0) {
    // error
}

// After:
if (!SDL_GL_MakeCurrent(window, context)) {
    // error
}
```

### 2. GL Attribute Constants

| SDL2 | SDL3 | Notes |
|------|------|-------|
| `SDL_GL_CONTEXT_PROFILE_MASK` | `SDL_GL_CONTEXT_PROFILE_MASK` | Unchanged |
| `SDL_GL_CONTEXT_PROFILE_CORE` | `SDL_GL_CONTEXT_PROFILE_CORE` | Unchanged |
| `SDL_GL_CONTEXT_PROFILE_COMPATIBILITY` | `SDL_GL_CONTEXT_PROFILE_COMPATIBILITY` | Unchanged |
| `SDL_GL_CONTEXT_PROFILE_ES` | `SDL_GL_CONTEXT_PROFILE_ES` | Unchanged |
| `SDL_GL_CONTEXT_MAJOR_VERSION` | `SDL_GL_CONTEXT_MAJOR_VERSION` | Unchanged |
| `SDL_GL_CONTEXT_MINOR_VERSION` | `SDL_GL_CONTEXT_MINOR_VERSION` | Unchanged |
| `SDL_GL_CONTEXT_FLAGS` | `SDL_GL_CONTEXT_FLAGS` | Unchanged |
| `SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG` | `SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG` | Unchanged |
| `SDL_GL_DOUBLEBUFFER` | `SDL_GL_DOUBLEBUFFER` | Unchanged |
| `SDL_GL_DEPTH_SIZE` | `SDL_GL_DEPTH_SIZE` | Unchanged |
| `SDL_GL_STENCIL_SIZE` | `SDL_GL_STENCIL_SIZE` | Unchanged |
| `SDL_GL_RED_SIZE` / `GREEN` / `BLUE` / `ALPHA` | Same names | Unchanged |

### 3. GL Function Pointer Loading (`gl_platform.cpp`)

The file `gl_platform.cpp` loads ~46 OpenGL function pointers using `SDL_GL_GetProcAddress()`. This function is unchanged in SDL3:

```cpp
// Before and After (unchanged):
glp_GenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
```

**Type of SDL_GLContext**: In SDL2, `SDL_GLContext` is `void*`. In SDL3, it's `SDL_GLContextState*`. Update any explicit type casts if present.

### 4. Hint Migration

Review and update all `SDL_SetHint()` calls:

| SDL2 Hint | SDL3 Hint | Notes |
|-----------|-----------|-------|
| `SDL_HINT_RENDER_SCALE_QUALITY` | **Removed** | Use `SDL_SetTextureScaleMode()` per-texture (done in Step 06) |
| `SDL_HINT_GRAB_KEYBOARD` | `SDL_HINT_GRAB_KEYBOARD` | Likely unchanged — verify in SDL3 headers |
| `SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS` | `SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS` | Likely unchanged |
| `SDL_HINT_IME_SHOW_UI` | `SDL_HINT_IME_IMPLEMENTED_UI` | Renamed — verify exact name |
| `SDL_HINT_ANDROID_TRAP_BACK_BUTTON` | `SDL_HINT_ANDROID_TRAP_BACK_BUTTON` | Unchanged |
| `SDL_HINT_ORIENTATIONS` | `SDL_HINT_ORIENTATIONS` | Unchanged |
| `SDL_HINT_ACCELEROMETER_AS_JOYSTICK` | **Removed** | Already handled in Step 09 |
| `SDL_HINT_TOUCH_MOUSE_EVENTS` | `SDL_HINT_TOUCH_MOUSE_EVENTS` | May be renamed — check SDL3 |
| `SDL_HINT_MOUSE_TOUCH_EVENTS` | `SDL_HINT_MOUSE_TOUCH_EVENTS` | May be renamed — check SDL3 |

**Important**: `SDL_SetHint()` return type changes from `SDL_bool` to `bool`.

### 5. Clipboard API

```cpp
// Before (SDL2):
char* text = SDL_GetClipboardText();
// use text...
SDL_free(text);  // MUST free

SDL_SetClipboardText("some text");

// After (SDL3):
const char* text = SDL_GetClipboardText();
// use text...
// Do NOT free — SDL3 manages the string

SDL_SetClipboardText("some text");  // Unchanged
```

**Key locations**:
- `src/osdep/clipboard.cpp`: Main clipboard operations
- Any ImGui panel that uses clipboard (ImGui handles its own clipboard via callbacks)

Also check:
```cpp
// Before:
SDL_HasClipboardText()  // Returns SDL_bool

// After:
SDL_HasClipboardText()  // Returns bool
```

### 6. SDL_GLContext Type

```cpp
// Before (SDL2):
SDL_GLContext gl_context;  // typedef void*

// After (SDL3):
SDL_GLContext gl_context;  // typedef SDL_GLContextState*
```

The variable declarations are the same, but if any code casts `SDL_GLContext` to `void*` or compares against `NULL`, it should still work.

### 7. GL Reset Attributes

```cpp
// Before:
SDL_GL_ResetAttributes();

// After:
SDL_GL_ResetAttributes();  // Unchanged
```

## Search Patterns

```bash
# GL functions
grep -rn 'SDL_GL_DeleteContext\|SDL_GL_DestroyContext' src/
grep -rn 'SDL_GL_GetDrawableSize' src/
grep -rn 'SDL_GL_MakeCurrent\|SDL_GL_SwapWindow' src/
grep -rn 'SDL_GL_GetProcAddress' src/

# Hints
grep -rn 'SDL_SetHint\|SDL_HINT_' src/
grep -rn 'SDL_HINT_RENDER_SCALE_QUALITY' src/
grep -rn 'SDL_HINT_IME_SHOW_UI' src/

# Clipboard
grep -rn 'SDL_GetClipboardText\|SDL_SetClipboardText\|SDL_HasClipboardText' src/

# GL context type
grep -rn 'SDL_GLContext' src/
```

## Acceptance Criteria

1. OpenGL rendering works on all platforms that support it
2. CRT shader (built-in types: tv, pc, lite, 1084) renders correctly
3. External GLSL shaders load and render correctly
4. RetroArch .glslp shader presets work
5. GL context creation and destruction clean (no GL errors logged)
6. `SDL_GL_GetProcAddress` loads all required function pointers
7. VSync toggle works via `SDL_GL_SetSwapInterval`
8. Clipboard copy/paste works in ImGui text fields
9. All hints apply correctly (keyboard grab, minimize on focus loss, etc.)
10. No memory leaks from clipboard operations (removed unnecessary `SDL_free`)
