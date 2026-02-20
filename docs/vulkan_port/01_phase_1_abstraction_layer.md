# Phase 1: Rendering Abstraction Layer

## Objective
The goal of this phase is to untangle the hardcoded `#ifdef USE_OPENGL` statements and OpenGL/SDL_Render specific code from `src/osdep/amiberry_gfx.cpp`. We need a clean, uniform abstraction (e.g., `IRenderer` interface) so that adding Vulkan does not require duplicate `if/else` logic scattered across the file. 

## Target Files to Modify
- `src/osdep/amiberry_gfx.cpp`
- `src/osdep/amiberry_gfx.h` (or introduce a new `renderer.h`)

## Context and Instructions
Currently, `SDL2_renderframe()` handles both pure SDL falling-back paths and OpenGL paths.

### 1. Define the Interface `IRenderer`
Create a new header file `src/osdep/renderer.h` containing an abstract base class `IRenderer`. The interface should cover the following operations:
- `bool initialize(...)`: Set up the API context.
- `void destroy()`: Clean up resources.
- `bool allocate_texture(int w, int h, const char* shader_name)`: Prepare the backbuffer texture or shader pipelines.
- `bool render_frame(...)`: Execute the main drawing logic.
- `void present(...)`: Present the buffer to the screen.

### 2. Implement `SDLRenderer` and `OpenGLRenderer`
Create `src/osdep/sdl_renderer.cpp` and `src/osdep/gl_renderer.cpp` that both inherit from `IRenderer`. 
- Move all SDL texture/surface copying code (`SDL_UpdateTexture`, `SDL_RenderClear`, `SDL_RenderCopy`) into `SDLRenderer::render_frame()`.
- Move all OpenGL specific shader setup (from `crt_frame.h`, `crtemu_shader`, `glUseProgram`) into `OpenGLRenderer::render_frame()`.

### 3. Wire into `amiberry_gfx.cpp`
In `amiberry_gfx.cpp`, instantiate the appropriate renderer subclass based on the build configuration or user preference. Replace calls inside `SDL2_renderframe` and `SDL2_alloctexture` with, for example:
```cpp
if (g_renderer) {
    return g_renderer->render_frame(monid, mode, immediate);
}
```

## Acceptance Criteria
- Code compiles correctly with OpenGL and SDL backends dynamically selected or cleanly isolated.
- The emulator still runs and renders natively or via RTG exactly as before.
- `amiberry_gfx.cpp` no longer contains direct `<GL/gl.h>` or shader compilation logic directly mixed with its window event loops.
