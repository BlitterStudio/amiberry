/*
 * vulkan_renderer.cpp - Vulkan backend implementation of IRenderer
 *
 * Phase 2: Vulkan initialization, device selection, and swapchain creation.
 * No actual rendering happens yet — pure virtual stubs return immediately.
 * Vulkan validation layers are enabled in debug builds.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifdef USE_VULKAN

#include "sysdeps.h"
#include "vulkan_renderer.h"
#include "options.h"
#include "uae.h"
#include "xwin.h"
#include "amiberry_gfx.h"
#include "gfx_platform_internal.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <limits>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

// Required device extensions
static const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef NDEBUG
// Validation layers for debug builds
static const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};
static constexpr bool enable_validation = true;
#else
static const std::vector<const char*> validation_layers = {};
static constexpr bool enable_validation = false;
#endif

static const uint32_t __amiberry_fullscreen_vert_spv[] = {
	0x07230203,0x00010000,0x0008000b,0x00000033,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x0000001a,0x00000029,
	0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00060005,
	0x0000000b,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000000b,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000000b,0x00000001,0x505f6c67,0x746e696f,
	0x657a6953,0x00000000,0x00070006,0x0000000b,0x00000002,0x435f6c67,0x4470696c,0x61747369,
	0x0065636e,0x00070006,0x0000000b,0x00000003,0x435f6c67,0x446c6c75,0x61747369,0x0065636e,
	0x00030005,0x0000000d,0x00000000,0x00060005,0x0000001a,0x565f6c67,0x65747265,0x646e4978,
	0x00007865,0x00050005,0x0000001d,0x65646e69,0x6c626178,0x00000065,0x00040005,0x00000029,
	0x5574756f,0x00000056,0x00050005,0x00000030,0x65646e69,0x6c626178,0x00000065,0x00030047,
	0x0000000b,0x00000002,0x00050048,0x0000000b,0x00000000,0x0000000b,0x00000000,0x00050048,
	0x0000000b,0x00000001,0x0000000b,0x00000001,0x00050048,0x0000000b,0x00000002,0x0000000b,
	0x00000003,0x00050048,0x0000000b,0x00000003,0x0000000b,0x00000004,0x00040047,0x0000001a,
	0x0000000b,0x0000002a,0x00040047,0x00000029,0x0000001e,0x00000000,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
	0x00000006,0x00000004,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,0x00000008,
	0x00000009,0x00000001,0x0004001c,0x0000000a,0x00000006,0x00000009,0x0006001e,0x0000000b,
	0x00000007,0x00000006,0x0000000a,0x0000000a,0x00040020,0x0000000c,0x00000003,0x0000000b,
	0x0004003b,0x0000000c,0x0000000d,0x00000003,0x00040015,0x0000000e,0x00000020,0x00000001,
	0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040017,0x00000010,0x00000006,0x00000002,
	0x0004002b,0x00000008,0x00000011,0x00000003,0x0004001c,0x00000012,0x00000010,0x00000011,
	0x0004002b,0x00000006,0x00000013,0xbf800000,0x0005002c,0x00000010,0x00000014,0x00000013,
	0x00000013,0x0004002b,0x00000006,0x00000015,0x40400000,0x0005002c,0x00000010,0x00000016,
	0x00000015,0x00000013,0x0005002c,0x00000010,0x00000017,0x00000013,0x00000015,0x0006002c,
	0x00000012,0x00000018,0x00000014,0x00000016,0x00000017,0x00040020,0x00000019,0x00000001,
	0x0000000e,0x0004003b,0x00000019,0x0000001a,0x00000001,0x00040020,0x0000001c,0x00000007,
	0x00000012,0x00040020,0x0000001e,0x00000007,0x00000010,0x0004002b,0x00000006,0x00000021,
	0x00000000,0x0004002b,0x00000006,0x00000022,0x3f800000,0x00040020,0x00000026,0x00000003,
	0x00000007,0x00040020,0x00000028,0x00000003,0x00000010,0x0004003b,0x00000028,0x00000029,
	0x00000003,0x0005002c,0x00000010,0x0000002a,0x00000021,0x00000021,0x0004002b,0x00000006,
	0x0000002b,0x40000000,0x0005002c,0x00000010,0x0000002c,0x0000002b,0x00000021,0x0005002c,
	0x00000010,0x0000002d,0x00000021,0x0000002b,0x0006002c,0x00000012,0x0000002e,0x0000002a,
	0x0000002c,0x0000002d,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003b,0x0000001c,0x0000001d,0x00000007,0x0004003b,0x0000001c,0x00000030,
	0x00000007,0x0004003d,0x0000000e,0x0000001b,0x0000001a,0x0003003e,0x0000001d,0x00000018,
	0x00050041,0x0000001e,0x0000001f,0x0000001d,0x0000001b,0x0004003d,0x00000010,0x00000020,
	0x0000001f,0x00050051,0x00000006,0x00000023,0x00000020,0x00000000,0x00050051,0x00000006,
	0x00000024,0x00000020,0x00000001,0x00070050,0x00000007,0x00000025,0x00000023,0x00000024,
	0x00000021,0x00000022,0x00050041,0x00000026,0x00000027,0x0000000d,0x0000000f,0x0003003e,
	0x00000027,0x00000025,0x0004003d,0x0000000e,0x0000002f,0x0000001a,0x0003003e,0x00000030,
	0x0000002e,0x00050041,0x0000001e,0x00000031,0x00000030,0x0000002f,0x0004003d,0x00000010,
	0x00000032,0x00000031,0x0003003e,0x00000029,0x00000032,0x000100fd,0x00010038
};

static const uint32_t __amiberry_fullscreen_frag_spv[] = {
	0x07230203,0x00010000,0x0008000b,0x00000029,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000018,0x00000021,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00030005,0x00000009,0x00007675,0x00060005,0x0000000b,0x68737550,0x736e6f43,
	0x746e6174,0x00000073,0x00050006,0x0000000b,0x00000000,0x706f7263,0x00005655,0x00030005,
	0x0000000d,0x00006370,0x00040005,0x00000018,0x56556e69,0x00000000,0x00050005,0x00000021,
	0x4374756f,0x726f6c6f,0x00000000,0x00050005,0x00000025,0x78655475,0x65727574,0x00000000,
	0x00030047,0x0000000b,0x00000002,0x00050048,0x0000000b,0x00000000,0x00000023,0x00000000,
	0x00040047,0x00000018,0x0000001e,0x00000000,0x00040047,0x00000021,0x0000001e,0x00000000,
	0x00040047,0x00000025,0x00000021,0x00000000,0x00040047,0x00000025,0x00000022,0x00000000,
	0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,
	0x00040017,0x00000007,0x00000006,0x00000002,0x00040020,0x00000008,0x00000007,0x00000007,
	0x00040017,0x0000000a,0x00000006,0x00000004,0x0003001e,0x0000000b,0x0000000a,0x00040020,
	0x0000000c,0x00000009,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000009,0x00040015,
	0x0000000e,0x00000020,0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,
	0x00000010,0x00000009,0x0000000a,0x00040020,0x00000017,0x00000001,0x00000007,0x0004003b,
	0x00000017,0x00000018,0x00000001,0x0004002b,0x00000006,0x0000001a,0x00000000,0x0005002c,
	0x00000007,0x0000001b,0x0000001a,0x0000001a,0x0004002b,0x00000006,0x0000001c,0x3f800000,
	0x0005002c,0x00000007,0x0000001d,0x0000001c,0x0000001c,0x00040020,0x00000020,0x00000003,
	0x0000000a,0x0004003b,0x00000020,0x00000021,0x00000003,0x00090019,0x00000022,0x00000006,
	0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x00000023,
	0x00000022,0x00040020,0x00000024,0x00000000,0x00000023,0x0004003b,0x00000024,0x00000025,
	0x00000000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000008,0x00000009,0x00000007,0x00050041,0x00000010,0x00000011,0x0000000d,
	0x0000000f,0x0004003d,0x0000000a,0x00000012,0x00000011,0x0007004f,0x00000007,0x00000013,
	0x00000012,0x00000012,0x00000000,0x00000001,0x00050041,0x00000010,0x00000014,0x0000000d,
	0x0000000f,0x0004003d,0x0000000a,0x00000015,0x00000014,0x0007004f,0x00000007,0x00000016,
	0x00000015,0x00000015,0x00000002,0x00000003,0x0004003d,0x00000007,0x00000019,0x00000018,
	0x0008000c,0x00000007,0x0000001e,0x00000001,0x0000002b,0x00000019,0x0000001b,0x0000001d,
	0x0008000c,0x00000007,0x0000001f,0x00000001,0x0000002e,0x00000013,0x00000016,0x0000001e,
	0x0003003e,0x00000009,0x0000001f,0x0004003d,0x00000023,0x00000026,0x00000025,0x0004003d,
	0x00000007,0x00000027,0x00000009,0x00050057,0x0000000a,0x00000028,0x00000026,0x00000027,
	0x0003003e,0x00000021,0x00000028,0x000100fd,0x00010038
};

struct VulkanCropPushConstants {
	float u_min;
	float v_min;
	float u_max;
	float v_max;
};

static_assert(sizeof(VulkanCropPushConstants) == sizeof(float) * 4,
	"VulkanCropPushConstants must be 16 bytes");

// --- Debug messenger callback ---
#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT /*type*/,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* /*user_data*/)
{
	const char* severity_str = "UNKNOWN";
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		severity_str = "ERROR";
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		severity_str = "WARNING";
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		severity_str = "INFO";
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		severity_str = "VERBOSE";

	write_log("Vulkan [%s]: %s\n", severity_str, callback_data->pMessage);
	return VK_FALSE;
}

