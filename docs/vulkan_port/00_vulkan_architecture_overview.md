# Overview: Amiberry Vulkan Backend Port

This directory contains a phased approach to implementing a multi-threaded Vulkan back-end for Amiberry. The current rendering model relies heavily on direct OpenGL and SDL2 calls intermingled with emulation logic inside `src/osdep/amiberry_gfx.cpp`.

To introduce Vulkan without breaking existing features, the work is divided into 5 distinct phases.

## The Strategy
- **Decoupling**: We first abstract the rendering logic so that different backends (OpenGL, SDL, Vulkan) can be swapped cleanly.
- **Incremental Implementation**: We build the Vulkan backend piece-by-piece. We start by just getting a window and a swapchain up, then buffer uploads, then we handle shaders, and finally, we optimize with multithreading.

## The Phases
1. **[Phase 1: Abstraction Layer](01_phase_1_abstraction_layer.md)**: Create the `IRenderer` interface.
2. **[Phase 2: Context & Swapchain](02_phase_2_vulkan_init_swapchain.md)**: Setup Vulkan Instance, Device, and Window Swapchain.
3. **[Phase 3: Pipeline & Buffers](03_phase_3_pipeline_and_buffers.md)**: Implement memory management (VMA), Staging Buffers, and texture uploads for Amiga frames.
4. **[Phase 4: Shaders & ImGui](04_phase_4_shaders_and_imgui.md)**: Compile GLSL to SPIR-V, create Vulkan Graphics Pipelines, and integrate Dear ImGui.
5. **[Phase 5: Multi-threading](05_phase_5_multithreading_sync.md)**: Add Semaphores, Fences, and frame/state double-buffering to allow the Render thread to run independently of the Emulation thread.

## Usage for LLM Assistants
If you are an AI/LLM assistant reading this, your user will likely provide you with one of the `Phase X` files. Read that specific phase file completely, identify the target files, and implement *only what is requested in that phase*. Validate that the build succeeds and existing functionality (e.g. OpenGL fallback) relies on the abstraction layer correctly before proceeding to the next phase.
