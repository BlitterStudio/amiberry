# Phase 3: Pipeline and Frame Buffering

## Objective
Move pixel data from the Amiga CPU thread memory (`amiga_surface`) to the GPU, and handle rendering commands.

## Target Files to Modify
- `src/osdep/vulkan_renderer.cpp`

## Context and Instructions

### 1. Memory Management
Vulkan requires explicit memory allocation (which includes dealing with host-visible/device-local memory limits).
- We highly recommend integrating **VulkanMemoryAllocator (VMA)** as a single-header library (`vk_mem_alloc.h`) to handle buffer and image memory allocations.
- Allocate a Host-Visible "Staging Buffer". The size should be large enough to hold the maximum resolution of `amiga_surface`.

### 2. Texture Uploads (Zero Copy / RTG Data)
Amiberry calculates the frame in RAM. We need to upload this frame to a Vulkan Image.
- In `VulkanRenderer::render_frame(...)`, map the Staging Buffer and `memcpy` the pixels from `amiga_surface->pixels`.
- Use a `VkCommandBuffer` to execute a `vkCmdCopyBufferToImage` transition the image layout (`VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`), copy the buffer, and transition it again for reading in shaders (`VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`).

### 3. Command Pools & Render Passes
- Set up a `VkCommandPool` and allocate `VkCommandBuffer`s (typically one per swapchain image).
- Create a `VkRenderPass` representing the layout transitions needed before presenting to the screen.
- Create `VkFramebuffer`s that link your Swapchain Image Views to the Render Pass.

## Acceptance Criteria
- A simple flat texture is written to the command buffer and presented to the screen.
- The `vkCmdCopyBufferToImage` operation correctly translates Amiga pixel format to the Vulkan Image format.
- Screen clears (e.g., pure black or a solid color) work, indicating that the swapchain synchronization loop (`vkAcquireNextImageKHR`, `vkQueueSubmit`, `vkQueuePresentKHR`) is functioning.
