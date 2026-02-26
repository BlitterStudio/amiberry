# Step 03: Timer and Utility Function Migration

## Objective

Migrate SDL timer functions, cursor management, error handling, logging, and miscellaneous utility APIs from SDL2 to SDL3 equivalents. These are low-risk, scattered changes affecting ~7 files.

## Prerequisites

- Step 02 complete (types and includes migrated)

## Files to Modify

| File | Functions to migrate |
|------|---------------------|
| `src/osdep/amiberry.cpp` | `SDL_GetTicks`, `SDL_Delay`, `SDL_ShowCursor`, `SDL_GetModState`, `SDL_CreateSystemCursor` |
| `src/osdep/amiberry_gfx.cpp` | `SDL_GetTicks`, `SDL_Delay`, `SDL_ShowCursor`, `SDL_GetError` |
| `src/osdep/gui/main_window.cpp` | `SDL_GetTicks`, `SDL_Delay` |
| `src/osdep/imgui/controller_map.cpp` | `SDL_GetTicks` |
| `src/osdep/writelog.cpp` | `SDL_GetTicks` |
| `src/osdep/bsdsocket_host.cpp` | `SDL_GetTicks` |
| `src/osdep/amiberry_platform_internal_host.h` | `SDL_CreateSystemCursor`, `SDL_GetModState`, `SDL_free` |

## Detailed Changes

### 1. `SDL_GetTicks()`

**SDL2**: Returns `Uint32` (milliseconds since SDL_Init, wraps at ~49 days).
**SDL3**: Returns `uint64_t` (milliseconds since SDL_Init, no practical wrap).

**Action**: Update all variables receiving the return value:
```cpp
// Before:
Uint32 start_time = SDL_GetTicks();

// After:
uint64_t start_time = SDL_GetTicks();
```

**Key locations** (approximate line numbers):
- `amiberry.cpp`: frame timing, event loop timing
- `amiberry_gfx.cpp`: VSync timing, frame rate calculation
- `main_window.cpp`: GUI animation timing
- `controller_map.cpp`: input polling intervals
- `writelog.cpp`: log timestamps
- `bsdsocket_host.cpp`: network timeout calculations

**Warning**: If any code does `Uint32 elapsed = SDL_GetTicks() - start;`, this still works with `uint64_t` since subtraction handles wrap-around for `Uint32` but not for `uint64_t`. However, since SDL3's counter doesn't wrap in practice, this is fine. Just ensure the variable types are `uint64_t`.

### 2. `SDL_Delay()`

**SDL2**: `void SDL_Delay(Uint32 ms)`
**SDL3**: `void SDL_Delay(uint32_t ms)`

**Action**: No functional change needed (just the type rename already handled in Step 02).

### 3. `SDL_ShowCursor()` / `SDL_HideCursor()`

**SDL2**: `int SDL_ShowCursor(int toggle)` — pass `SDL_ENABLE`, `SDL_DISABLE`, or `SDL_QUERY`.
**SDL3**: Split into separate functions:
- `bool SDL_ShowCursor(void)` — shows the cursor, returns success
- `bool SDL_HideCursor(void)` — hides the cursor, returns success
- `bool SDL_CursorVisible(void)` — query current state

**Action**: Replace all `SDL_ShowCursor()` calls:
```cpp
// Before:
SDL_ShowCursor(SDL_ENABLE);
SDL_ShowCursor(SDL_DISABLE);
if (SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE) ...

// After:
SDL_ShowCursor();
SDL_HideCursor();
if (SDL_CursorVisible()) ...
```

**Key locations**:
- `amiberry_gfx.cpp`: cursor visibility during fullscreen, emulation
- `amiberry.cpp`: cursor management in event loop

### 4. `SDL_CreateSystemCursor()`

**SDL2**: `SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW)`
**SDL3**: `SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT)`

