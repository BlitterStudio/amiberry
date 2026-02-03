#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "gui/gui_handling.h"
#include <filesystem>
#include <vector>
#include <string>

// Built-in shader names
static const char* builtin_shaders[] = { "none", "tv", "pc", "lite", "1084" };

// Shader list storage
static std::vector<std::string> shader_names;
static std::vector<const char*> shader_items;
static bool shaders_initialized = false;

// Help marker (same pattern as other panels)
static void ShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// Scan for available shaders (built-in + .glsl files)
static void scan_shaders()
{
	shader_names.clear();
	shader_items.clear();

	// Add built-in shaders first
	for (auto & builtin_shader : builtin_shaders) {
		shader_names.emplace_back(builtin_shader);
	}

	// Scan shaders directory for .glsl files
	std::string shaders_dir = get_shaders_path();
	if (!shaders_dir.empty()) {
		try {
			for (const auto& entry : std::filesystem::directory_iterator(shaders_dir)) {
				if (entry.is_regular_file()) {
					std::string filename = entry.path().filename().string();
					if (filename.size() > 5 &&
						filename.substr(filename.size() - 5) == ".glsl") {
						shader_names.push_back(filename);
					}
				}
			}
		} catch (...) {
			// Directory doesn't exist or can't be read - ignore
		}
	}

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
	if (changed_prefs.gfx_manual_crop) ImGui::EndDisabled();
	ImGui::SameLine();
	if (changed_prefs.gfx_auto_crop) ImGui::BeginDisabled();
	AmigaCheckbox("Manual Crop", &changed_prefs.gfx_manual_crop);
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
		ImGui::SameLine();
		ImGui::Text("Shader for RTG/Picasso modes");
	}
	EndGroupBox("RTG Display Shader");

	ImGui::Spacing();

	// Mobile Shader Options
	BeginGroupBox("Performance Options");
	{
		AmigaCheckbox("Force mobile-optimized shaders",
					  &amiberry_options.force_mobile_shaders);
		ImGui::SameLine();
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

	// Save button
	if (AmigaButton("Save Settings", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT))) {
		save_amiberry_settings();
	}
}
