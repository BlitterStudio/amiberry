#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <SDL3_image/SDL_image.h>
#include <dpi_handler.hpp>
#include <unordered_map>
#include <cmath>
#include <functional>

#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "irenderer.h"
#ifdef USE_OPENGL
#include "opengl_renderer.h"
#endif
#include "fsdb_host.h"
#include "autoconf.h"
#include "blkdev.h"
#include "inputdevice.h"
#include "xwin.h"
#include "custom.h"
#include "disk.h"
#include "zfile.h"
#include "imgui_internal.h"
#include "savestate.h"
#include "target.h"
#include "tinyxml2.h"
#include "file_dialog.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#if defined(_WIN32) && defined(USE_OPENGL)
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL_opengl.h>
#endif
#ifdef USE_VULKAN
#include "imgui_impl_vulkan.h"
#include "vulkan_renderer.h"
#endif
#include <array>
#include <fstream>
#include <sstream>
#include "imgui/imgui_panels.h"
#include "imgui/imgui_help_text.h"
#include "icons_fa.h"
#include "fa_solid_900_compressed.h"
#include "AmigaTopaz_compressed.h"
#include "amiberry_png_data.h"

bool ctrl_state = false, shift_state = false, alt_state = false, win_state = false;
int last_x = 0;
int last_y = 0;

bool gui_running = false;
static int last_active_panel = 3;
bool joystick_refresh_needed = false;

// Touch-drag scrolling state (converts finger swipes to ImGui scroll wheel events)
static bool touch_scrolling = false;
static bool touch_scroll_on_widget = false;  // true when finger-down landed on an active ImGui widget (e.g. scrollbar)
static float touch_scroll_accum = 0.0f;
static SDL_FingerID touch_scroll_finger = 0;

static TCHAR startup_title[MAX_STARTUP_TITLE] = _T("");
static TCHAR startup_message[MAX_STARTUP_MESSAGE] = _T("");

#ifdef _WIN32
static int saved_emu_x = 0, saved_emu_y = 0, saved_emu_w = 0, saved_emu_h = 0;
static SDL_WindowFlags saved_emu_flags = 0;
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
static bool gui_use_opengl = false;
#endif
#ifdef USE_VULKAN
static bool gui_use_vulkan = false;
// Track Vulkan resources for GUI textures so we can clean them up
struct VkGuiTextureResources {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
};
static std::unordered_map<uint64_t, VkGuiTextureResources> vk_gui_textures;
#endif

