#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_expansions()
{
	ImGui::Text("Expansion Board Settings");
	// TODO: Implement Expansion Board Settings

	ImGui::Separator();
	ImGui::Text("Accelerator Board Settings");
	// TODO: Implement Accelerator Board Settings

	ImGui::Separator();
	ImGui::Text("Miscellaneous Expansions");
	ImGui::Checkbox("bsdsocket.library", &changed_prefs.socket_emu);
	ImGui::Checkbox("uaescsi.device", (bool*)&changed_prefs.scsi);
	ImGui::Checkbox("CD32 Full Motion Video cartridge", &changed_prefs.cs_cd32fmv);
	ImGui::Checkbox("uaenet.device", &changed_prefs.sana2);
}
