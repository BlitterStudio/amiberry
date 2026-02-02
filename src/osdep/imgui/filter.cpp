#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "gui/gui_handling.h"
#include <filesystem>
#include <vector>
#include <string>

// External declarations
std::string get_shaders_path();
void save_amiberry_settings();

// Built-in shader names
static const char* builtin_shaders[] = { "none", "tv", "pc", "lite", "1084" };
static constexpr int builtin_shader_count = 5;

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
	for (int i = 0; i < builtin_shader_count; i++) {
		shader_names.push_back(builtin_shaders[i]);
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
