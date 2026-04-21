#include "sysconfig.h"
#include "sysdeps.h"

#include "imgui_overlay.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#ifdef USE_OPENGL
#include <imgui_impl_opengl3.h>
#include "gl_platform.h"
#endif

#ifdef USE_VULKAN
#include <imgui_impl_vulkan.h>
#endif

#include "gui/AmigaTopaz_compressed.h"

static ImGuiContext* s_overlay_ctx = nullptr;
static ImGuiContext* s_prev_ctx = nullptr;
static SDL_Window* s_window = nullptr;
static SDL_Renderer* s_sdl_renderer = nullptr;
static bool s_use_opengl = false;
static bool s_use_vulkan = false;
#ifdef USE_VULKAN
static VkDescriptorPool s_vk_descriptor_pool = VK_NULL_HANDLE;
static VkDevice s_vk_device = VK_NULL_HANDLE;
static uint32_t s_vk_image_count = 0;
#endif
static ImFont* s_osk_font = nullptr;
static ImFont* s_osk_font_small = nullptr;

// Common setup: create context, load fonts, configure IO.
// Called by all init paths. Leaves the overlay context as current.
// Returns the previously active context (caller must restore if needed).
static ImGuiContext* create_overlay_context()
{
	ImGuiContext* prev = ImGui::GetCurrentContext();

	s_overlay_ctx = ImGui::CreateContext();
	ImGui::SetCurrentContext(s_overlay_ctx);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

	ImFontConfig cfg;
	cfg.OversampleH = 1;
	cfg.OversampleV = 1;
	cfg.PixelSnapH = true;
	s_osk_font = io.Fonts->AddFontFromMemoryCompressedTTF(
		AmigaTopaz_compressed_data, AmigaTopaz_compressed_size, 24.0f, &cfg);

	ImFontConfig cfg_small;
	cfg_small.OversampleH = 1;
	cfg_small.OversampleV = 1;
	cfg_small.PixelSnapH = true;
	s_osk_font_small = io.Fonts->AddFontFromMemoryCompressedTTF(
		AmigaTopaz_compressed_data, AmigaTopaz_compressed_size, 16.0f, &cfg_small);

	return prev;
}

void imgui_overlay_init(SDL_Window* window, SDL_Renderer* sdl_renderer, void* gl_context)
{
	if (s_overlay_ctx)
		return;

	s_window = window;
	s_sdl_renderer = sdl_renderer;
	s_use_opengl = false;
	s_use_vulkan = false;

	ImGuiContext* prev = create_overlay_context();

#ifdef USE_OPENGL
	if (gl_context)
	{
		s_use_opengl = true;
		ImGui_ImplSDL3_InitForOpenGL(window, gl_context);

		// Detect the correct GLSL version at runtime.
		const char* glsl_version = "#version 130";
		const char* gl_ver_str = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		if (gl_ver_str) {
			bool is_gles = (strstr(gl_ver_str, "OpenGL ES") != nullptr);
			int gl_major = 0, gl_minor = 0;
			const char* v = gl_ver_str;
			while (*v && (*v < '0' || *v > '9')) v++;
			if (*v) {
				gl_major = atoi(v);
				while (*v && *v != '.') v++;
				if (*v == '.') gl_minor = atoi(v + 1);
			}
			if (is_gles && gl_major >= 3)
				glsl_version = "#version 300 es";
			else if (!is_gles && (gl_major > 3 || (gl_major == 3 && gl_minor >= 2)))
				glsl_version = "#version 150";
		}
		ImGui_ImplOpenGL3_Init(glsl_version);
	}
	else
#endif
	{
		ImGui_ImplSDL3_InitForSDLRenderer(window, sdl_renderer);
		ImGui_ImplSDLRenderer3_Init(sdl_renderer);
	}

	ImGui::SetCurrentContext(prev);
}

#ifdef USE_VULKAN
void imgui_overlay_init_vulkan(SDL_Window* window, ImGui_ImplVulkan_InitInfo* vk_info)
{
	if (s_overlay_ctx)
		return;

	s_window = window;
	s_sdl_renderer = nullptr;
	s_use_opengl = false;
	s_use_vulkan = true;
	s_vk_device = vk_info->Device;

	// Create a dedicated descriptor pool for the overlay (isolated from GUI pool)
	VkDescriptorPoolSize pool_size = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 };
	VkDescriptorPoolCreateInfo pool_ci{};
	pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_ci.maxSets = 4;
	pool_ci.poolSizeCount = 1;
	pool_ci.pPoolSizes = &pool_size;
	VkResult result = vkCreateDescriptorPool(s_vk_device, &pool_ci, nullptr, &s_vk_descriptor_pool);
	if (result != VK_SUCCESS) {
		write_log("imgui_overlay: failed to create Vulkan descriptor pool (VkResult=%d)\n", result);
		return;
	}

	// Use our own pool instead of the GUI's shared pool
	vk_info->DescriptorPool = s_vk_descriptor_pool;

	ImGuiContext* prev = create_overlay_context();

	ImGui_ImplSDL3_InitForVulkan(window);
	ImGui_ImplVulkan_Init(vk_info);
	s_vk_image_count = vk_info->ImageCount;

	ImGui::SetCurrentContext(prev);
}

