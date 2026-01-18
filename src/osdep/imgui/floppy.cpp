#include "imgui.h"
#include "imgui_internal.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "disk.h"
#include "uae.h"
#include "parser.h"
#include <string>
#include <vector>

#ifdef FLOPPYBRIDGE
#include "floppybridge_lib.h"
static std::vector<FloppyBridgeAPI::DriverInformation> driver_list;
static bool drawbridge_initialized = false;
#endif

extern std::vector<std::string> lstMRUDiskList;

static const char* drive_speed_list[] = {"Turbo", "100% (compatible)", "200%", "400%", "800%"};
static const int drive_speed_values[] = {0, 100, 200, 400, 800};

static const char* drive_cable_list[] = {
	"Drive A (IBM PC)",
	"Drive B (IBM PC)",
	"Drive 0 (SHUGART)",
	"Drive 1 (SHUGART)",
	"Drive 2 (SHUGART)",
	"Drive 3 (SHUGART)"
};

// Mode strings referenced in PanelFloppy.cpp
static const std::vector<std::string> floppy_bridge_modes = { "Normal", "Compatible", "Turbo", "Stalling" };

// Full list of drive types matching gui_handling.h (excluding "Disabled" as we have a checkbox)
static const char* drive_types[] = {
    "3.5\" DD",
    "3.5\" HD",
    "5.25\" (40)",
    "5.25\" (80)", 
    "3.5\" ESCOM",
    "FB: Normal",
    "FB: Compatible",
    "FB: Turbo",
    "FB: Stalling"
};

enum class FloppyDialogMode {
    None,
    SelectDF0,
    SelectDF1,
    SelectDF2,
    SelectDF3,
    CreateDD,
    CreateHD
};
static FloppyDialogMode current_floppy_dialog_mode = FloppyDialogMode::None;

static std::string format_mru_entry(const std::string& fullpath) {
    size_t last_slash = fullpath.find_last_of("/\\");
    std::string filename = (last_slash == std::string::npos) ? fullpath : fullpath.substr(last_slash + 1);
    return filename + " { " + fullpath + " }";
}

