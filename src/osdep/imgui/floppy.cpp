#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_floppy()
{
    for (int i = 0; i < 4; ++i)
    {
        ImGui::PushID(i);
        char label[16];
        snprintf(label, sizeof(label), "DF%d:", i);
        ImGui::Checkbox(label, (bool*)&changed_prefs.floppyslots[i].dfxtype);
        ImGui::SameLine();
        ImGui::Checkbox("Write-protected", &changed_prefs.floppy_read_only);
        ImGui::SameLine();
        // Use a unique ID for each input to avoid collisions and potential crashes
        ImGui::InputText("##FloppyPath", changed_prefs.floppyslots[i].df, MAX_DPATH);
        ImGui::PopID();
    }
    ImGui::SliderInt("Floppy Drive Emulation Speed", &changed_prefs.floppy_speed, 0, 800);
    if (ImGui::Button("Create 3.5\" DD disk"))
    {
        // Create 3.5" DD Disk
    }
    if (ImGui::Button("Create 3.5\" HD disk"))
    {
        // Create 3.5" HD Disk
    }
    if (ImGui::Button("Save config for disk"))
    {
        // Save configuration for current disk
    }
}