static VkResult create_debug_utils_messenger(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* create_info,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* messenger)
{
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (func)
		return func(instance, create_info, allocator, messenger);
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroy_debug_utils_messenger(
	VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks* allocator)
{
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func)
		func(instance, messenger, allocator);
}
#endif // NDEBUG

// ============================================================================
// Destructor
// ============================================================================

VulkanRenderer::~VulkanRenderer()
{
	destroy_context();
}

// ============================================================================
// Context lifecycle
// ============================================================================

bool VulkanRenderer::init_context(SDL_Window* window)
{
	if (m_context_valid) {
		write_log("VulkanRenderer: context already initialized\n");
		return true;
	}

	m_window = window;

	if (!create_instance()) {
		write_log("VulkanRenderer: failed to create Vulkan instance\n");
		destroy_context();
		return false;
	}

#ifndef NDEBUG
	setup_debug_messenger();
#endif

	if (!create_surface()) {
		write_log("VulkanRenderer: failed to create window surface\n");
		destroy_context();
		return false;
	}

	if (!pick_physical_device()) {
		write_log("VulkanRenderer: failed to find a suitable GPU\n");
		destroy_context();
		return false;
	}

	if (!create_logical_device()) {
		write_log("VulkanRenderer: failed to create logical device\n");
		destroy_context();
		return false;
	}

	if (!create_command_infrastructure()) {
		write_log("VulkanRenderer: failed to create command infrastructure\n");
		destroy_context();
		return false;
	}

	if (!create_sync_objects()) {
		write_log("VulkanRenderer: failed to create sync objects\n");
		destroy_context();
		return false;
	}

	if (!create_swapchain()) {
		write_log("VulkanRenderer: failed to create swapchain\n");
		destroy_context();
		return false;
	}

	if (!create_persistent_draw_resources()) {
		write_log("VulkanRenderer: failed to create persistent draw resources\n");
		destroy_context();
		return false;
	}

	if (!create_swapchain_draw_resources()) {
		write_log("VulkanRenderer: failed to create swapchain draw resources\n");
		destroy_context();
		return false;
	}

	m_context_valid = true;
	write_log("VulkanRenderer: initialization complete (swapchain %ux%u, %zu images)\n",
		m_swapchain_extent.width, m_swapchain_extent.height,
		m_swapchain_images.size());

	return true;
}

void VulkanRenderer::destroy_context()
{
	if (m_device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(m_device);
	}

	m_swapchain_recreate_requested = false;
	m_logged_zero_extent_skip = false;

	cleanup_sync_objects();
	cleanup_command_infrastructure();
	cleanup_upload_resources();
	cleanup_swapchain();
	cleanup_persistent_draw_resources();

	if (m_allocator != nullptr) {
		vmaDestroyAllocator(m_allocator);
		m_allocator = nullptr;
	}

	if (m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_graphics_queue = VK_NULL_HANDLE;
	m_present_queue = VK_NULL_HANDLE;
	m_physical_device = VK_NULL_HANDLE;
	m_graphics_queue_family = UINT32_MAX;
	m_present_queue_family = UINT32_MAX;

#ifndef NDEBUG
	if (m_debug_messenger != VK_NULL_HANDLE) {
		destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr);
		m_debug_messenger = VK_NULL_HANDLE;
	}
#endif

	if (m_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}

	if (m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}

	m_window = nullptr;
	m_context_valid = false;
}

bool VulkanRenderer::has_context() const
{
	return m_context_valid;
}

// ============================================================================
// Window creation support
// ============================================================================

SDL_WindowFlags VulkanRenderer::get_window_flags() const
{
	return SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
}

bool VulkanRenderer::set_context_attributes(int /*mode*/)
{
	// Vulkan doesn't need GL-style context attributes
	return true;
}

bool VulkanRenderer::create_platform_renderer(AmigaMonitor* /*mon*/)
{
	// Vulkan doesn't use SDL_Renderer — no-op
	return true;
}

void VulkanRenderer::destroy_platform_renderer(AmigaMonitor* /*mon*/)
{
	// No-op for Vulkan
}

// ============================================================================
// Texture / shader allocation (Phase 2: stubs)
// ============================================================================