// Texture helpers: abstract SDL_Texture (SDLRenderer2) vs GLuint (OpenGL3) vs VkDescriptorSet (Vulkan)
ImTextureID gui_create_texture(SDL_Surface* surface, int* out_w, int* out_h)
{
	if (!surface) return ImTextureID_Invalid;
	if (out_w) *out_w = surface->w;
	if (out_h) *out_h = surface->h;
#ifdef USE_VULKAN
	if (gui_use_vulkan) {
		auto* vk = get_vulkan_renderer();
		if (!vk) return ImTextureID_Invalid;

		SDL_Surface* rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
		if (!rgba) return ImTextureID_Invalid;

		VkDevice device = vk->get_vk_device();

		// Create staging buffer
		VkDeviceSize image_size = static_cast<VkDeviceSize>(rgba->w) * rgba->h * 4;
		VkBuffer staging_buffer;
		VkDeviceMemory staging_memory;
		{
			VkBufferCreateInfo buf_info{};
			buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buf_info.size = image_size;
			buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			if (vkCreateBuffer(device, &buf_info, nullptr, &staging_buffer) != VK_SUCCESS) {
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
			VkMemoryRequirements mem_reqs;
			vkGetBufferMemoryRequirements(device, staging_buffer, &mem_reqs);

			VkPhysicalDeviceMemoryProperties mem_props;
			vkGetPhysicalDeviceMemoryProperties(vk->get_vk_physical_device(), &mem_props);
			uint32_t mem_type = UINT32_MAX;
			for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
				if ((mem_reqs.memoryTypeBits & (1 << i)) &&
					(mem_props.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
					(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
					mem_type = i;
					break;
				}
			}
			if (mem_type == UINT32_MAX) {
				vkDestroyBuffer(device, staging_buffer, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
			VkMemoryAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = mem_reqs.size;
			alloc_info.memoryTypeIndex = mem_type;
			if (vkAllocateMemory(device, &alloc_info, nullptr, &staging_memory) != VK_SUCCESS) {
				vkDestroyBuffer(device, staging_buffer, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
			vkBindBufferMemory(device, staging_buffer, staging_memory, 0);
			void* mapped;
			vkMapMemory(device, staging_memory, 0, image_size, 0, &mapped);
			const auto* src = static_cast<const uint8_t*>(rgba->pixels);
			auto* dst = static_cast<uint8_t*>(mapped);
			const size_t dst_pitch = static_cast<size_t>(rgba->w) * 4;
			for (int y = 0; y < rgba->h; y++) {
				memcpy(dst + y * dst_pitch, src + y * rgba->pitch, dst_pitch);
			}
			vkUnmapMemory(device, staging_memory);
		}

		// Create image
		VkImage image;
		VkDeviceMemory image_memory;
		{
			VkImageCreateInfo img_info{};
			img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			img_info.imageType = VK_IMAGE_TYPE_2D;
			img_info.format = VK_FORMAT_R8G8B8A8_UNORM;
			img_info.extent = { static_cast<uint32_t>(rgba->w), static_cast<uint32_t>(rgba->h), 1 };
			img_info.mipLevels = 1;
			img_info.arrayLayers = 1;
			img_info.samples = VK_SAMPLE_COUNT_1_BIT;
			img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			img_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (vkCreateImage(device, &img_info, nullptr, &image) != VK_SUCCESS) {
				vkDestroyBuffer(device, staging_buffer, nullptr);
				vkFreeMemory(device, staging_memory, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
			VkMemoryRequirements mem_reqs;
			vkGetImageMemoryRequirements(device, image, &mem_reqs);
			VkPhysicalDeviceMemoryProperties mem_props;
			vkGetPhysicalDeviceMemoryProperties(vk->get_vk_physical_device(), &mem_props);
			uint32_t mem_type = UINT32_MAX;
			for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
				if ((mem_reqs.memoryTypeBits & (1 << i)) &&
					(mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
					mem_type = i;
					break;
				}
			}
			if (mem_type == UINT32_MAX) {
				vkDestroyImage(device, image, nullptr);
				vkDestroyBuffer(device, staging_buffer, nullptr);
				vkFreeMemory(device, staging_memory, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
			VkMemoryAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = mem_reqs.size;
			alloc_info.memoryTypeIndex = mem_type;
			if (vkAllocateMemory(device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) {
				vkDestroyImage(device, image, nullptr);
				vkDestroyBuffer(device, staging_buffer, nullptr);
				vkFreeMemory(device, staging_memory, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
			vkBindImageMemory(device, image, image_memory, 0);
		}

		// Create image view
		VkImageView image_view;
		{
			VkImageViewCreateInfo view_info{};
			view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_info.image = image;
			view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_info.subresourceRange.levelCount = 1;
			view_info.subresourceRange.layerCount = 1;
			if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
				vkDestroyImage(device, image, nullptr);
				vkFreeMemory(device, image_memory, nullptr);
				vkDestroyBuffer(device, staging_buffer, nullptr);
				vkFreeMemory(device, staging_memory, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
		}

		// Create sampler
		VkSampler sampler;
		{
			VkSamplerCreateInfo sampler_info{};
			sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler_info.magFilter = VK_FILTER_LINEAR;
			sampler_info.minFilter = VK_FILTER_LINEAR;
			sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			if (vkCreateSampler(device, &sampler_info, nullptr, &sampler) != VK_SUCCESS) {
				vkDestroyImageView(device, image_view, nullptr);
				vkDestroyImage(device, image, nullptr);
				vkFreeMemory(device, image_memory, nullptr);
				vkDestroyBuffer(device, staging_buffer, nullptr);
				vkFreeMemory(device, staging_memory, nullptr);
				SDL_DestroySurface(rgba);
				return ImTextureID_Invalid;
			}
		}

		// Upload: transition, copy, transition
		{
			VkCommandPool cmd_pool;
			VkCommandPoolCreateInfo pool_info{};
			pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool_info.queueFamilyIndex = vk->get_vk_graphics_queue_family();
			vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool);

			VkCommandBuffer cmd;
			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandPool = cmd_pool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = 1;
			vkAllocateCommandBuffers(device, &alloc_info, &cmd);

			VkCommandBufferBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(cmd, &begin_info);

			// Transition UNDEFINED → TRANSFER_DST
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			// Copy buffer to image
			VkBufferImageCopy region{};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = { static_cast<uint32_t>(rgba->w), static_cast<uint32_t>(rgba->h), 1 };
			vkCmdCopyBufferToImage(cmd, staging_buffer, image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			// Transition TRANSFER_DST → SHADER_READ_ONLY
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			vkEndCommandBuffer(cmd);

			VkSubmitInfo submit{};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmd;
			vkQueueSubmit(vk->get_vk_graphics_queue(), 1, &submit, VK_NULL_HANDLE);
			vkQueueWaitIdle(vk->get_vk_graphics_queue());

			vkDestroyCommandPool(device, cmd_pool, nullptr);
		}

		// Cleanup staging
		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_memory, nullptr);
		SDL_DestroySurface(rgba);

		// Register with ImGui
		VkDescriptorSet ds = ImGui_ImplVulkan_AddTexture(sampler, image_view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Store Vulkan resources for cleanup
		uint64_t key = reinterpret_cast<uint64_t>(ds);
		vk_gui_textures[key] = {image, image_memory, image_view, sampler};

		return (ImTextureID)ds;
	}
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl) {
		SDL_Surface* rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
		if (!rgba) return ImTextureID_Invalid;
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
		SDL_DestroySurface(rgba);
		return (ImTextureID)(intptr_t)tex;
	}
#endif
	AmigaMonitor* mon = &AMonitors[0];
	return (ImTextureID)SDL_CreateTextureFromSurface(mon->gui_renderer, surface);
}

void gui_destroy_texture(ImTextureID tex)
{
	if (tex == ImTextureID_Invalid) return;
#ifdef USE_VULKAN
	if (gui_use_vulkan) {
		auto* vk = get_vulkan_renderer();
		if (!vk) return;
		VkDevice device = vk->get_vk_device();
		VkDescriptorSet ds = (VkDescriptorSet)tex;
		uint64_t key = reinterpret_cast<uint64_t>(ds);

		auto it = vk_gui_textures.find(key);
		if (it != vk_gui_textures.end()) {
			vkDeviceWaitIdle(device);
			ImGui_ImplVulkan_RemoveTexture(ds);
			vkDestroySampler(device, it->second.sampler, nullptr);
			vkDestroyImageView(device, it->second.view, nullptr);
			vkDestroyImage(device, it->second.image, nullptr);
			vkFreeMemory(device, it->second.memory, nullptr);
			vk_gui_textures.erase(it);
		}
		return;
	}
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl) {
		GLuint gl_tex = (GLuint)(intptr_t)tex;
		glDeleteTextures(1, &gl_tex);
		return;
	}
#endif
	SDL_DestroyTexture((SDL_Texture*)tex);
}

void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	_tcsncpy(startup_title, title, MAX_STARTUP_TITLE);
	_tcsncpy(startup_message, msg, MAX_STARTUP_MESSAGE);
}

// Forward declaration
void apply_imgui_theme();

float DISTANCE_BORDER = 10;
float DISTANCE_NEXT_X = 15;
float DISTANCE_NEXT_Y = 15;
float BUTTON_WIDTH = 80;
float BUTTON_HEIGHT = 30;
float SMALL_BUTTON_WIDTH = 30;
float SMALL_BUTTON_HEIGHT = 22;
float LABEL_HEIGHT = 20;
float TEXTFIELD_HEIGHT = 20;
float DROPDOWN_HEIGHT = 20;
float SLIDER_HEIGHT = 20;
float TITLEBAR_HEIGHT = 24;
float SELECTOR_WIDTH = 165;
float SELECTOR_HEIGHT = 24;
float SCROLLBAR_WIDTH = 20;
static ImVec4 rgb_to_vec4(int r, int g, int b, float a = 1.0f) { return ImVec4{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, a }; }
static ImVec4 lighten(const ImVec4& c, float f) { return ImVec4{ std::min(c.x + f, 1.0f), std::min(c.y + f, 1.0f), std::min(c.z + f, 1.0f), c.w }; }
static ImVec4 darken(const ImVec4& c, float f) { return ImVec4{ std::max(c.x - f, 0.0f), std::max(c.y - f, 0.0f), std::max(c.z - f, 0.0f), c.w }; }
static bool parse_rgb_csv(const std::string& s, int& r, int& g, int& b) {
	std::stringstream ss(s); std::string tok;
	try {
		if (!std::getline(ss, tok, ',')) return false; r = std::stoi(tok);
		if (!std::getline(ss, tok, ',')) return false; g = std::stoi(tok);
		if (!std::getline(ss, tok, ',')) return false; b = std::stoi(tok);
	} catch (...) { return false; }
	auto clamp = [](int v){ return std::max(0, std::min(255, v)); };
	r = clamp(r); g = clamp(g); b = clamp(b);
	return true;
}

void apply_imgui_theme()
{
	auto c = [](const amiberry_gui_color& col) { return rgb_to_vec4(col.r, col.g, col.b); };

	const ImVec4 col_base    = c(gui_theme.base_color);
	const ImVec4 col_frame   = c(gui_theme.frame_color);
	const ImVec4 col_text    = c(gui_theme.font_color);
	const ImVec4 col_textdis = c(gui_theme.text_disabled_color);
	const ImVec4 col_accent  = c(gui_theme.selector_active);
	const ImVec4 col_sel     = c(gui_theme.selection_color);
	const ImVec4 col_border  = c(gui_theme.border_color);
	const ImVec4 col_btn     = c(gui_theme.button_color);
	const ImVec4 col_btnhov  = c(gui_theme.button_hover_color);
	const ImVec4 col_tblhdr  = c(gui_theme.table_header_color);
	const ImVec4 col_modal   = rgb_to_vec4(gui_theme.modal_dim_color.r, gui_theme.modal_dim_color.g, gui_theme.modal_dim_color.b, 0.55f);

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Text
	colors[ImGuiCol_Text]                 = col_text;
	colors[ImGuiCol_TextDisabled]         = col_textdis;
	colors[ImGuiCol_InputTextCursor]      = col_text;
	colors[ImGuiCol_TextSelectedBg]       = col_sel;

	// Window backgrounds
	colors[ImGuiCol_WindowBg]             = col_base;
	colors[ImGuiCol_ChildBg]              = col_base;
	colors[ImGuiCol_PopupBg]              = col_base;
	colors[ImGuiCol_MenuBarBg]            = col_base;
	colors[ImGuiCol_TitleBg]              = col_base;
	colors[ImGuiCol_TitleBgCollapsed]     = col_base;

	// Frames (input fields, checkboxes, combo boxes)
	colors[ImGuiCol_FrameBg]              = col_frame;
	colors[ImGuiCol_FrameBgHovered]       = lighten(col_frame, 0.06f);
	colors[ImGuiCol_FrameBgActive]        = lighten(col_frame, 0.12f);

	// Borders and separators
	colors[ImGuiCol_Border]               = col_border;
	colors[ImGuiCol_BorderShadow]         = lighten(col_base, 0.35f);
	colors[ImGuiCol_Separator]            = col_border;
	colors[ImGuiCol_SeparatorHovered]     = lighten(col_border, 0.15f);
	colors[ImGuiCol_SeparatorActive]      = col_accent;

	// Buttons
	colors[ImGuiCol_Button]               = col_btn;
	colors[ImGuiCol_ButtonHovered]        = col_btnhov;
	colors[ImGuiCol_ButtonActive]         = col_accent;

	// Headers (CollapsingHeader, TreeNode, Selectable, MenuItem)
	colors[ImGuiCol_Header]               = col_btn;
	colors[ImGuiCol_HeaderHovered]        = col_btnhov;
	colors[ImGuiCol_HeaderActive]         = col_accent;

	// Accent elements
	colors[ImGuiCol_TitleBgActive]        = col_accent;
	colors[ImGuiCol_CheckMark]            = darken(col_accent, 0.15f);
	colors[ImGuiCol_SliderGrab]           = col_accent;
	colors[ImGuiCol_SliderGrabActive]     = lighten(col_accent, 0.15f);
	colors[ImGuiCol_NavCursor]            = col_accent;
	colors[ImGuiCol_ResizeGrip]           = darken(col_border, 0.10f);
	colors[ImGuiCol_ResizeGripHovered]    = col_accent;
	colors[ImGuiCol_ResizeGripActive]     = lighten(col_accent, 0.10f);

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg]          = col_frame;
	colors[ImGuiCol_ScrollbarGrab]        = col_btn;
	colors[ImGuiCol_ScrollbarGrabHovered] = col_btnhov;
	colors[ImGuiCol_ScrollbarGrabActive]  = col_accent;

	// Tables
	colors[ImGuiCol_TableHeaderBg]        = col_tblhdr;
	colors[ImGuiCol_TableBorderStrong]    = col_border;
	colors[ImGuiCol_TableBorderLight]     = lighten(col_border, 0.10f);
	colors[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0); // transparent
	colors[ImGuiCol_TableRowBgAlt]        = ImVec4(col_frame.x, col_frame.y, col_frame.z, 0.25f);

	// Tabs
	colors[ImGuiCol_Tab]                  = col_btn;
	colors[ImGuiCol_TabHovered]           = col_btnhov;
	colors[ImGuiCol_TabSelected]          = col_accent;
	colors[ImGuiCol_TabDimmed]            = darken(col_btn, 0.10f);
	colors[ImGuiCol_TabDimmedSelected]    = darken(col_accent, 0.20f);

	// Modal
	colors[ImGuiCol_ModalWindowDimBg]     = col_modal;

	style.FrameBorderSize = 0.0f; // We will draw our own bevels
	style.WindowBorderSize = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.ChildBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 6.0f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	{
		int touch_count = 0;
		SDL_TouchID* touch_devs = SDL_GetTouchDevices(&touch_count);
		SDL_free(touch_devs);
		if (touch_count > 0) {
			style.ScrollbarSize = 24.0f;  // Wider for touch targets
			style.GrabMinSize = 20.0f;
		} else {
			style.ScrollbarSize = 16.0f;
			style.GrabMinSize = 10.0f;
		}
	}
	style.TabRounding = 0.0f;

	// Add a bit more padding inside windows/child areas
	style.WindowPadding = ImVec2(10.0f, 10.0f);
	style.FramePadding = ImVec2(5.0f, 4.0f);
}

// Deferred font rebuild state — font changes are requested during rendering
// but applied between frames to avoid modifying the atlas mid-render.
static bool s_font_rebuild_pending = false;
// Tracks the font file currently loaded in the atlas.
static std::string s_loaded_font_file;

void rebuild_gui_fonts()
{
	// Mark for rebuild; actual work happens in apply_pending_font_rebuild()
	s_font_rebuild_pending = true;
}

static void apply_pending_font_rebuild()
{
	if (!s_font_rebuild_pending)
		return;
	s_font_rebuild_pending = false;

	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();

	const float font_px = gui_theme.font_size > 0 ? static_cast<float>(gui_theme.font_size) : 15.0f;
	const std::string font_file = gui_theme.font_name.empty() ? std::string("AmigaTopaz.ttf") : gui_theme.font_name;

	// Font size: update FontSizeBase for the dynamic atlas to render at.
	style.FontSizeBase = font_px;

	// Full atlas rebuild needed when the font file changes.
	// Size changes are handled by FontSizeBase + the dynamic atlas.
	// Pixel-snap changes require a GUI restart (ImGui's dynamic atlas does
	// not support changing OversampleH after the font is loaded).
	const bool needs_rebuild = (font_file != s_loaded_font_file);

	if (needs_rebuild)
	{
		const float scaling_factor = DPIHandler::get_layout_scale();
		const float font_scale_dpi = style.FontScaleDpi;
		const float font_dpi_scale = (font_scale_dpi > 0.0f) ? (1.0f / font_scale_dpi) : 1.0f;
		const float font_load_px = font_px * scaling_factor * font_dpi_scale;
		io.Fonts->Clear();

		const bool use_default_font = (font_file == "AmigaTopaz.ttf");
		ImFont* loaded_font = nullptr;
		if (use_default_font) {
			ImFontConfig pixel_cfg;
			pixel_cfg.OversampleH = 1;
			pixel_cfg.PixelSnapH = true;
			loaded_font = io.Fonts->AddFontFromMemoryCompressedTTF(
				AmigaTopaz_compressed_data, AmigaTopaz_compressed_size, font_load_px, &pixel_cfg);
		} else {
			const std::string font_path = std::filesystem::path(font_file).is_absolute()
				? font_file
				: prefix_with_data_path(font_file);
			if (!font_path.empty() && std::filesystem::exists(font_path)) {
				loaded_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_load_px);
			}
			if (!loaded_font) {
				ImFontConfig pixel_cfg;
				pixel_cfg.OversampleH = 1;
				pixel_cfg.PixelSnapH = true;
				loaded_font = io.Fonts->AddFontFromMemoryCompressedTTF(
					AmigaTopaz_compressed_data, AmigaTopaz_compressed_size, font_load_px, &pixel_cfg);
			}
		}
		if (!loaded_font) {
			ImFontConfig fallback_cfg;
			fallback_cfg.SizePixels = font_load_px;
			loaded_font = io.Fonts->AddFontDefault(&fallback_cfg);
		}

		// Merge icon font
		{
			ImFontConfig icons_cfg;
			icons_cfg.MergeMode = true;
			icons_cfg.PixelSnapH = true;
			icons_cfg.GlyphMinAdvanceX = font_load_px;
			static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			io.Fonts->AddFontFromMemoryCompressedTTF(
				fa_solid_900_compressed_data, fa_solid_900_compressed_size,
				font_load_px, &icons_cfg, icon_ranges);
		}

		io.FontDefault = loaded_font;
		s_loaded_font_file = font_file;
	}
}

#endif

// Helper: get usable bounds for a display index (fallback to full bounds)
static SDL_Rect get_display_usable_bounds(SDL_DisplayID display_id)
{
	SDL_Rect bounds{0,0,0,0};
	if (!display_id)
		display_id = SDL_GetPrimaryDisplay();
	if (!SDL_GetDisplayUsableBounds(display_id, &bounds)) {
		// Fallback if usable bounds fail
		SDL_GetDisplayBounds(display_id, &bounds);
	}
	return bounds;
}

// Helper: find the display index that contains the rect (by its topleft); fallback to primary
static SDL_DisplayID find_display_for_rect(const SDL_Rect& rect)
{
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (displays) {
		for (int i = 0; i < count; ++i) {
			SDL_Rect b = get_display_usable_bounds(displays[i]);
			if (rect.x >= b.x && rect.x < b.x + b.w && rect.y >= b.y && rect.y < b.y + b.h) {
				SDL_DisplayID result = displays[i];
				SDL_free(displays);
				return result;
			}
		}
		SDL_free(displays);
	}
	return SDL_GetPrimaryDisplay();
}

// Helper: clamp rect position (and optionally size) into display usable bounds; returns true if changed
static bool clamp_rect_to_bounds(SDL_Rect& rect, const SDL_Rect& bounds, bool clamp_size)
{
	bool changed = false;
	SDL_Rect r = rect;
	if (clamp_size) {
		if (r.w > bounds.w) { r.w = bounds.w; changed = true; }
		if (r.h > bounds.h) { r.h = bounds.h; changed = true; }
	}
	if (r.x < bounds.x) { r.x = bounds.x; changed = true; }
	if (r.y < bounds.y) { r.y = bounds.y; changed = true; }
	if (r.x + r.w > bounds.x + bounds.w) { r.x = bounds.x + bounds.w - r.w; changed = true; }
	if (r.y + r.h > bounds.y + bounds.h) { r.y = bounds.y + bounds.h - r.h; changed = true; }
	if (changed) rect = r;
	return changed;
}

/*
* SDL Stuff we need
*/
SDL_Joystick* gui_joystick;
SDL_Surface* gui_screen;
SDL_Event gui_event;
SDL_Event touch_event;
SDL_Texture* gui_texture;
SDL_Rect gui_renderQuad;
SDL_Rect gui_window_rect{0, 0, GUI_WIDTH, GUI_HEIGHT};

static bool gui_window_size_initialized = false;

/* Flag for changes in rtarea:
  Bit 0: any HD in config?
  Bit 1: force because add/remove HD was clicked or new config loaded
  Bit 2: socket_emu on
  Bit 3: mousehack on
  Bit 4: rtgmem on
  Bit 5: chipmem larger than 2MB
  
  gui_rtarea_flags_onenter is set before GUI is shown, bit 1 may change during GUI display.
*/
static int gui_rtarea_flags_onenter;

static int gui_create_rtarea_flag(uae_prefs* p)
{
	auto flag = 0;

	if (p->mountitems > 0)
		flag |= 1;

	// We allow on-the-fly switching of this in Amiberry
#ifndef AMIBERRY
	if (p->input_tablet > 0)
		flag |= 8;
#endif

	return flag;
}

std::string get_json_timestamp(const std::string& json_filename)
{
	std::ifstream f(json_filename);
	if (!f.is_open())
		return "";

	try
	{
		// Read only enough to find the timestamp fields near the top
		// Parse the full file since nlohmann doesn't support partial parsing easily
		nlohmann::json doc = nlohmann::json::parse(f, nullptr, false);
		f.close();
		if (doc.is_discarded())
			return "";

		// Prefer upstream_timestamp, fall back to generated
		if (doc.contains("upstream_timestamp") && doc["upstream_timestamp"].is_string())
			return doc["upstream_timestamp"].get<std::string>();
		if (doc.contains("generated") && doc["generated"].is_string())
			return doc["generated"].get<std::string>();
	}
	catch (...)
	{
		// Parse failed
	}
	return "";
}

std::string get_xml_timestamp(const std::string& xml_filename)
{
	std::string result;
	tinyxml2::XMLDocument doc;
	auto error = false;

	auto* f = fopen(xml_filename.c_str(), _T("rb"));
	if (f)
	{
		auto err = doc.LoadFile(f);
		if (err != tinyxml2::XML_SUCCESS)
		{
			write_log(_T("Failed to parse '%s':  %d\n"), xml_filename.c_str(), err);
			error = true;
		}
		fclose(f);
	}
	else
	{
		error = true;
	}
	if (!error)
	{
		auto* whdbooter = doc.FirstChildElement("whdbooter");
		result = whdbooter->Attribute("timestamp");
	}
	if (!result.empty())
		return result;
	return "";
}

void gui_force_rtarea_hdchange()
{
	gui_rtarea_flags_onenter |= 2;
}

struct archive_disk_scan_state {
	std::vector<std::string> images;
};

static int archive_disk_scan_cb(struct zfile *f, void *user)
{
	auto *state = static_cast<archive_disk_scan_state *>(user);
	if (state->images.size() >= MAX_SPARE_DRIVES)
		return 1;
	int type = zfile_gettype(f);
	const TCHAR *name = zfile_getname(f);
	if (name && (type == ZFILE_DISKIMAGE || type == ZFILE_EXECUTABLE)) {
		state->images.emplace_back(name);
	}
	return 0;
}

int populate_diskswapper_from_archive(const TCHAR *path, struct uae_prefs *prefs)
{
	archive_disk_scan_state state;
	if (!zfile_zopen(std::string(path), archive_disk_scan_cb, &state))
		return 0;
	if (state.images.size() <= 1)
		return 0;

	std::sort(state.images.begin(), state.images.end());

	for (int i = 0; i < MAX_SPARE_DRIVES; i++)
		prefs->dfxlist[i][0] = 0;

	int count = std::min(static_cast<int>(state.images.size()), MAX_SPARE_DRIVES);
	for (int i = 0; i < count; i++) {
		_tcsncpy(prefs->dfxlist[i], state.images[i].c_str(), MAX_DPATH - 1);
		prefs->dfxlist[i][MAX_DPATH - 1] = '\0';
	}
	return count;
}

int disk_in_drive(int entry)
{
	for (int i = 0; i < 4; i++) {
		if (_tcslen(changed_prefs.dfxlist[entry]) > 0 && !_tcscmp(changed_prefs.dfxlist[entry], changed_prefs.floppyslots[i].df))
			return i;
	}
	return -1;
}

int disk_swap(int entry, int mode)
{
	int drv, i, drvs[4] = { -1, -1, -1, -1 };

	for (i = 0; i < MAX_SPARE_DRIVES; i++) {
		drv = disk_in_drive(i);
		if (drv >= 0)
			drvs[drv] = i;
	}
	if ((drv = disk_in_drive(entry)) >= 0) {
		if (mode < 0) {
			changed_prefs.floppyslots[drv].df[0] = 0;
			return 1;
		}

		if (_tcscmp(changed_prefs.floppyslots[drv].df, currprefs.floppyslots[drv].df)) {
			_tcscpy(changed_prefs.floppyslots[drv].df, currprefs.floppyslots[drv].df);
			disk_insert(drv, changed_prefs.floppyslots[drv].df);
		}
		else {
			changed_prefs.floppyslots[drv].df[0] = 0;
		}
		if (drvs[0] < 0 || drvs[1] < 0 || drvs[2] < 0 || drvs[3] < 0) {
			drv++;
			while (drv < 4 && drvs[drv] >= 0)
				drv++;
			if (drv < 4 && changed_prefs.floppyslots[drv].dfxtype >= 0) {
				_tcscpy(changed_prefs.floppyslots[drv].df, changed_prefs.dfxlist[entry]);
				disk_insert(drv, changed_prefs.floppyslots[drv].df);
			}
		}
		return 1;
	}
	for (i = 0; i < 4; i++) {
		if (drvs[i] < 0 && changed_prefs.floppyslots[i].dfxtype >= 0) {
			_tcscpy(changed_prefs.floppyslots[i].df, changed_prefs.dfxlist[entry]);
			disk_insert(i, changed_prefs.floppyslots[i].df);
			return 1;
		}
	}
	_tcscpy(changed_prefs.floppyslots[0].df, changed_prefs.dfxlist[entry]);
	disk_insert(0, changed_prefs.floppyslots[0].df);
	return 1;
}

void gui_restart()
{
	gui_running = false;
}

#ifdef USE_IMGUI
// IMGUI runtime state and helpers
static bool show_message_box = false;
static char message_box_title[128] = {0};
static std::string message_box_message;
static bool start_disabled = false; // Added for disable_resume logic

// Helper to draw Amiga 3D bevels
// recessed = true (In), recessed = false (Out)
void AmigaBevel(const ImVec2 min, const ImVec2 max, const bool recessed)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImU32 col_shine = ImGui::GetColorU32(ImGuiCol_BorderShadow); // Usually White
	const ImU32 col_shadow = ImGui::GetColorU32(ImGuiCol_Border);      // Usually Dark Gray

	ImU32 top_left = recessed ? col_shadow : col_shine;
	ImU32 bottom_right = recessed ? col_shine : col_shadow;

	// Top
	draw_list->AddLine(ImVec2(min.x, min.y), ImVec2(max.x - 1, min.y), top_left, 1.0f);
	// Left
	draw_list->AddLine(ImVec2(min.x, min.y), ImVec2(min.x, max.y - 1), top_left, 1.0f);
	// Bottom
	draw_list->AddLine(ImVec2(min.x, max.y - 1), ImVec2(max.x - 1, max.y - 1), bottom_right, 1.0f);
	// Right
	draw_list->AddLine(ImVec2(max.x - 1, min.y), ImVec2(max.x - 1, max.y - 1), bottom_right, 1.0f);
}

void AmigaCircularBevel(const ImVec2 center, const float radius, const bool recessed)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImU32 col_shine = ImGui::GetColorU32(ImGuiCol_BorderShadow); // Usually White
	const ImU32 col_shadow = ImGui::GetColorU32(ImGuiCol_Border);      // Usually Dark Gray

	ImU32 col_top_left = recessed ? col_shadow : col_shine;
	ImU32 col_bottom_right = recessed ? col_shine : col_shadow;

	// Top-Left Arc (135 deg to 315 deg, covering Left and Top)
	// 3PI/4 (135) to 7PI/4 (315)
	const float a_min_tl = 3.0f * IM_PI / 4.0f;
	const float a_max_tl = 7.0f * IM_PI / 4.0f;

	draw_list->PathArcTo(center, radius, a_min_tl, a_max_tl);
	draw_list->PathStroke(col_top_left, 0, 1.0f);

	// Bottom-Right Arc (315 deg to 135 deg, covering Right and Bottom)
	// -PI/4 (-45) to 3PI/4 (135)
	draw_list->PathArcTo(center, radius, -IM_PI / 4.0f, 3.0f * IM_PI / 4.0f);
	draw_list->PathStroke(col_bottom_right, 0, 1.0f);
}

static bool g_groupbox_collapsed = false;
static bool g_groupbox_collapsible = false;

bool BeginGroupBox(const char* name, bool collapsible)
{
	// Add breathing room between consecutive group boxes.
	// Skip the extra spacing when we are near the top of the panel
	// (first group box) so we don't waste space above it.
	const float cursor_y = ImGui::GetCursorPosY();
	if (cursor_y > ImGui::GetTextLineHeightWithSpacing() * 2.0f)
		ImGui::Dummy(ImVec2(0.0f, 4.0f));

	ImGui::BeginGroup();
	ImGui::PushID(name);

	g_groupbox_collapsed = false;
	g_groupbox_collapsible = collapsible;
	if (collapsible) {
		ImGuiID id = ImGui::GetID("##collapse");
		ImGuiStorage* storage = ImGui::GetStateStorage();
		bool open = storage->GetBool(id, true);
		const char* arrow = open ? ICON_FA_CHEVRON_DOWN : ICON_FA_CHEVRON_RIGHT;
		std::string toggle_label = std::string(arrow) + " " + name;
		if (ImGui::Selectable(toggle_label.c_str(), false, 0, ImVec2(0, ImGui::GetTextLineHeightWithSpacing()))) {
			open = !open;
			storage->SetBool(id, open);
		}
		if (!open) {
			g_groupbox_collapsed = true;
			return false;
		}
	}

	if (collapsible) {
		// Gap between chevron header and border/content
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
	} else {
		// Space reserved for the title text drawn by EndGroupBox
		ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() + 2.0f));
	}
	ImGui::Indent(10.0f);
	return true;
}

void EndGroupBox(const char* name)
{
	if (g_groupbox_collapsed) {
		ImGui::PopID();
		ImGui::EndGroup();
		g_groupbox_collapsed = false;
		g_groupbox_collapsible = false;
		return;
	}

	const bool was_collapsible = g_groupbox_collapsible;
	g_groupbox_collapsible = false;

	ImGui::Unindent(10.0f);
	ImGui::PopID();
	ImGui::EndGroup();

	constexpr float text_padding = 8.0f;
	constexpr float box_padding = 4.0f;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 item_min = ImGui::GetItemRectMin();
	ImVec2 item_max = ImGui::GetItemRectMax();

	const float text_height = ImGui::GetTextLineHeight();

	// For collapsible groups the Selectable already rendered the title,
	// so the border starts below that row instead of at the text midpoint.
	const float border_y = was_collapsible
		? item_min.y + ImGui::GetTextLineHeightWithSpacing() + 4.0f
		: item_min.y + text_height * 0.5f;

	ImU32 shadow_col = ImGui::GetColorU32(ImGuiCol_Border);
	ImU32 shine_col = ImGui::GetColorU32(ImGuiCol_BorderShadow);
	ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

	ImVec2 text_size = ImGui::CalcTextSize(name);
	float text_start_x = item_min.x + text_padding;
	item_min.x -= box_padding;

	ImVec2 content_max = ImGui::GetWindowContentRegionMax();
	ImVec2 window_pos = ImGui::GetWindowPos();
	float max_x = window_pos.x + content_max.x;

	if (item_max.x + box_padding > max_x)
		item_max.x = max_x;
	else
		item_max.x += box_padding;

	item_max.y += box_padding;

	// Draw Etched Group Border (Double line bevel)
	// Outer Shadow, Inner Shine
	draw_list->AddRect(ImVec2(item_min.x, border_y), ImVec2(item_max.x, item_max.y), shadow_col);
	draw_list->AddRect(ImVec2(item_min.x + 1, border_y + 1), ImVec2(item_max.x + 1, item_max.y + 1), shine_col);

	if (!was_collapsible) {
		// Clear background for text and draw title (non-collapsible only;
		// collapsible groups already show the title via the Selectable toggle)
		draw_list->AddRectFilled(ImVec2(text_start_x - text_padding, border_y - text_height * 0.5f - 1), ImVec2(text_start_x + text_size.x + text_padding, border_y + text_height * 0.5f + 1), ImGui::GetColorU32(ImGuiCol_WindowBg));
		draw_list->AddText(ImVec2(text_start_x, item_min.y), text_col, name);
	}

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
}

void ShowHelpMarker(const char* desc)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

bool AmigaButton(const char* label, const ImVec2& size)
{
	ImGui::PushStyleColor(ImGuiCol_Border, 0); // Hide default border if any
	const bool pressed = ImGui::Button(label, size);
	ImGui::PopStyleColor();

	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const bool active = ImGui::IsItemActive();

	AmigaBevel(min, max, active);

	return pressed;
}

bool AmigaCheckbox(const char* label, bool* v)
{
	// Use ImGui's standard checkbox (checkmark) and only add the Amiga-style bevel.
	const bool changed = ImGui::Checkbox(label, v);

	const float sz = ImGui::GetFrameHeight();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const ImVec2 box_min = pos;
	const ImVec2 box_max = ImVec2(pos.x + sz, pos.y + sz);

	AmigaBevel(box_min, box_max, false);

	return changed;
}

bool AmigaInputText(const char* label, char* buf, const size_t buf_size)
{
	// Recessed frame
	const bool changed = ImGui::InputText(label, buf, buf_size);
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	// InputText frame is slightly smaller than the item rect if there is a label,
	// but here we usually use ## labels.
	// Actually, ImGui::InputText draws the frame.
	AmigaBevel(min, max, true);

	return changed;
}

// Amiga-style radio button wrapper (bool variant)
// Returns true when pressed (matching ImGui::RadioButton semantics).
bool AmigaRadioButton(const char* label, const bool active)
{
	const bool pressed = ImGui::RadioButton(label, active);

	// Draw a circular recessed bevel around the radio indicator
	const float sz = ImGui::GetFrameHeight();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const float radius = (sz * 0.5f) - 0.5f;
	const ImVec2 center = ImVec2(floor(pos.x + sz * 0.5f + 0.5f), floor(pos.y + sz * 0.5f + 0.5f));
	
	AmigaCircularBevel(center, radius, true);

	return pressed;
}

// Amiga-style radio button wrapper (int* variant)
bool AmigaRadioButton(const char* label, int* v, const int v_button)
{
	const bool pressed = ImGui::RadioButton(label, v, v_button);

	// Draw a circular recessed bevel around the radio indicator
	const float sz = ImGui::GetFrameHeight();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const float radius = (sz * 0.5f) - 0.5f;
	const ImVec2 center = ImVec2(floor(pos.x + sz * 0.5f + 0.5f), floor(pos.y + sz * 0.5f + 0.5f));

	AmigaCircularBevel(center, radius, true);

	return pressed;
}

// Sidebar icons cache
struct IconTex { ImTextureID tex; int w; int h; };
static std::unordered_map<std::string, IconTex> g_sidebar_icons;

static void release_sidebar_icons()
{
	for (auto& kv : g_sidebar_icons) {
		if (kv.second.tex != ImTextureID_Invalid) {
			gui_destroy_texture(kv.second.tex);
			kv.second.tex = ImTextureID_Invalid;
		}
	}
	g_sidebar_icons.clear();
}

static void ensure_sidebar_icons_loaded()
{
	for (int i = 0; categories[i].category != nullptr; ++i) {
		if (categories[i].icon) continue;
		if (!categories[i].imagepath) continue;
		std::string key = categories[i].imagepath;
		if (g_sidebar_icons.find(key) != g_sidebar_icons.end())
			continue;
		const auto full = prefix_with_data_path(categories[i].imagepath);
		SDL_Surface* surf = IMG_Load(full.c_str());
		if (!surf) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Sidebar icon load failed: %s: %s", full.c_str(), SDL_GetError());
			g_sidebar_icons.emplace(key, IconTex{ImTextureID_Invalid, 0, 0});
			continue;
		}
		ImTextureID tex = gui_create_texture(surf, nullptr, nullptr);
		IconTex it{ tex, surf->w, surf->h };
		SDL_DestroySurface(surf);
		g_sidebar_icons.emplace(std::move(key), it);
	}
}
#endif

void amiberry_gui_init()
{
	AmigaMonitor* mon = &AMonitors[0];
	sdl_video_driver = SDL_GetCurrentVideoDriver();

	// Initialize gui_window_rect size early so all paths (Windows shared-window,
	// KMSDRM, new-window creation) use the correct dimensions.
	if (!gui_window_size_initialized) {
		const float gui_scale = DPIHandler::get_layout_scale();
		gui_window_rect.w = std::max(GUI_WIDTH, static_cast<int>(std::lround(static_cast<float>(GUI_WIDTH) * gui_scale)));
		gui_window_rect.h = std::max(GUI_HEIGHT, static_cast<int>(std::lround(static_cast<float>(GUI_HEIGHT) * gui_scale)));

		// Override with persisted size if available, clamped to minimum
		int saved_w = 0, saved_h = 0;
		if (regqueryint(nullptr, _T("GUISizeW"), &saved_w) && regqueryint(nullptr, _T("GUISizeH"), &saved_h)) {
			gui_window_rect.w = std::max(GUI_WIDTH, saved_w);
			gui_window_rect.h = std::max(GUI_HEIGHT, saved_h);
			write_log("Restoring GUI window size %dx%d from settings\n", gui_window_rect.w, gui_window_rect.h);
		}

		gui_window_size_initialized = true;
	}

#ifdef __ANDROID__
	if (mon->amiga_window && !mon->gui_window)
		mon->gui_window = mon->amiga_window;
	if (mon->amiga_renderer && !mon->gui_renderer)
		mon->gui_renderer = mon->amiga_renderer;
#endif

#ifdef _WIN32
	// On Windows, reuse the emulation window to avoid dual D3D11/OpenGL context
	// conflicts that cause crashes. This mirrors the Android/KMSDRM pattern.
	if (mon->amiga_window && !mon->gui_window)
		mon->gui_window = mon->amiga_window;
	if (mon->gui_window == mon->amiga_window && g_renderer) {
		g_renderer->prepare_gui_sharing(mon);
	}
	if (mon->gui_window == mon->amiga_window) {
		SDL_GetWindowPosition(mon->gui_window, &saved_emu_x, &saved_emu_y);
		SDL_GetWindowSize(mon->gui_window, &saved_emu_w, &saved_emu_h);
		saved_emu_flags = SDL_GetWindowFlags(mon->gui_window);
		// Exit fullscreen for GUI if needed
		if (saved_emu_flags & SDL_WINDOW_FULLSCREEN)
			SDL_SetWindowFullscreen(mon->gui_window, false);
		// Use saved position if available, otherwise center
		int gui_pos_x = 0, gui_pos_y = 0;
		if (regqueryint(nullptr, _T("GUIPosX"), &gui_pos_x) && regqueryint(nullptr, _T("GUIPosY"), &gui_pos_y)) {
			gui_window_rect.x = gui_pos_x;
			gui_window_rect.y = gui_pos_y;
			const int target_display = find_display_for_rect(gui_window_rect);
			SDL_Rect usable = get_display_usable_bounds(target_display);
			clamp_rect_to_bounds(gui_window_rect, usable, true);
			SDL_SetWindowSize(mon->gui_window, gui_window_rect.w, gui_window_rect.h);
			SDL_SetWindowPosition(mon->gui_window, gui_window_rect.x, gui_window_rect.y);
		} else {
			SDL_SetWindowSize(mon->gui_window, gui_window_rect.w, gui_window_rect.h);
			SDL_SetWindowPosition(mon->gui_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}
		SDL_SetWindowTitle(mon->gui_window, "Amiberry GUI");
		SDL_SetWindowResizable(mon->gui_window, true);
		SDL_SetWindowMouseGrab(mon->gui_window, false);
	}
#endif

#if defined(_WIN32) && defined(USE_OPENGL)
	gui_use_opengl = (g_renderer && g_renderer->has_context() && mon->gui_window == mon->amiga_window);
#endif
#ifdef USE_VULKAN
	gui_use_vulkan = (get_vulkan_renderer() != nullptr && g_renderer->has_context()
		&& mon->gui_window == mon->amiga_window);
#endif

	if (sdl_video_driver != nullptr && strcmpi(sdl_video_driver, "KMSDRM") == 0)
	{
		kmsdrm_detected = true;
		if (!mon->gui_window && mon->amiga_window)
		{
			mon->gui_window = mon->amiga_window;
		}
		if (!mon->gui_renderer && mon->amiga_renderer)
		{
			mon->gui_renderer = mon->amiga_renderer;
		}
		// SDL3: scale quality is per-texture, not a global hint
	}
	{
		const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
		if (dm) sdl_mode = *dm;
	}

	if (!mon->gui_window)
	{
		write_log("Creating Amiberry GUI window...\n");
		int has_x = regqueryint(nullptr, _T("GUIPosX"), &gui_window_rect.x);
		int has_y = regqueryint(nullptr, _T("GUIPosY"), &gui_window_rect.y);
		if (!has_x || !has_y) {
			gui_window_rect.x = SDL_WINDOWPOS_CENTERED;
			gui_window_rect.y = SDL_WINDOWPOS_CENTERED;
		}

		uint32_t mode;
		if (!kmsdrm_detected)
		{
#ifdef __ANDROID__
			mode = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE;
			SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait PortraitUpsideDown");
#else
			// Only enable Windowed mode if we're running under a window environment
			mode = SDL_WINDOW_RESIZABLE;
#endif
		}
		else
		{
			// otherwise go for Full-window (borderless fullscreen)
			mode = SDL_WINDOW_FULLSCREEN;
		}

		if (currprefs.gui_alwaysontop)
			mode |= SDL_WINDOW_ALWAYS_ON_TOP;
		if (currprefs.start_minimized)
			mode |= SDL_WINDOW_HIDDEN;
		// Request native-resolution framebuffer on HiDPI displays.
		// Android: NOT needed — display scaling is handled entirely via layout_scale.
#if !defined(__ANDROID__)
		mode |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif

		mon->gui_window = SDL_CreateWindow("Amiberry GUI",
										   gui_window_rect.w,
										   gui_window_rect.h,
										   mode);
		if (mon->gui_window && kmsdrm_detected) {
			// For KMSDRM borderless fullscreen, use desktop mode
			SDL_SetWindowFullscreenMode(mon->gui_window, NULL);
		}
		if (mon->gui_window) {
			// Sync rect to actual window size (SDL may adjust)
			int ww, wh;
			SDL_GetWindowSize(mon->gui_window, &ww, &wh);
			gui_window_rect.w = ww;
			gui_window_rect.h = wh;

			// Position: use saved position if available, otherwise center
			if (has_x && has_y) {
				// Clamp saved position+size to the target display's usable bounds
				const int target_display = find_display_for_rect(gui_window_rect);
				SDL_Rect usable = get_display_usable_bounds(target_display);
				clamp_rect_to_bounds(gui_window_rect, usable, true);
				if (gui_window_rect.w != ww || gui_window_rect.h != wh)
					SDL_SetWindowSize(mon->gui_window, gui_window_rect.w, gui_window_rect.h);
				SDL_SetWindowPosition(mon->gui_window, gui_window_rect.x, gui_window_rect.y);
			} else {
				// Center on the usable area of the current display (respects taskbar)
				SDL_DisplayID disp = SDL_GetDisplayForWindow(mon->gui_window);
				SDL_Rect usable = get_display_usable_bounds(disp);
				// Clamp size to display if needed
				SDL_Rect clamped = gui_window_rect;
				if (clamp_rect_to_bounds(clamped, usable, true)) {
					if (clamped.w != gui_window_rect.w || clamped.h != gui_window_rect.h)
						SDL_SetWindowSize(mon->gui_window, clamped.w, clamped.h);
					gui_window_rect = clamped;
				}
				int win_w, win_h;
				SDL_GetWindowSize(mon->gui_window, &win_w, &win_h);
				int cx = usable.x + (usable.w - win_w) / 2;
				int cy = usable.y + (usable.h - win_h) / 2;
				SDL_SetWindowPosition(mon->gui_window, cx, cy);
				gui_window_rect.x = cx;
				gui_window_rect.y = cy;
			}
		}

#ifndef __MACH__
		{
			SDL_IOStream* io = SDL_IOFromConstMem(amiberry_png_data, amiberry_png_size);
			auto* const icon_surface = io ? IMG_Load_IO(io, true) : nullptr;
			if (icon_surface != nullptr)
			{
				SDL_SetWindowIcon(mon->gui_window, icon_surface);
				SDL_DestroySurface(icon_surface);
			}
		}
#endif
	}
	else if (kmsdrm_detected)
	{
		// Reset emulation's logical presentation on the shared renderer,
		// otherwise the GUI overflows the smaller Amiga logical space.
		if (mon->gui_renderer) {
			SDL_SetRenderLogicalPresentation(mon->gui_renderer, 0, 0,
				SDL_LOGICAL_PRESENTATION_DISABLED);
		}
	}

{
		bool skip_sdl_renderer = false;
#if defined(_WIN32) && defined(USE_OPENGL)
		if (gui_use_opengl) skip_sdl_renderer = true;
#endif
#ifdef USE_VULKAN
		if (gui_use_vulkan) skip_sdl_renderer = true;
#endif
		if (!skip_sdl_renderer) {
			if (mon->gui_renderer == nullptr)
			{
				mon->gui_renderer = SDL_CreateRenderer(mon->gui_window, NULL);
				check_error_sdl(mon->gui_renderer == nullptr, "Unable to create a renderer:");
				if (mon->gui_renderer)
					SDL_SetRenderVSync(mon->gui_renderer, 1);
			}
			DPIHandler::set_render_scale(mon->gui_renderer);
		}
	}

	file_dialog_init();

#ifdef USE_IMGUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	{
		int touch_count = 0;
		SDL_TouchID* touch_devs = SDL_GetTouchDevices(&touch_count);
		SDL_free(touch_devs);
		if (touch_count > 0)
			io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;  // Optimize for touch input
	}

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	// Ensure WindowMinSize is valid after scaling (avoid ImGui assert)
	if (style.WindowMinSize.x < 1.0f || style.WindowMinSize.y < 1.0f)
		style.WindowMinSize = ImVec2(32.0f, 36.0f);

	// Load GUI theme (for font selection)
	load_theme(amiberry_options.gui_theme);
	// Apply theme colors to ImGui (map GUISAN theme fields)
	apply_imgui_theme();

	float scaling_factor = DPIHandler::get_layout_scale();

	// Apply scaling to GUI constants
	BUTTON_WIDTH = 105 * scaling_factor;
	BUTTON_HEIGHT = 30 * scaling_factor;
	SMALL_BUTTON_WIDTH = 30 * scaling_factor;
	SMALL_BUTTON_HEIGHT = 22 * scaling_factor;
	DISTANCE_BORDER = 10 * scaling_factor;
	DISTANCE_NEXT_X = 15 * scaling_factor;
	DISTANCE_NEXT_Y = 15 * scaling_factor;
	LABEL_HEIGHT = 20 * scaling_factor;
	TEXTFIELD_HEIGHT = 20 * scaling_factor;
	DROPDOWN_HEIGHT = 20 * scaling_factor;
	SLIDER_HEIGHT = 20 * scaling_factor;
	TITLEBAR_HEIGHT = 24 * scaling_factor;
	SELECTOR_WIDTH = 165 * scaling_factor;
	SELECTOR_HEIGHT = 24 * scaling_factor;
	SCROLLBAR_WIDTH = 20 * scaling_factor;
	style.ScaleAllSizes(scaling_factor);

	// Build font atlas at physical pixel resolution for crisp text on HiDPI displays.
	// On macOS Retina (dpi=2.0): atlas at 30px, FontScaleDpi=0.5 → effective 15px logical
	// On Linux 150% (dpi=1.5): atlas at 33.75px, FontScaleDpi=0.667 → effective 22.5px logical
	// On non-HiDPI (dpi=1.0): atlas at 15px, FontScaleDpi=1.0 → unchanged
#ifdef __ANDROID__
	const float font_dpi_scale = 1.0f;
#else
	int win_w = 0, win_h = 0, pix_w = 0, pix_h = 0;
	SDL_GetWindowSize(mon->gui_window, &win_w, &win_h);
	SDL_GetWindowSizeInPixels(mon->gui_window, &pix_w, &pix_h);
	const float font_dpi_scale = (win_w > 0) ? std::max(1.0f, static_cast<float>(pix_w) / static_cast<float>(win_w)) : 1.0f;
#endif
	style.FontScaleDpi = 1.0f / font_dpi_scale;

	const std::string font_file = gui_theme.font_name.empty() ? std::string("AmigaTopaz.ttf") : gui_theme.font_name;
	const bool use_default_font = (font_file == "AmigaTopaz.ttf");
	const float font_px = gui_theme.font_size > 0 ? static_cast<float>(gui_theme.font_size) : 15.0f;
	const float font_load_px = font_px * scaling_factor * font_dpi_scale;

	ImFont* loaded_font = nullptr;
	if (use_default_font) {
		// Load embedded Amiga Topaz with pixel-snapping for crisp rendering
		ImFontConfig pixel_cfg;
		pixel_cfg.OversampleH = 1;
		pixel_cfg.PixelSnapH = true;
		loaded_font = io.Fonts->AddFontFromMemoryCompressedTTF(
			AmigaTopaz_compressed_data, AmigaTopaz_compressed_size, font_load_px, &pixel_cfg);
	} else {
		// User-configured custom font: load from filesystem
		// Absolute paths are used directly; relative names go through the data-path prefix
		const std::string font_path = std::filesystem::path(font_file).is_absolute()
			? font_file
			: prefix_with_data_path(font_file);
		if (!font_path.empty() && std::filesystem::exists(font_path)) {
			loaded_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_load_px);
		}
		if (!loaded_font) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				"ImGui: failed to load font '%s', falling back to embedded Amiga Topaz", font_path.c_str());
			ImFontConfig pixel_cfg;
			pixel_cfg.OversampleH = 1;
			pixel_cfg.PixelSnapH = true;
			loaded_font = io.Fonts->AddFontFromMemoryCompressedTTF(
				AmigaTopaz_compressed_data, AmigaTopaz_compressed_size, font_load_px, &pixel_cfg);
		}
	}
	if (!loaded_font) {
		ImFontConfig fallback_cfg;
		fallback_cfg.SizePixels = font_load_px;
		loaded_font = io.Fonts->AddFontDefault(&fallback_cfg);
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ImGui: all font loading failed, using ImGui default");
	}

	// Merge Font Awesome icon font (embedded, no file dependency)
	{
		ImFontConfig icons_cfg;
		icons_cfg.MergeMode = true;
		icons_cfg.PixelSnapH = true;
		icons_cfg.GlyphMinAdvanceX = font_load_px; // monospace-width icons
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromMemoryCompressedTTF(
			fa_solid_900_compressed_data, fa_solid_900_compressed_size,
			font_load_px, &icons_cfg, icon_ranges);
	}

	io.FontDefault = loaded_font;
	s_loaded_font_file = font_file;

	// Setup Platform/Renderer backends
#ifdef USE_VULKAN
	if (gui_use_vulkan) {
		auto* vk = get_vulkan_renderer();
		if (vk && vk->has_context()) {
			if (!vk->get_imgui_descriptor_pool()) {
				vk->create_imgui_descriptor_pool();
			}
			ImGui_ImplSDL3_InitForVulkan(mon->gui_window);
			ImGui_ImplVulkan_InitInfo init_info{};
			init_info.Instance = vk->get_vk_instance();
			init_info.PhysicalDevice = vk->get_vk_physical_device();
			init_info.Device = vk->get_vk_device();
			init_info.QueueFamily = vk->get_vk_graphics_queue_family();
			init_info.Queue = vk->get_vk_graphics_queue();
			init_info.DescriptorPool = vk->get_imgui_descriptor_pool();
			init_info.MinImageCount = vk->get_vk_min_image_count();
			init_info.ImageCount = vk->get_vk_image_count();
			init_info.PipelineInfoMain.RenderPass = vk->get_vk_render_pass();
			init_info.PipelineInfoMain.Subpass = 0;
			init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			ImGui_ImplVulkan_Init(&init_info);
			vk->set_imgui_init_image_count(init_info.ImageCount);
		}
	} else
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl) {
		auto* gl_renderer = dynamic_cast<OpenGLRenderer*>(g_renderer.get());
		SDL_GLContext ctx = gl_renderer ? gl_renderer->get_gl_context() : nullptr;
		SDL_GL_MakeCurrent(mon->gui_window, ctx);
		ImGui_ImplSDL3_InitForOpenGL(mon->gui_window, ctx);
		ImGui_ImplOpenGL3_Init("#version 130");
	} else
#endif
	{
		AmigaMonitor* mon0 = &AMonitors[0];
		ImGui_ImplSDL3_InitForSDLRenderer(mon0->gui_window, mon0->gui_renderer);
		ImGui_ImplSDLRenderer3_Init(mon0->gui_renderer);
	}

	if (amiberry_options.quickstart_start && !emulating && strlen(last_loaded_config) == 0)
	{
		Quickstart_ApplyDefaults();
	}
#endif

	// Quickstart models and configs initialization
	qs_models.clear();
	for (const auto & amodel : amodels) {
		qs_models.push_back(amodel.name);
	}
	qs_configs.clear();
	for (const auto & config : amodels[quickstart_model].configs) {
		qs_configs.push_back(config);
	}
}