**Enum renames**:
| SDL2 | SDL3 |
|------|------|
| `SDL_SYSTEM_CURSOR_ARROW` | `SDL_SYSTEM_CURSOR_DEFAULT` |
| `SDL_SYSTEM_CURSOR_IBEAM` | `SDL_SYSTEM_CURSOR_TEXT` |
| `SDL_SYSTEM_CURSOR_WAIT` | `SDL_SYSTEM_CURSOR_WAIT` |
| `SDL_SYSTEM_CURSOR_CROSSHAIR` | `SDL_SYSTEM_CURSOR_CROSSHAIR` |
| `SDL_SYSTEM_CURSOR_SIZENWSE` | `SDL_SYSTEM_CURSOR_NWSE_RESIZE` |
| `SDL_SYSTEM_CURSOR_SIZENESW` | `SDL_SYSTEM_CURSOR_NESW_RESIZE` |
| `SDL_SYSTEM_CURSOR_SIZEWE` | `SDL_SYSTEM_CURSOR_EW_RESIZE` |
| `SDL_SYSTEM_CURSOR_SIZENS` | `SDL_SYSTEM_CURSOR_NS_RESIZE` |
| `SDL_SYSTEM_CURSOR_SIZEALL` | `SDL_SYSTEM_CURSOR_MOVE` |
| `SDL_SYSTEM_CURSOR_NO` | `SDL_SYSTEM_CURSOR_NOT_ALLOWED` |
| `SDL_SYSTEM_CURSOR_HAND` | `SDL_SYSTEM_CURSOR_POINTER` |

**Location**: `amiberry_platform_internal_host.h` line ~55.

### 5. `SDL_GetModState()`

**SDL2**: Returns `SDL_Keymod` (typedef for `int`).
**SDL3**: Returns `SDL_Keymod` (typedef for `uint16_t`).

**Action**: The function name is unchanged. Ensure the receiving variable is compatible:
```cpp
// Before:
SDL_Keymod mod = SDL_GetModState();

// After (same — SDL3 SDL_Keymod is uint16_t):
SDL_Keymod mod = SDL_GetModState();
```

**Modifier constants** may be renamed:
| SDL2 | SDL3 |
|------|------|
| `KMOD_LSHIFT` | `SDL_KMOD_LSHIFT` |
| `KMOD_RSHIFT` | `SDL_KMOD_RSHIFT` |
| `KMOD_LCTRL` | `SDL_KMOD_LCTRL` |
| `KMOD_RCTRL` | `SDL_KMOD_RCTRL` |
| `KMOD_LALT` | `SDL_KMOD_LALT` |
| `KMOD_RALT` | `SDL_KMOD_RALT` |
| `KMOD_LGUI` | `SDL_KMOD_LGUI` |
| `KMOD_RGUI` | `SDL_KMOD_RGUI` |

Search for all `KMOD_` usage and add `SDL_` prefix.

### 6. `SDL_free()`

**SDL3 changes to SDL_free() usage**:
- `SDL_GetClipboardText()`: SDL3 returns `const char*` that must NOT be freed. Remove `SDL_free()` calls after clipboard reads.
- `event.drop.file`: SDL3 manages drop file strings internally. Remove `SDL_free(event.drop.file)` calls.

**Key locations**:
- `src/osdep/clipboard.cpp`: After `SDL_GetClipboardText()`
- `src/osdep/amiberry_platform_internal_host.h` line ~110: `SDL_free(event.drop.file)`
- `src/osdep/amiberry.cpp`: Drop file handling
- `src/archivers/chd/osdlib_unix.cpp`: `SDL_free()` after `SDL_GetBasePath()` — SDL3 `SDL_GetBasePath()` returns `const char*`, no free needed.

### 7. `SDL_Log*()` Functions

**SDL2/SDL3**: `SDL_LogError()`, `SDL_LogWarn()`, `SDL_Log()` are unchanged in SDL3.

**Action**: No changes needed.

### 8. `SDL_GetError()`

**SDL2/SDL3**: Unchanged.

**Action**: No changes needed.

### 9. `SDL_PumpEvents()`

**SDL2/SDL3**: Unchanged.

**Action**: No changes needed.

## Acceptance Criteria

1. All `SDL_GetTicks()` return values stored in `uint64_t` variables
2. All `SDL_ShowCursor(SDL_ENABLE/DISABLE)` replaced with `SDL_ShowCursor()`/`SDL_HideCursor()`
3. All system cursor enum values updated to SDL3 names
4. All `KMOD_*` constants prefixed with `SDL_`
5. All unnecessary `SDL_free()` calls removed (clipboard, drop files, base path)
6. Code compiles without warnings related to timer, cursor, or utility functions
7. Cursor visibility works correctly in windowed and fullscreen modes
