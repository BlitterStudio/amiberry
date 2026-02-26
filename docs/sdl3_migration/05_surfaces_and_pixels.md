# Step 05: Surface and Pixel Format Migration

## Objective

Migrate all SDL_Surface creation, destruction, pixel format access, and blitting operations from SDL2 to SDL3 equivalents. This is a medium-risk step affecting ~15 files and ~60 call sites.

## Prerequisites

- Step 04 complete (threading migrated)

## Files to Modify

| File | Surface Operations |
|------|-------------------|
| `src/osdep/amiberry_gfx.cpp` | Surface create/free (~15 sites), pixel format access, format conversion |
| `src/osdep/amiberry_gfx.h` | `SYSTEM_*_SHIFT` macros using `surface->format->*` |
| `src/osdep/on_screen_joystick.cpp` | 8 surface create/free calls |
| `src/osdep/picasso96.cpp` | Cursor overlay surfaces |
| `src/osdep/gui/main_window.cpp` | Icon surface, GUI screen surface |
| `src/osdep/imgui/about.cpp` | Logo image surface |
| `src/osdep/imgui/controller_map.cpp` | Controller image surface |
| `src/osdep/imgui/savestates.cpp` | Savestate screenshot surface |
| `src/osdep/shader_preset.cpp` | LUT texture surfaces |
| `src/osdep/vkbd/vkbd.cpp` | Keyboard skin surfaces |
| `src/osdep/gfx_platform_internal_host.h` | Window icon surface, screenshot PNG |
| `src/osdep/clipboard.cpp` | Possible surface usage |
| `src/osdep/amiberry.cpp` | Icon loading |

## Detailed Changes

### 1. Surface Creation

**`SDL_CreateRGBSurfaceWithFormat()`** → **`SDL_CreateSurface()`**:
```cpp
// Before (SDL2):
SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);

// After (SDL3):
SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB8888);
```
- The `flags` parameter (first arg, always 0) is removed
- The `depth` parameter (fourth arg) is removed (derived from format)

**`SDL_CreateRGBSurface()`** → **`SDL_CreateSurface()`**:
```cpp
// Before (SDL2):
SDL_Surface* s = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);

// After (SDL3):
// Derive the pixel format from the masks, then use SDL_CreateSurface
SDL_PixelFormat fmt = SDL_GetPixelFormatForMasks(32, rmask, gmask, bmask, amask);
SDL_Surface* s = SDL_CreateSurface(w, h, fmt);
```
Or, if the format is known (which it usually is in Amiberry), just use the format constant directly.

**`SDL_CreateRGBSurfaceWithFormatFrom()`** → **`SDL_CreateSurfaceFrom()`**:
```cpp
// Before (SDL2):
SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, depth, pitch, format);

// After (SDL3) — NOTE PARAMETER ORDER CHANGE:
SDL_Surface* s = SDL_CreateSurfaceFrom(w, h, format, pixels, pitch);
```
- `depth` parameter removed
- Parameter order changed: `(w, h, format, pixels, pitch)` instead of `(pixels, w, h, depth, pitch, format)`

**`SDL_CreateRGBSurfaceFrom()`** → **`SDL_CreateSurfaceFrom()`**:
```cpp
// Before (SDL2):
SDL_Surface* s = SDL_CreateRGBSurfaceFrom(pixels, w, h, bpp, pitch, rmask, gmask, bmask, amask);

// After (SDL3):
SDL_PixelFormat fmt = SDL_GetPixelFormatForMasks(bpp, rmask, gmask, bmask, amask);
SDL_Surface* s = SDL_CreateSurfaceFrom(w, h, fmt, pixels, pitch);
```

### 2. Surface Destruction

```cpp
// Before:
SDL_FreeSurface(surface);

// After:
SDL_DestroySurface(surface);
```

This is a simple global rename. ~40 occurrences across the codebase.

### 3. Surface Blitting

```cpp
// Before:
SDL_BlitSurface(src, &srcrect, dst, &dstrect);
SDL_BlitScaled(src, &srcrect, dst, &dstrect);

// After:
SDL_BlitSurface(src, &srcrect, dst, &dstrect);       // Unchanged
SDL_BlitSurfaceScaled(src, &srcrect, dst, &dstrect, SDL_SCALEMODE_NEAREST);  // Renamed + scale mode param
```

Note: `SDL_BlitSurfaceScaled()` in SDL3 takes an additional `SDL_ScaleMode` parameter.

### 4. Surface Format Conversion

```cpp
// Before:
SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);

// After:
SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);
```

`SDL_ConvertSurfaceFormat()` is renamed to `SDL_ConvertSurface()` and the flags parameter is removed.

### 5. Fill Rect

```cpp
// Before:
SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, r, g, b));

// After:
SDL_FillSurfaceRect(surface, &rect, SDL_MapSurfaceRGB(surface, r, g, b));
```

- `SDL_FillRect()` → `SDL_FillSurfaceRect()`
- `SDL_MapRGB(format, r, g, b)` → `SDL_MapSurfaceRGB(surface, r, g, b)` (takes surface instead of format)
- `SDL_MapRGBA(format, r, g, b, a)` → `SDL_MapSurfaceRGBA(surface, r, g, b, a)`