void amiberry_gui_halt()
{
	AmigaMonitor* mon = &AMonitors[0];
	file_dialog_shutdown();

#ifdef USE_IMGUI
	// Release any About panel resources
	if (about_logo_texture != ImTextureID_Invalid) {
		gui_destroy_texture(about_logo_texture);
		about_logo_texture = ImTextureID_Invalid;
	}
	// Release sidebar icon textures
	release_sidebar_icons();
	// Properly shutdown ImGui backends/context
#ifdef USE_VULKAN
	if (gui_use_vulkan) {
		auto* vk = get_vulkan_renderer();
		if (vk) vkDeviceWaitIdle(vk->get_vk_device());
		ImGui_ImplVulkan_Shutdown();
	} else
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
	if (gui_use_opengl)
		ImGui_ImplOpenGL3_Shutdown();
	else
#endif
	{
		ImGui_ImplSDLRenderer3_Shutdown();
	}
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	// Restore SDL's default so clicks used to refocus the emulation window
	// are not forwarded after closing the GUI (see #1871).
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "0");
#endif

	if (gui_screen != nullptr)
	{
		SDL_DestroySurface(gui_screen);
		gui_screen = nullptr;
	}
	if (gui_texture != nullptr)
	{
		SDL_DestroyTexture(gui_texture);
		gui_texture = nullptr;
	}
	if (mon->gui_renderer && !kmsdrm_detected)
	{
#if defined(__ANDROID__)
		// Don't destroy the renderer on Android, as we reuse it
#elif defined(_WIN32)
		if (mon->gui_renderer == mon->amiga_renderer) {
			mon->gui_renderer = nullptr;  // Shared — don't destroy
		} else {
			SDL_DestroyRenderer(mon->gui_renderer);  // OpenGL path — separate renderer
			mon->gui_renderer = nullptr;
		}
#else
		if (mon->gui_renderer == SDL_GetRenderer(mon->amiga_window)) {
			mon->gui_renderer = nullptr;
		} else
		{
			SDL_DestroyRenderer(mon->gui_renderer);
			mon->gui_renderer = nullptr;
		}
#endif
	}

	if (mon->gui_window && !kmsdrm_detected) {
		// Always save position and size together so the ini stays consistent.
		// Avoids stale position-only state causing top-left placement on first
		// launch after upgrading from a version that didn't persist size.
		regsetint(nullptr, _T("GUIPosX"), gui_window_rect.x);
		regsetint(nullptr, _T("GUIPosY"), gui_window_rect.y);
		regsetint(nullptr, _T("GUISizeW"), gui_window_rect.w);
		regsetint(nullptr, _T("GUISizeH"), gui_window_rect.h);
#if defined(__ANDROID__)
		// Don't destroy the window on Android, as we reuse it
#elif defined(_WIN32)
		if (mon->gui_window == mon->amiga_window) {
			mon->gui_window = nullptr;  // Shared — don't destroy
		} else {
			SDL_DestroyWindow(mon->gui_window);
			mon->gui_window = nullptr;
		}
#else
		SDL_DestroyWindow(mon->gui_window);
		mon->gui_window = nullptr;
#endif
	}

	if (kmsdrm_detected && mon->gui_window && mon->gui_window == mon->amiga_window) {
		mon->pending_fullwindow_refresh = true;
	}

