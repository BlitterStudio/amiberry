# Step 06: Window, Display, and Renderer Migration

## Objective

Migrate window creation/management, display enumeration, and renderer operations from SDL2 to SDL3. This is the highest-risk step, affecting `amiberry_gfx.cpp` (133 SDL2 call sites) — the single most impacted file in the migration.

## Prerequisites

- Step 05 complete (surfaces and pixel formats migrated)

## Files to Modify

| File | Call Sites | Key Operations |
|------|-----------|----------------|
| `src/osdep/amiberry_gfx.cpp` | ~80 | Window create, renderer create, texture create/update, fullscreen, display mode |
| `src/osdep/amiberry_gfx.h` | ~10 | Struct declarations, externs |
| `src/osdep/gui/main_window.cpp` | ~30 | GUI window create, renderer create, ImGui texture management |
| `src/osdep/dpi_handler.cpp` | ~6 | Display mode queries, DPI calculation |
| `src/osdep/gfx_platform_internal_host.h` | ~5 | Window icon, screenshot |

## Detailed Changes

### 1. Window Creation

**SDL2**: `SDL_CreateWindow(title, x, y, w, h, flags)`
**SDL3**: `SDL_CreateWindow(title, w, h, flags)` — position is set separately.

```cpp
// Before:
SDL_Window* win = SDL_CreateWindow("Amiberry",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    width, height,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

// After:
SDL_Window* win = SDL_CreateWindow("Amiberry", width, height,
    SDL_WINDOW_RESIZABLE);  // SHOWN and ALLOW_HIGHDPI removed
SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
```

**Removed flags**:
- `SDL_WINDOW_SHOWN` — windows are shown by default in SDL3 (use `SDL_WINDOW_HIDDEN` to start hidden)
- `SDL_WINDOW_ALLOW_HIGHDPI` — always enabled in SDL3

**Key locations**:
- `amiberry_gfx.cpp` (~line 4559): Amiga emulation window
- `main_window.cpp` (~line 863): GUI window

### 2. Window Fullscreen

**SDL2**: `SDL_SetWindowFullscreen(window, flags)` where flags = `SDL_WINDOW_FULLSCREEN` (exclusive) or `SDL_WINDOW_FULLSCREEN_DESKTOP` (borderless).

**SDL3**: `SDL_SetWindowFullscreen(window, bool fullscreen)` — boolean. For borderless fullscreen (formerly `FULLSCREEN_DESKTOP`), call `SDL_SetWindowFullscreenMode(window, NULL)` before enabling fullscreen. For exclusive fullscreen, call `SDL_SetWindowFullscreenMode(window, &mode)` first.

```cpp
// Before (borderless fullscreen):
SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

// After (borderless fullscreen):
SDL_SetWindowFullscreenMode(window, NULL);  // NULL = use desktop mode
SDL_SetWindowFullscreen(window, true);

// Before (exclusive fullscreen):
SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

// After (exclusive fullscreen):
SDL_DisplayMode mode = { ... };  // desired mode
SDL_SetWindowFullscreenMode(window, &mode);
SDL_SetWindowFullscreen(window, true);

// Before (windowed):
SDL_SetWindowFullscreen(window, 0);

// After (windowed):
SDL_SetWindowFullscreen(window, false);
```

**Internal state tracking**: The `currentmode.flags` pattern in `amiberry_gfx.cpp` uses `SDL_WINDOW_FULLSCREEN` and `SDL_WINDOW_FULLSCREEN_DESKTOP` as bit flags. Refactor to use an internal enum:
```cpp
enum FullscreenMode {
    FSM_WINDOWED = 0,
    FSM_FULLSCREEN_EXCLUSIVE = 1,
    FSM_FULLSCREEN_DESKTOP = 2
};
```

### 3. Window Queries

| SDL2 | SDL3 | Notes |
|------|------|-------|
| `SDL_GetWindowFlags(win)` | `SDL_GetWindowFlags(win)` | Return type changes to `SDL_WindowFlags` |
| `SDL_GetWindowSize(win, &w, &h)` | `SDL_GetWindowSize(win, &w, &h)` | Unchanged |
| `SDL_GetWindowPosition(win, &x, &y)` | `SDL_GetWindowPosition(win, &x, &y)` | Unchanged |
| `SDL_GetWindowDisplayIndex(win)` | `SDL_GetDisplayForWindow(win)` | Returns `SDL_DisplayID` not `int` |
| `SDL_GetWindowFromID(id)` | `SDL_GetWindowFromID(id)` | Unchanged |
| `SDL_GetWindowID(win)` | `SDL_GetWindowID(win)` | Unchanged |

### 4. Display Enumeration

**SDL2**: Index-based (`int displayIndex` from 0 to `SDL_GetNumVideoDisplays()-1`).
**SDL3**: ID-based (`SDL_DisplayID` from `SDL_GetDisplays()`).

```cpp
// Before (SDL2):
int num = SDL_GetNumVideoDisplays();
for (int i = 0; i < num; i++) {
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(i, &mode);
    // use mode.w, mode.h, mode.refresh_rate
}

// After (SDL3):
int count;
SDL_DisplayID* displays = SDL_GetDisplays(&count);
for (int i = 0; i < count; i++) {
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[i]);
    // use mode->w, mode->h, mode->refresh_rate (now float)
}
SDL_free(displays);
```

