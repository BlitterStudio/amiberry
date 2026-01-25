#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "amiberry_gfx.h" // For getdisplay and MultiDisplay
#include "imgui_panels.h"

static std::vector<std::string> fullscreen_resolutions;
static std::vector<std::pair<int, int>> fullscreen_resolution_values;

static void refresh_fullscreen_resolutions()
{
	fullscreen_resolutions.clear();
	fullscreen_resolution_values.clear();

	struct MultiDisplay *md = getdisplay(&changed_prefs, 0);
	if (!md || !md->DisplayModes) return;

	int idx = -1;
	for (int i = 0; md->DisplayModes[i].inuse; i++)
	{
		if (md->DisplayModes[i].residx != idx) {
			char tmp[64];
			snprintf(tmp, sizeof(tmp), "%dx%d%s", md->DisplayModes[i].res.width, md->DisplayModes[i].res.height, md->DisplayModes[i].lace ? "i" : "");
			if (md->DisplayModes[i].rawmode)
				strncat(tmp, " (*)", sizeof(tmp) - strlen(tmp) - 1);
			fullscreen_resolutions.emplace_back(tmp);
			fullscreen_resolution_values.push_back({ (int)md->DisplayModes[i].res.width, (int)md->DisplayModes[i].res.height });
			idx = md->DisplayModes[i].residx;
		}
	}
	fullscreen_resolutions.emplace_back("Native");
	fullscreen_resolution_values.push_back({ 0, 0 }); // Native special case
}

