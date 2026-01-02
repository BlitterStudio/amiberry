#include "sysdeps.h"
#include "options.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "filesys.h"
#include "autoconf.h"
#include "blkdev.h"
#include "rommgr.h"

extern void new_cddrive(int entry);
extern void new_tapedrive(int entry);

// Enum for Dialog Modes (Modals)
enum class HDDialogMode {
    None,
    AddDir,
    AddHDF,
    AddHardDrive,
    CreateHDF,
    EditEntry,
    AddCD,
    AddTape
};


static HDDialogMode current_hd_dialog_mode = HDDialogMode::None;
static int edit_entry_index = -1;
static int selected_row = -1; // Track selected drive in the list

extern void default_fsvdlg(struct hardfiledata *hfd);
extern void default_hfdlg(struct hardfiledata *hfd, bool rdb); 
extern void CreateDefaultDevicename(char* name); 
extern int tweakbootpri(int bp, int ab, int dnm);
extern void new_filesys(int entry);
extern void new_hardfile(int entry);
extern void updatehdfinfo(bool force, bool defaults, bool realdrive, std::string& txtHdfInfo, std::string& txtHdfInfo2);
extern std::vector<controller_map> controller; 

extern int hdf_getnumharddrives(void);
extern TCHAR* hdf_getnameharddrive(int index, int flags, int* sectorsize, int* dangerousdrive, uae_u32* outflags);
extern int vhd_create(const TCHAR* name, uae_u64 size, uae_u32 flags);
extern void new_harddrive(int entry);
extern void new_cddrive(int entry);
extern void new_tapedrive(int entry);

extern std::vector<std::string> lstMRUCDList;

static std::string hdf_info_text1;
static std::string hdf_info_text2;

static bool show_virtual_filesys_modal = false;
static bool show_hardfile_modal = false;
static bool show_create_hdf_modal = false;
static bool show_add_harddrive_modal = false;
static bool show_cd_modal = false;
static bool show_tape_modal = false;

static int create_hdf_size_mb = 100;
static bool create_hdf_dynamic = false;
static bool create_hdf_rdb = false;

// Helper to handle TCHAR arrays in ImGui
static bool InputTextT(const char* label, TCHAR* buf, size_t buf_size, ImGuiInputTextFlags flags = 0)
{
    size_t char_buf_size = buf_size * 3; 
    char* tempbuffer = (char*)malloc(char_buf_size);
    if (!tempbuffer) return false;
    
    // Copy TCHAR to char (UTF8/ANSI)
    ua_copy(tempbuffer, char_buf_size, buf);
    bool changed = ImGui::InputText(label, tempbuffer, char_buf_size, flags);
    if (changed) {
        // Copy char to TCHAR
        au_copy(buf, buf_size, tempbuffer);
    }
    
    free(tempbuffer);
    return changed;
}

static void harddisktype(char* s, const struct uaedev_config_info* ci)
{
	switch (ci->type)
	{
	case UAEDEV_CD:
		strcpy(s, "CD");
		break;
	case UAEDEV_TAPE:
		strcpy(s, "TAPE");
		break;
	case UAEDEV_HDF:
		strcpy(s, "HDF");
		break;
	default:
		strcpy(s, "n/a");
		break;
	}
}

static std::string format_size(long long size) {
    char buffer[32];
    if (size >= 1024 * 1024 * 1024)
        snprintf(buffer, sizeof(buffer), "%.1fG", (double)(size / (1024 * 1024)) / 1024.0);
    else if (size < 10 * 1024 * 1024)
        snprintf(buffer, sizeof(buffer), "%lldK", size / 1024);
    else
        snprintf(buffer, sizeof(buffer), "%.1fM", (double)(size / 1024) / 1024.0);
    return std::string(buffer);
}