void imgui_overlay_handle_vulkan_swapchain_change(ImGui_ImplVulkan_InitInfo* vk_info)
{
	if (!s_overlay_ctx || !s_use_vulkan)
		return;

	ImGuiContext* prev = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(s_overlay_ctx);

	const uint32_t new_count = vk_info->ImageCount;

	if (s_vk_image_count != 0 && new_count != s_vk_image_count) {
		// Image count changed — full shutdown + re-init required.
		// SetMinImageCount only updates MinImageCount; ImageCount has no public setter.
		write_log("imgui_overlay: Vulkan image count changed (%u -> %u), re-initializing\n",
			s_vk_image_count, new_count);
		vkDeviceWaitIdle(s_vk_device);
		ImGui_ImplVulkan_Shutdown();

		// Use our own descriptor pool
		vk_info->DescriptorPool = s_vk_descriptor_pool;
		ImGui_ImplVulkan_Init(vk_info);
		s_vk_image_count = new_count;
	} else {
		// Same image count — rebuild pipeline for the new render pass
		ImGui_ImplVulkan_SetMinImageCount(new_count);

		ImGui_ImplVulkan_PipelineInfo pipe_info{};
		pipe_info.RenderPass = vk_info->PipelineInfoMain.RenderPass;
		pipe_info.Subpass = vk_info->PipelineInfoMain.Subpass;
		pipe_info.MSAASamples = vk_info->PipelineInfoMain.MSAASamples;
		ImGui_ImplVulkan_CreateMainPipeline(&pipe_info);

		if (s_vk_image_count == 0)
			s_vk_image_count = new_count;
	}

	ImGui::SetCurrentContext(prev);
}
#endif

void imgui_overlay_shutdown()
{
	if (!s_overlay_ctx)
		return;

	ImGuiContext* prev = ImGui::GetCurrentContext();
	ImGuiContext* destroyed = s_overlay_ctx;

	ImGui::SetCurrentContext(s_overlay_ctx);

#ifdef USE_VULKAN
	if (s_use_vulkan)
		ImGui_ImplVulkan_Shutdown();
	else
#endif
#ifdef USE_OPENGL
	if (s_use_opengl)
		ImGui_ImplOpenGL3_Shutdown();
	else
#endif
		ImGui_ImplSDLRenderer3_Shutdown();

	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext(s_overlay_ctx);
	s_overlay_ctx = nullptr;
	s_osk_font = nullptr;
	s_osk_font_small = nullptr;
	s_window = nullptr;
	s_sdl_renderer = nullptr;

#ifdef USE_VULKAN
	if (s_use_vulkan && s_vk_descriptor_pool != VK_NULL_HANDLE && s_vk_device != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(s_vk_device, s_vk_descriptor_pool, nullptr);
		s_vk_descriptor_pool = VK_NULL_HANDLE;
	}
	s_vk_device = VK_NULL_HANDLE;
#endif
	s_use_opengl = false;
	s_use_vulkan = false;

	if (prev != destroyed)
		ImGui::SetCurrentContext(prev);
	else
		ImGui::SetCurrentContext(nullptr);
}

bool imgui_overlay_is_initialized()
{
	return s_overlay_ctx != nullptr;
}

void imgui_overlay_begin_frame()
{
	if (!s_overlay_ctx)
		return;

	s_prev_ctx = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(s_overlay_ctx);

#ifdef USE_VULKAN
	if (s_use_vulkan)
		ImGui_ImplVulkan_NewFrame();
	else
#endif
#ifdef USE_OPENGL
	if (s_use_opengl)
		ImGui_ImplOpenGL3_NewFrame();
	else
#endif
		ImGui_ImplSDLRenderer3_NewFrame();

	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void imgui_overlay_end_frame()
{
	if (!s_overlay_ctx)
		return;

	ImGui::Render();

	// For Vulkan, the caller must render the draw data inside its own render pass
	// via imgui_overlay_get_draw_data(), then call imgui_overlay_restore_context().
	if (s_use_vulkan)
		return;

#ifdef USE_OPENGL
	if (s_use_opengl)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	else
#endif
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), s_sdl_renderer);

	ImGui::SetCurrentContext(s_prev_ctx);
	s_prev_ctx = nullptr;
}

ImDrawData* imgui_overlay_get_draw_data()
{
	if (!s_overlay_ctx || !s_use_vulkan)
		return nullptr;
	return ImGui::GetDrawData();
}

void imgui_overlay_restore_context()
{
	ImGui::SetCurrentContext(s_prev_ctx);
	s_prev_ctx = nullptr;
}

bool imgui_overlay_is_vulkan()
{
	return s_use_vulkan;
}

ImFont* imgui_overlay_get_font()
{
	return s_osk_font;
}

ImFont* imgui_overlay_get_font_small()
{
	return s_osk_font_small;
}
