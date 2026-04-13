#ifndef IMGUI_OVERLAY_H
#define IMGUI_OVERLAY_H

struct SDL_Window;
struct SDL_Renderer;
struct ImFont;
struct ImDrawData;

// Initialize the overlay ImGui context for rendering on top of the emulation display.
// Call after the display and renderer context are fully set up.
// gl_context may be nullptr if using SDL renderer backend.
void imgui_overlay_init(SDL_Window* window, SDL_Renderer* sdl_renderer, void* gl_context);

#ifdef USE_VULKAN
#include <vulkan/vulkan.h>
struct ImGui_ImplVulkan_InitInfo;
// Vulkan-specific init: call INSTEAD of the above when the Vulkan renderer is active.
void imgui_overlay_init_vulkan(SDL_Window* window, ImGui_ImplVulkan_InitInfo* vk_info);

// Rebuild the overlay's Vulkan state after swapchain recreation.
// Handles both render pass changes and image count changes (full re-init if needed).
void imgui_overlay_handle_vulkan_swapchain_change(ImGui_ImplVulkan_InitInfo* vk_info);
#endif

// Shut down the overlay context and free resources.
void imgui_overlay_shutdown();

// Returns true if the overlay context is initialized and ready.
bool imgui_overlay_is_initialized();

// Begin a new overlay ImGui frame. Sets the overlay context as current.
// Call before any ImGui drawing (e.g., on-screen keyboard).
void imgui_overlay_begin_frame();

// End the overlay ImGui frame.
// For OpenGL/SDL: calls ImGui::Render() + backend RenderDrawData, restores context.
// For Vulkan: calls ImGui::Render() only. The caller must retrieve draw data with
// imgui_overlay_get_draw_data() and render it inside the active render pass, then
// call imgui_overlay_restore_context().
void imgui_overlay_end_frame();

// For Vulkan: get the draw data after end_frame. Returns nullptr if not Vulkan.
ImDrawData* imgui_overlay_get_draw_data();

// For Vulkan: restore the previous ImGui context after rendering draw data.
void imgui_overlay_restore_context();

// Returns true if the overlay uses the Vulkan backend.
bool imgui_overlay_is_vulkan();

// Access the overlay fonts (returns nullptr if not initialized).
ImFont* imgui_overlay_get_font();
ImFont* imgui_overlay_get_font_small();

#endif // IMGUI_OVERLAY_H