static void RenderMountedDrives()
{
    // WinUAE-style list: Device, Volume, Path, R/W, Size, BootPri
    // No explicit Edit/Del columns. Selection -> Button action.
    
    BeginGroupBox("Mounted Drives");

    // Reserve more space for 3 rows of buttons + CD section
    ImGui::BeginChild("MountedDrivesList", ImVec2(0, -280), true); 

    if (ImGui::BeginTable("MountedDrivesTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("Device", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("R/W", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Boot", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableHeadersRow();

        for (int row = 0; row < changed_prefs.mountitems; ++row)
        {
            ImGui::PushID(row);
            auto* uci = &changed_prefs.mountconfig[row];
            auto* const ci = &uci->ci;
            
            struct mountedinfo mi{};
            int type = get_filesys_unitconfig(&changed_prefs, row, &mi);
            
            bool nosize = false;
            if (type < 0) {
                type = (ci->type == UAEDEV_HDF || ci->type == UAEDEV_CD || ci->type == UAEDEV_TAPE) ? FILESYS_HARDFILE : FILESYS_VIRTUAL;
                nosize = true;
            }
            if (mi.size < 0) nosize = true;

            // Handle TCHAR rootdir
            std::string rootdir_str;
            char* tmp_root = ua(mi.rootdir ? mi.rootdir : ci->rootdir);
            if (tmp_root) {
                rootdir_str = tmp_root;
                xfree(tmp_root);
            }
            
            if (rootdir_str.find("HD_") == 0) rootdir_str = rootdir_str.substr(3);
            if (rootdir_str.size() > 1 && rootdir_str[0] == ':') {
                 size_t colon = rootdir_str.find(':', 1);
                 if (colon != std::string::npos) rootdir_str = rootdir_str.substr(1, colon - 1); 
                 else rootdir_str = rootdir_str.substr(1);
            }

            char size_str[32];
            if (nosize) strcpy(size_str, "n/a");
            else strcpy(size_str, format_size(mi.size).c_str());

            char devname_str[256];
            char volname_str[256];
            char bootpri_str[32];
            
            char* tmp_dev = ua(ci->devname);
            char* tmp_vol = ua(ci->volname);
            strncpy(devname_str, tmp_dev, sizeof(devname_str));
            strncpy(volname_str, tmp_vol, sizeof(volname_str));
            xfree(tmp_dev); xfree(tmp_vol);
            
            snprintf(bootpri_str, sizeof(bootpri_str), "%d", ci->bootpri);

            int ctype = ci->controller_type;
            if (ctype >= HD_CONTROLLER_TYPE_IDE_FIRST && ctype <= HD_CONTROLLER_TYPE_IDE_LAST) {
                const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
                if (ert) {
                    char* friendly = ua(ert->friendlyname);
                    if (friendly) {
                        if (ci->controller_type_unit == 0)
                            snprintf(devname_str, sizeof(devname_str), "%s:%d", friendly, ci->controller_unit);
                        else
                            snprintf(devname_str, sizeof(devname_str), "%s:%d/%d", friendly, ci->controller_unit, ci->controller_type_unit + 1);
                        xfree(friendly);
                    } else {
                         strcpy(devname_str, "Unknown");
                    }
                } else {
                    const char* idedevs[] = { "IDE:%d", "A600/A1200/A4000:%d" };
                    int idx = ctype - HD_CONTROLLER_TYPE_IDE_FIRST;
                    if (idx >= 0 && idx < 2)
                        snprintf(devname_str, sizeof(devname_str), idedevs[idx], ci->controller_unit);
                    else
                        snprintf(devname_str, sizeof(devname_str), "IDE:%d", ci->controller_unit);
                }
                harddisktype(volname_str, ci);
                strcpy(bootpri_str, "n/a");
            } 
            else if (ctype >= HD_CONTROLLER_TYPE_SCSI_FIRST && ctype <= HD_CONTROLLER_TYPE_SCSI_LAST) {
                const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
                char sid[32];
                char* ert_name = ert ? ua(ert->name) : NULL;
                
                if (ci->controller_unit == 8 && ert_name && !strcmp(ert_name, "a2091")) strcpy(sid, "XT");
                else if (ci->controller_unit == 8 && ert_name && !strcmp(ert_name, "a2090a")) strcpy(sid, "ST-506");
                else snprintf(sid, sizeof(sid), "%d", ci->controller_unit);
                
                if (ert_name) xfree(ert_name);

                if (ert) {
                    char* friendly = ua(ert->friendlyname);
                    if (friendly) {
                        if (ci->controller_type_unit == 0)
                            snprintf(devname_str, sizeof(devname_str), "%s:%s", friendly, sid);
                        else
                            snprintf(devname_str, sizeof(devname_str), "%s:%s/%d", friendly, sid, ci->controller_type_unit + 1);
                        xfree(friendly);
                    } else {
                        strcpy(devname_str, "Unknown");
                    }
                } else {
                     const char* scsidevs[] = { "SCSI:%s", "A3000:%s", "A4000T:%s", "CDTV:%s" };
                     int idx = ctype - HD_CONTROLLER_TYPE_SCSI_FIRST;
                     if (idx >= 0 && idx < 4)
                        snprintf(devname_str, sizeof(devname_str), scsidevs[idx], sid);
                     else
                        snprintf(devname_str, sizeof(devname_str), "SCSI:%s", sid);
                }
                harddisktype(volname_str, ci);
                strcpy(bootpri_str, "n/a");
            }

            ImGui::TableNextRow();
            
            // Selection logic
            bool is_selected = (selected_row == row);
            
            ImGui::TableNextColumn();
            if (ImGui::Selectable(devname_str, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_row = row;
            }
            
            ImGui::TableNextColumn(); ImGui::Text("%s", volname_str);
            ImGui::TableNextColumn(); ImGui::Text("%s", rootdir_str.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", ci->readonly ? "RO" : "RW");
            ImGui::TableNextColumn(); ImGui::Text("%s", size_str);
            ImGui::TableNextColumn(); ImGui::Text("%s", bootpri_str);

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    EndGroupBox("Mounted Drives");
}

static void RenderActionButtons()
{    
    float avail = ImGui::GetContentRegionAvail().x;
    float btn_w_3 = (avail - 20) / 3.0f; // Width of Row 1 buttons (roughly)
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    // Row 1: Add Dir, Add HDF, Add HD
    if (ImGui::Button("Add Directory...", ImVec2(btn_w_3, 0))) {
        current_hd_dialog_mode = HDDialogMode::AddDir;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Hardfile...", ImVec2(btn_w_3, 0))) {
        current_hd_dialog_mode = HDDialogMode::AddHDF;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Hard Drive...", ImVec2(btn_w_3, 0))) {
        current_hd_dialog_mode = HDDialogMode::AddHardDrive;
    }

    // Row 2: Add CD, Add Tape, Properties, Remove
    // "Add CD Drive", "Add Tape Drive", "Properties", "Remove"
    
    // User Requirements:
    // Add CD width == Add Directory (btn_w_3)
    // Add Tape width == Add Hardfile (btn_w_3)
    // Properties + Remove width == Add Hard Drive (btn_w_3)
    
    // Calculate widths for Row 2
    float btn_w_cd = btn_w_3;
    float btn_w_tape = btn_w_3;
    float btn_w_props_remove = (btn_w_3 - spacing) / 2.0f;

    // Add CD Drive
    if (ImGui::Button("Add CD Drive...", ImVec2(btn_w_cd, 0))) {
         current_hd_dialog_mode = HDDialogMode::AddCD;
    }
    
    ImGui::SameLine();

    // Add Tape Drive
    if (ImGui::Button("Add Tape Drive...", ImVec2(btn_w_tape, 0))) {
         current_hd_dialog_mode = HDDialogMode::AddTape;
    }

    ImGui::SameLine();
    
    // Properties (Edit)
    ImGui::BeginDisabled(selected_row < 0 || selected_row >= changed_prefs.mountitems);
    if (ImGui::Button("Properties", ImVec2(btn_w_props_remove, 0))) {
        current_hd_dialog_mode = HDDialogMode::EditEntry;
        edit_entry_index = selected_row;
    }
    ImGui::SameLine();
    // Remove
    if (ImGui::Button("Remove", ImVec2(btn_w_props_remove, 0))) {
        kill_filesys_unitconfig(&changed_prefs, selected_row);
        if (selected_row >= changed_prefs.mountitems) selected_row = changed_prefs.mountitems - 1;
    }
    ImGui::EndDisabled();

    // Row 3: Create Hardfile (Full width or similar to top buttons?) 
    // WinUAE puts it on its own row usually or grouped. User asked for "new, third row, by itself".
    if (ImGui::Button("Create Hardfile...", ImVec2(btn_w_3, 0))) {
        current_hd_dialog_mode = HDDialogMode::CreateHDF;
    }
}

static void RenderCDSection()
{
    ImGui::Checkbox("CDFS automount CD/DVD drives", &changed_prefs.automount_cddrives);
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    
    bool turbo = (changed_prefs.cd_speed == 0);
    if (ImGui::Checkbox("Turbo CD speed", &turbo)) {
        changed_prefs.cd_speed = turbo ? 0 : 100;
    }
    
    ImGui::Dummy(ImVec2(0, 5));

    BeginGroupBox("CD Drive / Image");
    
    bool cd_enabled = changed_prefs.cdslots[0].inuse;
    if (ImGui::Checkbox("CD drive/image", &cd_enabled)) {
        changed_prefs.cdslots[0].inuse = cd_enabled;
        if (!cd_enabled) {
            changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;
            changed_prefs.cdslots[0].name[0] = 0;
        } else {
            changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
        }
    }

    ImGui::BeginDisabled(!cd_enabled);
    
    ImGui::SameLine();
    
    // CD Image path as Dropdown (Combo)
    char* cd_name = ua(changed_prefs.cdslots[0].name);
    // Preview value
    const char* preview_val = (cd_name && *cd_name) ? cd_name : "";

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 160); // Reserve space for buttons
    if (ImGui::BeginCombo("##CDPath", preview_val))
    {
        // Add current value if not empty
        if (cd_name && *cd_name) {
             bool is_selected = true;
             if (ImGui::Selectable(cd_name, is_selected)) {}
             if (is_selected) ImGui::SetItemDefaultFocus();
        }
        
        // MRU List
        for (const auto& path : lstMRUCDList) {
             if (path.empty()) continue;
             bool is_selected = (strcmp(cd_name, path.c_str()) == 0);
             if (ImGui::Selectable(path.c_str(), is_selected)) {
                 au_copy(changed_prefs.cdslots[0].name, MAX_DPATH, path.c_str());
             }
             if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    
    xfree(cd_name);
    
    ImGui::SameLine();
    if (ImGui::Button("Eject", ImVec2(70, 0))) {
        changed_prefs.cdslots[0].name[0] = 0;
        changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
    }
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(30, 0))) {
         char* curr_path = ua(changed_prefs.cdslots[0].name);
         std::string startPath = curr_path;
         if (startPath.empty()) {
             char* tmp = ua(get_cdrom_path().c_str());
             startPath = tmp;
             xfree(tmp);
         }
         xfree(curr_path);
         OpenFileDialog("Select CD image file", "CD Images (*.cue,*.iso,*.ccd,*.mds,*.chd,*.nrg){.cue,.iso,.ccd,.mds,.chd,.nrg},All Files (*){.*}", startPath);
    }
    
    ImGui::EndDisabled();

    std::string result_path;
    if (ConsumeFileDialogResult(result_path)) { 
        if (!result_path.empty()) {
             au_copy(changed_prefs.cdslots[0].name, MAX_DPATH, result_path.c_str());
             changed_prefs.cdslots[0].inuse = true;
             changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
             
             // Add to MRU logic (simplistic: check existence and push back if new)
             bool found = false;
             for(const auto& p : lstMRUCDList) {
                 if (p == result_path) { found = true; break; }
             }
             if (!found) lstMRUCDList.insert(lstMRUCDList.begin(), result_path);
             // Limit MRU size?
             if (lstMRUCDList.size() > 10) lstMRUCDList.resize(10);
        }
    }

    EndGroupBox("CD Drive / Image");
}

// Variables to track which dialog was opened so we can consume the result correctly
static bool selecting_virtual_dir = false;
static bool selecting_virtual_arch = false;
static bool selecting_create_hdf_path = false;
static bool selecting_hdf_path = false;

static void ShowEditFilesysVirtualModal()
{
    if (ImGui::BeginPopupModal("Volume Settings", &show_virtual_filesys_modal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Device Name:");
        ImGui::SameLine();
        InputTextT("##Device", current_fsvdlg.ci.devname, sizeof(current_fsvdlg.ci.devname));

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Volume Label:");
        ImGui::SameLine();
        InputTextT("##Volume", current_fsvdlg.ci.volname, sizeof(current_fsvdlg.ci.volname));
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Path:");
        ImGui::SameLine();
        InputTextT("##Path", current_fsvdlg.ci.rootdir, sizeof(current_fsvdlg.ci.rootdir));
        ImGui::SameLine();
        if (ImGui::Button("... Dir"))
        {
            char* current_root = ua(current_fsvdlg.ci.rootdir);
            OpenDirDialog(current_root);
            xfree(current_root);
            selecting_virtual_dir = true;
            selecting_virtual_arch = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("... Arch"))
        {
             char* current_root = ua(current_fsvdlg.ci.rootdir);
             OpenFileDialog("Select archive", "Archives (*.zip,*.7z,*.rar,*.lha,*.lzh,*.lzx){.zip,.7z,.rar,.lha,.lzh,.lzx},All Files (*){.*}", current_root);
             xfree(current_root);
             selecting_virtual_dir = false;
             selecting_virtual_arch = true;
        }
        ImGui::EndGroup();

        // Consume dialog results
        std::string result_path;
        if (selecting_virtual_dir && ConsumeDirDialogResult(result_path)) {
            if (!result_path.empty()) {
                au_copy(current_fsvdlg.ci.rootdir, sizeof(current_fsvdlg.ci.rootdir), result_path.c_str());
                if (current_fsvdlg.ci.devname[0] == 0) {
                     char devname[256];
                     CreateDefaultDevicename(devname); 
                     au_copy(current_fsvdlg.ci.devname, sizeof(current_fsvdlg.ci.devname), devname);
                }
                if (current_fsvdlg.ci.volname[0] == 0) _tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
            }
            selecting_virtual_dir = false;
        }
        if (selecting_virtual_arch && ConsumeFileDialogResult(result_path)) {
            if (!result_path.empty()) {
                au_copy(current_fsvdlg.ci.rootdir, sizeof(current_fsvdlg.ci.rootdir), result_path.c_str());
                if (current_fsvdlg.ci.devname[0] == 0) {
                     char devname[256];
                     CreateDefaultDevicename(devname); 
                     au_copy(current_fsvdlg.ci.devname, sizeof(current_fsvdlg.ci.devname), devname);
                }
                if (current_fsvdlg.ci.volname[0] == 0) _tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
            }
            selecting_virtual_arch = false;
        }

        ImGui::Separator();

        bool rw = !current_fsvdlg.ci.readonly;
        if (ImGui::Checkbox("Read/Write", &rw)) current_fsvdlg.ci.readonly = !rw;
        
        ImGui::SameLine();
        bool bootable = (current_fsvdlg.ci.bootpri != BOOTPRI_NOAUTOBOOT);
        if (ImGui::Checkbox("Bootable", &bootable)) {
             if (bootable) current_fsvdlg.ci.bootpri = 0;
             else current_fsvdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
        }

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Boot priority:");
        ImGui::SameLine();
        int bp = current_fsvdlg.ci.bootpri;
        if (bp == BOOTPRI_NOAUTOBOOT) bp = -128;
        if (ImGui::InputInt("##BootPri", &bp)) {
            if (bp < -129) bp = -129;
            if (bp > 127) bp = 127;
            current_fsvdlg.ci.bootpri = bp;
        }

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            new_filesys(edit_entry_index);
            current_hd_dialog_mode = HDDialogMode::None;
            ImGui::CloseCurrentPopup();
            show_virtual_filesys_modal = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            current_hd_dialog_mode = HDDialogMode::None;
            ImGui::CloseCurrentPopup();
            show_virtual_filesys_modal = false;
        }
        
        ImGui::EndPopup();
    }
}

static void ShowEditFilesysHardfileModal()
{
    if (ImGui::BeginPopupModal("Hardfile Settings", &show_hardfile_modal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Path:");
        ImGui::SameLine();
        InputTextT("##HDFPath", current_hfdlg.ci.rootdir, sizeof(current_hfdlg.ci.rootdir));
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
             char* current_root = ua(current_hfdlg.ci.rootdir);
             OpenFileDialog("Select hard disk file", "Hardfiles (*.hdf,*.hdz,*.lha,*.zip,*.vhd,*.chd,*.7z){.hdf,.hdz,.lha,.zip,.vhd,.chd,.7z},All Files (*){.*}", current_root);
             xfree(current_root);
             selecting_hdf_path = true;
        }
        
        std::string result_path;
        if (selecting_hdf_path && ConsumeFileDialogResult(result_path)) {
             if (!result_path.empty()) {
                  au_copy(current_hfdlg.ci.rootdir, sizeof(current_hfdlg.ci.rootdir), result_path.c_str());
                  if (current_hfdlg.ci.devname[0] == 0) {
                     char devname[256];
                     CreateDefaultDevicename(devname);
                     au_copy(current_hfdlg.ci.devname, sizeof(current_hfdlg.ci.devname), devname);
                  }
                  updatehdfinfo(true, true, false, hdf_info_text1, hdf_info_text2);
                  updatehdfinfo(false, false, false, hdf_info_text1, hdf_info_text2);
             }
             selecting_hdf_path = false;
        }

        ImGui::Separator();

        ImGui::BeginGroup();
        ImGui::Text("Geometry");
        
        bool manual_geo = current_hfdlg.ci.physical_geometry; 
        if (ImGui::Checkbox("Manual Geometry", &manual_geo)) {
             current_hfdlg.ci.physical_geometry = manual_geo;
             updatehdfinfo(true, false, false, hdf_info_text1, hdf_info_text2);
        }

        int surfaces = manual_geo ? current_hfdlg.ci.pheads : current_hfdlg.ci.surfaces;
        int sectors = manual_geo ? current_hfdlg.ci.psecs : current_hfdlg.ci.sectors;
        int reserved = manual_geo ? current_hfdlg.ci.pcyls : current_hfdlg.ci.reserved;
        int blocksize = current_hfdlg.ci.blocksize;

        bool geo_changed = false;
        if (ImGui::InputInt("Surfaces", &surfaces)) {
             if (manual_geo) current_hfdlg.ci.pheads = surfaces; else current_hfdlg.ci.surfaces = surfaces;
             geo_changed = true;
        }
        if (ImGui::InputInt("Sectors", &sectors)) {
             if (manual_geo) current_hfdlg.ci.psecs = sectors; else current_hfdlg.ci.sectors = sectors;
             geo_changed = true;
        }
        if (ImGui::InputInt("Reserved", &reserved)) {
             if (manual_geo) current_hfdlg.ci.pcyls = reserved; else current_hfdlg.ci.reserved = reserved;
             geo_changed = true;
        }
        if (ImGui::InputInt("Blocksize", &blocksize)) {
             current_hfdlg.ci.blocksize = blocksize;
             geo_changed = true;
        }
        
        if (geo_changed) {
             updatehdfinfo(true, false, false, hdf_info_text1, hdf_info_text2);
        }
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Controller");
        
        if (controller.empty()) {
            ImGui::Text("No controllers available");
        } else {
            int current_ctrl_idx = 0;
            for (size_t i = 0; i < controller.size(); ++i) {
                if (controller[i].type == current_hfdlg.ci.controller_type) {
                    current_ctrl_idx = (int)i;
                    break;
                }
            }
            
            if (ImGui::BeginCombo("Interface", controller[current_ctrl_idx].display.c_str())) {
                for (size_t i = 0; i < controller.size(); ++i) {
                    bool is_selected = (current_ctrl_idx == i);
                    if (ImGui::Selectable(controller[i].display.c_str(), is_selected)) {
                        current_hfdlg.ci.controller_type = controller[i].type;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::InputInt("Unit", &current_hfdlg.ci.controller_unit); 

        InputTextT("Device Name", current_hfdlg.ci.devname, sizeof(current_hfdlg.ci.devname));
        
        ImGui::EndGroup();

        ImGui::Separator();
        
        bool rw = !current_hfdlg.ci.readonly;
        if (ImGui::Checkbox("Read/Write", &rw)) current_hfdlg.ci.readonly = !rw;
        ImGui::SameLine();
        bool bootable = (current_hfdlg.ci.bootpri != BOOTPRI_NOAUTOBOOT);
        if (ImGui::Checkbox("Bootable", &bootable)) {
             if (bootable) current_hfdlg.ci.bootpri = 0;
             else current_hfdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
        }
        ImGui::SameLine();
        ImGui::InputInt("Boot Pri", &current_hfdlg.ci.bootpri);

        ImGui::Separator();
        
        ImGui::TextWrapped("%s", hdf_info_text1.c_str());
        ImGui::TextWrapped("%s", hdf_info_text2.c_str());

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            new_hardfile(edit_entry_index);
            current_hd_dialog_mode = HDDialogMode::None;
            ImGui::CloseCurrentPopup();
            show_hardfile_modal = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            current_hd_dialog_mode = HDDialogMode::None;
            ImGui::CloseCurrentPopup();
            show_hardfile_modal = false;
        }

        ImGui::EndPopup();
    }
}

static void ShowCreateHardfileModal()
{
    if (ImGui::BeginPopupModal("Create Hardfile", &show_create_hdf_modal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::BeginGroup();
        ImGui::Text("Settings");
        ImGui::InputInt("Size (MB)", &create_hdf_size_mb);
        ImGui::Checkbox("Dynamic HDF (Sparse)", &create_hdf_dynamic);
        ImGui::Checkbox("RDB Mode", &create_hdf_rdb);
        ImGui::EndGroup();

        ImGui::Separator();
        
        ImGui::BeginGroup();
        ImGui::Text("Path");
        InputTextT("##CreatePath", current_hfdlg.ci.rootdir, sizeof(current_hfdlg.ci.rootdir));
        ImGui::SameLine();
        if (ImGui::Button("...")) {
             char* current_root = ua(current_hfdlg.ci.rootdir);
             // SelectFile for CREATE needs different handling? OpenFileDialog helper handles create too?
             // Usually just picking a path is enough.
             OpenFileDialog("Select new hard disk file", "Hardfiles (*.hdf,*.hdz,*.vhd,*.chd){.hdf,.hdz,.vhd,.chd},All Files (*){.*}", current_root);
             xfree(current_root);
             selecting_create_hdf_path = true;
        }
        
        std::string result_path;
        if (selecting_create_hdf_path && ConsumeFileDialogResult(result_path)) {
             if (!result_path.empty()) {
                  au_copy(current_hfdlg.ci.rootdir, sizeof(current_hfdlg.ci.rootdir), result_path.c_str());
             }
             selecting_create_hdf_path = false;
        }
        
        ImGui::EndGroup();

        ImGui::Separator();

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            uae_u64 size_bytes = (uae_u64)create_hdf_size_mb * 1024 * 1024;
            if (vhd_create(current_hfdlg.ci.rootdir, size_bytes, create_hdf_dynamic ? 1 : 0)) {
                show_create_hdf_modal = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_create_hdf_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void ShowAddHardDriveModal()
{
    if (ImGui::BeginPopupModal("Add Hard Drive", &show_add_harddrive_modal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Select Physical Drive:");
        
        static int selected_drive_idx = -1;
        static std::vector<std::string> drive_names;
        static std::vector<std::string> drive_paths;
        static bool drives_scanned = false;

        if (!drives_scanned) {
            drive_names.clear();
            drive_paths.clear();
            int num = hdf_getnumharddrives();
            for (int i = 0; i < num; ++i) {
                int sectorsize = 0;
                int dangerous = 0;
                uae_u32 flags = 0;
                TCHAR* name = hdf_getnameharddrive(i, 0, &sectorsize, &dangerous, &flags);
                if (name) {
                    char* tmp = ua(name);
                    drive_names.push_back(std::string(tmp)); 
                    drive_paths.push_back(std::string(tmp)); 
                    xfree(tmp);
                    xfree(name);
                }
            }
            drives_scanned = true;
        }

        if (ImGui::BeginListBox("##PhysicalDrives", ImVec2(-FLT_MIN, 200))) {
            for (int i = 0; i < drive_names.size(); ++i) {
                bool is_selected = (selected_drive_idx == i);
                if (ImGui::Selectable(drive_names[i].c_str(), is_selected)) {
                    selected_drive_idx = i;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }
        
        bool rw = !current_hfdlg.ci.readonly;
        if (ImGui::Checkbox("Read/Write", &rw)) current_hfdlg.ci.readonly = !rw;
        ImGui::SameLine();
        bool bootable = (current_hfdlg.ci.bootpri != BOOTPRI_NOAUTOBOOT);
        if (ImGui::Checkbox("Bootable", &bootable)) {
             if (bootable) current_hfdlg.ci.bootpri = 0;
             else current_hfdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
        }

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (selected_drive_idx >= 0 && selected_drive_idx < drive_names.size()) {
                au_copy(current_hfdlg.ci.rootdir, sizeof(current_hfdlg.ci.rootdir), drive_paths[selected_drive_idx].c_str());
                 char devname[256];
                 CreateDefaultDevicename(devname);
                 au_copy(current_hfdlg.ci.devname, sizeof(current_hfdlg.ci.devname), devname);
                 new_harddrive(edit_entry_index);
                 show_add_harddrive_modal = false;
                 ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_add_harddrive_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}


// Helper for CD/Tape Modals to find controller index
static int GetControllerIndex(int type, int unit) {
    int idx = 0;
    for (const auto& c : controller) {
        int ctype = c.type % HD_CONTROLLER_NEXT_UNIT;
        int cunit = c.type / HD_CONTROLLER_NEXT_UNIT;
        if (ctype == type && cunit == unit) {
            return idx;
        }
        idx++;
    }
    return 0;
}

static void ShowEditCDDriveModal()
{
    if (ImGui::BeginPopupModal("CD Drive Settings", &show_cd_modal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int selected_controller = 0;
        static int selected_unit = 0;
        static char path_buf[MAX_DPATH];
        static bool initialized = false;

        if (!initialized) {
            strncpy(path_buf, current_cddlg.ci.rootdir, MAX_DPATH);
            selected_controller = GetControllerIndex(current_cddlg.ci.controller_type, current_cddlg.ci.controller_type_unit);
            selected_unit = current_cddlg.ci.controller_unit;
            initialized = true;
        } else if (!show_cd_modal) {
             initialized = false; 
        }

        InputTextT("Path", path_buf, MAX_DPATH);
        ImGui::SameLine();
        if (ImGui::Button("...")) {
            OpenFileDialog("Select CD Image", "CD Images (*.cue,*.iso,*.ccd,*.mds,*.chd,*.nrg){.cue,.iso,.ccd,.mds,.chd,.nrg},All Files (*){.*}", path_buf);
        }

        std::string result_path;
        if (ConsumeFileDialogResult(result_path)) {
             if (!result_path.empty()) strncpy(path_buf, result_path.c_str(), MAX_DPATH);
        }

        if (ImGui::BeginCombo("Controller", controller.size() > selected_controller ? controller[selected_controller].display.c_str() : "Unknown")) {
             for (int i = 0; i < controller.size(); ++i) {
                 if (ImGui::Selectable(controller[i].display.c_str(), selected_controller == i)) {
                     selected_controller = i;
                 }
                 if (selected_controller == i) ImGui::SetItemDefaultFocus();
             }
             ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Unit", std::to_string(selected_unit).c_str())) {
            for (int i = 0; i < 8; ++i) {
                 if (ImGui::Selectable(std::to_string(i).c_str(), selected_unit == i)) {
                     selected_unit = i;
                 }
                 if (selected_unit == i) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            strncpy(current_cddlg.ci.rootdir, path_buf, MAX_DPATH);
            current_cddlg.ci.controller_unit = selected_unit;
            
            if (selected_controller >= 0 && selected_controller < controller.size()) {
                int posn = controller[selected_controller].type;
                current_cddlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
                current_cddlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
            }
            
            inithdcontroller(current_cddlg.ci.controller_type, current_cddlg.ci.controller_type_unit, UAEDEV_CD, path_buf[0] != 0);

            new_cddrive(-1); 
            gui_force_rtarea_hdchange(); 
            ImGui::CloseCurrentPopup();
            show_cd_modal = false;
            initialized = false;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            show_cd_modal = false;
            initialized = false;
        }

        ImGui::EndPopup();
    }
}

static void ShowEditTapeDriveModal()
{
    if (ImGui::BeginPopupModal("Tape Drive Settings", &show_tape_modal, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int selected_controller = 0;
        static int selected_unit = 0;
        static char path_buf[MAX_DPATH];
        static bool initialized = false;

        if (!initialized) {
            strncpy(path_buf, current_tapedlg.ci.rootdir, MAX_DPATH);
            selected_controller = GetControllerIndex(current_tapedlg.ci.controller_type, current_tapedlg.ci.controller_type_unit);
            selected_unit = current_tapedlg.ci.controller_unit;
            initialized = true;
        }

        InputTextT("Path", path_buf, MAX_DPATH);
        ImGui::SameLine();
        if (ImGui::Button("... Dir")) {
             char* current_root = ua(path_buf);
             OpenDirDialog(current_root);
             xfree(current_root);
             selecting_virtual_dir = true; 
        }
        ImGui::SameLine();
        if (ImGui::Button("... File")) {
            OpenFileDialog("Select Tape Image", "All Files (*){.*}", path_buf);
        }
        
        std::string result_path;
        if (ConsumeDirDialogResult(result_path)) {
             if (!result_path.empty()) strncpy(path_buf, result_path.c_str(), MAX_DPATH);
        }
        if (ConsumeFileDialogResult(result_path)) {
             if (!result_path.empty()) strncpy(path_buf, result_path.c_str(), MAX_DPATH);
        }

        if (ImGui::BeginCombo("Controller", controller.size() > selected_controller ? controller[selected_controller].display.c_str() : "Unknown")) {
             for (int i = 0; i < controller.size(); ++i) {
                 if (ImGui::Selectable(controller[i].display.c_str(), selected_controller == i)) {
                     selected_controller = i;
                 }
                 if (selected_controller == i) ImGui::SetItemDefaultFocus();
             }
             ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Unit", std::to_string(selected_unit).c_str())) {
            for (int i = 0; i < 8; ++i) {
                 if (ImGui::Selectable(std::to_string(i).c_str(), selected_unit == i)) {
                     selected_unit = i;
                 }
                 if (selected_unit == i) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (path_buf[0] == 0) {
                 // Warning?
            } else {
                strncpy(current_tapedlg.ci.rootdir, path_buf, MAX_DPATH);
                current_tapedlg.ci.controller_unit = selected_unit;
                
                if (selected_controller >= 0 && selected_controller < controller.size()) {
                    int posn = controller[selected_controller].type;
                    current_tapedlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
                    current_tapedlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
                }
                
                inithdcontroller(current_tapedlg.ci.controller_type, current_tapedlg.ci.controller_type_unit, UAEDEV_TAPE, path_buf[0] != 0);

                new_tapedrive(-1); 
                gui_force_rtarea_hdchange(); 
                ImGui::CloseCurrentPopup();
                show_tape_modal = false;
                initialized = false;
            }
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            show_tape_modal = false;
            initialized = false;
        }

        ImGui::EndPopup();
    }
}

void render_panel_hd()
{
    RenderMountedDrives();
    ImGui::Dummy(ImVec2(0, 5));
    RenderActionButtons();
    ImGui::Dummy(ImVec2(0, 5));
    RenderCDSection();
    
    // Logic to open Modals - Add Directory
    if (current_hd_dialog_mode == HDDialogMode::AddDir) {
        default_fsvdlg(&current_fsvdlg);
        char devname[256];
        CreateDefaultDevicename(devname);
        au_copy(current_fsvdlg.ci.devname, sizeof(current_fsvdlg.ci.devname), devname);
        _tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
        
        edit_entry_index = -1;
        ImGui::OpenPopup("Volume Settings");
        show_virtual_filesys_modal = true;
        current_hd_dialog_mode = HDDialogMode::None; 
    }
    
    // Logic to open Modals - Add Hardfile
    if (current_hd_dialog_mode == HDDialogMode::AddHDF) {
        default_hfdlg(&current_hfdlg, false); // false = not rdb
        char devname[256];
        CreateDefaultDevicename(devname);
        au_copy(current_hfdlg.ci.devname, sizeof(current_hfdlg.ci.devname), devname);
        
        edit_entry_index = -1;
        hdf_info_text1.clear(); hdf_info_text2.clear(); 
        
        ImGui::OpenPopup("Hardfile Settings");
        show_hardfile_modal = true;
        current_hd_dialog_mode = HDDialogMode::None;
    }

    // Logic to open Modals - Add Hard Drive
    if (current_hd_dialog_mode == HDDialogMode::AddHardDrive) {
        default_hfdlg(&current_hfdlg, false); 
        char devname[256];
        CreateDefaultDevicename(devname);
        au_copy(current_hfdlg.ci.devname, sizeof(current_hfdlg.ci.devname), devname);
        
        edit_entry_index = -1;
        
        ImGui::OpenPopup("Add Hard Drive");
        show_add_harddrive_modal = true;
        current_hd_dialog_mode = HDDialogMode::None;
    }

    // Logic to open Modals - Create HDF
    if (current_hd_dialog_mode == HDDialogMode::CreateHDF) {
        default_hfdlg(&current_hfdlg, false);
        current_hfdlg.ci.rootdir[0] = 0; 
        
        ImGui::OpenPopup("Create Hardfile");
        show_create_hdf_modal = true;
        current_hd_dialog_mode = HDDialogMode::None;
    }

    // Logic to open Modals - Add CD
    if (current_hd_dialog_mode == HDDialogMode::AddCD) {
        // Init default CD config similar to old EditCDDrive( -1 )
        if (current_cddlg.ci.controller_type == HD_CONTROLLER_TYPE_UAE)
            current_cddlg.ci.controller_type = (is_board_enabled(&changed_prefs, ROMTYPE_A2091, 0) ||
                is_board_enabled(&changed_prefs, ROMTYPE_GVPS2, 0) || is_board_enabled(&changed_prefs, ROMTYPE_A4091, 0) ||
                (changed_prefs.cs_mbdmac & 3)) ? HD_CONTROLLER_TYPE_SCSI_AUTO : HD_CONTROLLER_TYPE_IDE_AUTO;
        inithdcontroller(current_cddlg.ci.controller_type, current_cddlg.ci.controller_type_unit, UAEDEV_CD, current_cddlg.ci.rootdir[0] != 0);
        
        ImGui::OpenPopup("CD Drive Settings");
        show_cd_modal = true;
        current_hd_dialog_mode = HDDialogMode::None;
    }

    // Logic to open Modals - Add Tape
    if (current_hd_dialog_mode == HDDialogMode::AddTape) {
        default_tapedlg(&current_tapedlg);
        inithdcontroller(current_tapedlg.ci.controller_type, current_tapedlg.ci.controller_type_unit, UAEDEV_TAPE, current_tapedlg.ci.rootdir[0] != 0);

        ImGui::OpenPopup("Tape Drive Settings");
        show_tape_modal = true;
        current_hd_dialog_mode = HDDialogMode::None;
    }

    // Handle opening Edit - existing logic needs update for CD/Tape edit?
    if (current_hd_dialog_mode == HDDialogMode::EditEntry) {
         struct uaedev_config_info* uci = &changed_prefs.mountconfig[edit_entry_index].ci;
         struct mountedinfo mi{};
         int type = get_filesys_unitconfig(&changed_prefs, edit_entry_index, &mi);
         if (type < 0) type = (uci->type == UAEDEV_HDF) ? FILESYS_HARDFILE : FILESYS_VIRTUAL;
         
         if (uci->type == UAEDEV_CD) {
             memcpy(&current_cddlg.ci, uci, sizeof(struct uaedev_config_info));
             ImGui::OpenPopup("CD Drive Settings");
             show_cd_modal = true;
         }
         else if (uci->type == UAEDEV_TAPE) {
             memcpy(&current_tapedlg.ci, uci, sizeof(struct uaedev_config_info));
             ImGui::OpenPopup("Tape Drive Settings");
             show_tape_modal = true;
         }
         else if (type == FILESYS_VIRTUAL) {
             memcpy(&current_fsvdlg.ci, uci, sizeof(struct uaedev_config_info));
             ImGui::OpenPopup("Volume Settings");
             show_virtual_filesys_modal = true;
         }
         else if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB) {
             memcpy(&current_hfdlg.ci, uci, sizeof(struct uaedev_config_info));
             updatehdfinfo(true, false, false, hdf_info_text1, hdf_info_text2); 
             ImGui::OpenPopup("Hardfile Settings");
             show_hardfile_modal = true;
         }
         
         current_hd_dialog_mode = HDDialogMode::None;
    }

    ShowEditFilesysVirtualModal();
    ShowEditFilesysHardfileModal();
    ShowCreateHardfileModal();
    ShowAddHardDriveModal();
    ShowEditCDDriveModal();
    ShowEditTapeDriveModal();
}


