#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "amiberry_gfx.h"
#include <algorithm>
#include <filesystem>
#include <vector>
#include <string>

#ifdef USE_OPENGL
#include "external_shader.h"
#include "shader_preset.h"
extern ExternalShader* external_shader;
extern ShaderPreset* shader_preset;
#endif

// Built-in shader names
static const char* builtin_shaders[] = { "none", "tv", "pc", "lite", "1084" };

// Shader list storage
static std::vector<std::string> shader_names;
static std::vector<const char*> shader_items;
static bool shaders_initialized = false;

// Scan for available shaders (built-in + .glsl + .glslp files)
static void scan_shaders()
{
	shader_names.clear();
	shader_items.clear();

	// Add built-in shaders first
	for (auto & builtin_shader : builtin_shaders) {
		shader_names.emplace_back(builtin_shader);
	}

	// Scan shaders directory for shader files:
	// - .glsl files: top-level only (user's custom single-pass shaders)
	// - .glslp presets: recursively (e.g. crt/crt-aperture.glslp)
	std::string shaders_dir = get_shaders_path();
	if (!shaders_dir.empty()) {
		namespace fs = std::filesystem;
		fs::path base_path(shaders_dir);
		try {
			for (const auto& entry : fs::recursive_directory_iterator(base_path,
				fs::directory_options::skip_permission_denied)) {
				if (!entry.is_regular_file()) continue;

				std::string filename = entry.path().filename().string();
				bool is_glslp = filename.size() > 6 &&
					filename.substr(filename.size() - 6) == ".glslp";
				bool is_glsl = !is_glslp && filename.size() > 5 &&
					filename.substr(filename.size() - 5) == ".glsl";

				if (!is_glsl && !is_glslp) continue;

				// For .glsl files, only include those at the top level
				// (subdirectory .glsl files are shader components, not standalone)
				if (is_glsl && entry.path().parent_path() != base_path) continue;

				// Store path relative to shaders base dir
				fs::path rel = fs::relative(entry.path(), base_path);
				std::string rel_str = rel.generic_string(); // use forward slashes
				shader_names.push_back(rel_str);
			}
		} catch (...) {
			// Directory doesn't exist or can't be read - ignore
		}
	}

	// Sort external shaders alphabetically (after built-in shaders)
	std::sort(shader_names.begin() + 5, shader_names.end());

	// Build const char* array for ImGui
	for (const auto& name : shader_names) {
		shader_items.push_back(name.c_str());
	}

	shaders_initialized = true;
}

// Find index of shader name in list
static int find_shader_index(const char* shader_name)
{
	for (size_t i = 0; i < shader_names.size(); i++) {
		if (shader_names[i] == shader_name) {
			return static_cast<int>(i);
		}
	}
	return 2;  // Default to "pc" (index 2)
}

#ifdef USE_OPENGL
static bool show_shader_params_popup = false;

