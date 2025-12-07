#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "savestate.h"
#include <string>

void render_panel_savestates()
{
	for (int i = 0; i < 15; ++i)
	{
		ImGui::RadioButton(std::to_string(i).c_str(), &current_state_num, i);
	}
	ImGui::Separator();
	ImGui::Text("Filename: %s", savestate_fname);
	ImGui::Text("Timestamp: %s", "");
	if (ImGui::Button("Load from Slot"))
	{
		// Load state from selected slot
	}
	ImGui::SameLine();
	if (ImGui::Button("Save to Slot", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Save current state to selected slot
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete Slot", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Delete state from selected slot
	}
	if (ImGui::Button("Load state...", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Load state from file
	}
	ImGui::SameLine();
	if (ImGui::Button("Save state...", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		// Save current state to file
	}
}
