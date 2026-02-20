# Phase 5: Multi-threading and Synchronization

## Objective
Take full advantage of Vulkan by separating the emulation (CPU) thread from the rendering submission (GPU) thread, removing CPU render stalls.

## Target Files to Modify
- `src/osdep/vulkan_renderer.cpp`
- `src/osdep/amiberry_gfx.cpp`

## Context and Instructions

### 1. The Rendering Thread Problem
Currently, the emulation loop calls `SDL2_renderframe`. This function locks the emulation thread until `SDL_RenderCopy` processes the texture. If `Vsync` is forced, it blocks the entire VM execution (causing the black screen or audio stutter issues seen previously).

### 2. Double-Buffering the Render State
- Instead of the emulation thread mapping memory and submitting commands linearly, implement a ring buffer (size 2 or 3) indicating Frames In Flight.
- We need:
  - 2 x `amiga_surface` equivalent CPU-side backing buffers.
  - 2 x Staging Buffers.
  - 2 x Command Buffers.
- The emulation thread should write to `buffer[N]`, signal a condition variable, and immediately begin working on `buffer[N+1]` (which shouldn't block unless the queue is entirely full).

### 3. Vulkan Synchronization Primitives
- Create a dedicated Render Thread for the `VulkanRenderer` if one doesn't exist.
- Use `VkSemaphore` to synchronize operations on the GPU:
  - `imageAvailableSemaphore`: Sent to `vkAcquireNextImageKHR`. Let the Command Buffer wait on it at the `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT` stage.
  - `renderFinishedSemaphore`: Signaled by the Command Buffer completion. Give this to `vkQueuePresentKHR`.
- Use `VkFence` to synchronize GPU to CPU:
  - Wait on an `inFlightFence` at the start of your render loop *before* overwriting staging buffers.
  - Reset the fence `vkResetFences` and pass it to `vkQueueSubmit`.

### 4. Zero-Copy Picasso96 Optimization
If `currprefs.rtg_zerocopy` is enabled, Amiga VRAM changes can be streamed directly into mapped device memory (if the Vulkan memory allocator supports `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`). This avoids the `staging buffer -> device local` copy overhead entirely for RTG screens.

## Acceptance Criteria
- Stuttering previously caused by vertical sync or heavy emulation CPU usage should disappear.
- Enabling Vulkan validation layers continues to report zero synchronization/lifetime hazard errors.
- Amiberry remains stable during fast-forward mode and normal 50/60Hz modes.