bool VulkanRenderer::alloc_texture(int monid, int w, int h)
{
	if (currprefs.headless)
		return true;
	if (w == 0 || h == 0)
		return false;
	if (gfx_platform_skip_alloctexture(monid, w, h))
		return true;

	if (w < 0 || h < 0) {
		const int requested_w = -w;
		const int requested_h = -h;
		const bool can_reuse =
			m_upload_staging_buffer != VK_NULL_HANDLE &&
			m_upload_staging_allocation != nullptr &&
			m_upload_staging_mapped != nullptr &&
			m_upload_texture_image != VK_NULL_HANDLE &&
			m_upload_texture_allocation != nullptr &&
			m_upload_texture_view != VK_NULL_HANDLE &&
			m_upload_texture_width == requested_w &&
			m_upload_texture_height == requested_h &&
			m_upload_texture_cached_format == k_upload_texture_format;

		if (can_reuse) {
			if (!m_logged_upload_reuse) {
				write_log("VulkanRenderer: alloc_texture reuse fast-path hit (%dx%d, format=%d)\n",
					requested_w, requested_h, m_upload_texture_cached_format);
				m_logged_upload_reuse = true;
			}
			set_scaling(monid, &currprefs, requested_w, requested_h);
			return true;
		}

		if (!m_logged_upload_recreate) {
			write_log("VulkanRenderer: alloc_texture reuse miss, recreate required (requested=%dx%d cached=%dx%d format=%d)\n",
				requested_w, requested_h, m_upload_texture_width, m_upload_texture_height,
				m_upload_texture_cached_format);
			m_logged_upload_recreate = true;
		}
		return false;
	}

	if (m_device == VK_NULL_HANDLE || m_allocator == nullptr || m_graphics_queue == VK_NULL_HANDLE ||
		m_graphics_queue_family == UINT32_MAX) {
		write_log("VulkanRenderer: alloc_texture called before Vulkan device/allocator are ready\n");
		return false;
	}

	const VkDeviceSize required_size = static_cast<VkDeviceSize>(w) * static_cast<VkDeviceSize>(h) * 4;
	const bool had_resources =
		m_upload_staging_buffer != VK_NULL_HANDLE ||
		m_upload_texture_image != VK_NULL_HANDLE ||
		m_upload_texture_view != VK_NULL_HANDLE;

	if (had_resources && !m_logged_upload_recreate) {
		write_log("VulkanRenderer: alloc_texture recreating upload resources (%dx%d)\n", w, h);
		m_logged_upload_recreate = true;
	}

	for (uint32_t i = 0; i < k_max_frames_in_flight; ++i) {
		if (m_in_flight_fences[i] == VK_NULL_HANDLE)
			continue;
		VkResult wait_result = vkWaitForFences(m_device, 1, &m_in_flight_fences[i], VK_TRUE, UINT64_MAX);
		if (wait_result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkWaitForFences before upload resource recreate failed (frame=%u, VkResult=%d)\n",
				i,
				wait_result);
			return false;
		}
	}

	VkBuffer new_staging_buffer = VK_NULL_HANDLE;
	VmaAllocation new_staging_allocation = nullptr;
	void* new_staging_mapped = nullptr;
	bool new_staging_mapped_by_vma_map = false;
	VkImage new_texture_image = VK_NULL_HANDLE;
	VmaAllocation new_texture_allocation = nullptr;
	VkImageView new_texture_view = VK_NULL_HANDLE;

	auto cleanup_new_upload_resources = [&]() {
		if (new_texture_view != VK_NULL_HANDLE) {
			vkDestroyImageView(m_device, new_texture_view, nullptr);
			new_texture_view = VK_NULL_HANDLE;
		}
		if (new_texture_image != VK_NULL_HANDLE && new_texture_allocation != nullptr) {
			vmaDestroyImage(m_allocator, new_texture_image, new_texture_allocation);
			new_texture_image = VK_NULL_HANDLE;
			new_texture_allocation = nullptr;
		}
		if (new_staging_allocation != nullptr && new_staging_mapped != nullptr && new_staging_mapped_by_vma_map) {
			vmaUnmapMemory(m_allocator, new_staging_allocation);
		}
		new_staging_mapped = nullptr;
		new_staging_mapped_by_vma_map = false;
		if (new_staging_buffer != VK_NULL_HANDLE && new_staging_allocation != nullptr) {
			vmaDestroyBuffer(m_allocator, new_staging_buffer, new_staging_allocation);
			new_staging_buffer = VK_NULL_HANDLE;
			new_staging_allocation = nullptr;
		}
	};

	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = required_size;
	buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo staging_alloc_info{};
	staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocationInfo staging_info{};
	VkResult result = vmaCreateBuffer(m_allocator, &buffer_info, &staging_alloc_info,
		&new_staging_buffer, &new_staging_allocation, &staging_info);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vmaCreateBuffer for upload staging failed (VkResult=%d)\n", result);
		cleanup_new_upload_resources();
		return false;
	}

	new_staging_mapped = staging_info.pMappedData;
	if (new_staging_mapped == nullptr) {
		result = vmaMapMemory(m_allocator, new_staging_allocation, &new_staging_mapped);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vmaMapMemory for upload staging failed (VkResult=%d)\n", result);
			cleanup_new_upload_resources();
			return false;
		}
		new_staging_mapped_by_vma_map = true;
	}

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = static_cast<uint32_t>(w);
	image_info.extent.height = static_cast<uint32_t>(h);
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = k_upload_texture_format;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo image_alloc_info{};
	image_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	result = vmaCreateImage(m_allocator, &image_info, &image_alloc_info,
		&new_texture_image, &new_texture_allocation, nullptr);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vmaCreateImage for upload texture failed (VkResult=%d)\n", result);
		cleanup_new_upload_resources();
		return false;
	}

	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = new_texture_image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = k_upload_texture_format;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	result = vkCreateImageView(m_device, &view_info, nullptr, &new_texture_view);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateImageView for upload texture failed (VkResult=%d)\n", result);
		cleanup_new_upload_resources();
		return false;
	}

	if (!transition_image_to_shader_read(new_texture_image)) {
		write_log("VulkanRenderer: failed to transition upload texture to shader-read layout\n");
		cleanup_new_upload_resources();
		return false;
	}

	const VkBuffer old_staging_buffer = m_upload_staging_buffer;
	const VmaAllocation old_staging_allocation = m_upload_staging_allocation;
	void* old_staging_mapped = m_upload_staging_mapped;
	const bool old_staging_mapped_by_vma_map = m_upload_staging_mapped_by_vma_map;
	const VkDeviceSize old_staging_size = m_upload_staging_size;
	const VkImage old_texture_image = m_upload_texture_image;
	const VmaAllocation old_texture_allocation = m_upload_texture_allocation;
	const VkImageView old_texture_view = m_upload_texture_view;
	const int old_texture_width = m_upload_texture_width;
	const int old_texture_height = m_upload_texture_height;
	const VkFormat old_texture_cached_format = m_upload_texture_cached_format;
	const VkExtent2D old_texture_extent = m_upload_texture_extent;
	const bool old_has_uploaded_frame = m_has_uploaded_frame;

	m_upload_staging_buffer = new_staging_buffer;
	m_upload_staging_allocation = new_staging_allocation;
	m_upload_staging_mapped = new_staging_mapped;
	m_upload_staging_mapped_by_vma_map = new_staging_mapped_by_vma_map;
	m_upload_staging_size = required_size;
	m_upload_texture_image = new_texture_image;
	m_upload_texture_allocation = new_texture_allocation;
	m_upload_texture_view = new_texture_view;
	m_upload_texture_width = w;
	m_upload_texture_height = h;
	m_upload_texture_cached_format = k_upload_texture_format;
	m_upload_texture_extent = {
		static_cast<uint32_t>(w),
		static_cast<uint32_t>(h)
	};

	if (!update_texture_descriptor_set()) {
		write_log("VulkanRenderer: failed to update descriptor set for upload texture\n");

		m_upload_staging_buffer = old_staging_buffer;
		m_upload_staging_allocation = old_staging_allocation;
		m_upload_staging_mapped = old_staging_mapped;
		m_upload_staging_mapped_by_vma_map = old_staging_mapped_by_vma_map;
		m_upload_staging_size = old_staging_size;
		m_upload_texture_image = old_texture_image;
		m_upload_texture_allocation = old_texture_allocation;
		m_upload_texture_view = old_texture_view;
		m_upload_texture_width = old_texture_width;
		m_upload_texture_height = old_texture_height;
		m_upload_texture_cached_format = old_texture_cached_format;
		m_upload_texture_extent = old_texture_extent;
		m_has_uploaded_frame = old_has_uploaded_frame;

		cleanup_new_upload_resources();
		return false;
	}

	if (old_staging_allocation != nullptr && old_staging_mapped != nullptr && old_staging_mapped_by_vma_map) {
		vmaUnmapMemory(m_allocator, old_staging_allocation);
	}
	if (old_staging_buffer != VK_NULL_HANDLE && old_staging_allocation != nullptr) {
		vmaDestroyBuffer(m_allocator, old_staging_buffer, old_staging_allocation);
	}
	if (old_texture_view != VK_NULL_HANDLE) {
		vkDestroyImageView(m_device, old_texture_view, nullptr);
	}
	if (old_texture_image != VK_NULL_HANDLE && old_texture_allocation != nullptr) {
		vmaDestroyImage(m_allocator, old_texture_image, old_texture_allocation);
	}

	write_log("VulkanRenderer: staging buffer created (%zu bytes for %dx%d)\n",
		static_cast<size_t>(m_upload_staging_size), w, h);
	write_log("VulkanRenderer: texture image created (%dx%d, format=%d)\n", w, h, k_upload_texture_format);

	m_has_uploaded_frame = false;

	return true;
}

void VulkanRenderer::set_scaling(int /*monid*/, const uae_prefs* /*p*/, int /*w*/, int /*h*/)
{
	// Phase 2: No scaling yet
}

// ============================================================================
// VSync
// ============================================================================

void VulkanRenderer::update_vsync(int /*monid*/)
{
	// Phase 2: VSync is determined by swapchain present mode
}

// ============================================================================
// ============================================================================

bool VulkanRenderer::render_frame(int monid, int /*mode*/, int /*immediate*/)
{
	if (currprefs.headless)
		return true;

	SDL_Surface* surface = get_amiga_surface(monid);
	if (surface == nullptr || surface->pixels == nullptr || surface->w <= 0 || surface->h <= 0) {
		m_has_uploaded_frame = false;
		return false;
	}

	const auto* format_details = SDL_GetPixelFormatDetails(surface->format);
	const int source_bpp = format_details ? format_details->bytes_per_pixel : 0;
	if (source_bpp != 4) {
		write_log("VulkanRenderer: unsupported surface format for upload (format=%u, bpp=%d)\n",
			static_cast<unsigned int>(surface->format), source_bpp);
		m_has_uploaded_frame = false;
		return false;
	}

	const bool upload_resources_ready =
		m_upload_staging_buffer != VK_NULL_HANDLE &&
		m_upload_staging_allocation != nullptr &&
		m_upload_staging_mapped != nullptr &&
		m_upload_texture_image != VK_NULL_HANDLE &&
		m_upload_texture_allocation != nullptr &&
		m_upload_texture_view != VK_NULL_HANDLE &&
		m_upload_texture_width == surface->w &&
		m_upload_texture_height == surface->h &&
		m_upload_texture_cached_format == k_upload_texture_format;

	if (!upload_resources_ready) {
		if (!alloc_texture(monid, surface->w, surface->h)) {
			m_has_uploaded_frame = false;
			return false;
		}
	}

	if (m_upload_staging_mapped == nullptr ||
		m_upload_staging_allocation == nullptr ||
		m_upload_staging_buffer == VK_NULL_HANDLE ||
		m_upload_texture_image == VK_NULL_HANDLE ||
		m_upload_texture_view == VK_NULL_HANDLE) {
		m_has_uploaded_frame = false;
		return false;
	}

	if (surface->pitch <= 0) {
		write_log("VulkanRenderer: invalid source pitch for upload (%d)\n", surface->pitch);
		m_has_uploaded_frame = false;
		return false;
	}

	const size_t src_pitch = static_cast<size_t>(surface->pitch);
	const size_t dst_pitch = static_cast<size_t>(surface->w) * 4;
	const size_t upload_size = dst_pitch * static_cast<size_t>(surface->h);

	if (src_pitch < dst_pitch || upload_size > static_cast<size_t>(m_upload_staging_size)) {
		write_log("VulkanRenderer: upload buffer size mismatch (src_pitch=%zu dst_pitch=%zu upload_size=%zu staging_size=%zu)\n",
			src_pitch, dst_pitch, upload_size, static_cast<size_t>(m_upload_staging_size));
		m_has_uploaded_frame = false;
		return false;
	}

	const auto* src_bytes = static_cast<const uae_u8*>(surface->pixels);
	auto* dst_bytes = static_cast<uae_u8*>(m_upload_staging_mapped);
	for (int y = 0; y < surface->h; ++y) {
		std::memcpy(
			dst_bytes + static_cast<size_t>(y) * dst_pitch,
			src_bytes + static_cast<size_t>(y) * src_pitch,
			dst_pitch);
	}

	const VkResult flush_result = vmaFlushAllocation(m_allocator, m_upload_staging_allocation, 0, upload_size);
	if (flush_result != VK_SUCCESS) {
		write_log("VulkanRenderer: vmaFlushAllocation failed for frame upload (VkResult=%d)\n", flush_result);
		m_has_uploaded_frame = false;
		return false;
	}

	m_has_uploaded_frame = true;
	if (!m_logged_first_upload) {
		write_log("VulkanRenderer: first upload complete\n");
		m_logged_first_upload = true;
	}

	return true;
}