#ifdef _WIN32
	// Restore emulation window state
	if (mon->amiga_window && saved_emu_w > 0) {
		SDL_SetWindowTitle(mon->amiga_window, "Amiberry");
		SDL_SetWindowSize(mon->amiga_window, saved_emu_w, saved_emu_h);
		SDL_SetWindowPosition(mon->amiga_window, saved_emu_x, saved_emu_y);
		if (saved_emu_flags & SDL_WINDOW_FULLSCREEN)
			SDL_SetWindowFullscreen(mon->amiga_window, true);
		saved_emu_w = saved_emu_h = 0;
	}
	// Restore emulation rendering context (GL: MakeCurrent + reset; Vulkan: wait idle; SDL: no-op)
	if (gui_use_opengl && mon->amiga_window && g_renderer && g_renderer->has_context()) {
		g_renderer->restore_emulation_context(mon->amiga_window);
	}
#endif
#ifdef USE_VULKAN
	if (gui_use_vulkan && mon->amiga_window && g_renderer && g_renderer->has_context()) {
		g_renderer->restore_emulation_context(mon->amiga_window);
	}
#endif
}

std::string get_filename_extension(const TCHAR* filename);

static void handle_drop_file_event(const SDL_Event& event)
{
	const char* dropped_file = event.drop.data;
	const auto ext = get_filename_extension(dropped_file);

	if (strcasecmp(ext.c_str(), ".uae") == 0)
	{
		// Load configuration file
		uae_restart(&currprefs, -1, dropped_file);
		gui_running = false;
	}
	else if (strcasecmp(ext.c_str(), ".adf") == 0 || strcasecmp(ext.c_str(), ".adz") == 0 || strcasecmp(ext.c_str(), ".dms") == 0 || strcasecmp(ext.c_str(), ".ipf") == 0 || strcasecmp(ext.c_str(), ".zip") == 0)
	{
		// Insert floppy image
		disk_insert(0, dropped_file);
	}
	else if (strcasecmp(ext.c_str(), ".lha") == 0)
	{
		// WHDLoad archive
		whdload_auto_prefs(&currprefs, dropped_file);
		uae_restart(&currprefs, -1, nullptr);
		gui_running = false;
	}
	else if (strcasecmp(ext.c_str(), ".cue") == 0 || strcasecmp(ext.c_str(), ".iso") == 0 || strcasecmp(ext.c_str(), ".chd") == 0)
	{
		// CD image
		cd_auto_prefs(&currprefs, const_cast<char*>(dropped_file));
		uae_restart(&currprefs, -1, nullptr);
		gui_running = false;
	}
}

