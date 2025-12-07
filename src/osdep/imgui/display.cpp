#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_display()
{
	ImGui::Text("Amiga Screen");
	const char* screenmode_items[] = { "Windowed", "Fullscreen", "Full-window" };
	ImGui::Combo("Screen mode", &changed_prefs.gfx_apmode[0].gfx_fullscreen, screenmode_items, IM_ARRAYSIZE(screenmode_items));
	ImGui::Checkbox("Manual Crop", &changed_prefs.gfx_manual_crop);
	ImGui::SliderInt("Width", &changed_prefs.gfx_manual_crop_width, 0, 800);
	ImGui::SliderInt("Height", &changed_prefs.gfx_manual_crop_height, 0, 600);
	ImGui::Checkbox("Auto Crop", &changed_prefs.gfx_auto_crop);
	ImGui::Checkbox("Borderless", &changed_prefs.borderless);
	const char* vsync_items[] = { "-", "Lagless", "Lagless 50/60Hz", "Standard", "Standard 50/60Hz" };
	ImGui::Combo("VSync Native", &changed_prefs.gfx_apmode[0].gfx_vsync, vsync_items, IM_ARRAYSIZE(vsync_items));
	ImGui::Combo("VSync RTG", &changed_prefs.gfx_apmode[1].gfx_vsync, vsync_items, IM_ARRAYSIZE(vsync_items));
	ImGui::SliderInt("H. Offset", &changed_prefs.gfx_horizontal_offset, -80, 80);
	ImGui::SliderInt("V. Offset", &changed_prefs.gfx_vertical_offset, -80, 80);

	ImGui::Separator();
	ImGui::Text("Centering");
	ImGui::Checkbox("Horizontal", (bool*)&changed_prefs.gfx_xcenter);
	ImGui::Checkbox("Vertical", (bool*)&changed_prefs.gfx_ycenter);

	ImGui::Separator();
	ImGui::Text("Line mode");
	ImGui::RadioButton("Single", &changed_prefs.gfx_vresolution, 0);
	ImGui::RadioButton("Double", &changed_prefs.gfx_vresolution, 1);
	ImGui::RadioButton("Scanlines", &changed_prefs.gfx_pscanlines, 1);
	ImGui::RadioButton("Double, frames", &changed_prefs.gfx_iscanlines, 0);
	ImGui::RadioButton("Double, fields##Interlaced", &changed_prefs.gfx_iscanlines, 1);
	ImGui::RadioButton("Double, fields+##Interlaced", &changed_prefs.gfx_iscanlines, 2);

	ImGui::Separator();
	const char* scaling_items[] = { "Auto", "Pixelated", "Smooth", "Integer" };
	ImGui::Combo("Scaling method", &changed_prefs.scaling_method, scaling_items, IM_ARRAYSIZE(scaling_items));
	const char* resolution_items[] = { "LowRes", "HighRes (normal)", "SuperHighRes" };
	ImGui::Combo("Resolution", &changed_prefs.gfx_resolution, resolution_items, IM_ARRAYSIZE(resolution_items));
	ImGui::Checkbox("Filtered Low Res", (bool*)&changed_prefs.gfx_lores_mode);
	const char* res_autoswitch_items[] = { "Disabled", "Always On", "10%", "33%", "66%" };
	ImGui::Combo("Res. autoswitch", &changed_prefs.gfx_autoresolution, res_autoswitch_items, IM_ARRAYSIZE(res_autoswitch_items));
	ImGui::Checkbox("Frameskip", (bool*)&changed_prefs.gfx_framerate);
	ImGui::SliderInt("Refresh", &changed_prefs.gfx_framerate, 1, 10);
	ImGui::Checkbox("FPS Adj:", (bool*)&changed_prefs.cr[changed_prefs.cr_selected].locked);
	ImGui::SliderFloat("##FPSAdj", &changed_prefs.cr[changed_prefs.cr_selected].rate, 1, 100);
	ImGui::Checkbox("Correct Aspect Ratio", (bool*)&changed_prefs.gfx_correct_aspect);
	ImGui::Checkbox("Blacker than black", &changed_prefs.gfx_blackerthanblack);
	ImGui::Checkbox("Remove interlace artifacts", &changed_prefs.gfx_scandoubler);
	ImGui::SliderInt("Brightness", &changed_prefs.gfx_luminance, -200, 200);
}