void VulkanRenderer::present_frame(int /*monid*/, int /*mode*/)
{
	if (currprefs.headless || !m_context_valid)
		return;

	if (m_swapchain_recreate_requested) {
		recreate_swapchain();
		if (m_swapchain_recreate_requested)
			return;
	}

	if (!m_has_uploaded_frame)
		return;

	if (m_device == VK_NULL_HANDLE ||
		m_swapchain == VK_NULL_HANDLE ||
		m_graphics_queue == VK_NULL_HANDLE ||
		m_present_queue == VK_NULL_HANDLE ||
		m_graphics_pipeline == VK_NULL_HANDLE ||
		m_graphics_pipeline_layout == VK_NULL_HANDLE ||
		m_render_pass == VK_NULL_HANDLE ||
		m_texture_descriptor_set == VK_NULL_HANDLE ||
		m_upload_staging_buffer == VK_NULL_HANDLE ||
		m_upload_texture_image == VK_NULL_HANDLE ||
		m_swapchain_framebuffers.empty() ||
		m_swapchain_render_finished_semaphores.empty() ||
		m_swapchain_images_in_flight.size() != m_swapchain_images.size()) {
		return;
	}

	if (m_current_frame >= k_max_frames_in_flight)
		m_current_frame = 0;

	const uint32_t frame_index = m_current_frame;
	VkFence frame_fence = m_in_flight_fences[frame_index];
	VkCommandBuffer command_buffer = m_graphics_command_buffers[frame_index];
	if (frame_fence == VK_NULL_HANDLE || command_buffer == VK_NULL_HANDLE) {
		write_log("VulkanRenderer: present_frame missing fence or command buffer for frame %u\n", frame_index);
		return;
	}

	VkResult result = vkWaitForFences(m_device, 1, &frame_fence, VK_TRUE, UINT64_MAX);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkWaitForFences failed (VkResult=%d)\n", result);
		return;
	}

	uint32_t swapchain_image_index = 0;
	result = vkAcquireNextImageKHR(
		m_device,
		m_swapchain,
		UINT64_MAX,
		m_image_available_semaphores[frame_index],
		VK_NULL_HANDLE,
		&swapchain_image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		request_swapchain_recreate(result == VK_ERROR_OUT_OF_DATE_KHR ?
			"vkAcquireNextImageKHR out of date" :
			"vkAcquireNextImageKHR suboptimal");
		recreate_swapchain();
		return;
	}
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkAcquireNextImageKHR failed (VkResult=%d)\n", result);
		return;
	}

	if (swapchain_image_index >= m_swapchain_framebuffers.size()) {
		write_log("VulkanRenderer: acquired swapchain image index out of range (%u >= %zu)\n",
			swapchain_image_index, m_swapchain_framebuffers.size());
		return;
	}

	if (swapchain_image_index >= m_swapchain_render_finished_semaphores.size() ||
		swapchain_image_index >= m_swapchain_images_in_flight.size()) {
		write_log("VulkanRenderer: acquired swapchain sync index out of range (%u, semaphores=%zu, images_in_flight=%zu)\n",
			swapchain_image_index,
			m_swapchain_render_finished_semaphores.size(),
			m_swapchain_images_in_flight.size());
		return;
	}

	VkFence image_in_flight_fence = m_swapchain_images_in_flight[swapchain_image_index];
	if (image_in_flight_fence != VK_NULL_HANDLE && image_in_flight_fence != frame_fence) {
		result = vkWaitForFences(m_device, 1, &image_in_flight_fence, VK_TRUE, UINT64_MAX);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkWaitForFences(image_in_flight) failed (VkResult=%d, image=%u)\n",
				result,
				swapchain_image_index);
			return;
		}
	}

	result = vkResetCommandBuffer(command_buffer, 0);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkResetCommandBuffer failed (VkResult=%d)\n", result);
		return;
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkBeginCommandBuffer failed (VkResult=%d)\n", result);
		return;
	}

	record_image_layout_transition(
		command_buffer,
		m_upload_texture_image,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkBufferImageCopy copy_region{};
	copy_region.bufferOffset = 0;
	copy_region.bufferRowLength = 0;
	copy_region.bufferImageHeight = 0;
	copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy_region.imageSubresource.mipLevel = 0;
	copy_region.imageSubresource.baseArrayLayer = 0;
	copy_region.imageSubresource.layerCount = 1;
	copy_region.imageOffset = {0, 0, 0};
	copy_region.imageExtent = {
		m_upload_texture_extent.width,
		m_upload_texture_extent.height,
		1
	};

	vkCmdCopyBufferToImage(
		command_buffer,
		m_upload_staging_buffer,
		m_upload_texture_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copy_region);

	record_image_layout_transition(
		command_buffer,
		m_upload_texture_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	VkClearValue clear_value{};
	clear_value.color.float32[0] = 0.0f;
	clear_value.color.float32[1] = 0.0f;
	clear_value.color.float32[2] = 0.0f;
	clear_value.color.float32[3] = 1.0f;

	VkRenderPassBeginInfo render_pass_begin{};
	render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin.renderPass = m_render_pass;
	render_pass_begin.framebuffer = m_swapchain_framebuffers[swapchain_image_index];
	render_pass_begin.renderArea.offset = {0, 0};
	render_pass_begin.renderArea.extent = m_swapchain_extent;
	render_pass_begin.clearValueCount = 1;
	render_pass_begin.pClearValues = &clear_value;

	vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

	VkRect2D draw_scissor{};
	bool has_valid_draw_rect = false;
	if (render_quad.w > 0 && render_quad.h > 0) {
		const int32_t max_x = static_cast<int32_t>(m_swapchain_extent.width);
		const int32_t max_y = static_cast<int32_t>(m_swapchain_extent.height);
		draw_scissor.offset.x = std::clamp(render_quad.x, 0, max_x);
		draw_scissor.offset.y = std::clamp(render_quad.y, 0, max_y);

		const int32_t available_w = std::max(0, max_x - draw_scissor.offset.x);
		const int32_t available_h = std::max(0, max_y - draw_scissor.offset.y);
		const int32_t clamped_w = std::clamp(render_quad.w, 0, available_w);
		const int32_t clamped_h = std::clamp(render_quad.h, 0, available_h);

		if (clamped_w > 0 && clamped_h > 0) {
			draw_scissor.extent.width = static_cast<uint32_t>(clamped_w);
			draw_scissor.extent.height = static_cast<uint32_t>(clamped_h);
			has_valid_draw_rect = true;
		}
	}

	if (!has_valid_draw_rect) {
		draw_scissor.offset = {0, 0};
		draw_scissor.extent = m_swapchain_extent;
	}

	VkViewport viewport{};
	viewport.x = static_cast<float>(draw_scissor.offset.x);
	viewport.y = static_cast<float>(draw_scissor.offset.y);
	viewport.width = static_cast<float>(draw_scissor.extent.width);
	viewport.height = static_cast<float>(draw_scissor.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &draw_scissor);

	vkCmdBindDescriptorSets(
		command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphics_pipeline_layout,
		0,
		1,
		&m_texture_descriptor_set,
		0,
		nullptr);

	VulkanCropPushConstants crop_push_constants{};
	crop_push_constants.u_min = 0.0f;
	crop_push_constants.v_min = 0.0f;
	crop_push_constants.u_max = 1.0f;
	crop_push_constants.v_max = 1.0f;

	if (m_upload_texture_width > 0 && m_upload_texture_height > 0) {
		bool is_cropped =
			(crop_rect.x != 0 || crop_rect.y != 0 ||
			crop_rect.w != m_upload_texture_width || crop_rect.h != m_upload_texture_height) &&
			(crop_rect.w > 0 && crop_rect.h > 0);

		int crop_x = 0;
		int crop_y = 0;
		int crop_w = m_upload_texture_width;
		int crop_h = m_upload_texture_height;

		if (is_cropped) {
			crop_x = std::max(0, crop_rect.x);
			crop_y = std::max(0, crop_rect.y);
			crop_w = std::min(crop_rect.w, m_upload_texture_width - crop_x);
			crop_h = std::min(crop_rect.h, m_upload_texture_height - crop_y);
			if (crop_w <= 0 || crop_h <= 0) {
				crop_x = 0;
				crop_y = 0;
				crop_w = m_upload_texture_width;
				crop_h = m_upload_texture_height;
			}
		}

		const float inv_w = 1.0f / static_cast<float>(m_upload_texture_width);
		const float inv_h = 1.0f / static_cast<float>(m_upload_texture_height);
		crop_push_constants.u_min = std::clamp(static_cast<float>(crop_x) * inv_w, 0.0f, 1.0f);
		crop_push_constants.v_min = std::clamp(static_cast<float>(crop_y) * inv_h, 0.0f, 1.0f);
		crop_push_constants.u_max = std::clamp(static_cast<float>(crop_x + crop_w) * inv_w, 0.0f, 1.0f);
		crop_push_constants.v_max = std::clamp(static_cast<float>(crop_y + crop_h) * inv_h, 0.0f, 1.0f);
	}

	vkCmdPushConstants(
		command_buffer,
		m_graphics_pipeline_layout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(VulkanCropPushConstants),
		&crop_push_constants);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(command_buffer);

	result = vkEndCommandBuffer(command_buffer);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkEndCommandBuffer failed (VkResult=%d)\n", result);
		return;
	}

	result = vkResetFences(m_device, 1, &frame_fence);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkResetFences failed (VkResult=%d)\n", result);
		return;
	}

	VkSemaphore wait_semaphores[] = {m_image_available_semaphores[frame_index]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore signal_semaphores[] = {m_swapchain_render_finished_semaphores[swapchain_image_index]};

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, frame_fence);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkQueueSubmit failed (VkResult=%d)\n", result);
		const VkResult idle_result = vkDeviceWaitIdle(m_device);
		if (idle_result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkDeviceWaitIdle during submit-failure recovery failed (VkResult=%d)\n",
				idle_result);
		}
		if (!recreate_frame_sync_objects(frame_index)) {
			write_log("VulkanRenderer: failed to recreate frame %u sync objects after submit failure\n",
				frame_index);
		}
		return;
	}

	m_swapchain_images_in_flight[swapchain_image_index] = frame_fence;

	if (!m_logged_first_submit) {
		write_log("VulkanRenderer: first submit complete\n");
		m_logged_first_submit = true;
	}

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;
	present_info.pImageIndices = &swapchain_image_index;

	result = vkQueuePresentKHR(m_present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		request_swapchain_recreate(result == VK_ERROR_OUT_OF_DATE_KHR ?
			"vkQueuePresentKHR out of date" :
			"vkQueuePresentKHR suboptimal");
		recreate_swapchain();
		return;
	}
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkQueuePresentKHR failed (VkResult=%d)\n", result);
		return;
	}

	if (!m_logged_first_present) {
		write_log("VulkanRenderer: first present complete\n");
		m_logged_first_present = true;
	}

	m_current_frame = (m_current_frame + 1) % k_max_frames_in_flight;
}

