# Step 02: Type System and Include Path Migration

## Objective

Update all `#include <SDL.h>` to `#include <SDL3/SDL.h>`, all `#include <SDL_image.h>` to `#include <SDL3_image/SDL_image.h>`, and migrate SDL-specific integer types to standard C types. After this step, all SDL header resolution works and type names are correct.

## Prerequisites

- Step 01 complete (build system finds SDL3)

## Files to Modify

Every file that includes SDL headers (~47 files). The key groups are:

### Headers (9 files)
| File | SDL includes to update |
|------|----------------------|
| `src/threaddep/thread.h` | `<SDL.h>` |
| `src/osdep/target.h` | `<SDL.h>` |
| `src/osdep/gfx_platform_internal_host.h` | `<SDL.h>`, `<SDL_image.h>` |
| `src/osdep/amiberry_input.h` | `<SDL.h>` |
| `src/osdep/on_screen_joystick.h` | `<SDL.h>` |
| `src/osdep/gl_platform.h` | `<SDL.h>` |
| `src/osdep/dpi_handler.hpp` | `<SDL.h>` |
| `src/osdep/amiberry_gfx.h` | `<SDL.h>` |
| `src/include/uae/string.h` | `<SDL.h>` |

### Implementation files (38 files)
All `.cpp` files in `src/osdep/`, `src/osdep/gui/`, `src/osdep/imgui/`, `src/osdep/vkbd/`, `src/sounddep/`, `src/threaddep/`, plus `src/main.cpp`, `src/cfgfile.cpp`, `src/devices.cpp`, `src/sampler.cpp`, `src/ppc/ppc.cpp`, `src/archivers/chd/osdlib_*.cpp`, `src/jit/arm/compemu_support_arm.cpp`.

## Detailed Changes

### 1. Include Path Changes

**SDL core**:
```cpp
// Before:
#include <SDL.h>
// After:
#include <SDL3/SDL.h>
```

**SDL_image**:
```cpp
// Before:
#include <SDL_image.h>
// After:
#include <SDL3_image/SDL_image.h>
```

**SDL_main** (if used):
```cpp
// Before:
#include <SDL_main.h>
// After:
#include <SDL3/SDL_main.h>
```

### 2. Integer Type Migration

SDL3 uses standard C/C++ integer types. Replace globally across all files:

| SDL2 Type | SDL3 / Standard Type | Notes |
|-----------|---------------------|-------|
| `Uint8` | `uint8_t` | Include `<cstdint>` or rely on SDL3 |
| `Uint16` | `uint16_t` | |
| `Uint32` | `uint32_t` | Most common — ~50+ occurrences |
| `Uint64` | `uint64_t` | |
| `Sint8` | `int8_t` | |
| `Sint16` | `int16_t` | |
| `Sint32` | `int32_t` | |
| `Sint64` | `int64_t` | |

**Key locations**:
- `src/osdep/amiberry_gfx.h` line 14: `extern Uint32 pixel_format;` → `extern uint32_t pixel_format;`
- `src/osdep/amiberry_gfx.cpp` line 167: pixel format declarations
- `src/sounddep/sound.cpp`: audio format types
- `src/osdep/amiberry_input.cpp`: joystick axis types
- Many other files — use find-and-replace across the codebase

**Important**: `Uint32` etc. are also used in Amiberry's own headers (not just SDL contexts). Only replace instances that are SDL types. Amiberry's UAE core uses `uae_u32`, `uae_s16` etc. which are separate and must NOT be changed.

### 3. SDL_bool Migration

SDL3 uses native C/C++ `bool` instead of `SDL_bool`:

| SDL2 | SDL3 |
|------|------|
| `SDL_bool` | `bool` |
| `SDL_TRUE` | `true` |
| `SDL_FALSE` | `false` |

**Key locations**:
- `src/osdep/amiberry_gfx.cpp`: `SDL_SetHint()` return value checks
- `src/osdep/amiberry.cpp`: various boolean comparisons
- Any function returning or accepting `SDL_bool`

### 4. Audio Format Constants

```cpp
// Before:
AUDIO_S16SYS
AUDIO_S16
AUDIO_F32SYS

// After:
SDL_AUDIO_S16
SDL_AUDIO_S16LE / SDL_AUDIO_S16BE  // (SYS variants may be removed)
SDL_AUDIO_F32
```

**Location**: `src/sounddep/sound.cpp` line ~415, `src/osdep/ahi_v1.cpp`

### 5. Miscellaneous Type Renames

| SDL2 | SDL3 |
|------|------|
| `SDL_sem` | `SDL_Semaphore` |
| `SDL_mutex` | `SDL_Mutex` |
| `SDL_cond` | `SDL_Condition` |
| `SDL_threadID` | `SDL_ThreadID` |

These are listed here for awareness but will be fully addressed in Step 04 (Threading).

## Search Patterns

Use these regex patterns to find all instances:

```bash
# SDL includes
grep -rn '#include.*<SDL\.h>' src/
grep -rn '#include.*<SDL_image\.h>' src/
grep -rn '#include.*<SDL_main\.h>' src/

# SDL integer types (be careful not to match uae_u32 etc.)
grep -rn '\bUint8\b' src/
grep -rn '\bUint16\b' src/
grep -rn '\bUint32\b' src/
grep -rn '\bUint64\b' src/
grep -rn '\bSint16\b' src/
grep -rn '\bSint32\b' src/

# SDL_bool
grep -rn '\bSDL_bool\b' src/
grep -rn '\bSDL_TRUE\b' src/
grep -rn '\bSDL_FALSE\b' src/

# Audio format constants
grep -rn 'AUDIO_S16' src/
grep -rn 'AUDIO_F32' src/
```

## Acceptance Criteria

1. All files include `<SDL3/SDL.h>` instead of `<SDL.h>`
2. All files include `<SDL3_image/SDL_image.h>` instead of `<SDL_image.h>`
3. No `Uint8`, `Uint16`, `Uint32`, `Uint64`, `Sint*` types remain (outside of UAE core's own `uae_*` types)
4. No `SDL_bool`, `SDL_TRUE`, `SDL_FALSE` remain
5. Audio format constants use SDL3 names
6. Headers resolve correctly (no "file not found" errors from includes)
7. Type mismatches produce no compilation warnings