static void RenderDriveSlot(int i)
{
    ImGui::PushID(i);
        
    char table_id[32];
    snprintf(table_id, sizeof(table_id), "DriveTable%d", i);
    
    if (ImGui::BeginTable(table_id, 7, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("En", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 130.0f); 
        ImGui::TableSetupColumn("WP", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Filler", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Eject", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Sel", ImGuiTableColumnFlags_WidthFixed, 30.0f);

        ImGui::TableNextRow();
        
        // 1. Enabled Checkbox
        ImGui::TableNextColumn();
        bool enabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
        char label[8];
        snprintf(label, sizeof(label), "DF%d:", i);
        if (ImGui::Checkbox(label, &enabled)) {
             if (enabled && changed_prefs.floppyslots[i].dfxtype == DRV_NONE)
                 changed_prefs.floppyslots[i].dfxtype = DRV_35_DD;
             else if (!enabled)
                 changed_prefs.floppyslots[i].dfxtype = DRV_NONE;
        }

        ImGui::BeginDisabled(!enabled);

        // 2. Type Dropdown
        ImGui::TableNextColumn();
        int type_idx = fromdfxtype(i, changed_prefs.floppyslots[i].dfxtype, changed_prefs.floppyslots[i].dfxsubtype);
        
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##Type", drive_types[type_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(drive_types); n++) {
                const bool is_selected = (type_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(drive_types[n], is_selected)) {
                    type_idx = n;
                    int sub;
                    int dfxtype = todfxtype(i, type_idx, &sub);
                    changed_prefs.floppyslots[i].dfxtype = dfxtype;
                    changed_prefs.floppyslots[i].dfxsubtype = sub;

                    if (dfxtype == DRV_FB) {
                        if (type_idx >= 5 && (type_idx - 5) < 4) {
                            TCHAR tmp[32];
                            extern const std::vector<std::string> floppy_bridge_modes;
                            _sntprintf(tmp, sizeof tmp, _T("%d:%s"), type_idx - 4, floppy_bridge_modes[type_idx - 5].c_str());
                            _tcscpy(changed_prefs.floppyslots[i].dfxsubtypeid, tmp);
                        }
                    } else {
                        changed_prefs.floppyslots[i].dfxsubtypeid[0] = 0;
                    }
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        // 3. WP Checkbox
        ImGui::TableNextColumn();
        bool wp = disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i);
        if (ImGui::Checkbox("##WP", &wp)) {
            disk_setwriteprotect(&changed_prefs, i, changed_prefs.floppyslots[i].df, wp);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Write-protected");
        
        // 4. Filler
        ImGui::TableNextColumn();
        
        // 5. Info Button
        ImGui::TableNextColumn();
        if (ImGui::Button("?", ImVec2(-1, 0))) {
             if (strlen(changed_prefs.floppyslots[i].df) > 0)
                DisplayDiskInfo(i);
        }
        
        // 6. Eject Button
        ImGui::TableNextColumn();
        if (ImGui::Button("Eject", ImVec2(-1, 0))) {
             disk_eject(i);
             changed_prefs.floppyslots[i].df[0] = 0;
        }
        
        // 7. Select Button
        ImGui::TableNextColumn();
        if (ImGui::Button("...", ImVec2(-1, 0))) {
             current_floppy_dialog_mode = static_cast<FloppyDialogMode>(static_cast<int>(FloppyDialogMode::SelectDF0) + i);
             std::string startPath = changed_prefs.floppyslots[i].df;
             if (startPath.empty()) startPath = get_floppy_path(); 
             OpenFileDialog("Select Disk Image", "All Supported Files (*.adf,*.adz,*.dms,*.ipf,*.zip,*.7z,*.lha,*.lzh,*.lzx,*.fdi,*.scp,*.gz,*.xz,*.hdf,*.img){.adf,.adz,.dms,.ipf,.zip,.7z,.lha,.lzh,.lzx,.fdi,.scp,.gz,.xz,.hdf,.img},All Files (*){.*}", startPath);
        }
        
        ImGui::EndDisabled();
        ImGui::EndTable();
    }

    // --- ROW 2: File Path Combo ---
    bool enabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
    ImGui::BeginDisabled(!enabled);
    
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    std::string current_val = changed_prefs.floppyslots[i].df;
    if (current_val.empty()) current_val = "";
    
    if (ImGui::BeginCombo("##File", current_val.c_str(), 0))
    {
        for (int h = 0; h < lstMRUDiskList.size(); ++h) {
            std::string label = format_mru_entry(lstMRUDiskList[h]);
            bool is_selected = (current_val == lstMRUDiskList[h]);
            if (is_selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            }

            if (ImGui::Selectable(label.c_str(), is_selected)) {
                 strncpy(changed_prefs.floppyslots[i].df, lstMRUDiskList[h].c_str(), MAX_DPATH);
                 disk_insert(i, changed_prefs.floppyslots[i].df);
                 DISK_history_add(changed_prefs.floppyslots[i].df, -1, HISTORY_FLOPPY, 0);
            }

            if (is_selected)
            {
                ImGui::PopStyleColor(1);
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::PopID(); 
}

#ifdef FLOPPYBRIDGE
static void RenderDrawBridge()
{
    if (ImGui::CollapsingHeader("DrawBridge (FloppyBridge)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginGroup();
        
        // --- Driver Selection ---
        ImGui::Text("DrawBridge driver:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300);
        
        std::string current_driver = "Select Driver...";
        if (changed_prefs.drawbridge_driver >= 0 && changed_prefs.drawbridge_driver < driver_list.size()) {
            current_driver = driver_list[changed_prefs.drawbridge_driver].name;
        } else if (driver_list.empty()) {
            current_driver = "No drivers found";
        }
        
        if (ImGui::BeginCombo("##DBDriver", current_driver.c_str())) {
            if (driver_list.empty()) {
                ImGui::Selectable("No drivers found", false, ImGuiSelectableFlags_Disabled);
            }
            for (int i = 0; i < driver_list.size(); ++i) {
                bool is_selected = (changed_prefs.drawbridge_driver == i);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(driver_list[i].name, is_selected)) {
                     changed_prefs.drawbridge_driver = i;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            FloppyBridgeAPI::getDriverList(driver_list);
        }

        // --- Serial Port ---
        bool auto_serial = changed_prefs.drawbridge_serial_auto;
        if(ImGui::Checkbox("DrawBridge: Auto-Detect serial port", &auto_serial)) 
            changed_prefs.drawbridge_serial_auto = auto_serial;
        
        ImGui::BeginDisabled(auto_serial);
        ImGui::SetNextItemWidth(300);
        
        std::string current_port = "none";
        if (changed_prefs.drawbridge_serial_port[0] != 0) {
            current_port = changed_prefs.drawbridge_serial_port;
        }

        // Fetch serial ports dynamically if needed, accessing the global vector
        extern std::vector<std::string> serial_ports;

        if (ImGui::BeginCombo("##DBSerial", current_port.c_str())) {
            bool is_none_selected = (changed_prefs.drawbridge_serial_port[0] == 0);
            if (is_none_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable("none", is_none_selected)) {
                changed_prefs.drawbridge_serial_port[0] = 0;
            }
            if (is_none_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
            
            for (const auto& port : serial_ports) {
                bool is_selected = (current_port == port);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(port.c_str(), is_selected)) {
                    _sntprintf(changed_prefs.drawbridge_serial_port, 256, "%s", port.c_str());
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        
        // --- Smart Speed ---
        bool smart_speed = changed_prefs.drawbridge_smartspeed;
        if (ImGui::Checkbox("DrawBridge: Smart Speed (Dynamically switch on Turbo)", &smart_speed))
            changed_prefs.drawbridge_smartspeed = smart_speed;

        // --- Auto Cache ---
        bool auto_cache = changed_prefs.drawbridge_autocache;
        if (ImGui::Checkbox("DrawBridge: Auto-Cache (Cache disk data while drive is idle)", &auto_cache))
            changed_prefs.drawbridge_autocache = auto_cache;

        // --- Drive Cable ---
        ImGui::Text("DrawBridge: Drive cable:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        int cable_idx = changed_prefs.drawbridge_drive_cable;
        if (ImGui::BeginCombo("##DBCable", drive_cable_list[cable_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(drive_cable_list); n++) {
                const bool is_selected = (cable_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(drive_cable_list[n], is_selected)) {
                    changed_prefs.drawbridge_drive_cable = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::EndGroup();
    }
}
#endif

void render_panel_floppy()
{
#ifdef FLOPPYBRIDGE
    if (!drawbridge_initialized) {
        FloppyBridgeAPI::getDriverList(driver_list);
        drawbridge_initialized = true;
    }
#endif

    ImGui::Indent(5.0f);
    
    // ---------------------------------------------------------
    // FLOPPY DRIVES GROUP
    // ---------------------------------------------------------
    BeginGroupBox("Floppy Drives");
    for (int i = 0; i < 4; ++i)
    {
        RenderDriveSlot(i);
    }
    EndGroupBox("Floppy Drives");

    // ---------------------------------------------------------
    // EMULATION SPEED
    // ---------------------------------------------------------
    BeginGroupBox("Floppy Drive Emulation Speed");
    
    int speed_idx = 1; 
    for(int i=0; i<5; ++i) {
        if (changed_prefs.floppy_speed == drive_speed_values[i]) {
            speed_idx = i;
            break;
        }
    }
    
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    if (ImGui::SliderInt("##SpeedSlider", &speed_idx, 0, 4, "")) {
        changed_prefs.floppy_speed = drive_speed_values[speed_idx];
    }
    ImGui::SameLine();
    
    ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    char speed_label[32];
    strncpy(speed_label, drive_speed_list[speed_idx], 32);
    ImGui::InputText("##SpeedLabel", speed_label, 32, ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();

    EndGroupBox("Floppy Drive Emulation Speed");

    // ---------------------------------------------------------
    // ACTIONS
    // ---------------------------------------------------------
    BeginGroupBox("New Floppy Disk Image");
    
    float avail_w = ImGui::GetContentRegionAvail().x;
    float btn_w = (avail_w - 20) / 3.0f;
    
    if (ImGui::Button("Create 3.5'' DD disk", ImVec2(btn_w, 0))) {
        current_floppy_dialog_mode = FloppyDialogMode::CreateDD;
        OpenFileDialog("Create 3.5\" DD disk file", "Amiga Disk File (*.adf){.adf}", current_dir); 
    }
    ImGui::SameLine();
    if (ImGui::Button("Create 3.5'' HD disk", ImVec2(btn_w, 0))) {
        current_floppy_dialog_mode = FloppyDialogMode::CreateHD;
        OpenFileDialog("Create 3.5\" HD disk file", "Amiga Disk File (*.adf){.adf}", current_dir);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(strlen(changed_prefs.floppyslots[0].df) == 0);
    if (ImGui::Button("Save config for disk", ImVec2(btn_w, 0))) {
        char filename[MAX_DPATH];
        char diskname[MAX_DPATH];
        extract_filename(changed_prefs.floppyslots[0].df, diskname);
        remove_file_extension(diskname);
        get_configuration_path(filename, MAX_DPATH);
        strncat(filename, diskname, MAX_DPATH - 1);
        strncat(filename, ".uae", MAX_DPATH - 1);
        
        _sntprintf(changed_prefs.description, sizeof changed_prefs.description, "Configuration for disk '%s'", diskname);
        cfgfile_save(&changed_prefs, filename, 0);
    }
    ImGui::EndDisabled();

    EndGroupBox("New Floppy Disk Image");

#ifdef FLOPPYBRIDGE
    RenderDrawBridge();
#endif

    ImGui::Unindent(5.0f);

    std::string result_path;
    if (ConsumeFileDialogResult(result_path)) {
        if (!result_path.empty()) {
            if (current_floppy_dialog_mode == FloppyDialogMode::CreateDD) {
                char diskname[MAX_DPATH];
                extract_filename(result_path.c_str(), diskname);
                remove_file_extension(diskname);
                diskname[31] = '\0';
                disk_creatediskfile(&changed_prefs, result_path.c_str(), 0, DRV_35_DD, -1, diskname, false, false, nullptr);
                DISK_history_add(result_path.c_str(), -1, HISTORY_FLOPPY, 0);
            }
            else if (current_floppy_dialog_mode == FloppyDialogMode::CreateHD) {
                char diskname[MAX_DPATH];
                extract_filename(result_path.c_str(), diskname);
                remove_file_extension(diskname);
                diskname[31] = '\0';
                disk_creatediskfile(&changed_prefs, result_path.c_str(), 0, DRV_35_HD, -1, diskname, false, false, nullptr);
                DISK_history_add(result_path.c_str(), -1, HISTORY_FLOPPY, 0);
            }
            else {
                int drive_idx = -1;
                switch(current_floppy_dialog_mode) {
                    case FloppyDialogMode::SelectDF0: drive_idx = 0; break;
                    case FloppyDialogMode::SelectDF1: drive_idx = 1; break;
                    case FloppyDialogMode::SelectDF2: drive_idx = 2; break;
                    case FloppyDialogMode::SelectDF3: drive_idx = 3; break;
                    default: break;
                }
                
                if (drive_idx >= 0) {
                     strncpy(changed_prefs.floppyslots[drive_idx].df, result_path.c_str(), MAX_DPATH);
                     disk_insert(drive_idx, result_path.c_str());
                     DISK_history_add(result_path.c_str(), -1, HISTORY_FLOPPY, 0);
                }
            }
        }
        current_floppy_dialog_mode = FloppyDialogMode::None;
    }
}