static void render_shader_parameters_popup()
{
	if (!show_shader_params_popup) return;

	ImGui::SetNextWindowSize(ImVec2(BUTTON_WIDTH * 5, BUTTON_HEIGHT * 10), ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	if (ImGui::Begin("Shader Parameters", &show_shader_params_popup)) {
		std::vector<ShaderParameter>* params = nullptr;

		if (shader_preset && shader_preset->is_valid()) {
			params = &shader_preset->get_all_parameters();
		} else if (external_shader && external_shader->is_valid()) {
			params = const_cast<std::vector<ShaderParameter>*>(&external_shader->get_parameters());
		}

		if (params && !params->empty()) {
			ImGui::Text("Adjust shader parameters:");
			ImGui::Separator();
			ImGui::Spacing();

			for (auto& param : *params) {
				ImGui::PushID(param.name.c_str());
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", param.description.c_str());
				ShowHelpMarker(param.name.c_str());

				ImGui::SetNextItemWidth(-1);
				if (ImGui::SliderFloat("##val", &param.current_value,
					param.min_value, param.max_value, "%.3f")) {
					// Apply the parameter change
					if (shader_preset) {
						shader_preset->set_parameter(param.name, param.current_value);
					} else if (external_shader) {
						external_shader->set_parameter(param.name, param.current_value);
					}
				}
				AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::Separator();
			if (AmigaButton("Reset to Defaults")) {
				for (auto& param : *params) {
					param.current_value = param.default_value;
					if (shader_preset) {
						shader_preset->set_parameter(param.name, param.default_value);
					} else if (external_shader) {
						external_shader->set_parameter(param.name, param.default_value);
					}
				}
			}
		} else {
			ImGui::Text("No adjustable parameters for the current shader.");
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
#endif

void render_panel_filter()
{
	ImGui::Indent(4.0f);

	// Initialize shader list on first call
	if (!shaders_initialized) {
		scan_shaders();
	}

	BeginGroupBox("Crop and Offset");
	if (changed_prefs.gfx_manual_crop) ImGui::BeginDisabled();
	AmigaCheckbox("Auto Crop", &changed_prefs.gfx_auto_crop);
	ShowHelpMarker("Automatically crop black borders from the display.");
	if (changed_prefs.gfx_manual_crop) ImGui::EndDisabled();
	ImGui::SameLine();
	if (changed_prefs.gfx_auto_crop) ImGui::BeginDisabled();
	AmigaCheckbox("Manual Crop", &changed_prefs.gfx_manual_crop);
	ShowHelpMarker("Manually set the visible display area size.");
	if (changed_prefs.gfx_auto_crop) ImGui::EndDisabled();
	if (changed_prefs.gfx_manual_crop) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH);
		ImGui::SliderInt("##W", &changed_prefs.gfx_manual_crop_width, 0, 800);
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH);
		ImGui::SliderInt("##H", &changed_prefs.gfx_manual_crop_height, 0, 600);
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
	}
	// Scaling Method
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Scaling method:");
	ImGui::SameLine();
	const char *scaling_items[] = {"Auto", "Nearest", "Linear", "Integer"};
	int scaling_idx = changed_prefs.scaling_method + 1; // -1 -> 0, 0 -> 1, etc.
	if (scaling_idx < 0) scaling_idx = 0;
	if (scaling_idx > 3) scaling_idx = 3;
	ImGui::SetNextItemWidth(BUTTON_WIDTH);
	if (ImGui::BeginCombo("##ScalingMethod", scaling_items[scaling_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(scaling_items); n++) {
			const bool is_selected = (scaling_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(scaling_items[n], is_selected)) {
				scaling_idx = n;
				changed_prefs.scaling_method = scaling_idx - 1;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
	ShowHelpMarker("How the display is scaled to fit the window.\nAuto: best fit, Nearest: sharp pixels, Linear: smooth, Integer: pixel-perfect.");
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text("H. Offset:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
	ImGui::SliderInt("##HOffsetSlider", &changed_prefs.gfx_horizontal_offset, -80, 80);
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("V. Offset:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
	ImGui::SliderInt("##VOffsetSlider", &changed_prefs.gfx_vertical_offset, -80, 80);
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
	ImGui::Spacing();
	EndGroupBox("Crop and Offset");

	// Native Shader Selection
	BeginGroupBox("Native Display Shader");
	{
		int native_idx = find_shader_index(amiberry_options.shader);
		ImGui::SetNextItemWidth(BUTTON_WIDTH * 3);
		if (ImGui::BeginCombo("##NativeShader", shader_items[native_idx])) {
			for (int i = 0; i < static_cast<int>(shader_items.size()); i++) {
				bool is_selected = (i == native_idx);
				if (ImGui::Selectable(shader_items[i], is_selected)) {
					strncpy(amiberry_options.shader, shader_names[i].c_str(),
							sizeof(amiberry_options.shader) - 1);
				}
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
		ShowHelpMarker("CRT shader applied to native Amiga chipset display modes.");
		ImGui::SameLine();
		ImGui::Text("Shader for native Amiga modes");
	}
	EndGroupBox("Native Display Shader");

	ImGui::Spacing();

	// RTG Shader Selection
	BeginGroupBox("RTG Display Shader");
	{
		int rtg_idx = find_shader_index(amiberry_options.shader_rtg);
		ImGui::SetNextItemWidth(BUTTON_WIDTH * 3);
		if (ImGui::BeginCombo("##RTGShader", shader_items[rtg_idx])) {
			for (int i = 0; i < static_cast<int>(shader_items.size()); i++) {
				bool is_selected = (i == rtg_idx);
				if (ImGui::Selectable(shader_items[i], is_selected)) {
					strncpy(amiberry_options.shader_rtg, shader_names[i].c_str(),
							sizeof(amiberry_options.shader_rtg) - 1);
				}
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
		ShowHelpMarker("CRT shader applied to RTG/Picasso96 graphics card display modes.");
		ImGui::SameLine();
		ImGui::Text("Shader for RTG/Picasso modes");
	}
	EndGroupBox("RTG Display Shader");

	ImGui::Spacing();

	// CRT Bezel Overlay
	BeginGroupBox("CRT Bezel Overlay");
	{
		if (AmigaCheckbox("Show bezel frame", &amiberry_options.use_bezel)) {
#ifdef USE_OPENGL
			update_crtemu_bezel();
#endif
		}
		ShowHelpMarker("Overlay a CRT television bezel frame around the display.\n"
					   "Only works with built-in CRT shaders (tv, pc, 1084).\n"
					   "Has no effect with the 'none', 'lite' or external shaders.");
	}
	EndGroupBox("CRT Bezel Overlay");

	ImGui::Spacing();

	// Mobile Shader Options
	BeginGroupBox("Performance Options");
	{
		AmigaCheckbox("Force mobile-optimized shaders",
					  &amiberry_options.force_mobile_shaders);
		ShowHelpMarker("Enable mobile shader variants regardless of auto-detection.\n"
					   "Mobile shaders are faster but have reduced visual effects.\n"
					   "Useful for testing on desktop or when auto-detection fails.");
	}
	EndGroupBox("Performance Options");

	ImGui::Spacing();
	ImGui::Spacing();

	// Rescan button
	if (AmigaButton("Rescan Shaders", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT))) {
		shaders_initialized = false;  // Force rescan on next frame
	}
	ImGui::SameLine();

#ifdef USE_OPENGL
	// Shader Parameters button
	if (AmigaButton("Shader Parameters...", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT))) {
		show_shader_params_popup = true;
	}
	ImGui::SameLine();
#endif

	// Save button
	if (AmigaButton("Save Settings", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT))) {
		save_amiberry_settings();
	}

#ifdef USE_OPENGL
	// Render the shader parameters popup if open
	render_shader_parameters_popup();
#endif
}
