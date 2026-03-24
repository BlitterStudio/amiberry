#pragma once

/*
 * vulkan_renderer.h - Vulkan backend implementation of IRenderer
 *
 * Phase 2: Vulkan initialization, device selection, and swapchain creation.
 * No actual rendering yet — this provides the Vulkan context lifecycle.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifdef USE_VULKAN

#include "irenderer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanRenderer : public IRenderer {
public:
	VulkanRenderer() = default;
	~VulkanRenderer() override;

	// Context lifecycle
	bool init_context(SDL_Window* window) override;
	void destroy_context() override;
	bool has_context() const override;

	// Window creation support
	SDL_WindowFlags get_window_flags() const override;
	bool set_context_attributes(int mode) override;
	bool create_platform_renderer(AmigaMonitor* mon) override;
	void destroy_platform_renderer(AmigaMonitor* mon) override;

	// Texture / shader allocation (Phase 2: stubs)
	bool alloc_texture(int monid, int w, int h) override;
	void set_scaling(int monid, const uae_prefs* p, int w, int h) override;

	// VSync
	void update_vsync(int monid) override;

	// Frame rendering (Phase 2: stubs)
	bool render_frame(int monid, int mode, int immediate) override;
	void present_frame(int monid, int mode) override;

	// Input coordinate translation
	void get_gfx_offset(int monid, float src_w, float src_h, float src_x, float src_y,
		float* dx, float* dy, float* mx, float* my) override;

	// Drawable size query
	void get_drawable_size(SDL_Window* w, int* width, int* height) override;

	// Cleanup of window-associated resources
	void close_hwnds_cleanup(AmigaMonitor* mon) override;

private:
	// --- Vulkan core handles ---
	VkInstance m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphics_queue = VK_NULL_HANDLE;
	VkQueue m_present_queue = VK_NULL_HANDLE;
	uint32_t m_graphics_queue_family = UINT32_MAX;
	uint32_t m_present_queue_family = UINT32_MAX;

	// --- Swapchain ---
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkFormat m_swapchain_format = VK_FORMAT_UNDEFINED;
	VkExtent2D m_swapchain_extent = {0, 0};
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;

	// --- Debug ---
	VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;

	// --- Associated window ---
	SDL_Window* m_window = nullptr;
	bool m_context_valid = false;

	// --- Private helpers ---
	bool create_instance();
	bool create_surface();
	bool pick_physical_device();
	bool create_logical_device();
	bool create_swapchain();
	void cleanup_swapchain();
#ifndef NDEBUG
	bool setup_debug_messenger();
#endif

	// Device suitability checks
	bool is_device_suitable(VkPhysicalDevice device) const;
	bool check_device_extension_support(VkPhysicalDevice device) const;

	struct QueueFamilyIndices {
		uint32_t graphics = UINT32_MAX;
		uint32_t present = UINT32_MAX;
		bool is_complete() const { return graphics != UINT32_MAX && present != UINT32_MAX; }
	};
	QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};
	SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device) const;

	// Swapchain configuration selection
	static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
	static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes);
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const;
};

// Helper to get the Vulkan renderer from the global g_renderer.
// Returns nullptr if g_renderer is null or not a VulkanRenderer.
VulkanRenderer* get_vulkan_renderer();

#endif // USE_VULKAN
