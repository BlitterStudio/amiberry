# Phase 2: Vulkan Initialization & Swapchain

## Objective
Initialize the Vulkan API context, create a Device, and hook it up to the SDL Window via a Swapchain.

## Target Files to Modify
- `CMakeLists.txt`
- `Makefile`
- `src/osdep/vulkan_renderer.cpp` (New)
- `src/osdep/vulkan_renderer.h` (New)

## Context and Instructions

### 1. Build System Updates
Modify the build scripts to detect and link against Vulkan (`vulkan-headers` and `vulkan-loader` / `libvulkan.so` or `MoltenVK` on macOS). Add `-DUSE_VULKAN` flag to toggle the feature using `#ifdef USE_VULKAN`.

### 2. Vulkan Context Implementation
Implement the `VulkanRenderer` class that inherits from the `IRenderer` interface created in Phase 1.

The `initialize()` method should perform the following:
- Create a `VkInstance`. Ensure you request the appropriate extensions (e.g., `VK_KHR_surface`).
- Use `SDL_Vulkan_GetInstanceExtensions(window, ...)` to get required window extensions.
- Create an `SDL_Vulkan_CreateSurface` to bind the window.
- Pick a Physical Device (`VkPhysicalDevice`) prioritizing discrete GPUs but settling for integrated if needed. Check for `VK_KHR_swapchain` support.
- Create a Logical Device (`VkDevice`) and retrieve the Graphics and Present queues.

### 3. The Swapchain
Create the Swapchain based on the current window size:
- Query Surface Capabilities, Formats, and Presentation Modes.
- Choose `VK_PRESENT_MODE_MAILBOX_KHR` or `VK_PRESENT_MODE_FIFO_KHR` (for VSync).
- Create the `VkSwapchainKHR`.
- Retrieve the Swapchain Images (`VkImage`) and create `VkImageView` for each of them.

## Acceptance Criteria
- Building with `CMake` successfully links Vulkan libraries.
- The emulator launches successfully when the Vulkan backend is selected.
- No rendering happens yet, but Vulkan validation layers (if enabled) report 0 errors after creating the Swapchain and shutting down cleanly.