// ============================================================================
// Input coordinate translation
// ============================================================================

void VulkanRenderer::get_gfx_offset(int monid, float src_w, float src_h,
	float src_x, float src_y, float* dx, float* dy, float* mx, float* my)
{
	// Use same logic as SDL renderer: direct mapping based on render_quad
	*dx = static_cast<float>(render_quad.x) - src_x;
	*dy = static_cast<float>(render_quad.y) - src_y;

	if (src_w > 0 && render_quad.w > 0)
		*mx = static_cast<float>(render_quad.w) / src_w;
	else
		*mx = 1.0f;

	if (src_h > 0 && render_quad.h > 0)
		*my = static_cast<float>(render_quad.h) / src_h;
	else
		*my = 1.0f;
}

// ============================================================================
// Drawable size query
// ============================================================================

void VulkanRenderer::get_drawable_size(SDL_Window* w, int* width, int* height)
{
	SDL_GetWindowSizeInPixels(w, width, height);
}

// ============================================================================
// Cleanup of window-associated resources
// ============================================================================

void VulkanRenderer::close_hwnds_cleanup(AmigaMonitor* /*mon*/)
{
	destroy_context();
}

// ============================================================================
// Private helpers — Vulkan initialization
// ============================================================================

bool VulkanRenderer::create_instance()
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Amiberry";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Amiberry";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	// Get required extensions from SDL
	Uint32 sdl_ext_count = 0;
	const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);
	if (!sdl_extensions || sdl_ext_count == 0) {
		write_log("VulkanRenderer: SDL_Vulkan_GetInstanceExtensions failed: %s\n",
			SDL_GetError());
		return false;
	}

	std::vector<const char*> extensions(sdl_extensions, sdl_extensions + sdl_ext_count);

#ifndef NDEBUG
	{
		uint32_t ext_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
		std::vector<VkExtensionProperties> available_exts(ext_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, available_exts.data());
		bool has_debug_utils = false;
		for (const auto& ext : available_exts) {
			if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				has_debug_utils = true;
				break;
			}
		}
		if (has_debug_utils) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} else {
			write_log("VulkanRenderer: %s not available, debug messenger disabled\n",
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}
#endif

	// Check validation layer support — only enable layers that are actually installed
	std::vector<const char*> active_layers;
	if (enable_validation) {
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : validation_layers) {
			bool found = false;
			for (const auto& layer : available_layers) {
				if (strcmp(layer_name, layer.layerName) == 0) {
					found = true;
					break;
				}
			}
			if (found) {
				active_layers.push_back(layer_name);
			} else {
				write_log("VulkanRenderer: validation layer '%s' not available, skipping\n", layer_name);
			}
		}
	}

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (!active_layers.empty()) {
		create_info.enabledLayerCount = static_cast<uint32_t>(active_layers.size());
		create_info.ppEnabledLayerNames = active_layers.data();
	}

#ifdef __APPLE__
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();
	create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VkResult result = vkCreateInstance(&create_info, nullptr, &m_instance);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateInstance failed (VkResult=%d)\n", result);
		return false;
	}

	write_log("VulkanRenderer: Vulkan instance created with %u extensions\n",
		static_cast<uint32_t>(extensions.size()));
	return true;
}

bool VulkanRenderer::create_surface()
{
	if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface)) {
		write_log("VulkanRenderer: SDL_Vulkan_CreateSurface failed: %s\n",
			SDL_GetError());
		return false;
	}
	return true;
}

bool VulkanRenderer::pick_physical_device()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
	if (device_count == 0) {
		write_log("VulkanRenderer: no GPUs with Vulkan support found\n");
		return false;
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

	// Prefer discrete GPU, fall back to integrated
	VkPhysicalDevice fallback = VK_NULL_HANDLE;

	for (const auto& device : devices) {
		if (!is_device_suitable(device))
			continue;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);

		write_log("VulkanRenderer: found suitable GPU: %s (type=%d)\n",
			props.deviceName, props.deviceType);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			m_physical_device = device;
			write_log("VulkanRenderer: selected discrete GPU: %s\n", props.deviceName);
			return true;
		}

		if (fallback == VK_NULL_HANDLE)
			fallback = device;
	}

	if (fallback != VK_NULL_HANDLE) {
		m_physical_device = fallback;
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(m_physical_device, &props);
		write_log("VulkanRenderer: selected GPU: %s\n", props.deviceName);
		return true;
	}

	return false;
}

bool VulkanRenderer::is_device_suitable(VkPhysicalDevice device) const
{
	QueueFamilyIndices indices = find_queue_families(device);
	if (!indices.is_complete())
		return false;

	if (!check_device_extension_support(device))
		return false;

	SwapchainSupportDetails details = query_swapchain_support(device);
	return !details.formats.empty() && !details.present_modes.empty();
}

bool VulkanRenderer::check_device_extension_support(VkPhysicalDevice device) const
{
	uint32_t ext_count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);
	std::vector<VkExtensionProperties> available(ext_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, available.data());

	std::set<std::string> required(device_extensions.begin(), device_extensions.end());
	for (const auto& ext : available) {
		required.erase(ext.extensionName);
	}

	return required.empty();
}

VulkanRenderer::QueueFamilyIndices VulkanRenderer::find_queue_families(VkPhysicalDevice device) const
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, families.data());

	for (uint32_t i = 0; i < queue_family_count; i++) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics = i;
		}

		VkBool32 present_support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);
		if (present_support) {
			indices.present = i;
		}

		if (indices.is_complete())
			break;
	}

	return indices;
}

bool VulkanRenderer::create_logical_device()
{
	QueueFamilyIndices indices = find_queue_families(m_physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_families = {indices.graphics, indices.present};

	float queue_priority = 1.0f;
	for (uint32_t family : unique_families) {
		VkDeviceQueueCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = family;
		info.queueCount = 1;
		info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(info);
	}

	VkPhysicalDeviceFeatures device_features{};

	std::vector<const char*> extensions_to_enable(device_extensions);

#ifdef __APPLE__
	// MoltenVK portability subset
	extensions_to_enable.push_back("VK_KHR_portability_subset");
#endif

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions_to_enable.size());
	create_info.ppEnabledExtensionNames = extensions_to_enable.data();

	VkResult result = vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateDevice failed (VkResult=%d)\n", result);
		return false;
	}

	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.physicalDevice = m_physical_device;
	allocator_info.device = m_device;
	allocator_info.instance = m_instance;
	result = vmaCreateAllocator(&allocator_info, &m_allocator);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vmaCreateAllocator failed (VkResult=%d)\n", result);
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
		return false;
	}
	write_log("VulkanRenderer: VMA allocator created\n");

	m_graphics_queue_family = indices.graphics;
	m_present_queue_family = indices.present;
	vkGetDeviceQueue(m_device, indices.graphics, 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, indices.present, 0, &m_present_queue);

	write_log("VulkanRenderer: logical device created (graphics queue=%u, present queue=%u)\n",
		indices.graphics, indices.present);
	return true;
}