**SDL_DisplayMode struct changes**:
- `refresh_rate`: `int` → `float` in SDL3
- `format`: may change type
- The struct is now returned as `const SDL_DisplayMode*` (pointer), not filled into a stack variable

**Key locations**:
- `amiberry_gfx.cpp`: display mode enumeration for `Displays[]` array
- `dpi_handler.cpp`: DPI calculation from display mode

### 5. Renderer Creation

**SDL2**: `SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)`
**SDL3**: `SDL_CreateRenderer(window, NULL)` — renderer name (NULL = auto), no flags.

```cpp
// Before:
SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

// After:
SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
SDL_SetRenderVSync(renderer, 1);  // Enable VSync separately
```

**VSync control**:
- `SDL_RENDERER_PRESENTVSYNC` flag removed
- Use `SDL_SetRenderVSync(renderer, 1)` to enable, `SDL_SetRenderVSync(renderer, 0)` to disable

### 6. Render Operations

| SDL2 | SDL3 | Notes |
|------|------|-------|
| `SDL_RenderCopy(r, tex, src, dst)` | `SDL_RenderTexture(r, tex, src, dst)` | Renamed |
| `SDL_RenderCopyEx(r, tex, src, dst, angle, center, flip)` | `SDL_RenderTextureRotated(r, tex, src, dst, angle, center, flip)` | Renamed |
| `SDL_RenderClear(r)` | `SDL_RenderClear(r)` | Unchanged |
| `SDL_RenderPresent(r)` | `SDL_RenderPresent(r)` | Unchanged but returns `bool` |
| `SDL_SetRenderDrawColor(r, r, g, b, a)` | `SDL_SetRenderDrawColor(r, r, g, b, a)` | Args now `uint8_t` (same in practice) |
| `SDL_RenderFillRect(r, rect)` | `SDL_RenderFillRect(r, rect)` | Unchanged |
| `SDL_RenderSetScale(r, sx, sy)` | `SDL_SetRenderScale(r, sx, sy)` | Renamed |

**Rect types**: SDL3 introduces `SDL_FRect` (float rect) alongside `SDL_Rect` (int rect). Many render functions now use `SDL_FRect*` instead of `SDL_Rect*`. Check parameter types:
- `SDL_RenderTexture(renderer, texture, const SDL_FRect* srcrect, const SDL_FRect* dstrect)` — uses `SDL_FRect`!

If Amiberry uses `SDL_Rect` for render calls, convert to `SDL_FRect`:
```cpp
// Before:
SDL_Rect dst = {x, y, w, h};
SDL_RenderCopy(renderer, texture, NULL, &dst);

// After:
SDL_FRect dst = {(float)x, (float)y, (float)w, (float)h};
SDL_RenderTexture(renderer, texture, NULL, &dst);
```

### 7. Texture Operations

| SDL2 | SDL3 | Notes |
|------|------|-------|
| `SDL_CreateTexture(r, fmt, access, w, h)` | `SDL_CreateTexture(r, fmt, access, w, h)` | Unchanged |
| `SDL_CreateTextureFromSurface(r, s)` | `SDL_CreateTextureFromSurface(r, s)` | Unchanged |
| `SDL_UpdateTexture(tex, rect, pixels, pitch)` | `SDL_UpdateTexture(tex, rect, pixels, pitch)` | Unchanged |
| `SDL_QueryTexture(tex, &fmt, &access, &w, &h)` | `SDL_GetTextureSize(tex, &w, &h)` | Simplified; fmt/access via properties |
| `SDL_DestroyTexture(tex)` | `SDL_DestroyTexture(tex)` | Unchanged |
| `SDL_SetTextureBlendMode(tex, mode)` | `SDL_SetTextureBlendMode(tex, mode)` | Unchanged |
| `SDL_SetTextureColorMod(tex, r, g, b)` | `SDL_SetTextureColorMod(tex, r, g, b)` | Unchanged |

**Texture access constants**: `SDL_TEXTUREACCESS_STREAMING` → `SDL_TEXTUREACCESS_STREAMING` (unchanged).

### 8. Render Scale Quality (Texture Filtering)

**SDL2**: Global hint `SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear")` before creating texture.
**SDL3**: Per-texture: `SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR)` after creating texture.

```cpp
// Before:
SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
SDL_Texture* tex = SDL_CreateTexture(renderer, ...);

// After:
SDL_Texture* tex = SDL_CreateTexture(renderer, ...);
SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
```

Scale modes: `SDL_SCALEMODE_NEAREST`, `SDL_SCALEMODE_LINEAR`.

### 9. Renderer Info

**SDL2**: `SDL_GetRendererInfo(renderer, &info)` fills `SDL_RendererInfo`.
**SDL3**: `SDL_GetRendererName(renderer)` returns `const char*`. Max texture size via `SDL_GetRendererProperties()`.

## Acceptance Criteria

1. Emulation window creates and displays correctly on all platforms
2. GUI window creates and displays correctly
3. Fullscreen toggle works (both borderless desktop and exclusive modes)
4. Display mode enumeration returns valid results for all connected displays
5. Renderer creates with hardware acceleration and optional VSync
6. Textures create, update, and render correctly
7. Amiga display, status line, and OSD all render correctly
8. Window resizing works
9. Multi-monitor support works (window positioning, display detection)
10. No flickering, tearing, or rendering artifacts
