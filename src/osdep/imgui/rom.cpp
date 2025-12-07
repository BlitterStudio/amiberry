#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_rom()
{
	ImGui::Text("Main ROM File:");
	ImGui::InputText("##MainROM", changed_prefs.romfile, MAX_DPATH);
	ImGui::Text("Extended ROM File:");
	ImGui::InputText("##ExtROM", changed_prefs.romextfile, MAX_DPATH);
	ImGui::Text("Cartridge ROM File:");
	ImGui::InputText("##CartROM", changed_prefs.cartfile, MAX_DPATH);
	ImGui::Checkbox("MapROM emulation", (bool*)&changed_prefs.maprom);
	ImGui::Checkbox("ShapeShifter support", &changed_prefs.kickshifter);
	const char* uae_items[] = { "ROM disabled", "Original UAE (FS + F0 ROM)", "New UAE (64k + F0 ROM)", "New UAE (128k, ROM, Direct)", "New UAE (128k, ROM, Indirect)" };
	ImGui::Combo("Advanced UAE expansion board/Boot ROM", &changed_prefs.uaeboard, uae_items, IM_ARRAYSIZE(uae_items));
}
