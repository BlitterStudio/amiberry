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

#include <algorithm>
#include <cstring>
#include <set>
#include <limits>

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

	if (!create_swapchain()) {
		write_log("VulkanRenderer: failed to create swapchain\n");
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

	cleanup_swapchain();

	if (m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_graphics_queue = VK_NULL_HANDLE;
	m_present_queue = VK_NULL_HANDLE;
	m_physical_device = VK_NULL_HANDLE;

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

bool VulkanRenderer::alloc_texture(int /*monid*/, int /*w*/, int /*h*/)
{
	// Phase 2: No texture allocation yet
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
// Frame rendering (Phase 2: stubs)
// ============================================================================

bool VulkanRenderer::render_frame(int /*monid*/, int /*mode*/, int /*immediate*/)
{
	// Phase 2: No rendering yet
	return true;
}

void VulkanRenderer::present_frame(int /*monid*/, int /*mode*/)
{
	// Phase 2: No presentation yet
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

	m_graphics_queue_family = indices.graphics;
	m_present_queue_family = indices.present;
	vkGetDeviceQueue(m_device, indices.graphics, 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, indices.present, 0, &m_present_queue);

	write_log("VulkanRenderer: logical device created (graphics queue=%u, present queue=%u)\n",
		indices.graphics, indices.present);
	return true;
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
		if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
			fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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

bool VulkanRenderer::create_swapchain()
{
	SwapchainSupportDetails details = query_swapchain_support(m_physical_device);

	VkSurfaceFormatKHR surface_format = choose_surface_format(details.formats);
	VkPresentModeKHR present_mode = choose_present_mode(details.present_modes);
	VkExtent2D extent = choose_swap_extent(details.capabilities);

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
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain);
	if (result != VK_SUCCESS) {
		write_log("VulkanRenderer: vkCreateSwapchainKHR failed (VkResult=%d)\n", result);
		return false;
	}

	// Retrieve swapchain images
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());

	m_swapchain_format = surface_format.format;
	m_swapchain_extent = extent;

	// Create image views
	m_swapchain_image_views.resize(m_swapchain_images.size());
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

		result = vkCreateImageView(m_device, &view_info, nullptr, &m_swapchain_image_views[i]);
		if (result != VK_SUCCESS) {
			write_log("VulkanRenderer: vkCreateImageView[%zu] failed (VkResult=%d)\n", i, result);
			return false;
		}
	}

	write_log("VulkanRenderer: swapchain created (format=%d, mode=%d, %u images)\n",
		surface_format.format, present_mode, image_count);
	return true;
}

void VulkanRenderer::cleanup_swapchain()
{
	for (auto view : m_swapchain_image_views) {
		if (view != VK_NULL_HANDLE)
			vkDestroyImageView(m_device, view, nullptr);
	}
	m_swapchain_image_views.clear();
	m_swapchain_images.clear();

	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}

	m_swapchain_format = VK_FORMAT_UNDEFINED;
	m_swapchain_extent = {0, 0};
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