#ifdef USE_IMGUI
// IMGUI runtime state and helpers

static bool show_disk_info = false;
static char disk_info_title[128] = {0};
static std::vector<std::string> disk_info_text;
static bool open_legacy_cleanup_popup = false;
static std::vector<std::string> legacy_cleanup_text;
static std::string legacy_cleanup_error_message;

void ShowMessageBox(const char* title, const char* message)
{
    // Safely copy and ensure null-termination
    strncpy(message_box_title, title ? title : "Message", sizeof(message_box_title) - 1);
    message_box_title[sizeof(message_box_title) - 1] = '\0';
    message_box_message = message ? message : "";
    show_message_box = true;
}

void ShowDiskInfo(const char* title, const std::vector<std::string>& text)
{
    strncpy(disk_info_title, title ? title : "Disk Info", sizeof(disk_info_title) - 1);
    disk_info_title[sizeof(disk_info_title) - 1] = '\0';
    disk_info_text = text;
    show_disk_info = true;
}

// Provide IMGUI categories using centralized panel list
ConfigCategory categories[] = {
#define PANEL(id, label, img, ico) { label, img, ico, render_panel_##id, help_text_##id },
	IMGUI_PANEL_LIST
#undef PANEL
	{ nullptr, nullptr, nullptr, nullptr, nullptr }
};

