#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "disk.h"

void render_panel_diskswapper()
{
    ImGui::BeginChild("DiskSwapperList", ImVec2(0, -50), true);
    ImGui::Columns(3, "DiskSwapperColumns");
    ImGui::Separator();
    ImGui::Text("#"); ImGui::NextColumn();
    ImGui::Text("Disk Image"); ImGui::NextColumn();
    ImGui::Text("Drive"); ImGui::NextColumn();
    ImGui::Separator();
    for (int i = 0; i < MAX_SPARE_DRIVES; ++i)
    {
        ImGui::PushID(i);
        ImGui::Text("%d", i + 1); ImGui::NextColumn();
        // Unique IDs per row to avoid collisions and potential crashes
        ImGui::InputText("##DiskPath", changed_prefs.dfxlist[i], MAX_DPATH);
        ImGui::SameLine();
        if (ImGui::Button("...##Browse"))
        {
            // Select file (TODO)
        }
        ImGui::SameLine();
        if (ImGui::Button("X##Clear"))
        {
            changed_prefs.dfxlist[i][0] = 0;
        }
        ImGui::NextColumn();
        const int drive = disk_in_drive(i);
        char drive_btn[16];
        if (drive < 0)
            snprintf(drive_btn, sizeof(drive_btn), "-##%d", i);
        else
            snprintf(drive_btn, sizeof(drive_btn), "%d##%d", drive, i);
        if (ImGui::Button(drive_btn))
        {
            disk_swap(i, 1);
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    if (ImGui::Button("Remove All"))
    {
        for (int row = 0; row < MAX_SPARE_DRIVES; ++row)
            changed_prefs.dfxlist[row][0] = 0;
    }
}
