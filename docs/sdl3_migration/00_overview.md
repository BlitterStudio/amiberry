# Step 00: SDL2 to SDL3 Migration Overview & Compatibility Strategy

## Objective

Establish the overall migration strategy, document the SDL3 version pin, and create a thin compatibility header to enable incremental migration across 12 subsequent steps.

## Background

Amiberry uses SDL2 (release-2.30.10) with approximately 747 API call sites across 47 source files. SDL3 (stable since 2025) introduces numerous breaking changes but also significant improvements:

- **SDL_GPU**: Cross-platform GPU API supporting Vulkan, Metal, and D3D12 — aligns with the planned Vulkan rendering backend
- **SDL_AudioStream**: Modern audio model replacing the old device/callback/queue pattern
- **Unified Gamepad API**: "GameController" renamed to "Gamepad" with cleaner semantics
- **Properties API**: Type-safe configuration for windows, renderers, and audio devices
- **Native types**: Uses standard C `uint32_t` instead of `Uint32`, `bool` instead of `SDL_bool`
- **Flattened window events**: `SDL_WINDOWEVENT` sub-events become top-level `SDL_EVENT_WINDOW_*` events

### Key SDL3 Resources

- SDL2→SDL3 migration guide: https://wiki.libsdl.org/SDL3/README/migration
- SDL3 API reference: https://wiki.libsdl.org/SDL3
- SDL3 GitHub: https://github.com/libsdl-org/SDL (branch: main, tags: release-3.2.x)
- SDL3_image GitHub: https://github.com/libsdl-org/SDL_image (branch: main, tags: release-3.2.x)

### SDL3 Version Pin

Use the latest stable SDL3 release at execution time. As of February 2026, this is `release-3.2.8` (or newer). For SDL3_image, use the matching `release-3.2.x` tag.

## Migration Strategy

### Approach: Direct Port (No Compatibility Shim)

SDL3 ships an `sdl2-compat` library, but Amiberry will migrate natively to fully leverage SDL3 improvements. The migration is ordered by dependency:

1. **Build infrastructure** (Steps 00-01) — CMake, vcpkg, Brew, CI
2. **Mechanical renames** (Steps 02-03) — types, includes, timer/utility
3. **Subsystem-by-subsystem** (Steps 04-12) — threading, surfaces, window/renderer, events, audio, input, OpenGL, ImGui, platform/packaging

Each step is designed to compile cleanly when complete.

### Transition Compatibility Header

During the migration, a thin `src/osdep/sdl_compat.h` header provides macro aliases for the most pervasive mechanical renames. This header is **temporary** and removed in Step 12.

## Files to Create

| File | Purpose |
|------|---------|
| `docs/sdl3_migration/00_overview.md` | This document |
| `src/osdep/sdl_compat.h` | Temporary compatibility macros (created during Step 02) |

## Compatibility Header Design

The header `src/osdep/sdl_compat.h` will contain:

```c
#pragma once

// SDL3 migration compatibility header — TEMPORARY
// Remove after all migration steps are complete (Step 12)

#include <SDL3/SDL.h>

// === Surface API renames ===
// SDL_CreateRGBSurfaceWithFormat(flags, w, h, depth, format) -> SDL_CreateSurface(w, h, format)
// SDL_FreeSurface(s) -> SDL_DestroySurface(s)
// These cannot be trivially macro'd due to parameter count changes.
// Each call site must be updated individually in Step 05.

// === Type aliases (if needed for gradual migration) ===
// SDL3 uses standard C types natively. These are only needed if
// intermediate steps compile before all Uint32 -> uint32_t renames.
// typedef uint32_t Uint32;
// typedef uint16_t Uint16;
// typedef uint8_t  Uint8;

// === SDL_bool removal ===
// SDL3 uses native bool. Replace SDL_TRUE -> true, SDL_FALSE -> false.
```

**Note**: Most SDL2→SDL3 changes cannot be handled by simple macros because function signatures change (different parameter counts, different return types, renamed structs). The header primarily serves as documentation and a central include point during transition.

## Step Execution Order

Steps must be executed in order (00 → 12). Each step depends on all previous steps being complete. The dependency chain is:

```
00 (overview) → 01 (build system) → 02 (types/includes) → 03 (timer/util)
    → 04 (threading) → 05 (surfaces) → 06 (window/renderer) → 07 (events)
    → 08 (audio) → 09 (input) → 10 (OpenGL/hints) → 11 (ImGui/SDL_image)
    → 12 (platform/CI/packaging/cleanup)
```

Steps 04-10 have limited inter-dependencies and could theoretically be parallelized after Step 03, but sequential execution is safer.

## Relationship to Vulkan Port

The existing `docs/vulkan_port/` documents describe a 5-phase plan to add a Vulkan rendering backend. SDL3 migration is **complementary**:

- **Phase 1 (IRenderer abstraction)**: Already complete. `IRenderer` (`src/osdep/irenderer.h`) is implemented with `OpenGLRenderer` and `SDLRenderer` backends, wired through `renderer_factory.h/.cpp`. GL shader infrastructure for overlays (vkbd, on-screen joystick, OSD) has been extracted into dedicated `_gl.cpp` files. A future `VulkanRenderer` or `SDLGPURenderer` would implement the same interface.
- **SDL_GPU opportunity**: SDL3's `SDL_GPU` API could serve as the backend for `IRenderer` instead of raw Vulkan, providing Vulkan/Metal/D3D12 support through a single API. ImGui already has `imgui_impl_sdlgpu3.cpp` for this.
- **Migration order**: SDL3 migration can proceed independently; the rendering abstraction is already in place.

## Acceptance Criteria

- This overview document exists at `docs/sdl3_migration/00_overview.md`
- All 12 subsequent step documents exist and are self-contained
- No source code changes in this step (documentation only)
