#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "amiberry_gfx.h"
#include "target.h"
#include "display_catalog.h"
#include "settings/settings_definitions.h"
#include <algorithm>
#include <vector>
#include <string>

#include "irenderer.h"
#ifdef USE_OPENGL
#include "external_shader.h"
#include "shader_preset.h"
#include "opengl_renderer.h"
#endif

static bool show_shader_params_popup = false;
#ifdef USE_OPENGL
static bool shader_params_popup_rtg = false;
static std::string shader_params_popup_name;
static std::vector<ShaderParameter> shader_params_popup_parameters;
#endif

static void save_filter_defaults()
{
	strncpy(amiberry_options.shader, changed_prefs.shader, sizeof(amiberry_options.shader) - 1);
	amiberry_options.shader[sizeof(amiberry_options.shader) - 1] = '\0';
	strncpy(amiberry_options.shader_rtg, changed_prefs.shader_rtg, sizeof(amiberry_options.shader_rtg) - 1);
	amiberry_options.shader_rtg[sizeof(amiberry_options.shader_rtg) - 1] = '\0';
	amiberry_options.use_custom_bezel = changed_prefs.use_custom_bezel &&
		strcmp(changed_prefs.custom_bezel, "none") != 0;
	amiberry_options.use_bezel = amiberry_options.use_custom_bezel ? false : changed_prefs.use_bezel;
	strncpy(amiberry_options.custom_bezel, changed_prefs.custom_bezel, sizeof(amiberry_options.custom_bezel) - 1);
	amiberry_options.custom_bezel[sizeof(amiberry_options.custom_bezel) - 1] = '\0';
	save_amiberry_settings();
}

#ifdef USE_OPENGL
void ShaderParameters_Open(const char* shader_name, const bool rtg)
{
	shader_params_popup_name = shader_name ? shader_name : "none";
	shader_params_popup_rtg = rtg;
	shader_params_popup_parameters.clear();

	bool parameters_available = false;
	if (auto* gl_renderer = get_opengl_renderer()) {
		gl_renderer->ensure_shader_parameters(shader_params_popup_name.c_str(), rtg);
		parameters_available = gl_renderer->shader_parameters(
			shader_params_popup_name.c_str(), rtg) != nullptr;
	}

	if (!parameters_available
		&& load_shader_parameter_metadata(
			shader_params_popup_name.c_str(), shader_params_popup_parameters)) {
		for (auto& parameter : shader_params_popup_parameters) {
			for (const auto& saved : amiberry_options.shader_parameters) {
				if (saved.rtg == rtg && saved.shader == shader_params_popup_name
					&& saved.name == parameter.name) {
					parameter.current_value = std::max(parameter.min_value,
						std::min(parameter.max_value, saved.value));
					break;
				}
			}
		}
	}
	show_shader_params_popup = true;
}
#endif