### 6. Pixel Format Access (Critical)

This is the most complex change. SDL3 restructures how pixel format information is accessed.

**SDL2**: `surface->format` is an `SDL_PixelFormat*` struct with fields `Rmask`, `Gmask`, `Bmask`, `Amask`, `Rshift`, `Gshift`, `Bshift`, `Ashift`, `BytesPerPixel`, etc.

**SDL3**: `surface->format` is an `SDL_PixelFormat` **enum value** (not a pointer). To get masks/shifts, call:
```cpp
const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surface->format);
details->Rmask;
details->Rshift;
details->BytesPerPixel;  // Actually called bits_per_pixel / 8
```

**Critical macros in `amiberry_gfx.h`**:
```cpp
// Before (SDL2):
#define SYSTEM_RED_SHIFT   amiga_surface->format->Rshift
#define SYSTEM_GREEN_SHIFT amiga_surface->format->Gshift
#define SYSTEM_BLUE_SHIFT  amiga_surface->format->Bshift
#define SYSTEM_ALPHA_SHIFT amiga_surface->format->Ashift

// After (SDL3):
// Option A: Compute once and cache in global variables
extern int system_red_shift, system_green_shift, system_blue_shift, system_alpha_shift;
#define SYSTEM_RED_SHIFT   system_red_shift
#define SYSTEM_GREEN_SHIFT system_green_shift
#define SYSTEM_BLUE_SHIFT  system_blue_shift
#define SYSTEM_ALPHA_SHIFT system_alpha_shift

// Initialize after surface creation:
const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(amiga_surface->format);
system_red_shift = details->Rshift;
system_green_shift = details->Gshift;
system_blue_shift = details->Bshift;
system_alpha_shift = details->Ashift;
```

**Option B**: Use inline functions:
```cpp
static inline int get_red_shift(SDL_Surface* s) {
    return SDL_GetPixelFormatDetails(s->format)->Rshift;
}
```

**Recommended**: Option A (cache in globals), since these macros are used in hot render paths and should not call SDL functions per-pixel.

### 7. Surface Fields

| SDL2 | SDL3 |
|------|------|
| `surface->format->BitsPerPixel` | `SDL_GetPixelFormatDetails(surface->format)->bits_per_pixel` |
| `surface->format->BytesPerPixel` | `SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel` |
| `surface->format->Rmask` | `SDL_GetPixelFormatDetails(surface->format)->Rmask` |
| `surface->format->format` | `surface->format` (directly on surface in SDL3) |
| `surface->w` | `surface->w` (unchanged) |
| `surface->h` | `surface->h` (unchanged) |
| `surface->pitch` | `surface->pitch` (unchanged) |
| `surface->pixels` | `surface->pixels` (unchanged) |

### 8. Pixel Format Constants

Most SDL3 pixel format constants keep the same names. Verify these are still valid:
- `SDL_PIXELFORMAT_ABGR8888` ✓
- `SDL_PIXELFORMAT_ARGB8888` ✓
- `SDL_PIXELFORMAT_RGBA32` ✓ (alias for platform-endian RGBA)
- `SDL_PIXELFORMAT_RGB565` ✓
- `SDL_PIXELFORMAT_RGB555` → may be `SDL_PIXELFORMAT_XRGB1555` in SDL3

### 9. Lock/Unlock Surface

```cpp
// Before:
SDL_LockSurface(surface);
SDL_UnlockSurface(surface);

// After (unchanged):
SDL_LockSurface(surface);
SDL_UnlockSurface(surface);
```

Note: `SDL_LockSurface()` returns `bool` in SDL3 (was `int` in SDL2).

## Search Patterns

```bash
grep -rn 'SDL_CreateRGBSurface' src/
grep -rn 'SDL_CreateRGBSurfaceWithFormat' src/
grep -rn 'SDL_CreateRGBSurfaceFrom' src/
grep -rn 'SDL_FreeSurface' src/
grep -rn 'SDL_BlitSurface\|SDL_BlitScaled' src/
grep -rn 'SDL_ConvertSurface' src/
grep -rn 'SDL_FillRect' src/
grep -rn 'SDL_MapRGB\|SDL_MapRGBA' src/
grep -rn 'surface->format->' src/
grep -rn 'SYSTEM_RED_SHIFT\|SYSTEM_GREEN_SHIFT\|SYSTEM_BLUE_SHIFT' src/
```

## Acceptance Criteria

1. All `SDL_CreateRGBSurface*` calls replaced with `SDL_CreateSurface`/`SDL_CreateSurfaceFrom`
2. All `SDL_FreeSurface` → `SDL_DestroySurface`
3. `SDL_BlitScaled` → `SDL_BlitSurfaceScaled` with scale mode parameter
4. `SDL_FillRect` → `SDL_FillSurfaceRect`, `SDL_MapRGB` → `SDL_MapSurfaceRGB`
5. `SYSTEM_*_SHIFT` macros work correctly with cached pixel format details
6. Amiga display renders correctly (correct colors, no corrupted pixels)
7. Status line overlay, on-screen joystick, virtual keyboard all render correctly
8. Screenshot saving works
9. No pixel format assertion failures or color channel swaps
