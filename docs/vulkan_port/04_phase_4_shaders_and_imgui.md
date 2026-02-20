# Phase 4: Shaders and ImGui Integration

## Objective
Migrate the existing Dear ImGui pipeline to use Vulkan and port over the `crtemu` / OSD shaders to SPIR-V compatible Vulkan Graphics Pipelines.

## Target Files to Modify
- `src/osdep/vulkan_renderer.cpp`
- `src/osdep/gui/main_window.cpp`
- `src/osdep/crtemu.h` / `src/osdep/crtemu.cpp` (refactoring shaders)

## Context and Instructions

### 1. ImGui_ImplVulkan Setup
The Dear ImGui integration in `gui/main_window.cpp` directly invokes `ImGui_ImplOpenGL3_Init`. 
- Setup a condition for `#ifdef USE_VULKAN` inside `gui_init()`.
- Use `ImGui_ImplVulkan_InitInfo` to initialize `ImGui_ImplVulkan_Init(&init_info, render_pass)`.
- You will need a Vulkan Descriptor Pool created specifically for ImGui. Ensure it is sized properly for font textures and widget use.
- Replace `ImGui_ImplOpenGL3_RenderDrawData` with `ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer)`.

### 2. Shader Compilation (SPIR-V)
Vulkan drivers do not ingest raw `GLSL`.
- Use an external tool `glslangValidator` inside your build process (`CMakeFiles` custom command) to convert `crtemu` shaders to `.spv` headers.
- Alternatively, include `shaderc` as a library to compile the users' custom `.glslp` shader presets to SPIR-V at runtime.
- Load the generated SPIR-V into `VkShaderModule`.

### 3. Graphics Pipeline & OSD
The emulator draws an On-Screen Display (status line).
- Create a `VkPipelineLayout` describing the push constants (for basic scales/colors) and a `VkDescriptorSetLayout` for the texture samplers.
- Create a `VkPipeline` configuring Vertex Input, Input Assembly, Viewport, Rasterizer, Multisampling, Depth/Stencil, Color Blending, and your Shader Modules.
- The `osd_texture` updates via `update_leds` must also transition into a descriptor set that your Vulkan pipeline can sample from.

## Acceptance Criteria
- Dear ImGui window is clickable and behaves exactly as it did in OpenGL.
- Amiberry’s bottom OSD (On-Screen Display) with LEDs renders correctly over the video.
- At least one `crtemu` shader preset applied to the Amiga framebuffer works through the Vulkan shader pipeline without visual artifacts.