void ShaderParameters_RenderPopup()
{
	if (!show_shader_params_popup) return;

#ifdef USE_OPENGL
	ImGui::SetNextWindowSize(ImVec2(BUTTON_WIDTH * 5, BUTTON_HEIGHT * 10), ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	const char* popup_title = shader_params_popup_rtg ? "RTG Shader Parameters" : "Native Shader Parameters";
	if (ImGui::Begin(popup_title, &show_shader_params_popup)) {
		auto* gl_renderer = get_opengl_renderer();
		auto* params = gl_renderer
			? gl_renderer->shader_parameters(shader_params_popup_name.c_str(), shader_params_popup_rtg)
			: nullptr;
		if (!params)
			params = &shader_params_popup_parameters;

		if (params && !params->empty()) {
			ImGui::Text("Shader: %s", shader_params_popup_name.c_str());
			ImGui::Spacing();

			if (AmigaButton(ICON_FA_ARROW_ROTATE_LEFT " Reset to Shader Defaults",
				ImVec2(BUTTON_WIDTH * 2.25f, BUTTON_HEIGHT))) {
				for (auto& param : *params) {
					param.current_value = param.default_value;
					if (gl_renderer) {
						gl_renderer->set_shader_parameter(shader_params_popup_name.c_str(),
							shader_params_popup_rtg, param.name, param.default_value);
					}
				}
			}
			ShowHelpMarker("Restore the values declared by this shader.");
			ImGui::SameLine();
			if (AmigaButton(ICON_FA_FLOPPY_DISK " Save Parameters",
				ImVec2(BUTTON_WIDTH * 1.75f, BUTTON_HEIGHT))) {
				if (gl_renderer) {
					gl_renderer->save_shader_parameters(
						shader_params_popup_name.c_str(), shader_params_popup_rtg);
				} else {
					auto& saved = amiberry_options.shader_parameters;
					saved.erase(std::remove_if(saved.begin(), saved.end(),
						[](const amiberry_shader_parameter& parameter) {
							return parameter.rtg == shader_params_popup_rtg
								&& parameter.shader == shader_params_popup_name;
						}), saved.end());
					for (const auto& parameter : *params) {
						if (parameter.current_value != parameter.default_value) {
							saved.push_back({shader_params_popup_rtg, shader_params_popup_name,
								parameter.name, parameter.current_value});
						}
					}
				}
				save_amiberry_settings();
			}
			ShowHelpMarker(shader_params_popup_rtg
				? "Save these values as the defaults for the RTG shader."
				: "Save these values as the defaults for the native shader.");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Text("Adjust shader parameters:");
			ImGui::Spacing();

			for (auto& param : *params) {
				ImGui::PushID(param.name.c_str());
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", param.description.c_str());
				ShowHelpMarker(param.name.c_str());

				ImGui::SetNextItemWidth(-1);
				if (ImGui::SliderFloat("##val", &param.current_value,
					param.min_value, param.max_value, "%.3f")) {
					if (gl_renderer) {
						gl_renderer->set_shader_parameter(shader_params_popup_name.c_str(),
							shader_params_popup_rtg, param.name, param.current_value);
					}
				}
				AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
				ImGui::PopID();
			}
		} else {
			ImGui::Text("Shader: %s", shader_params_popup_name.c_str());
			ImGui::Spacing();
			ImGui::TextWrapped("No adjustable parameters are currently available for this %s shader.",
				shader_params_popup_rtg ? "RTG" : "native");
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
#endif
}

void render_panel_filter()
{
	ImGui::Indent(4.0f);
	const float spacing = ImGui::GetStyle().ItemSpacing.x;

	// The settings layer owns the rules: one cropping at a time, the window
	// within the frame at the render resolution, the built-in bezel or a
	// custom one, and the shaders and bezels the folders hold.
	namespace settings = amiberry_settings;
	const settings::context ctx{emulating != 0};
	auto option = [](const char* id) -> const settings::definition& { return settings::require(id); };
	auto value_of = [&](const char* id) {
		char text[MAX_DPATH] = "";
		settings::get(option(id), changed_prefs, text, sizeof text);
		return std::string(text);
	};
	auto available = [&](const char* id) { return settings::is_available(option(id), changed_prefs, ctx); };
	auto put = [&](const char* id, const std::string& value) { settings::set(option(id), changed_prefs, ctx, value); };
	auto checkbox = [&](const char* id, const char* label) {
		bool checked = value_of(id) == "true";
		ImGui::BeginDisabled(!available(id));
		const bool clicked = AmigaCheckbox(label, &checked);
		if (clicked)
			put(id, checked ? "true" : "false");
		ImGui::EndDisabled();
		return clicked;
	};
	// A slider and a number field over the option's current range.
	auto number = [&](const char* id, const char* slider_id, const char* input_id, float slider_width) {
		double min = 0, max = 0;
		settings::get_range(option(id), changed_prefs, min, max);
		int v = static_cast<int>(settings::get_number(option(id), changed_prefs));
		ImGui::SetNextItemWidth(slider_width);
		// Ctrl+Click can type past the slider's ends.
		if (ImGui::SliderInt(slider_id, &v, static_cast<int>(min), static_cast<int>(max)))
			settings::set_number(option(id), changed_prefs, ctx, std::clamp(v, static_cast<int>(min), static_cast<int>(max)));
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH * 0.45f);
		int typed = v;
		if (ImGui::InputInt(input_id, &typed, 0))
			settings::set_number(option(id), changed_prefs, ctx, std::clamp(typed, static_cast<int>(min), static_cast<int>(max)));
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
	};
	// A combo over the option's choices (the folder's shaders or bezels);
	// a configured name the folder no longer has still shows.
	auto combo = [&](const char* id, const char* combo_id) {
		const settings::definition& d = option(id);
		const std::string current = value_of(id);
		const settings::choice_list choices = settings::choices_of(d, changed_prefs);
		bool changed = false;
		if (ImGui::BeginCombo(combo_id, current.c_str())) {
			for (size_t i = 0; i < choices.count; i++) {
				const bool is_selected = current == choices.items[i].value;
				if (ImGui::Selectable(choices.items[i].label, is_selected)) {
					put(id, choices.items[i].value);
					changed = true;
				}
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
		return changed;
	};

	BeginGroupBox("Crop and Offset");
	checkbox("amiberry.gfx_auto_crop", "Auto Crop");
	ShowHelpMarker("Automatically crop borders by scanning frame content.");
	ImGui::SameLine();
	checkbox("amiberry.gfx_manual_crop", "Manual Crop");
	ShowHelpMarker("Manually set the visible display area size and position.");
	if (available("amiberry.gfx_manual_crop_width")) {
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Width:");
		ImGui::SameLine();
		number("amiberry.gfx_manual_crop_width", "##WCropSlider", "##WCropInput", BUTTON_WIDTH);
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Height:");
		ImGui::SameLine();
		number("amiberry.gfx_manual_crop_height", "##HCropSlider", "##HCropInput", BUTTON_WIDTH);

		// Offsets position the manual crop window and are only consumed on
		// the manual-crop render path, so they show with it.
		ImGui::AlignTextToFramePadding();
		ImGui::Text("H. Offset:");
		ImGui::SameLine();
		number("amiberry.gfx_horizontal_offset", "##HOffsetSlider", "##HOffsetInput",
			ImGui::GetContentRegionAvail().x - spacing * 2 - BUTTON_WIDTH * 0.45f);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("V. Offset:");
		ImGui::SameLine();
		number("amiberry.gfx_vertical_offset", "##VOffsetSlider", "##VOffsetInput",
			ImGui::GetContentRegionAvail().x - spacing * 2 - BUTTON_WIDTH * 0.45f);
	}
	ImGui::Spacing();
	EndGroupBox("Crop and Offset");

	// Native Shader Selection
	BeginGroupBox("Native Display Shader");
	{
		const float native_label_w = ImGui::CalcTextSize("Shader for native Amiga modes").x;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - native_label_w - spacing * 3);
		combo("amiberry.shader", "##NativeShader");
		ShowHelpMarker("CRT shader applied to native Amiga chipset display modes.");
		ImGui::SameLine();
		ImGui::Text("Shader for native Amiga modes");
		ImGui::Spacing();
#ifdef USE_OPENGL
		if (AmigaButton(ICON_FA_SLIDERS " Shader Parameters...##NativeShaderParameters",
			ImVec2(BUTTON_WIDTH * 1.75f, BUTTON_HEIGHT))) {
			ShaderParameters_Open(changed_prefs.shader, false);
		}
		ShowHelpMarker("Edit parameters for the native display shader.");
#endif
	}
	EndGroupBox("Native Display Shader");

	ImGui::Spacing();

	// RTG Shader Selection
	BeginGroupBox("RTG Display Shader");
	{
		const float rtg_label_w = ImGui::CalcTextSize("Shader for RTG/Picasso modes").x;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - rtg_label_w - spacing * 3);
		combo("amiberry.shader_rtg", "##RTGShader");
		ShowHelpMarker("CRT shader applied to RTG/Picasso96 graphics card display modes.");
		ImGui::SameLine();
		ImGui::Text("Shader for RTG/Picasso modes");
		ImGui::Spacing();
#ifdef USE_OPENGL
		if (AmigaButton(ICON_FA_SLIDERS " Shader Parameters...##RTGShaderParameters",
			ImVec2(BUTTON_WIDTH * 1.75f, BUTTON_HEIGHT))) {
			ShaderParameters_Open(changed_prefs.shader_rtg, true);
		}
		ShowHelpMarker("Edit parameters for the RTG display shader.");
#endif
	}
	EndGroupBox("RTG Display Shader");

	ImGui::Spacing();

	// Bezel Overlay
	BeginGroupBox("Bezel Overlay");
	{
		// The renderer shows a bezel change while the GUI is open.
		if (checkbox("amiberry.use_bezel", "Built-in CRT bezel") && g_renderer)
			g_renderer->update_crtemu_bezel();
		ShowHelpMarker("Built-in CRT television bezel frame.\n"
					   "Only works with built-in CRT shaders (tv, pc, 1084).\n"
					   "Disabled when a custom bezel is selected.");

		ImGui::Spacing();

		ImGui::BeginDisabled(!available("amiberry.custom_bezel"));
		const float bezel_label_w = ImGui::CalcTextSize("Custom bezel overlay").x;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - bezel_label_w - spacing * 3);
		if (combo("amiberry.custom_bezel", "##CustomBezel") && g_renderer) {
			g_renderer->update_crtemu_bezel();
			g_renderer->update_custom_bezel();
		}
		ShowHelpMarker("Select a custom bezel overlay image (PNG with transparency).\n"
					   "Place bezel images in the Bezels directory.\n"
					   "Works with all shader types.");
		ImGui::SameLine();
		ImGui::Text("Custom bezel overlay");
		ImGui::EndDisabled();
	}
	EndGroupBox("Bezel Overlay");

	ImGui::Spacing();
	ImGui::Spacing();

	// Rescan button
	if (AmigaButton(ICON_FA_ARROWS_ROTATE " Rescan Shaders & Bezels", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT))) {
		invalidate_shader_catalog();
		invalidate_bezel_catalog();
	}
	ImGui::SameLine();

	// Save button
	if (AmigaButton(ICON_FA_FLOPPY_DISK " Save Defaults", ImVec2(BUTTON_WIDTH * 1.75f, BUTTON_HEIGHT))) {
		save_filter_defaults();
	}
	ShowHelpMarker("Save the shader selections and bezel settings as application defaults.");

	// Render the shader parameters popup if open
	ShaderParameters_RenderPopup();
}