bool VulkanRenderer::create_command_infrastructure()
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = m_graphics_queue_family;

	VkResult result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_graphics_command_pool);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateCommandPool failed (VkResult=%d)\n", result);
		return false;
	}

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_graphics_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = k_max_frames_in_flight;

	result = vkAllocateCommandBuffers(m_device, &alloc_info, m_graphics_command_buffers.data());
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkAllocateCommandBuffers failed (VkResult=%d)\n", result);
		cleanup_command_infrastructure();
		return false;
	}

	write_log("VulkanRenderer: MAX_FRAMES_IN_FLIGHT=%u\n", k_max_frames_in_flight);
	write_log("VulkanRenderer: graphics command pool created (family=%u)\n", m_graphics_queue_family);
	write_log("VulkanRenderer: allocated %u primary graphics command buffers\n", k_max_frames_in_flight);
	return true;
}

void VulkanRenderer::cleanup_command_infrastructure()
{
	if (m_device == VK_NULL_HANDLE)
		return;

	if (m_graphics_command_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(m_device, m_graphics_command_pool, nullptr);
		m_graphics_command_pool = VK_NULL_HANDLE;
	}

	for (auto& command_buffer : m_graphics_command_buffers) {
		command_buffer = VK_NULL_HANDLE;
	}
}

bool VulkanRenderer::create_sync_objects()
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < k_max_frames_in_flight; ++i) {
		VkResult result = vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkCreateSemaphore(image_available)[%u] failed (VkResult=%d)\n", i, result);
			cleanup_sync_objects();
			return false;
		}

		result = vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkCreateFence(in_flight)[%u] failed (VkResult=%d)\n", i, result);
			cleanup_sync_objects();
			return false;
		}
	}

	write_log("VulkanRenderer: created per-frame sync objects (%u frames, fences signaled)\n",
		k_max_frames_in_flight);
	return true;
}

void VulkanRenderer::cleanup_sync_objects()
{
	if (m_device == VK_NULL_HANDLE)
		return;

	for (uint32_t i = 0; i < k_max_frames_in_flight; ++i) {
		if (m_in_flight_fences[i] != VK_NULL_HANDLE) {
			vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
			m_in_flight_fences[i] = VK_NULL_HANDLE;
		}

		if (m_image_available_semaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
			m_image_available_semaphores[i] = VK_NULL_HANDLE;
		}
	}
}

bool VulkanRenderer::recreate_frame_sync_objects(uint32_t frame_index)
{
	if (m_device == VK_NULL_HANDLE || frame_index >= k_max_frames_in_flight)
		return false;

	const VkFence old_frame_fence = m_in_flight_fences[frame_index];
	for (auto& image_fence : m_swapchain_images_in_flight) {
		if (image_fence == old_frame_fence) {
			image_fence = VK_NULL_HANDLE;
		}
	}

	if (m_in_flight_fences[frame_index] != VK_NULL_HANDLE) {
		vkDestroyFence(m_device, m_in_flight_fences[frame_index], nullptr);
		m_in_flight_fences[frame_index] = VK_NULL_HANDLE;
	}

	if (m_image_available_semaphores[frame_index] != VK_NULL_HANDLE) {
		vkDestroySemaphore(m_device, m_image_available_semaphores[frame_index], nullptr);
		m_image_available_semaphores[frame_index] = VK_NULL_HANDLE;
	}

	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result = vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[frame_index]);
	if (result != VK_SUCCESS)
		return false;

	result = vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[frame_index]);
	if (result != VK_SUCCESS) {
		vkDestroySemaphore(m_device, m_image_available_semaphores[frame_index], nullptr);
		m_image_available_semaphores[frame_index] = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

bool VulkanRenderer::create_swapchain_present_sync_objects()
{
	cleanup_swapchain_present_sync_objects();

	m_swapchain_render_finished_semaphores.resize(m_swapchain_images.size(), VK_NULL_HANDLE);
	m_swapchain_images_in_flight.resize(m_swapchain_images.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < m_swapchain_render_finished_semaphores.size(); ++i) {
		const VkResult result = vkCreateSemaphore(
			m_device,
			&semaphore_info,
			nullptr,
			&m_swapchain_render_finished_semaphores[i]);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkCreateSemaphore(swapchain_render_finished)[%zu] failed (VkResult=%d)\n", i, result);
			cleanup_swapchain_present_sync_objects();
			return false;
		}
	}

	return true;
}

void VulkanRenderer::cleanup_swapchain_present_sync_objects()
{
	if (m_device != VK_NULL_HANDLE) {
		for (auto semaphore : m_swapchain_render_finished_semaphores) {
			if (semaphore != VK_NULL_HANDLE) {
				vkDestroySemaphore(m_device, semaphore, nullptr);
			}
		}
	}

	m_swapchain_render_finished_semaphores.clear();
	m_swapchain_images_in_flight.clear();
}

// ============================================================================
// Swapchain
// ============================================================================

VulkanRenderer::SwapchainSupportDetails VulkanRenderer::query_swapchain_support(VkPhysicalDevice device) const
{
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
	if (format_count > 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.formats.data());
	}

	uint32_t mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &mode_count, nullptr);
	if (mode_count > 0) {
		details.present_modes.resize(mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &mode_count, details.present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanRenderer::choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats)
{
	for (const auto& fmt : formats) {
		if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM &&
			fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return fmt;
		}
	}
	for (const auto& fmt : formats) {
		if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM &&
			fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return fmt;
		}
	}
	for (const auto& fmt : formats) {
		if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM) {
			return fmt;
		}
	}
	for (const auto& fmt : formats) {
		if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM) {
			return fmt;
		}
	}
	// Fall back to first available
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::choose_present_mode(const std::vector<VkPresentModeKHR>& modes)
{
	// Prefer MAILBOX (triple-buffered, lowest latency with vsync)
	for (const auto& mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}
	// FIFO is guaranteed to be available (vsync)
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	// Query actual pixel size from SDL
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(m_window, &w, &h);

	VkExtent2D extent = {
		static_cast<uint32_t>(w),
		static_cast<uint32_t>(h)
	};

	extent.width = std::clamp(extent.width,
		capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = std::clamp(extent.height,
		capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return extent;
}

bool VulkanRenderer::create_swapchain(VkSwapchainKHR old_swapchain)
{
	SwapchainSupportDetails details = query_swapchain_support(m_physical_device);
	if (details.formats.empty() || details.present_modes.empty()) {
		write_log("VulkanRenderer: swapchain support query returned no usable formats/present modes\n");
		return false;
	}

	VkSurfaceFormatKHR surface_format = choose_surface_format(details.formats);
	VkPresentModeKHR present_mode = choose_present_mode(details.present_modes);
	VkExtent2D extent = choose_swap_extent(details.capabilities);
	write_log("VulkanRenderer: selected formats (swapchain=%d, upload=%d)\n",
		surface_format.format, k_upload_texture_format);

	// Request one more image than minimum for triple buffering
	uint32_t image_count = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 &&
		image_count > details.capabilities.maxImageCount) {
		image_count = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (details.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	QueueFamilyIndices indices = find_queue_families(m_physical_device);
	uint32_t queue_family_indices[] = {indices.graphics, indices.present};

	if (indices.graphics != indices.present) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	create_info.preTransform = details.capabilities.currentTransform;
	// Pick composite alpha from the supported mask, preferring OPAQUE
	VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (!(details.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
		const VkCompositeAlphaFlagBitsKHR candidates[] = {
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		for (auto candidate : candidates) {
			if (details.capabilities.supportedCompositeAlpha & candidate) {
				composite_alpha = candidate;
				break;
			}
		}
	}
	create_info.compositeAlpha = composite_alpha;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = old_swapchain;

	VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
	VkResult result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &new_swapchain);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateSwapchainKHR failed (VkResult=%d)\n", result);
		return false;
	}

	// Retrieve swapchain images
	vkGetSwapchainImagesKHR(m_device, new_swapchain, &image_count, nullptr);
	std::vector<VkImage> new_swapchain_images(image_count);
	vkGetSwapchainImagesKHR(m_device, new_swapchain, &image_count, new_swapchain_images.data());

	m_swapchain = new_swapchain;
	m_swapchain_images = std::move(new_swapchain_images);

	m_swapchain_format = surface_format.format;
	m_swapchain_extent = extent;

	if (!create_swapchain_image_views()) {
		if (m_swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
			m_swapchain = VK_NULL_HANDLE;
		}
		m_swapchain_images.clear();
		m_swapchain_format = VK_FORMAT_UNDEFINED;
		m_swapchain_extent = {0, 0};
		return false;
	}

	if (!create_swapchain_present_sync_objects()) {
		cleanup_swapchain_image_views();
		if (m_swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
			m_swapchain = VK_NULL_HANDLE;
		}
		m_swapchain_images.clear();
		m_swapchain_format = VK_FORMAT_UNDEFINED;
		m_swapchain_extent = {0, 0};
		return false;
	}

	write_log("VulkanRenderer: swapchain created (format=%d, mode=%d, %u images)\n",
		surface_format.format, present_mode, image_count);
	return true;
}

void VulkanRenderer::cleanup_swapchain()
{
	cleanup_swapchain_draw_resources();
	cleanup_swapchain_image_views();
	cleanup_swapchain_present_sync_objects();

	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}

	m_swapchain_images.clear();
	m_swapchain_format = VK_FORMAT_UNDEFINED;
	m_swapchain_extent = {0, 0};
}

bool VulkanRenderer::create_swapchain_image_views()
{
	m_swapchain_image_views.clear();
	m_swapchain_image_views.resize(m_swapchain_images.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < m_swapchain_images.size(); i++) {
		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = m_swapchain_images[i];
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = m_swapchain_format;
		view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;

		const VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &m_swapchain_image_views[i]);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkCreateImageView[%zu] failed (VkResult=%d)\n", i, result);
			cleanup_swapchain_image_views();
			return false;
		}
	}

	return true;
}