void disable_resume()
{
	if (emulating)
	{
		start_disabled = true;
	}
}

void run_gui()
{
	gui_running = true;
	const AmigaMonitor* mon = &AMonitors[0];

	amiberry_gui_init();
	start_disabled = false;

	{
		std::string migration_title;
		std::string migration_message;
		if (consume_startup_migration_notice(migration_title, migration_message))
			ShowMessageBox(migration_title.c_str(), migration_message.c_str());
	}

	open_legacy_cleanup_popup = should_show_legacy_cleanup_prompt();
	legacy_cleanup_text = get_legacy_cleanup_prompt_items();
	if (open_legacy_cleanup_popup)
		legacy_cleanup_error_message.clear();

	if (!emulating && amiberry_options.quickstart_start)
		last_active_panel = 2;

	// Main loop
	while (gui_running) {
		while (SDL_PollEvent(&gui_event)) {
			// Make sure ImGui sees all events
			ImGui_ImplSDL3_ProcessEvent(&gui_event);

			if (ControllerMap_HandleEvent(gui_event)) {
				continue;
			}

			if (gui_event.type == SDL_EVENT_QUIT)	{
				uae_quit();
				gui_running = false;
			}
			else if (gui_event.type == SDL_EVENT_KEY_DOWN && !ImGui::GetIO().WantTextInput) {
				if (gui_event.key.key == SDLK_F1) {
					// Show Help when F1 is pressed
					const char* help_ptr = nullptr;
					if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr)
						help_ptr = categories[last_active_panel].HelpText;
					if (help_ptr && *help_ptr)
						ShowMessageBox("Help", help_ptr);
				}
				else if (gui_event.key.key == SDLK_Q) {
					// Quit when Q is pressed
					uae_quit();
					gui_running = false;
				}
				else if (gui_event.key.key == SDLK_F12 && !start_disabled) {
					if (emulating) {
						gui_running = false;
					}
					else {
						uae_reset(0, 1);
						gui_running = false;
					}
				}
			}
			else if (gui_event.type == SDL_EVENT_DROP_FILE) {
				// Handle dropped files
				handle_drop_file_event(gui_event);
			}
			else if (gui_event.type == SDL_EVENT_JOYSTICK_ADDED
				|| gui_event.type == SDL_EVENT_JOYSTICK_REMOVED) {
				handle_joy_device_event(gui_event.jdevice.which,
					gui_event.type == SDL_EVENT_JOYSTICK_REMOVED, &changed_prefs);
			}
			else if (gui_event.type == SDL_EVENT_WINDOW_MOVED
				&& gui_event.window.windowID == SDL_GetWindowID(mon->gui_window)) {
				gui_window_rect.x = gui_event.window.data1;
				gui_window_rect.y = gui_event.window.data2;
				// Clamp move to current display usable bounds
				{
					SDL_DisplayID disp = SDL_GetDisplayForWindow(mon->gui_window);
					SDL_Rect usable = get_display_usable_bounds(disp);
					if (clamp_rect_to_bounds(gui_window_rect, usable, false)) {
						SDL_SetWindowPosition(mon->gui_window, gui_window_rect.x, gui_window_rect.y);
					}
				}

			}
			else if (gui_event.type == SDL_EVENT_WINDOW_RESIZED
				&& gui_event.window.windowID == SDL_GetWindowID(mon->gui_window)) {
				gui_window_rect.w = gui_event.window.data1;
				gui_window_rect.h = gui_event.window.data2;

			}
			else if (gui_event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
				&& gui_event.window.windowID == SDL_GetWindowID(mon->gui_window)) {
				gui_running = false;
			}
			// Touch-drag scrolling: convert finger swipes into ImGui scroll wheel events
			// SDL synthesizes mouse events from touch, but not mouse-wheel, so ImGui
			// child windows won't scroll from finger drags without this.
			else if (gui_event.type == SDL_EVENT_FINGER_DOWN) {
				touch_scroll_finger = gui_event.tfinger.fingerID;
				touch_scrolling = false;
				touch_scroll_on_widget = false;
				touch_scroll_accum = 0.0f;
			}
			else if (gui_event.type == SDL_EVENT_FINGER_MOTION
				&& gui_event.tfinger.fingerID == touch_scroll_finger
				&& !touch_scroll_on_widget) {
				int wh = 0;
				SDL_GetWindowSize(mon->gui_window, nullptr, &wh);
				const float dy_pixels = gui_event.tfinger.dy * static_cast<float>(wh);
				touch_scroll_accum += dy_pixels;

				// Dead-zone: require a minimum drag distance before scrolling,
				// so taps on buttons don't accidentally scroll
				const float drag_threshold = 8.0f * DPIHandler::get_layout_scale();
				if (!touch_scrolling && std::abs(touch_scroll_accum) > drag_threshold) {
					// Check if the finger is on a scrollbar — if so, let ImGui
					// handle the drag natively. We check here (not on FINGER_DOWN)
					// because on touch screens HoveredWindow is null/stale until
					// at least one frame renders with the touch position.
					ImGuiContext& g = *ImGui::GetCurrentContext();
					if (g.HoveredWindow) {
						const ImVec2 mouse_pos = g.IO.MousePos;
						for (int axis = 0; axis < 2; axis++) {
							if ((axis == ImGuiAxis_Y && g.HoveredWindow->ScrollbarY) ||
								(axis == ImGuiAxis_X && g.HoveredWindow->ScrollbarX)) {
								ImRect sb = ImGui::GetWindowScrollbarRect(g.HoveredWindow, (ImGuiAxis)axis);
								if (sb.Contains(mouse_pos)) {
									touch_scroll_on_widget = true;
									break;
								}
							}
						}
					}
					if (touch_scroll_on_widget)
						continue;

					touch_scrolling = true;
					// Cancel the synthetic mouse-down so ImGui doesn't treat
					// the ongoing touch as a widget click-drag
					ImGui::GetIO().AddMouseButtonEvent(0, false);
				}
				if (touch_scrolling) {
					// ImGui scrolls 5*FontRefSize pixels per 1.0 wheel unit.
					// Divide by that to get 1:1 finger-to-content movement.
					const float scroll_step = 5.0f * ImGui::GetFontSize();
					if (scroll_step > 0.0f) {
						const float scroll_units = dy_pixels / scroll_step;
						ImGui::GetIO().AddMouseWheelEvent(0.0f, scroll_units);
					}
				}
			}
			else if (gui_event.type == SDL_EVENT_FINGER_UP
				&& gui_event.tfinger.fingerID == touch_scroll_finger) {
				touch_scrolling = false;
				touch_scroll_on_widget = false;
			}
		}

#ifdef __ANDROID__
		// Toggle native soft keyboard based on ImGui's text input state.
		// ImGui sets WantTextInput when a text field is active.
		{
			bool want_text = ImGui::GetIO().WantTextInput;
			bool text_active = SDL_TextInputActive(mon->gui_window);
			if (want_text && !text_active)
				SDL_StartTextInput(mon->gui_window);
			else if (!want_text && text_active)
				SDL_StopTextInput(mon->gui_window);
		}
#endif

		// Skip rendering when minimized to save CPU
		if (SDL_GetWindowFlags(mon->gui_window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
#ifdef USE_VULKAN
		if (gui_use_vulkan)
			ImGui_ImplVulkan_NewFrame();
		else
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
		if (gui_use_opengl)
			ImGui_ImplOpenGL3_NewFrame();
		else
#endif
		{
			ImGui_ImplSDLRenderer3_NewFrame();
		}

		ImGui_ImplSDL3_NewFrame();

		// Apply deferred font changes after SDL3 NewFrame (window size known)
		// but before ImGui::NewFrame() (which reads the font atlas).
		apply_pending_font_rebuild();

		ImGui::NewFrame();

		// While touch-scrolling, suppress hover highlight so items don't
		// visually react to the finger position during a scroll gesture
		if (touch_scrolling) {
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		// Compute button bar height from style
		const auto button_bar_height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;

		// Make the main window occupies the full SDL window (no smaller inner window) and set its size
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 work_pos = vp->WorkPos;
		ImVec2 work_size = vp->WorkSize;
		ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(work_size, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::Begin("Amiberry", &gui_running, hostFlags);
		ImGui::PopStyleVar(2);

		const float content_width = ImGui::GetContentRegionAvail().x;
		const float content_height = ImGui::GetContentRegionAvail().y - button_bar_height;
		const float splitter_thickness = (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_IsTouchScreen) ? 16.0f : 6.0f;
		const float min_sidebar = 120.0f;
		const float max_sidebar = content_width * 0.40f;

		static float sidebar_width = 0.0f;
		if (sidebar_width <= 0.0f)
			sidebar_width = content_width * 0.22f;
		sidebar_width = std::clamp(sidebar_width, min_sidebar, max_sidebar);
		float panel_width = content_width - sidebar_width - splitter_thickness;

		// Sidebar
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
		ImGui::BeginChild("Sidebar", ImVec2(sidebar_width - 2.0f, -button_bar_height));
		ImGui::Indent(4.0f);
		ImGui::Dummy(ImVec2(0, 2.0f));

		static char sidebar_filter[64] = "";
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##sidebar_filter", ICON_FA_MAGNIFYING_GLASS " Search...", sidebar_filter, sizeof(sidebar_filter));
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
		ImGui::Dummy(ImVec2(0, 4.0f));

		ensure_sidebar_icons_loaded();

 		const ImGuiStyle& s = ImGui::GetStyle();
		const float base_text_h = ImGui::GetTextLineHeight();
		const float base_row_h = ImGui::GetTextLineHeightWithSpacing();
		// Make icons larger than text height using gui_scale
		const float icon_h_target = base_text_h;
		const float row_h = std::max(base_row_h, icon_h_target + 2.0f * s.FramePadding.y);
		const ImVec4 col_act = rgb_to_vec4(gui_theme.selector_active.r, gui_theme.selector_active.g, gui_theme.selector_active.b);

		struct SidebarGroup { int first_index; const char* label; };
		static const SidebarGroup sidebar_groups[] = {
			{ 0,  "General" },
			{ 4,  "Hardware" },
			{ 9,  "Storage" },
			{ 11, "Expansion" },
			{ 14, "Output" },
			{ 17, "Input / IO" },
			{ 20, "Utility" },
		};
		constexpr int num_groups = sizeof(sidebar_groups) / sizeof(sidebar_groups[0]);
		int next_group = 0;
		const bool has_filter = sidebar_filter[0] != '\0';

		auto icontains = [](const char* haystack, const char* needle) -> bool {
			for (const char* h = haystack; *h; ++h) {
				const char* hi = h;
				const char* ni = needle;
				while (*hi && *ni && tolower((unsigned char)*hi) == tolower((unsigned char)*ni)) { ++hi; ++ni; }
				if (!*ni) return true;
			}
			return false;
		};

		auto group_has_visible = [&](int group_idx) -> bool {
			if (!has_filter) return true;
			int start = sidebar_groups[group_idx].first_index;
			int end = (group_idx + 1 < num_groups) ? sidebar_groups[group_idx + 1].first_index : 999;
			for (int j = start; categories[j].category != nullptr && j < end; ++j) {
				if (icontains(categories[j].category, sidebar_filter))
					return true;
			}
			return false;
		};

		// Keyboard navigation (when filter box is not focused)
		if (!ImGui::GetIO().WantTextInput) {
			int total_panels = 0;
			while (categories[total_panels].category != nullptr) total_panels++;
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
				int next = last_active_panel - 1;
				if (has_filter) {
					while (next >= 0 && !icontains(categories[next].category, sidebar_filter)) --next;
				}
				if (next >= 0) last_active_panel = next;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
				int next = last_active_panel + 1;
				if (has_filter) {
					while (next < total_panels && !icontains(categories[next].category, sidebar_filter)) ++next;
				}
				if (next < total_panels) last_active_panel = next;
			}
		}

		bool any_rendered = false;
		for (int i = 0; categories[i].category != nullptr; ++i) {
			if (has_filter && !icontains(categories[i].category, sidebar_filter))
				continue;

			if (next_group < num_groups && i >= sidebar_groups[next_group].first_index) {
				if (group_has_visible(next_group)) {
					if (any_rendered) {
						ImGui::Dummy(ImVec2(0, 8.0f));
					}
					ImVec4 label_col = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
					label_col.w *= 0.7f;
					ImGui::PushStyleColor(ImGuiCol_Text, label_col);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + s.FramePadding.x + 2.0f);
					ImGui::SetWindowFontScale(0.85f);
					std::string upper_label;
					for (const char* c = sidebar_groups[next_group].label; *c; ++c)
						upper_label += static_cast<char>(toupper(static_cast<unsigned char>(*c)));
					ImGui::TextUnformatted(upper_label.c_str());
					ImGui::SetWindowFontScale(1.0f);
					ImGui::PopStyleColor();
					ImGui::Dummy(ImVec2(0, 2.0f));
				}
				next_group++;
				while (next_group < num_groups && sidebar_groups[next_group].first_index <= i)
					next_group++;
			}
			any_rendered = true;
			ImGui::Indent(6.0f);
			const bool selected = (last_active_panel == i);
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
			if (ImGui::Selectable( (std::string("##cat_") + std::to_string(i)).c_str(), selected, 0, ImVec2(ImGui::GetContentRegionAvail().x, row_h))) {
				last_active_panel = i;
			}
			ImGui::PopStyleColor(3);
			const bool item_hovered = ImGui::IsItemHovered();
			ImVec2 sel_min = ImGui::GetItemRectMin();
			ImVec2 sel_max = ImGui::GetItemRectMax();
			if (selected) {
				ImGui::GetWindowDrawList()->AddRectFilled(sel_min, sel_max,
					ImGui::GetColorU32(col_act), 4.0f);
				ImVec4 accent = lighten(col_act, 0.3f);
				ImGui::GetWindowDrawList()->AddRectFilled(
					ImVec2(sel_min.x, sel_min.y + 2.0f),
					ImVec2(sel_min.x + 3.0f, sel_max.y - 2.0f),
					ImGui::GetColorU32(accent), 1.5f);
			} else if (item_hovered) {
				ImVec4 hover_col = lighten(col_act, 0.05f);
				hover_col.w = 0.3f;
				ImGui::GetWindowDrawList()->AddRectFilled(sel_min, sel_max,
					ImGui::GetColorU32(hover_col), 4.0f);
			}
			ImGui::Unindent(6.0f);
			ImVec2 rmin = ImGui::GetItemRectMin();
			ImVec2 rmax = ImGui::GetItemRectMax();
			ImVec2 pos = rmin;
			float pad_y = s.FramePadding.y;
			float pad_x = s.FramePadding.x;
			float row_total_h = rmax.y - rmin.y;
			float text_h = ImGui::GetTextLineHeight();
			ImDrawList* dl = ImGui::GetWindowDrawList();

			if (categories[i].icon) {
				// Font icon with accent color; white when selected for contrast
				const ImU32 icon_col = selected
					? ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
					: ImGui::GetColorU32(col_act);
				const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
				float icon_y = pos.y + (row_total_h - text_h) * 0.5f;
				ImVec2 icon_size = ImGui::CalcTextSize(categories[i].icon);
				dl->AddText(ImVec2(pos.x + pad_x, icon_y), icon_col, categories[i].icon);
				float text_x = pos.x + pad_x + icon_size.x + pad_x * 2.0f;
				float text_y = pos.y + (row_total_h - text_h) * 0.5f;
				dl->AddText(ImVec2(text_x, text_y), text_col, categories[i].category);
			} else if (categories[i].imagepath) {
				// PNG fallback
				float avail_h = row_total_h - 2.0f * pad_y;
				float icon_h = std::min(icon_h_target, avail_h > 0.0f ? avail_h : (row_h - 2.0f * pad_y));
				float icon_w = icon_h;
				ImTextureID icon_tex = ImTextureID_Invalid;
				int tex_w = 0, tex_h = 0;
				auto it = g_sidebar_icons.find(categories[i].imagepath);
				if (it != g_sidebar_icons.end() && it->second.tex != ImTextureID_Invalid) {
					icon_tex = it->second.tex; tex_w = it->second.w; tex_h = it->second.h;
					if (tex_w > 0 && tex_h > 0) {
						float aspect = (float)tex_w / (float)tex_h;
						icon_w = icon_h * aspect;
					}
				}
				float icon_y = pos.y + (row_total_h - icon_h) * 0.5f;
				ImVec2 icon_p0 = ImVec2(pos.x + pad_x, icon_y);
				ImVec2 icon_p1 = ImVec2(icon_p0.x + icon_w, icon_p0.y + icon_h);
				if (icon_tex != ImTextureID_Invalid) {
					dl->AddImage(icon_tex, icon_p0, icon_p1);
				} else {
					dl->AddRectFilled(icon_p0, icon_p1, ImGui::GetColorU32(ImGuiCol_TextDisabled), 3.0f);
				}
				float text_x = icon_p1.x + pad_x;
				float text_y = pos.y + (row_total_h - text_h) * 0.5f;
				dl->AddText(ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), categories[i].category);
			} else {
				// No icon at all
				float text_y = pos.y + (row_total_h - text_h) * 0.5f;
				dl->AddText(ImVec2(pos.x + pad_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), categories[i].category);
			}
		}
		ImGui::Unindent(4.0f);
		ImGui::EndChild();
		// Draw Sidebar bevel (outside child to avoid clipping)
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		AmigaBevel(ImVec2(min.x - 1, min.y - 1), ImVec2(max.x + 1, max.y + 1), false);

		ImGui::SameLine();

		{
			ImGui::InvisibleButton("##splitter", ImVec2(splitter_thickness, content_height));
			const bool splitter_hovered = ImGui::IsItemHovered();
			const bool splitter_active = ImGui::IsItemActive();

			if (splitter_active)
				sidebar_width += ImGui::GetIO().MouseDelta.x;
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && splitter_hovered)
				sidebar_width = content_width * 0.22f;
			sidebar_width = std::clamp(sidebar_width, min_sidebar, max_sidebar);

			if (splitter_hovered || splitter_active)
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

			ImVec2 smin = ImGui::GetItemRectMin();
			ImVec2 smax = ImGui::GetItemRectMax();
			ImDrawList* sdl = ImGui::GetWindowDrawList();

			if (splitter_hovered || splitter_active) {
				ImU32 bg_col = ImGui::GetColorU32(ImGuiCol_Separator, splitter_active ? 0.4f : 0.2f);
				sdl->AddRectFilled(smin, smax, bg_col);
			}

			ImU32 grip_col = ImGui::GetColorU32(
				splitter_active ? ImGuiCol_SeparatorActive :
				splitter_hovered ? ImGuiCol_SeparatorHovered :
				ImGuiCol_Separator);
			float cx = (smin.x + smax.x) * 0.5f;
			sdl->AddLine(
				ImVec2(cx, smin.y + 4.0f), ImVec2(cx, smax.y - 4.0f),
				grip_col, 2.0f);
		}

		ImGui::SameLine();

		// Content
		ImGui::BeginChild("Content", ImVec2(0, -button_bar_height));

		// Render the currently active panel
		if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr) {
			if (categories[last_active_panel].RenderFunc)
				categories[last_active_panel].RenderFunc();
		}
		ImGui::EndChild();

		// Controller mapping modal (if active)
		ControllerMap_RenderModal();

		ImGui::Dummy(ImVec2(0, 2.0f));

		// Button bar
		// Left-aligned buttons (Shutdown, Reset, Quit, Restart, Help)
		if (!amiberry_options.disable_shutdown_button) {
			if (AmigaButton(ICON_FA_POWER_OFF " Shutdown", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
				uae_quit();
				gui_running = false;
				host_poweroff = true;
			}
			ImGui::SameLine();
		}
		if (AmigaButton(ICON_FA_ROTATE_RIGHT " Reset", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			uae_reset(1, 1);
			gui_running = false;
		}
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_RIGHT_FROM_BRACKET " Quit", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			uae_quit();
			gui_running = false;
		}
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_ARROWS_ROTATE " Restart", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			char tmp[MAX_DPATH] = {0};
			get_configuration_path(tmp, sizeof tmp);
			if (strlen(last_loaded_config) > 0) {
				strncat(tmp, last_loaded_config, MAX_DPATH - 1);
				strncat(tmp, ".uae", MAX_DPATH - 10);
			} else {
				strncat(tmp, OPTIONSFILENAME, MAX_DPATH - 1);
				strncat(tmp, ".uae", MAX_DPATH - 10);
			}
			uae_restart(&changed_prefs, -1, tmp);
			gui_running = false;
		}

		// Right-aligned buttons
		float right_buttons_width = (BUTTON_WIDTH * 2) + style.ItemSpacing.x;
		ImGui::SameLine();
		float cursor_x = ImGui::GetWindowWidth() - right_buttons_width - style.WindowPadding.x;
		if (cursor_x < ImGui::GetCursorPosX()) cursor_x = ImGui::GetCursorPosX(); // Prevent overlap
		ImGui::SetCursorPosX(cursor_x);

		if (start_disabled)
			ImGui::BeginDisabled();

		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, lighten(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive], 0.05f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, lighten(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive], 0.10f));
		if (emulating) {
			if (AmigaButton(ICON_FA_PLAY " Resume", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
				gui_running = false;
			}
		} else {
			if (AmigaButton(ICON_FA_PLAY " Start", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
				uae_reset(0, 1);
				gui_running = false;
			}
		}
		ImGui::PopStyleColor(3);

		if (start_disabled)
			ImGui::EndDisabled();
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_CIRCLE_QUESTION " Help", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
			const char* help_ptr = nullptr;
			if (last_active_panel >= 0 && categories[last_active_panel].category != nullptr && categories[last_active_panel].HelpText != nullptr)
				help_ptr = categories[last_active_panel].HelpText;
			if (help_ptr && *help_ptr)
				ShowMessageBox("Help", help_ptr);
		}

		if (show_message_box) {
            // Center the dialog on appearing; use viewport-relative sizes for proper scaling on all platforms
            const float vw = vp->Size.x;
            const float vh = vp->Size.y;
            const float desired_w = vw * 0.56f;
            ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(desired_w, 0.0f), ImGuiCond_Appearing);
            // Lock width to desired_w; allow height to grow up to 85% viewport
            ImGui::SetNextWindowSizeConstraints(ImVec2(desired_w, 0.0f), ImVec2(desired_w, vh * 0.85f));

            ImGui::OpenPopup(message_box_title);
            if (ImGui::BeginPopupModal(message_box_title, &show_message_box, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                // Scrollable message area; height scales with viewport
                const float max_child_h = vh * 0.65f;
                ImGui::BeginChild("MessageScroll", ImVec2(0, max_child_h), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextWrapped("%s", message_box_message.c_str());
                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::Separator();

                // Align OK button to the right
                const float btn_w = BUTTON_WIDTH;
                const float btn_h = BUTTON_HEIGHT;
                float avail = ImGui::GetContentRegionAvail().x;
                if (avail > btn_w)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w));
                if (AmigaButton("OK", ImVec2(btn_w, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    show_message_box = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsWindowAppearing())
                    ImGui::SetItemDefaultFocus();

                ImGui::EndPopup();
            }
        }

		if (open_legacy_cleanup_popup && !show_message_box && !show_disk_info) {
			const float vw = vp->Size.x;
			const float vh = vp->Size.y;
			const float desired_w = vw * 0.62f;
			ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(desired_w, 0.0f), ImGuiCond_Appearing);
			ImGui::SetNextWindowSizeConstraints(ImVec2(desired_w, 0.0f), ImVec2(desired_w, vh * 0.82f));
			ImGui::OpenPopup("Legacy Cleanup");
			open_legacy_cleanup_popup = false;
		}
		if (ImGui::BeginPopupModal("Legacy Cleanup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			const float vh = vp->Size.y;
			ImGui::TextWrapped("Amiberry found leftover files and folders from the previous layout.");
			ImGui::Spacing();
			ImGui::TextWrapped("Your active paths already use the new locations, so cleaning these up will not change the settings and visuals Amiberry is using now.");
			ImGui::Spacing();
			ImGui::TextWrapped("If everything looks right, Amiberry can move the leftovers %s for safekeeping. You can also keep them for now and decide later.",
				legacy_cleanup_uses_trash() ? "to Trash" : "aside");
			ImGui::Spacing();
			if (!legacy_cleanup_error_message.empty())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.10f, 0.10f, 1.0f));
				ImGui::TextWrapped("%s", legacy_cleanup_error_message.c_str());
				ImGui::PopStyleColor();
				ImGui::Spacing();
			}
			ImGui::Text("Items to clean up:");
			ImGui::BeginChild("LegacyCleanupItems", ImVec2(0, vh * 0.34f), ImGuiChildFlags_Borders);
			for (const auto& item : legacy_cleanup_text)
				ImGui::BulletText("%s", item.c_str());
			ImGui::EndChild();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			const char* cleanup_action_label = legacy_cleanup_uses_trash() ? "Move to Trash" : "Archive Old Items";
			if (AmigaButton(cleanup_action_label, ImVec2(BUTTON_WIDTH * 1.6f, BUTTON_HEIGHT)))
			{
				std::vector<std::string> failed_items;
				const bool cleanup_ok = cleanup_legacy_items(failed_items);
				if (cleanup_ok)
				{
					legacy_cleanup_error_message.clear();
					legacy_cleanup_text.clear();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					legacy_cleanup_error_message = legacy_cleanup_uses_trash()
						? "Some legacy items could not be moved to Trash:"
						: "Some legacy items could not be moved aside:";
					for (const auto& failed_item : failed_items)
						legacy_cleanup_error_message += "\n- " + failed_item;
					legacy_cleanup_error_message += "\n\nThey were left in place.";
					legacy_cleanup_text = get_legacy_cleanup_prompt_items();
				}
			}
			ImGui::SameLine();
			if (AmigaButton("Ask Later", ImVec2(BUTTON_WIDTH * 1.2f, BUTTON_HEIGHT)))
			{
				legacy_cleanup_error_message.clear();
				postpone_legacy_cleanup_prompt();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (AmigaButton("Don't Ask Again", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT)))
			{
				legacy_cleanup_error_message.clear();
				dismiss_legacy_cleanup_prompt();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (show_disk_info) {
            const float vw = vp->Size.x;
            const float vh = vp->Size.y;
            const float desired_w = vw * 0.7f;

            ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(desired_w, vh * 0.8f), ImGuiCond_Appearing);

            ImGui::OpenPopup(disk_info_title);
            if (ImGui::BeginPopupModal(disk_info_title, &show_disk_info, ImGuiWindowFlags_NoSavedSettings))
            {
                // Child region for text
                const float footer_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
                ImGui::BeginChild("DiskInfoText", ImVec2(0, -footer_h), true);

                for (const auto& line : disk_info_text) {
                    ImGui::TextUnformatted(line.c_str());
                }

                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::Separator();

                // Button alignment logic
                const float btn_w = BUTTON_WIDTH;
                float avail = ImGui::GetContentRegionAvail().x;
                if (avail > btn_w)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w));

                if (ImGui::Button("Close", ImVec2(btn_w, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    show_disk_info = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

		ImGui::End();

		// Pop touch-scrolling hover suppression colors
		if (touch_scrolling) {
			ImGui::PopStyleColor(3);
		}

		// Rendering
		ImGui::Render();
#ifdef USE_VULKAN
		if (gui_use_vulkan) {
			auto* vk = get_vulkan_renderer();
			if (vk) {
				vk->render_gui_frame(ImGui::GetDrawData());
			}
		} else
#endif
#if defined(_WIN32) && defined(USE_OPENGL)
		if (gui_use_opengl) {
			const ImGuiIO& gl_io = ImGui::GetIO();
			glViewport(0, 0,
				(int)(gl_io.DisplaySize.x * gl_io.DisplayFramebufferScale.x),
				(int)(gl_io.DisplaySize.y * gl_io.DisplayFramebufferScale.y));
			glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(mon->gui_window);
		} else
#endif
		{
			const ImGuiIO& render_io = ImGui::GetIO();
			SDL_SetRenderScale(mon->gui_renderer, render_io.DisplayFramebufferScale.x, render_io.DisplayFramebufferScale.y);
			SDL_SetRenderDrawColor(mon->gui_renderer, static_cast<uint8_t>(0.45f * 255), static_cast<uint8_t>(0.55f * 255),
							   static_cast<uint8_t>(0.60f * 255), static_cast<uint8_t>(1.00f * 255));
			SDL_RenderClear(mon->gui_renderer);
			ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);
			SDL_RenderPresent(mon->gui_renderer);
		}
	}

	amiberry_gui_halt();

	// Reset counter for access violations
	init_max_signals();
}
#endif
