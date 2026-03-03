# Phase 1: Rendering Abstraction Layer

> **Status: COMPLETE** — This phase has been fully implemented on the `refactor-amiberry-gfx` branch.

## Objective
The goal of this phase was to untangle the hardcoded `#ifdef USE_OPENGL` statements and OpenGL/SDL_Render specific code from `src/osdep/amiberry_gfx.cpp`. A clean, uniform abstraction (`IRenderer` interface) was created so that adding Vulkan does not require duplicate `if/else` logic scattered across the file.

## What Was Implemented

### 1. IRenderer Interface (`src/osdep/irenderer.h`)
Abstract base class with virtual methods covering:
- **Context lifecycle**: `init_context()`, `destroy_context()`, `has_context()`
- **Window/platform**: `get_window_flags()`, `set_context_attributes()`, `create_platform_renderer()`, `destroy_platform_renderer()`
- **Texture/rendering**: `alloc_texture()`, `set_scaling()`, `render_frame()`, `present_frame()`, `update_vsync()`
- **Shader management**: `destroy_shaders()`, `clear_shader_cache()`, `reset_state()`, `has_valid_shader()`, `has_shader_parameters()`
- **Bezel overlay**: `update_custom_bezel()`, `update_crtemu_bezel()`, `get_bezel_hole_info()`, `render_bezel()`, `destroy_bezel()`
- **OSD/overlays**: `sync_osd_texture()`, `render_osd()`, `render_vkbd()`, `render_onscreen_joystick()`, `render_software_cursor()`
- **Input**: `get_gfx_offset()` for mouse coordinate translation
- **GUI**: `prepare_gui_sharing()`, `restore_emulation_context()`

Shared layout state: `render_quad`, `crop_rect` (SDL_Rect members on IRenderer).

### 2. OpenGLRenderer (`src/osdep/opengl_renderer.h/.cpp`)
- Owns GL context (`SDL_GLContext`), `ShaderState` (crtemu, external GLSL, GLSLP presets), and `GLOverlayState` (OSD, bezel, cursor textures/shaders/VAOs)
- Accessed via typed helper `get_opengl_renderer()`

### 3. SDLRenderer (`src/osdep/sdl_renderer.h/.cpp`)
- Owns `SDL_Texture*` for Amiga frame and cursor overlay
- Shader/bezel methods use IRenderer defaults (no-op)
- Accessed via typed helper `get_sdl_renderer()`

### 4. Renderer Factory (`src/osdep/renderer_factory.h/.cpp`)
- `create_renderer()` returns `OpenGLRenderer` when `USE_OPENGL` is defined, `SDLRenderer` otherwise
- Global `g_renderer` pointer used throughout orchestration code

### 5. GL Infrastructure Extraction
GL shader compilation, texture upload, and quad rendering were extracted from overlay files:

| Component | Common logic | GL infrastructure |
|-----------|-------------|-------------------|
| Virtual keyboard | `src/osdep/vkbd/vkbd.cpp` | `src/osdep/vkbd/vkbd_gl.h/.cpp` |
| On-screen joystick | `src/osdep/on_screen_joystick.cpp` | `src/osdep/on_screen_joystick_gl.h/.cpp` |
| OSD overlays | `src/osdep/amiberry_gfx.cpp` | `src/osdep/gl_overlays.cpp` |

### 6. Orchestration Cleanup
`amiberry_gfx.cpp` dispatches through `g_renderer->` for all backend-specific operations. Direct GL calls and `#ifdef USE_OPENGL` blocks are largely eliminated from orchestration code.

## Files Created/Modified

| File | Status |
|------|--------|
| `src/osdep/irenderer.h` | Created — abstract interface |
| `src/osdep/opengl_renderer.h` | Created — GL backend header |
| `src/osdep/opengl_renderer.cpp` | Created — GL backend implementation |
| `src/osdep/sdl_renderer.h` | Created — SDL backend header |
| `src/osdep/sdl_renderer.cpp` | Created — SDL backend implementation |
| `src/osdep/renderer_factory.h` | Created — factory header |
| `src/osdep/renderer_factory.cpp` | Created — factory implementation |
| `src/osdep/gl_overlays.cpp` | Created — GL OSD overlay shader |
| `src/osdep/gfx_state.h` | Created — shared VSyncState |
| `src/osdep/gfx_window.cpp` | Created — window management |
| `src/osdep/gfx_colors.cpp` | Created — pixel format detection |
| `src/osdep/gfx_prefs_check.cpp` | Created — graphics pref validation |
| `src/osdep/display_modes.cpp` | Created — display mode enumeration |
| `src/osdep/vkbd/vkbd_gl.h` | Created — vkbd GL infrastructure header |
| `src/osdep/vkbd/vkbd_gl.cpp` | Created — vkbd GL shader/texture/quad |
| `src/osdep/on_screen_joystick_gl.h` | Created — OSJ GL infrastructure header |
| `src/osdep/on_screen_joystick_gl.cpp` | Created — OSJ GL shader/texture/quad |
| `src/osdep/amiberry_gfx.cpp` | Modified — dispatches through IRenderer |
| `cmake/SourceFiles.cmake` | Modified — added all new .cpp files |

## Acceptance Criteria — All Met
- Code compiles correctly with both OpenGL and SDL backends cleanly isolated.
- The emulator runs and renders natively and via RTG exactly as before.
- `amiberry_gfx.cpp` no longer contains shader compilation logic or direct GL state management mixed with window event loops.

## Next Steps
- **Phase 2**: Implement `VulkanRenderer` (or `SDLGPURenderer`) that inherits from `IRenderer`
- The existing interface is designed to accommodate Vulkan's different initialization and frame presentation model