void VulkanRenderer::cleanup_swapchain_image_views()
{

	for (auto view : m_swapchain_image_views) {
		if (view != VK_NULL_HANDLE)
			vkDestroyImageView(m_device, view, nullptr);
	}
	m_swapchain_image_views.clear();
}

void VulkanRenderer::request_swapchain_recreate(const char* reason)
{
	if (!m_swapchain_recreate_requested) {
		write_log("VulkanRenderer: swapchain recreate requested (%s)\n",
			reason != nullptr ? reason : "unspecified");
	}
	m_swapchain_recreate_requested = true;
}

bool VulkanRenderer::recreate_swapchain()
{
	if (!m_swapchain_recreate_requested)
		return true;

	if (m_device == VK_NULL_HANDLE || m_window == nullptr)
		return false;

	int drawable_w = 0;
	int drawable_h = 0;
	SDL_GetWindowSizeInPixels(m_window, &drawable_w, &drawable_h);
	if (drawable_w <= 0 || drawable_h <= 0) {
		if (!m_logged_zero_extent_skip) {
			write_log("VulkanRenderer: resize skipped due to zero extent (%dx%d)\n", drawable_w, drawable_h);
			m_logged_zero_extent_skip = true;
		}
		return false;
	}
	m_logged_zero_extent_skip = false;

	const VkResult idle_result = vkDeviceWaitIdle(m_device);
	if (idle_result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkDeviceWaitIdle before swapchain recreate failed (VkResult=%d)\n", idle_result);
		return false;
	}

	cleanup_swapchain();

	if (!create_swapchain()) {
		write_log("VulkanRenderer: swapchain recreate failed during swapchain creation\n");
		return false;
	}

	if (!create_swapchain_draw_resources()) {
		write_log("VulkanRenderer: swapchain recreate failed during draw-resource creation\n");
		cleanup_swapchain();
		return false;
	}

	m_swapchain_recreate_requested = false;
	write_log("VulkanRenderer: swapchain recreated (%ux%u, %zu images)\n",
		m_swapchain_extent.width,
		m_swapchain_extent.height,
		m_swapchain_images.size());
	return true;
}

bool VulkanRenderer::create_persistent_draw_resources()
{
	if (!create_texture_sampler())
		return false;

	if (!create_texture_descriptor_resources()) {
		cleanup_texture_sampler();
		return false;
	}

	if (!create_pipeline_layout()) {
		cleanup_texture_descriptor_resources();
		cleanup_texture_sampler();
		return false;
	}

	if (!create_shader_modules()) {
		cleanup_pipeline_layout();
		cleanup_texture_descriptor_resources();
		cleanup_texture_sampler();
		return false;
	}

	if (m_upload_texture_view != VK_NULL_HANDLE && !update_texture_descriptor_set()) {
		cleanup_shader_modules();
		cleanup_pipeline_layout();
		cleanup_texture_descriptor_resources();
		cleanup_texture_sampler();
		return false;
	}

	return true;
}

void VulkanRenderer::cleanup_persistent_draw_resources()
{
	cleanup_shader_modules();
	cleanup_pipeline_layout();
	cleanup_texture_descriptor_resources();
	cleanup_texture_sampler();
}

bool VulkanRenderer::create_swapchain_draw_resources()
{
	if (!create_render_pass())
		return false;

	if (!create_swapchain_framebuffers()) {
		cleanup_render_pass();
		return false;
	}

	if (!create_graphics_pipeline()) {
		cleanup_swapchain_framebuffers();
		cleanup_render_pass();
		return false;
	}

	return true;
}

void VulkanRenderer::cleanup_swapchain_draw_resources()
{
	cleanup_graphics_pipeline();
	cleanup_swapchain_framebuffers();
	cleanup_render_pass();
}

bool VulkanRenderer::create_texture_sampler()
{
	if (m_upload_texture_sampler != VK_NULL_HANDLE)
		return true;

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	const VkResult result = vkCreateSampler(m_device, &sampler_info, nullptr, &m_upload_texture_sampler);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateSampler failed (VkResult=%d)\n", result);
		return false;
	}

	write_log("VulkanRenderer: texture sampler created (nearest/clamp)\n");
	return true;
}

void VulkanRenderer::cleanup_texture_sampler()
{
	if (m_upload_texture_sampler != VK_NULL_HANDLE) {
		vkDestroySampler(m_device, m_upload_texture_sampler, nullptr);
		m_upload_texture_sampler = VK_NULL_HANDLE;
	}
}

bool VulkanRenderer::create_texture_descriptor_resources()
{
	if (m_texture_descriptor_set_layout != VK_NULL_HANDLE &&
		m_texture_descriptor_pool != VK_NULL_HANDLE &&
		m_texture_descriptor_set != VK_NULL_HANDLE) {
		return true;
	}

	VkDescriptorSetLayoutBinding binding{};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = 1;
	layout_info.pBindings = &binding;

	VkResult result = vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_texture_descriptor_set_layout);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateDescriptorSetLayout failed (VkResult=%d)\n", result);
		cleanup_texture_descriptor_resources();
		return false;
	}

	VkDescriptorPoolSize pool_size{};
	pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	pool_info.maxSets = 1;

	result = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_texture_descriptor_pool);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateDescriptorPool failed (VkResult=%d)\n", result);
		cleanup_texture_descriptor_resources();
		return false;
	}

	VkDescriptorSetAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = m_texture_descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &m_texture_descriptor_set_layout;

	result = vkAllocateDescriptorSets(m_device, &allocate_info, &m_texture_descriptor_set);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkAllocateDescriptorSets failed (VkResult=%d)\n", result);
		cleanup_texture_descriptor_resources();
		return false;
	}

	return true;
}

void VulkanRenderer::cleanup_texture_descriptor_resources()
{
	m_texture_descriptor_set = VK_NULL_HANDLE;

	if (m_texture_descriptor_pool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(m_device, m_texture_descriptor_pool, nullptr);
		m_texture_descriptor_pool = VK_NULL_HANDLE;
	}

	if (m_texture_descriptor_set_layout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(m_device, m_texture_descriptor_set_layout, nullptr);
		m_texture_descriptor_set_layout = VK_NULL_HANDLE;
	}
}

bool VulkanRenderer::update_texture_descriptor_set()
{
	if (m_texture_descriptor_set == VK_NULL_HANDLE ||
		m_upload_texture_sampler == VK_NULL_HANDLE ||
		m_upload_texture_view == VK_NULL_HANDLE) {
		write_log("VulkanRenderer: update_texture_descriptor_set called before descriptor/image resources are ready\n");
		return false;
	}

	VkDescriptorImageInfo image_info{};
	image_info.sampler = m_upload_texture_sampler;
	image_info.imageView = m_upload_texture_view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_texture_descriptor_set;
	write.dstBinding = 0;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &image_info;

	vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	write_log("VulkanRenderer: descriptor set updated for upload texture\n");
	return true;
}

bool VulkanRenderer::create_pipeline_layout()
{
	if (m_graphics_pipeline_layout != VK_NULL_HANDLE)
		return true;

	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(VulkanCropPushConstants);

	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = &m_texture_descriptor_set_layout;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges = &push_constant_range;

	const VkResult result = vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_graphics_pipeline_layout);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreatePipelineLayout failed (VkResult=%d)\n", result);
		return false;
	}

	return true;
}

void VulkanRenderer::cleanup_pipeline_layout()
{
	if (m_graphics_pipeline_layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(m_device, m_graphics_pipeline_layout, nullptr);
		m_graphics_pipeline_layout = VK_NULL_HANDLE;
	}
}

bool VulkanRenderer::create_shader_modules()
{
	if (m_vertex_shader_module != VK_NULL_HANDLE && m_fragment_shader_module != VK_NULL_HANDLE)
		return true;

	VkShaderModuleCreateInfo vertex_module_info{};
	vertex_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertex_module_info.codeSize = sizeof(__amiberry_fullscreen_vert_spv);
	vertex_module_info.pCode = __amiberry_fullscreen_vert_spv;

	VkResult result = vkCreateShaderModule(m_device, &vertex_module_info, nullptr, &m_vertex_shader_module);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateShaderModule(vertex) failed (VkResult=%d)\n", result);
		cleanup_shader_modules();
		return false;
	}

	VkShaderModuleCreateInfo fragment_module_info{};
	fragment_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragment_module_info.codeSize = sizeof(__amiberry_fullscreen_frag_spv);
	fragment_module_info.pCode = __amiberry_fullscreen_frag_spv;

	result = vkCreateShaderModule(m_device, &fragment_module_info, nullptr, &m_fragment_shader_module);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateShaderModule(fragment) failed (VkResult=%d)\n", result);
		cleanup_shader_modules();
		return false;
	}

	return true;
}

