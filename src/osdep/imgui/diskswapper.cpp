#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "target.h"

static int diskswapper_target_slot = -1;

void render_panel_diskswapper()
{
    ImGui::BeginChild("DiskSwapperList", ImVec2(0, -50), true);
    if (ImGui::BeginTable("DiskSwapperTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        // Set up column widths
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("Disk Image", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 4);
        ImGui::TableSetupColumn("Drive", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (int i = 0; i < MAX_SPARE_DRIVES; ++i)
        {
            ImGui::PushID(i);
            ImGui::TableNextRow();

            // Column 1: Index + Controls
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", i + 1);
            ImGui::SameLine();

            if (AmigaButton("..."))
            {
                 diskswapper_target_slot = i;
                 std::string startPath = changed_prefs.dfxlist[i];
                 if (startPath.empty()) startPath = get_floppy_path();

                 // Use a standard filter similar to Floppy panel
                 const char* filter = "All Supported Files (*.adf,*.adz,*.dms,*.ipf,*.zip,*.7z){.adf,.adz,.dms,.ipf,.zip,.7z},All Files (*){.*}";
                 OpenFileDialogKey("DISKSWAP", "Select disk image file", filter, startPath);
            }
            ImGui::SameLine();
            if (AmigaButton("X"))
            {
                changed_prefs.dfxlist[i][0] = 0;
            }

            // Column 2: Disk Image Path
            ImGui::TableNextColumn();
            const char *display_path = changed_prefs.dfxlist[i];
            std::string path_str = display_path;
            size_t last_sep = path_str.find_last_of("/\\");
            if (last_sep != std::string::npos) {
                display_path = display_path + last_sep + 1;
            }

            ImGui::BeginDisabled();
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            ImGui::InputText("##Path", (char*)display_path, MAX_DPATH, ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();

            // Column 3: Drive Button
            ImGui::TableNextColumn();
            const int drive = disk_in_drive(i);
            char drive_btn[16];
            if (drive < 0)
                snprintf(drive_btn, sizeof(drive_btn), " "); // Empty if not in drive
            else
                snprintf(drive_btn, sizeof(drive_btn), "DF%d:", drive);

            if (AmigaButton(drive_btn, ImVec2(-1, 0)))
            {
                disk_swap(i, 1);
                add_file_to_mru_list(lstMRUDiskList, std::string(changed_prefs.dfxlist[i]));
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    if (AmigaButton("Remove All", ImVec2(120, 0)))
    {
        for (int row = 0; row < MAX_SPARE_DRIVES; ++row)
        {
            changed_prefs.dfxlist[row][0] = 0;
            disk_swap(row, -1);
        }
    }
    
    // Handle Dialog Result
    std::string result_path;
    if (ConsumeFileDialogResultKey("DISKSWAP", result_path)) {
        if (!result_path.empty() && diskswapper_target_slot >= 0 && diskswapper_target_slot < MAX_SPARE_DRIVES) {
             // Check content of slot?
             if (strncmp(changed_prefs.dfxlist[diskswapper_target_slot], result_path.c_str(), MAX_DPATH) != 0)
             {
                 strncpy(changed_prefs.dfxlist[diskswapper_target_slot], result_path.c_str(), MAX_DPATH);
                 add_file_to_mru_list(lstMRUDiskList, result_path);
             }
        }
        diskswapper_target_slot = -1;
    }
}
