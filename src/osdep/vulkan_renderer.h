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
#include <array>
#include <vector>
#include <string>

struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;

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
	VmaAllocator m_allocator = nullptr;
	static constexpr VkFormat k_upload_texture_format = VK_FORMAT_R8G8B8A8_UNORM;
	static constexpr uint32_t k_max_frames_in_flight = 2;

	VkCommandPool m_graphics_command_pool = VK_NULL_HANDLE;
	std::array<VkCommandBuffer, k_max_frames_in_flight> m_graphics_command_buffers{};
	std::array<VkSemaphore, k_max_frames_in_flight> m_image_available_semaphores{};
	std::array<VkFence, k_max_frames_in_flight> m_in_flight_fences{};

	// --- Swapchain ---
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkFormat m_swapchain_format = VK_FORMAT_UNDEFINED;
	VkExtent2D m_swapchain_extent = {0, 0};
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	std::vector<VkSemaphore> m_swapchain_render_finished_semaphores;
	std::vector<VkFence> m_swapchain_images_in_flight;

	VkBuffer m_upload_staging_buffer = VK_NULL_HANDLE;
	VmaAllocation m_upload_staging_allocation = nullptr;
	void* m_upload_staging_mapped = nullptr;
	bool m_upload_staging_mapped_by_vma_map = false;
	VkDeviceSize m_upload_staging_size = 0;

	VkImage m_upload_texture_image = VK_NULL_HANDLE;
	VmaAllocation m_upload_texture_allocation = nullptr;
	VkImageView m_upload_texture_view = VK_NULL_HANDLE;
	VkExtent2D m_upload_texture_extent = {0, 0};
	VkSampler m_upload_texture_sampler = VK_NULL_HANDLE;

	VkDescriptorSetLayout m_texture_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorPool m_texture_descriptor_pool = VK_NULL_HANDLE;
	VkDescriptorSet m_texture_descriptor_set = VK_NULL_HANDLE;

	VkPipelineLayout m_graphics_pipeline_layout = VK_NULL_HANDLE;
	VkShaderModule m_vertex_shader_module = VK_NULL_HANDLE;
	VkShaderModule m_fragment_shader_module = VK_NULL_HANDLE;

	VkRenderPass m_render_pass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_swapchain_framebuffers;
	VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;

	int m_upload_texture_width = 0;
	int m_upload_texture_height = 0;
	VkFormat m_upload_texture_cached_format = VK_FORMAT_UNDEFINED;

	bool m_logged_upload_reuse = false;
	bool m_logged_upload_recreate = false;
	bool m_has_uploaded_frame = false;
	bool m_logged_first_upload = false;
	bool m_logged_first_submit = false;
	bool m_logged_first_present = false;
	bool m_swapchain_recreate_requested = false;
	bool m_logged_zero_extent_skip = false;
	uint32_t m_current_frame = 0;

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
	bool create_command_infrastructure();
	void cleanup_command_infrastructure();
	bool create_sync_objects();
	void cleanup_sync_objects();
	bool recreate_frame_sync_objects(uint32_t frame_index);
	bool create_swapchain_present_sync_objects();
	void cleanup_swapchain_present_sync_objects();
	bool create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
	bool create_swapchain_image_views();
	void cleanup_swapchain();
	void cleanup_swapchain_image_views();
	void request_swapchain_recreate(const char* reason);
	bool recreate_swapchain();
	bool create_persistent_draw_resources();
	void cleanup_persistent_draw_resources();
	bool create_swapchain_draw_resources();
	void cleanup_swapchain_draw_resources();
	bool create_texture_sampler();
	void cleanup_texture_sampler();
	bool create_texture_descriptor_resources();
	void cleanup_texture_descriptor_resources();
	bool update_texture_descriptor_set();
	bool create_pipeline_layout();
	void cleanup_pipeline_layout();
	bool create_shader_modules();
	void cleanup_shader_modules();
	bool create_render_pass();
	void cleanup_render_pass();
	bool create_swapchain_framebuffers();
	void cleanup_swapchain_framebuffers();
	bool create_graphics_pipeline();
	void cleanup_graphics_pipeline();
	void cleanup_upload_resources();
	void cleanup_upload_staging();
	void cleanup_upload_texture();
	VkCommandBuffer begin_one_time_graphics_commands();
	bool end_one_time_graphics_commands(VkCommandBuffer command_buffer);
	void record_image_layout_transition(VkCommandBuffer command_buffer,
		VkImage image,
		VkImageLayout old_layout,
		VkImageLayout new_layout,
		VkAccessFlags src_access_mask,
		VkAccessFlags dst_access_mask,
		VkPipelineStageFlags src_stage_mask,
		VkPipelineStageFlags dst_stage_mask);
	bool transition_image_to_shader_read(VkImage image);
	bool transition_upload_texture_to_shader_read();
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
