#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_hd()
{
	for (int i = 0; i < changed_prefs.mountitems; ++i)
	{
		ImGui::Text("Device: %s, Volume: %s, Path: %s", changed_prefs.mountconfig[i].ci.devname, changed_prefs.mountconfig[i].ci.volname, changed_prefs.mountconfig[i].ci.rootdir);
	}
	if (ImGui::Button("Add Directory/Archive", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Add Directory/Archive
	}
	if (ImGui::Button("Add Hardfile", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Add Hardfile
	}
	if (ImGui::Button("Add Hard Drive", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Add Hard Drive
	}
	if (ImGui::Button("Add CD Drive", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Add CD Drive
	}
	if (ImGui::Button("Add Tape Drive", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Add Tape Drive
	}
	if (ImGui::Button("Create Hardfile", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Create Hardfile
	}
	ImGui::Checkbox("CDFS automount CD/DVD drives", &changed_prefs.automount_cddrives);
	ImGui::Checkbox("CD drive/image", &changed_prefs.cdslots[0].inuse);
	ImGui::InputText("##CDFile", changed_prefs.cdslots[0].name, MAX_DPATH);
	if (ImGui::Button("Eject"))
	{
		// Eject CD
	}
	if (ImGui::Button("Select image file"))
	{
		// Select CD image file
	}
	ImGui::Checkbox("CDTV/CDTV-CR/CD32 turbo CD read speed", (bool*)&changed_prefs.cd_speed);
}