void render_panel_display()
{
	if (fullscreen_resolutions.empty()) {
		refresh_fullscreen_resolutions();
	}

	// Logic Check: RTG Enabled
	// WinUAE: ((!address_space_24 || configtype==2) && size) || type >= HARDWARE
	bool rtg_enabled = false;
	// Simplification: Check if any RTG board is configured with memory
	if (changed_prefs.rtgboards[0].rtgmem_size > 0) rtg_enabled = true;
#ifdef PICASSO96
	// Add proper check if headers allowed, but for now simple check
#else
	rtg_enabled = false;
#endif

	// Logic Check: Refresh Rate Slider / FPS Adj
	bool refresh_enabled = !changed_prefs.cpu_memory_cycle_exact;

	// Logic Check: Resolution & Line Mode Groups
	bool autoresol_enabled = changed_prefs.gfx_autoresolution > 0;
	bool linemode_enabled = !autoresol_enabled;
	bool resolution_enabled = !autoresol_enabled;

	// Logic Check: VGA Autoswitch
	// RES_HIRES=1, VRES_DOUBLE=1. 
	bool vga_autoswitch_enabled = (changed_prefs.gfx_resolution >= 1 && changed_prefs.gfx_vresolution >= 1);
	if (!vga_autoswitch_enabled && changed_prefs.gfx_autoresolution_vga) {
		changed_prefs.gfx_autoresolution_vga = false;
	}

	// ---------------------------------------------------------
	// Screen Group (Top)
	// ---------------------------------------------------------
	// Increased height to avoid scrollbar
	ImGui::BeginChild("ScreenGroup", ImVec2(0, 120), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Screen");
		ImGui::EndMenuBar();
	}

	// Monitor logic (Placeholder)
	ImGui::BeginDisabled();
	const char* monitor_items[] = { "Generic PnP Monitor" };
	int current_monitor = 0;
	ImGui::SetNextItemWidth(-1);
	ImGui::Combo("##Monitor", &current_monitor, monitor_items, IM_ARRAYSIZE(monitor_items));
	ImGui::EndDisabled();

	// Fullscreen & Refresh
	ImGui::Text("Fullscreen:"); ImGui::SameLine();
	
	// Find current selection based on prefs
	int current_fs_res_idx = fullscreen_resolutions.size() - 1; // Default to Native
	struct MultiDisplay *md = getdisplay(&changed_prefs, 0);
	if (changed_prefs.gfx_monitor[0].gfx_size_fs.special != WH_NATIVE && md) {
		for(size_t i = 0; i < fullscreen_resolution_values.size(); ++i) {
			if (fullscreen_resolution_values[i].first == changed_prefs.gfx_monitor[0].gfx_size_fs.width &&
				fullscreen_resolution_values[i].second == changed_prefs.gfx_monitor[0].gfx_size_fs.height) {
				current_fs_res_idx = (int)i;
				break;
			}
		}
	}
	
	ImGui::SetNextItemWidth(120); // Smaller width
	if (ImGui::BeginCombo("##Fullscreen", fullscreen_resolutions[current_fs_res_idx].c_str())) {
		for (int n = 0; n < (int)fullscreen_resolutions.size(); n++) {
			const bool is_selected = (current_fs_res_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(fullscreen_resolutions[n].c_str(), is_selected)) {
				// Apply selection
				if (n == (int)fullscreen_resolutions.size() - 1) {
					changed_prefs.gfx_monitor[0].gfx_size_fs.special = WH_NATIVE;
				} else {
					changed_prefs.gfx_monitor[0].gfx_size_fs.special = 0;
					changed_prefs.gfx_monitor[0].gfx_size_fs.width = fullscreen_resolution_values[n].first;
					changed_prefs.gfx_monitor[0].gfx_size_fs.height = fullscreen_resolution_values[n].second;
				}
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 160);
	
	const char* refresh_items[] = { "Default refresh rate", "50Hz", "60Hz" }; 
	static int current_refresh = 0;
	ImGui::SetNextItemWidth(160);
	if (ImGui::BeginCombo("##RefreshRate", refresh_items[current_refresh])) {
		for (int n = 0; n < IM_ARRAYSIZE(refresh_items); n++) {
			const bool is_selected = (current_refresh == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(refresh_items[n], is_selected)) {
				current_refresh = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Windowed & Buffering
	// WinUAE: Label "Windowed:" -> W Input -> H Input -> Resize Checkbox -> Triple Buffering (Right)
	ImGui::Text("Windowed:"); ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::InputInt("##WinW", &changed_prefs.gfx_monitor[0].gfx_size_win.width, 0, 0);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::InputInt("##WinH", &changed_prefs.gfx_monitor[0].gfx_size_win.height, 0, 0);
	ImGui::SameLine();
	AmigaCheckbox("Window resize", &changed_prefs.gfx_windowed_resize);

	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 160);
	
	const char* buffer_items[] = { "No buffering", "Double buffering", "Triple buffering" };
	int buffer_idx = changed_prefs.gfx_apmode[0].gfx_backbuffers - 1;
	if (buffer_idx < 0) buffer_idx = 0;
	if (buffer_idx > 2) buffer_idx = 2;
	ImGui::SetNextItemWidth(160);
	if (ImGui::BeginCombo("##Buffering", buffer_items[buffer_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(buffer_items); n++) {
			const bool is_selected = (buffer_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(buffer_items[n], is_selected)) {
				buffer_idx = n;
				changed_prefs.gfx_apmode[0].gfx_backbuffers = buffer_idx + 1;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndChild();

	// ---------------------------------------------------------
	// Amiberry Specific Group (Between Screen and Settings)
	// ---------------------------------------------------------
	// ---------------------------------------------------------
	// Amiberry Specific Group (Between Screen and Settings)
	// ---------------------------------------------------------
	// Reduced height to remove whitespace
	ImGui::BeginChild("AmiberryGroup", ImVec2(0, 115), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Amiberry Specific");
		ImGui::EndMenuBar();
	}
	AmigaCheckbox("Auto Crop", &changed_prefs.gfx_auto_crop);
	ImGui::SameLine();
	AmigaCheckbox("Manual Crop", &changed_prefs.gfx_manual_crop);
	if (changed_prefs.gfx_manual_crop) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderInt("W", &changed_prefs.gfx_manual_crop_width, 0, 800); 
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderInt("H", &changed_prefs.gfx_manual_crop_height, 0, 600);
	}
	
	// Scaling Method
	ImGui::Text("Scaling method:");
	ImGui::SameLine();
	const char* scaling_items[] = { "Auto", "Nearest", "Linear", "Integer" };
	int scaling_idx = changed_prefs.scaling_method + 1; // -1 -> 0, 0 -> 1, etc.
	if (scaling_idx < 0) scaling_idx = 0;
	if (scaling_idx > 3) scaling_idx = 3;
	ImGui::SetNextItemWidth(150);
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

	// Borderless (Enable only if Windowed)
	ImGui::SameLine();
	bool is_windowed = changed_prefs.gfx_apmode[0].gfx_fullscreen == 0;
	if (!is_windowed) ImGui::BeginDisabled();
	AmigaCheckbox("Borderless", &changed_prefs.borderless);
	if (!is_windowed) ImGui::EndDisabled();

	ImGui::Columns(2, "Offsets", false);
	ImGui::SliderInt("H. Offset", &changed_prefs.gfx_horizontal_offset, -80, 80);
	ImGui::NextColumn();
	ImGui::SliderInt("V. Offset", &changed_prefs.gfx_vertical_offset, -80, 80);
	ImGui::Columns(1);
	ImGui::EndChild();

	// ---------------------------------------------------------
	// Settings Group (Left Bottom)
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	// Increased Width Ratio to 0.75f to give more space
	ImGui::BeginChild("SettingsGroup", ImVec2(ImGui::GetContentRegionAvail().x * 0.75f, 0), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Settings");
		ImGui::EndMenuBar();
	}

	// Columns for Grid Layout
	ImGui::Columns(4, "SettingsGrid", false); 
	ImGui::SetColumnWidth(0, 60); // Labels "Native:", "RTG:"
	ImGui::SetColumnWidth(1, 130); // Combos 1
	ImGui::SetColumnWidth(2, 60); // Spacer/Label? or just spacing
	ImGui::SetColumnWidth(3, 140); // Combos 2

	// Row 1: Native
	ImGui::Text("Native:"); ImGui::NextColumn();
	const char* screenmode_items[] = { "Windowed", "Fullscreen", "Full-window" };
	ImGui::SetNextItemWidth(120);
	if (ImGui::BeginCombo("##NativeMode", screenmode_items[changed_prefs.gfx_apmode[0].gfx_fullscreen])) {
		for (int n = 0; n < IM_ARRAYSIZE(screenmode_items); n++) {
			const bool is_selected = (changed_prefs.gfx_apmode[0].gfx_fullscreen == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(screenmode_items[n], is_selected)) {
				changed_prefs.gfx_apmode[0].gfx_fullscreen = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::NextColumn(); ImGui::NextColumn(); // Skip col 2
	
	// VSync Mapping
	const char* vsync_items[] = { "-", "Lagless", "Lagless 50/60Hz", "Standard", "Standard 50/60Hz" }; 
	// Calc current index
	int vsync_idx = 0;
	if (changed_prefs.gfx_apmode[0].gfx_vsync == 1 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 1) vsync_idx = 1;
	else if (changed_prefs.gfx_apmode[0].gfx_vsync == 2 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 1) vsync_idx = 2;
	else if (changed_prefs.gfx_apmode[0].gfx_vsync == 1 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 0) vsync_idx = 3;
	else if (changed_prefs.gfx_apmode[0].gfx_vsync == 2 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 0) vsync_idx = 4;
	
	ImGui::SetNextItemWidth(150);
	if (ImGui::BeginCombo("##NativeVSync", vsync_items[vsync_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(vsync_items); n++) {
			const bool is_selected = (vsync_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(vsync_items[n], is_selected)) {
				vsync_idx = n;
				switch (vsync_idx) {
					case 0: changed_prefs.gfx_apmode[0].gfx_vsync = 0; changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0; break;
					case 1: changed_prefs.gfx_apmode[0].gfx_vsync = 1; changed_prefs.gfx_apmode[0].gfx_vsyncmode = 1; break;
					case 2: changed_prefs.gfx_apmode[0].gfx_vsync = 2; changed_prefs.gfx_apmode[0].gfx_vsyncmode = 1; break;
					case 3: changed_prefs.gfx_apmode[0].gfx_vsync = 1; changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0; break;
					case 4: changed_prefs.gfx_apmode[0].gfx_vsync = 2; changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0; break;
				}
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::NextColumn();

	// Row 2: RTG
	if (!rtg_enabled) ImGui::BeginDisabled();
	ImGui::Text("RTG:"); ImGui::NextColumn();
	ImGui::SetNextItemWidth(120);
	if (ImGui::BeginCombo("##RTGMode", screenmode_items[changed_prefs.gfx_apmode[1].gfx_fullscreen])) {
		for (int n = 0; n < IM_ARRAYSIZE(screenmode_items); n++) {
			const bool is_selected = (changed_prefs.gfx_apmode[1].gfx_fullscreen == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(screenmode_items[n], is_selected)) {
				changed_prefs.gfx_apmode[1].gfx_fullscreen = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::NextColumn(); ImGui::NextColumn();
	const char* vsync_rtg_items[] = { "-", "Lagless" };
	ImGui::SetNextItemWidth(150);
	if (ImGui::BeginCombo("##RTGVSync", vsync_rtg_items[changed_prefs.gfx_apmode[1].gfx_vsync])) {
		for (int n = 0; n < IM_ARRAYSIZE(vsync_rtg_items); n++) {
			const bool is_selected = (changed_prefs.gfx_apmode[1].gfx_vsync == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(vsync_rtg_items[n], is_selected)) {
				changed_prefs.gfx_apmode[1].gfx_vsync = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (!rtg_enabled) ImGui::EndDisabled();
	
	ImGui::Columns(1);
	ImGui::Separator();

	// Checkboxes
	ImGui::Columns(2, "ChkCols", false);
	AmigaCheckbox("Blacker than black", &changed_prefs.gfx_blackerthanblack);
	AmigaCheckbox("Remove interlace artifacts", &changed_prefs.gfx_scandoubler);
	AmigaCheckbox("Monochrome video out", &changed_prefs.gfx_grayscale);
	ImGui::NextColumn();
	AmigaCheckbox("Filtered low resolution", (bool*)&changed_prefs.gfx_lores_mode);
	
	if (!vga_autoswitch_enabled) ImGui::BeginDisabled();
	AmigaCheckbox("VGA mode resolution autoswitch", &changed_prefs.gfx_autoresolution_vga);
	if (!vga_autoswitch_enabled) ImGui::EndDisabled();
	
	bool resync_blank = changed_prefs.gfx_monitorblankdelay > 0;
	if (AmigaCheckbox("Display resync blanking", &resync_blank)) {
		changed_prefs.gfx_monitorblankdelay = resync_blank ? 1000 : 0;
	}
	ImGui::Columns(1);
	ImGui::Dummy(ImVec2(0, 5));

	// Resolution row
	ImGui::Columns(4, "ResCols", false);
	ImGui::SetColumnWidth(0, 80); // Label "Resolution:"
	ImGui::SetColumnWidth(1, 140); // Combo
	ImGui::SetColumnWidth(2, 80); // Label "Overscan:"
	ImGui::SetColumnWidth(3, 140); // Combo

	ImGui::Text("Resolution:"); ImGui::NextColumn();
	const char* resolution_items[] = { "LowRes", "HighRes (normal)", "SuperHighRes" };
	ImGui::SetNextItemWidth(130);
	if (!resolution_enabled) ImGui::BeginDisabled();
	if (ImGui::BeginCombo("##Resolution", resolution_items[changed_prefs.gfx_resolution])) {
		for (int n = 0; n < IM_ARRAYSIZE(resolution_items); n++) {
			const bool is_selected = (changed_prefs.gfx_resolution == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(resolution_items[n], is_selected)) {
				changed_prefs.gfx_resolution = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (!resolution_enabled) ImGui::EndDisabled();
	ImGui::NextColumn();
	
	ImGui::Text("Overscan:"); ImGui::NextColumn();
	const char* overscan_items[] = { "Standard", "Overscan", "Broadcast", "Extreme", "Ultra" }; 
	ImGui::SetNextItemWidth(130);
	if (ImGui::BeginCombo("##Overscan", overscan_items[changed_prefs.gfx_overscanmode])) {
		for (int n = 0; n < IM_ARRAYSIZE(overscan_items); n++) {
			const bool is_selected = (changed_prefs.gfx_overscanmode == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(overscan_items[n], is_selected)) {
				changed_prefs.gfx_overscanmode = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Columns(1);

	// Autoswitch
	ImGui::Dummy(ImVec2(0, 4));
	ImGui::Text("Resolution autoswitch:"); ImGui::SameLine();
	const char* res_autoswitch_items[] = { "Disabled", "Always On", "10%", "33%", "66%" };
	ImGui::SetNextItemWidth(200);
	// Convert int value to index (0, 1, 10, 33, 66 -> 0, 1, 2, 3, 4)
	int autoswitch_idx = 0;
	if (changed_prefs.gfx_autoresolution == 1) autoswitch_idx = 1;
	else if (changed_prefs.gfx_autoresolution >= 2 && changed_prefs.gfx_autoresolution <= 10) autoswitch_idx = 2;
	else if (changed_prefs.gfx_autoresolution > 10 && changed_prefs.gfx_autoresolution <= 33) autoswitch_idx = 3;
	else if (changed_prefs.gfx_autoresolution > 33) autoswitch_idx = 4;
	
	if (ImGui::BeginCombo("##ResAutoswitch", res_autoswitch_items[autoswitch_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(res_autoswitch_items); n++) {
			const bool is_selected = (autoswitch_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(res_autoswitch_items[n], is_selected)) {
				autoswitch_idx = n;
				switch (autoswitch_idx) {
					case 0: changed_prefs.gfx_autoresolution = 0; break;
					case 1: changed_prefs.gfx_autoresolution = 1; break;
					case 2: changed_prefs.gfx_autoresolution = 10; break;
					case 3: changed_prefs.gfx_autoresolution = 33; break;
					case 4: changed_prefs.gfx_autoresolution = 66; break;
				}
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0, 2));

	// Refresh Slider + PAL/NTSC
	// WinUAE: Label "Refresh:" -> Slider -> Dropdown "PAL"
	ImGui::Columns(3, "RefreshRow", false);
	ImGui::SetColumnWidth(0, 60);
	ImGui::SetColumnWidth(1, 250); // Increased with group wideth
	
	ImGui::Text("Refresh:"); ImGui::NextColumn();
	ImGui::SetNextItemWidth(240);
	if (!refresh_enabled) ImGui::BeginDisabled();
	ImGui::SliderInt("##RefreshSld", &changed_prefs.gfx_framerate, 1, 10, ""); // 1=Every, 2=Every 2nd
	if (!refresh_enabled) ImGui::EndDisabled();
	ImGui::NextColumn();
	const char* video_standards[] = { "PAL", "NTSC" };
	int current_vid_Standard = changed_prefs.ntscmode ? 1 : 0;
	ImGui::SetNextItemWidth(80);
	if (ImGui::BeginCombo("##VideoStandard", video_standards[current_vid_Standard])) {
		for (int n = 0; n < IM_ARRAYSIZE(video_standards); n++) {
			const bool is_selected = (current_vid_Standard == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(video_standards[n], is_selected)) {
				current_vid_Standard = n;
				changed_prefs.ntscmode = (current_vid_Standard == 1);
				if (changed_prefs.ntscmode) {
					changed_prefs.chipset_refreshrate = 60;
				} else {
					changed_prefs.chipset_refreshrate = 50;
				}
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Columns(1);

	// FPS Adj
	ImGui::Columns(3, "FPSAdjRow", false);
	ImGui::SetColumnWidth(0, 60);
	ImGui::SetColumnWidth(1, 250);

	ImGui::Text("FPS adj.:"); ImGui::NextColumn();
	ImGui::SetNextItemWidth(240);
	if (!refresh_enabled) ImGui::BeginDisabled();
	ImGui::SliderFloat("##FPSAdjSld", &changed_prefs.cr[changed_prefs.cr_selected].rate, 1.0f, 100.0f, "");
	if (!refresh_enabled) ImGui::EndDisabled();
	ImGui::NextColumn();
	
	ImGui::SetNextItemWidth(60);
	char buf[32];
	snprintf(buf, sizeof(buf), "%.6f", changed_prefs.cr[changed_prefs.cr_selected].rate);
	ImGui::InputText("##FPSVal", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly); 
	ImGui::SameLine();
	AmigaCheckbox("##FPSLocked", (bool*)&changed_prefs.cr[changed_prefs.cr_selected].locked);
	ImGui::Columns(1);

	ImGui::Separator();
	
	// Display Attributes (Brightness, Contrast, Gamma, etc.)
	static int current_attr_idx = 0;
	const char* attr_items[] = { "Brightness", "Contrast", "Gamma", "Gamma [R]", "Gamma [G]", "Gamma [B]", "Dark palette fix" };
	ImGui::SetNextItemWidth(120);
	if (ImGui::BeginCombo("##AttrCombo", attr_items[current_attr_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(attr_items); n++) {
			const bool is_selected = (current_attr_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(attr_items[n], is_selected)) {
				current_attr_idx = n;
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	
	int* val_ptr = nullptr;
	int min_val = -200;
	int max_val = 200;
	int multiplier = 10;
	
	switch (current_attr_idx) {
		case 0: val_ptr = &changed_prefs.gfx_luminance; break;
		case 1: val_ptr = &changed_prefs.gfx_contrast; break;
		case 2: val_ptr = &changed_prefs.gfx_gamma; break;
		case 3: val_ptr = &changed_prefs.gfx_gamma_ch[0]; break;
		case 4: val_ptr = &changed_prefs.gfx_gamma_ch[1]; break;
		case 5: val_ptr = &changed_prefs.gfx_gamma_ch[2]; break;
		case 6: 
			val_ptr = &changed_prefs.gfx_threebitcolors; 
			min_val = 0; max_val = 3; multiplier = 1;
			break;
	}

	if (val_ptr) {
		ImGui::SameLine();
		// Convert stored value to UI value
		int ui_val = *val_ptr / multiplier;
		ImGui::SetNextItemWidth(200);
		if (ImGui::SliderInt("##AttrSlider", &ui_val, min_val, max_val, "")) {
			*val_ptr = ui_val * multiplier;
		}
		ImGui::SameLine();
		// Input as float-like display to match WinUAE style logic "%.1f"
		// WinUAE displays (value / multiplier)
		// For editability, InputDouble or InputFloat is good.
		double disp_val = (double)*val_ptr / (double)multiplier;
		ImGui::SetNextItemWidth(60);
		if (ImGui::InputDouble("##AttrVal", &disp_val, 0, 0, "%.1f")) {
			*val_ptr = (int)(disp_val * multiplier);
			if (current_attr_idx == 6) { // clamp special case
				if (*val_ptr < 0) *val_ptr = 0;
				if (*val_ptr > 3) *val_ptr = 3;
			}
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::SameLine();

	// ---------------------------------------------------------
	// Right Column
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	
	// Centering Group
	// Increased height to avoid scrollbar
	ImGui::BeginChild("CenteringGroup", ImVec2(0, 90), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Centering");
		ImGui::EndMenuBar();
	}
	bool h_center = changed_prefs.gfx_xcenter == 2;
	if (AmigaCheckbox("Horizontal", &h_center)) changed_prefs.gfx_xcenter = h_center ? 2 : 0;
	
	bool v_center = changed_prefs.gfx_ycenter == 2;
	if (AmigaCheckbox("Vertical", &v_center)) changed_prefs.gfx_ycenter = v_center ? 2 : 0;
	ImGui::EndChild();

	// Line Mode Group
	// Increased height as requested
	ImGui::BeginChild("LineModeGroup", ImVec2(0, 160), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Line mode");
		ImGui::EndMenuBar();
	}
	
	if (!linemode_enabled) ImGui::BeginDisabled();
	if (ImGui::RadioButton("Single", changed_prefs.gfx_vresolution == 0 && changed_prefs.gfx_pscanlines == 0)) {
		changed_prefs.gfx_vresolution = 0; changed_prefs.gfx_pscanlines = 0;
	}
	if (ImGui::RadioButton("Double", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 0)) {
		changed_prefs.gfx_vresolution = 1; changed_prefs.gfx_pscanlines = 0;
	}
	if (ImGui::RadioButton("Scanlines", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 1)) {
		changed_prefs.gfx_vresolution = 1; changed_prefs.gfx_pscanlines = 1;
	}
	if (ImGui::RadioButton("Double, fields", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 2)) {
		changed_prefs.gfx_vresolution = 1; changed_prefs.gfx_pscanlines = 2;
	}
	if (ImGui::RadioButton("Double, fields+", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 3)) {
		changed_prefs.gfx_vresolution = 1; changed_prefs.gfx_pscanlines = 3;
	}
	if (!linemode_enabled) ImGui::EndDisabled();
	ImGui::EndChild();

	// Interlaced Line Mode Group
	ImGui::BeginChild("InterlacedLineModeGroup", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) {
		ImGui::Text("Interlaced line mode");
		ImGui::EndMenuBar();
	}
	
	bool is_double = changed_prefs.gfx_vresolution > 0;
	
	if (!linemode_enabled || is_double) ImGui::BeginDisabled();
	if (ImGui::RadioButton("Single##I", changed_prefs.gfx_iscanlines == 0 && !is_double)) {
		changed_prefs.gfx_iscanlines = 0;
	}
	if (!linemode_enabled || is_double) ImGui::EndDisabled();

	if (!linemode_enabled || !is_double) ImGui::BeginDisabled();
	// WinUAE has logic mapping these to 0, 1, 2 depending on values.
	// IDC_LM_IDOUBLED -> Frames? Is enabled if Double is selected.
	// My previous assumption regarding Double/Fields/Fields+ was iscanlines=1,2 etc.
	// Wait, CheckRadioButton says:
	// IDC_LM_INORMAL + (gfx_iscanlines ? gfx_iscanlines + 1: (gfx_vresolution ? 1 : 0))
	// If IDoubled (Double frames) is selected (val=1?), iscanlines must be...
	// If ISingle selected (val=0), iscanlines=0.
	// We need to match this.
	// Let's rely on radio button variable binding.
	// We need 4 options here.
	// Single, Double frames, Double fields, Double fields+
	// If Single##I is selected (RadioButton 0), it sets iscanlines = 0.
	// If Double frames##I (RadioButton 1) ?
	// If Double fields##I (RadioButton 2) ?
	// If Double fields+##I (RadioButton 3) ?
	// Mapping:
	// If ISingle: iscan=0.
	// If IDouble frames: iscan=0? No, that would overlap.
	// In WinUAE, if Double is selected, vres>0. 
	// The check code: INORMAL + (iscan? iscan+1 : (vres?1:0))
	// If vres=1, scan=0 -> INORMAL + 1 -> IDOUBLED. 
	// So IDOUBLED (Double frames) maps to iscan=0 when vres=1.
	// IDOUBLED2 (Double fields) maps to iscan=1.
	// IDOUBLED3 (Double fields+) maps to iscan=2.
	
	// So:
	// Single##I -> enabled if !is_double. iscan=0.
	// Double Frames##I -> enabled if is_double. iscan=0.
	// Double Fields##I -> enabled if is_double. iscan=1.
	// Double Fields+##I -> enabled if is_double. iscan=2.
	
	// Implementation:
	// Toggle radio button for "Double frames" manually if iscan==0.
	bool dbl_frames_active = (changed_prefs.gfx_iscanlines == 0 && is_double);
	if (ImGui::RadioButton("Double, frames##I", dbl_frames_active)) { changed_prefs.gfx_iscanlines = 0; }
	ImGui::RadioButton("Double, fields##I", &changed_prefs.gfx_iscanlines, 1);
	ImGui::RadioButton("Double, fields+##I", &changed_prefs.gfx_iscanlines, 2);
	if (!linemode_enabled || !is_double) ImGui::EndDisabled();
	ImGui::EndChild();

	ImGui::EndGroup(); // End Right Column
}