void VulkanRenderer::cleanup_shader_modules()
{
	if (m_fragment_shader_module != VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_device, m_fragment_shader_module, nullptr);
		m_fragment_shader_module = VK_NULL_HANDLE;
	}

	if (m_vertex_shader_module != VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_device, m_vertex_shader_module, nullptr);
		m_vertex_shader_module = VK_NULL_HANDLE;
	}
}

bool VulkanRenderer::create_render_pass()
{
	if (m_render_pass != VK_NULL_HANDLE)
		return true;

	VkAttachmentDescription color_attachment{};
	color_attachment.format = m_swapchain_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	const VkResult result = vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateRenderPass failed (VkResult=%d)\n", result);
		return false;
	}

	write_log("VulkanRenderer: render pass created (loadOp=clear)\n");
	return true;
}

void VulkanRenderer::cleanup_render_pass()
{
	if (m_render_pass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(m_device, m_render_pass, nullptr);
		m_render_pass = VK_NULL_HANDLE;
	}
}

bool VulkanRenderer::create_swapchain_framebuffers()
{
	if (!m_swapchain_framebuffers.empty())
		return true;

	m_swapchain_framebuffers.resize(m_swapchain_image_views.size(), VK_NULL_HANDLE);
	for (size_t i = 0; i < m_swapchain_image_views.size(); ++i) {
		VkImageView attachments[] = {m_swapchain_image_views[i]};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = m_render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = m_swapchain_extent.width;
		framebuffer_info.height = m_swapchain_extent.height;
		framebuffer_info.layers = 1;

		const VkResult result = vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_swapchain_framebuffers[i]);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkCreateFramebuffer[%zu] failed (VkResult=%d)\n", i, result);
			cleanup_swapchain_framebuffers();
			return false;
		}
	}

	write_log("VulkanRenderer: framebuffers created (%zu)\n", m_swapchain_framebuffers.size());
	return true;
}

void VulkanRenderer::cleanup_swapchain_framebuffers()
{
	for (auto framebuffer : m_swapchain_framebuffers) {
		if (framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}
	m_swapchain_framebuffers.clear();
}

bool VulkanRenderer::create_graphics_pipeline()
{
	if (m_graphics_pipeline != VK_NULL_HANDLE)
		return true;

	VkPipelineShaderStageCreateInfo vertex_stage{};
	vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_stage.module = m_vertex_shader_module;
	vertex_stage.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_stage{};
	fragment_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_stage.module = m_fragment_shader_module;
	fragment_stage.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_stage, fragment_stage};

	VkPipelineVertexInputStateCreateInfo vertex_input_state{};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer_state{};
	rasterizer_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_state.depthClampEnable = VK_FALSE;
	rasterizer_state.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_state.lineWidth = 1.0f;
	rasterizer_state.cullMode = VK_CULL_MODE_NONE;
	rasterizer_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer_state.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisample_state{};
	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state.sampleShadingEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_state{};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &color_blend_attachment;

	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(sizeof(dynamic_states) / sizeof(dynamic_states[0]));
	dynamic_state.pDynamicStates = dynamic_states;

	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = static_cast<uint32_t>(sizeof(shader_stages) / sizeof(shader_stages[0]));
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_state;
	pipeline_info.pInputAssemblyState = &input_assembly_state;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer_state;
	pipeline_info.pMultisampleState = &multisample_state;
	pipeline_info.pDepthStencilState = nullptr;
	pipeline_info.pColorBlendState = &color_blend_state;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = m_graphics_pipeline_layout;
	pipeline_info.renderPass = m_render_pass;
	pipeline_info.subpass = 0;

	const VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateGraphicsPipelines failed (VkResult=%d)\n", result);
		return false;
	}

	write_log("VulkanRenderer: graphics pipeline created (dynamic viewport/scissor)\n");
	return true;
}

void VulkanRenderer::cleanup_graphics_pipeline()
{
	if (m_graphics_pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
		m_graphics_pipeline = VK_NULL_HANDLE;
	}
}

void VulkanRenderer::cleanup_upload_resources()
{
	cleanup_upload_texture();
	cleanup_upload_staging();

	m_upload_texture_extent = {0, 0};
	m_upload_texture_width = 0;
	m_upload_texture_height = 0;
	m_upload_texture_cached_format = VK_FORMAT_UNDEFINED;
	m_upload_staging_size = 0;
	m_has_uploaded_frame = false;
}

void VulkanRenderer::cleanup_upload_staging()
{
	if (m_upload_staging_allocation != nullptr && m_upload_staging_mapped != nullptr && m_upload_staging_mapped_by_vma_map) {
		vmaUnmapMemory(m_allocator, m_upload_staging_allocation);
	}
	m_upload_staging_mapped = nullptr;
	m_upload_staging_mapped_by_vma_map = false;

	if (m_upload_staging_buffer != VK_NULL_HANDLE && m_upload_staging_allocation != nullptr) {
		vmaDestroyBuffer(m_allocator, m_upload_staging_buffer, m_upload_staging_allocation);
	}

	m_upload_staging_buffer = VK_NULL_HANDLE;
	m_upload_staging_allocation = nullptr;
}

void VulkanRenderer::cleanup_upload_texture()
{
	if (m_upload_texture_view != VK_NULL_HANDLE) {
		vkDestroyImageView(m_device, m_upload_texture_view, nullptr);
		m_upload_texture_view = VK_NULL_HANDLE;
	}

	if (m_upload_texture_image != VK_NULL_HANDLE && m_upload_texture_allocation != nullptr) {
		vmaDestroyImage(m_allocator, m_upload_texture_image, m_upload_texture_allocation);
	}

	m_upload_texture_image = VK_NULL_HANDLE;
	m_upload_texture_allocation = nullptr;
}

bool VulkanRenderer::transition_image_to_shader_read(VkImage image)
{
	VkCommandBuffer command_buffer = begin_one_time_graphics_commands();
	if (command_buffer == VK_NULL_HANDLE)
		return false;

	record_image_layout_transition(
		command_buffer,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		0,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	return end_one_time_graphics_commands(command_buffer);
}

bool VulkanRenderer::transition_upload_texture_to_shader_read()
{
	return transition_image_to_shader_read(m_upload_texture_image);
}

VkCommandBuffer VulkanRenderer::begin_one_time_graphics_commands()
{
	if (m_graphics_command_pool == VK_NULL_HANDLE) {
		write_log("VulkanRenderer: begin_one_time_graphics_commands called without command pool\n");
		return VK_NULL_HANDLE;
	}

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = m_graphics_command_pool;
	alloc_info.commandBufferCount = 1;

	VkResult result = vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkAllocateCommandBuffers for one-time command failed (VkResult=%d)\n", result);
		return VK_NULL_HANDLE;
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkBeginCommandBuffer for one-time command failed (VkResult=%d)\n", result);
		vkFreeCommandBuffers(m_device, m_graphics_command_pool, 1, &command_buffer);
		return VK_NULL_HANDLE;
	}

	return command_buffer;
}

bool VulkanRenderer::end_one_time_graphics_commands(VkCommandBuffer command_buffer)
{
	VkResult result = vkEndCommandBuffer(command_buffer);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkEndCommandBuffer for one-time command failed (VkResult=%d)\n", result);
		vkFreeCommandBuffers(m_device, m_graphics_command_pool, 1, &command_buffer);
		return false;
	}

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	result = vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkQueueSubmit for one-time command failed (VkResult=%d)\n", result);
		vkFreeCommandBuffers(m_device, m_graphics_command_pool, 1, &command_buffer);
		return false;
	}

	result = vkQueueWaitIdle(m_graphics_queue);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkQueueWaitIdle for one-time command failed (VkResult=%d)\n", result);
		vkFreeCommandBuffers(m_device, m_graphics_command_pool, 1, &command_buffer);
		return false;
	}

	vkFreeCommandBuffers(m_device, m_graphics_command_pool, 1, &command_buffer);
	return true;
}

void VulkanRenderer::record_image_layout_transition(VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout,
	VkImageLayout new_layout,
	VkAccessFlags src_access_mask,
	VkAccessFlags dst_access_mask,
	VkPipelineStageFlags src_stage_mask,
	VkPipelineStageFlags dst_stage_mask)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = src_access_mask;
	barrier.dstAccessMask = dst_access_mask;

	vkCmdPipelineBarrier(
		command_buffer,
		src_stage_mask,
		dst_stage_mask,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);
}

#ifndef NDEBUG
bool VulkanRenderer::setup_debug_messenger()
{
	VkDebugUtilsMessengerCreateInfoEXT create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_messenger_callback;

	VkResult result = create_debug_utils_messenger(m_instance, &create_info, nullptr, &m_debug_messenger);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: failed to set up debug messenger (VkResult=%d)\n", result);
		return false;
	}

	write_log("VulkanRenderer: debug messenger enabled\n");
	return true;
}
#endif // NDEBUG

// ============================================================================
// Global accessor
// ============================================================================

VulkanRenderer* get_vulkan_renderer()
{
	auto* renderer = get_renderer();
	return dynamic_cast<VulkanRenderer*>(renderer);
}

#endif // USE_VULKAN
