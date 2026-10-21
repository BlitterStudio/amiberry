#include "imgui.h"
#include "imgui_internal.h"
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "disk.h"
#include "settings/settings_definitions.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace settings = amiberry_settings;

static constexpr const char* floppy_file_filter =
    "All Supported Files (*.adf,*.adz,*.dms,*.ipf,*.zip,*.7z,*.lha,*.lzh,*.lzx,*.fdi,*.scp,*.wrp,*.dsq,*.gz,*.xz,*.hdf,*.img)"
    "{.adf,.adz,.dms,.ipf,.zip,.7z,.lha,.lzh,.lzx,.fdi,.scp,.wrp,.dsq,.gz,.xz,.hdf,.img},All Files (*){.*}";

namespace {

// The options this panel edits. The shared settings layer owns their
// spellings, availability rules and the dependent changes a new value
// implies (the FloppyBridge profile a drive type selects).
struct floppy_options {
    const settings::definition* type[4] = {
        &settings::require("floppy0type"), &settings::require("floppy1type"),
        &settings::require("floppy2type"), &settings::require("floppy3type"),
    };
    const settings::definition* subtype[4] = {
        &settings::require("floppy0subtype"), &settings::require("floppy1subtype"),
        &settings::require("floppy2subtype"), &settings::require("floppy3subtype"),
    };
    const settings::definition& speed = settings::require("floppy_speed");
    const settings::definition& driver = settings::require("amiberry.drawbridge_driver");
    const settings::definition& serial_auto = settings::require("amiberry.drawbridge_serial_autodetect");
    const settings::definition& serial_port = settings::require("amiberry.drawbridge_serial_port");
    const settings::definition& smartspeed = settings::require("amiberry.drawbridge_smartspeed");
    const settings::definition& autocache = settings::require("amiberry.drawbridge_autocache");
    const settings::definition& drive_cable = settings::require("amiberry.drawbridge_drive_cable");
};

const floppy_options& options()
{
    static const floppy_options o;
    return o;
}

// The type combo merges the drive type and the FloppyBridge mode, as the
// panel always did: the first entries are drive types (floppyNtype
// values), the FB entries select type 8 and a mode (floppyNsubtype).
struct drive_type_entry {
    const char* label;
    const char* type;
    const char* mode; // FloppyBridge mode, or nullptr
};

const drive_type_entry drive_types[] = {
    {"3.5\" DD", "0", nullptr},
    {"3.5\" HD", "1", nullptr},
    {"5.25\" (40)", "2", nullptr},
    {"5.25\" (80)", "7", nullptr},
    {"3.5\" ESCOM", "3", nullptr},
    {"FB: Normal", "8", "0"},
    {"FB: Compatible", "8", "1"},
    {"FB: Turbo", "8", "2"},
    {"FB: Stalling", "8", "3"},
};

int drive_type_index(int drive)
{
    const floppyslot& slot = changed_prefs.floppyslots[drive];
    char type[16];
    std::snprintf(type, sizeof type, "%d", slot.dfxtype);
    for (int n = 0; n < IM_ARRAYSIZE(drive_types); n++) {
        if (std::strcmp(drive_types[n].type, type) != 0)
            continue;
        if (!drive_types[n].mode || std::atoi(drive_types[n].mode) == slot.dfxsubtype)
            return n;
    }
    return -1;
}

bool available(const settings::definition& d, const settings::context& ctx)
{
    return settings::is_available(d, changed_prefs, ctx);
}

} // namespace

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

enum class FloppyRecoveryDialogState {
    Idle,
    Requested,
    Active,
    Selected,
    Cancelled
};

static FloppyRecoveryDialogState recovery_dialog_state = FloppyRecoveryDialogState::Idle;
static int recovery_dialog_drive = 0;
static std::string recovery_dialog_initial_path;
static std::string recovery_dialog_result;

void FloppyRecoveryDialog_Open(const int drive, const char* initial_path)
{
    recovery_dialog_drive = drive;
    recovery_dialog_initial_path = initial_path ? initial_path : "";
    recovery_dialog_result.clear();
    recovery_dialog_state = FloppyRecoveryDialogState::Requested;
}

void FloppyRecoveryDialog_Render()
{
    static constexpr const char* dialog_key = "FLOPPY_RECOVERY";

    if (recovery_dialog_state == FloppyRecoveryDialogState::Requested) {
        char title[64];
        snprintf(title, sizeof title, "Select replacement disk for DF%d:", recovery_dialog_drive);
        OpenFileDialogKey(dialog_key, title, floppy_file_filter, recovery_dialog_initial_path);
        recovery_dialog_state = FloppyRecoveryDialogState::Active;
    }

    if (recovery_dialog_state != FloppyRecoveryDialogState::Active)
        return;

    std::string selected_path;
    if (ConsumeFileDialogResultKey(dialog_key, selected_path)) {
        recovery_dialog_result = selected_path;
        recovery_dialog_state = FloppyRecoveryDialogState::Selected;
        gui_running = false;
    } else if (!IsFileDialogOpenKey(dialog_key)) {
        recovery_dialog_state = FloppyRecoveryDialogState::Cancelled;
        gui_running = false;
    }
}

bool FloppyRecoveryDialog_ConsumeResult(std::string& out_path)
{
    const bool selected = recovery_dialog_state == FloppyRecoveryDialogState::Selected;
    if (selected)
        out_path = recovery_dialog_result;

    recovery_dialog_initial_path.clear();
    recovery_dialog_result.clear();
    recovery_dialog_state = FloppyRecoveryDialogState::Idle;
    return selected;
}

// Cache write-protect state per drive to avoid re-opening (and unpacking)
// archive files on every frame.  Note: quickstart.cpp has its own identical
// cache (qs_wp_cache) because both panels render independently.
static struct {
	std::string path;
	bool wp = false;
} wp_cache[4];

static bool cached_disk_getwriteprotect(struct uae_prefs *p, const TCHAR *name, int num)
{
	if (num < 0 || num >= 4)
		return false;
	std::string current(name ? name : "");
	if (wp_cache[num].path != current) {
		wp_cache[num].path = current;
		wp_cache[num].wp = current.empty() ? false : (disk_getwriteprotect(p, name, num) != 0);
	}
	return wp_cache[num].wp;
}

static void invalidate_wp_cache(int num)
{
	if (num >= 0 && num < 4)
		wp_cache[num].path.clear();
}

static std::string format_mru_entry(const std::string& fullpath) {
    size_t last_slash = fullpath.find_last_of("/\\");
    std::string filename = (last_slash == std::string::npos) ? fullpath : fullpath.substr(last_slash + 1);
    return filename + " { " + fullpath + " }";
}

static void RenderDriveSlot(const int i)
{
    ImGui::PushID(i);
        
    char table_id[32];
    snprintf(table_id, sizeof(table_id), "DriveTable%d", i);
    
    if (ImGui::BeginTable(table_id, 6, ImGuiTableFlags_None))
    {
        float small_btn = SMALL_BUTTON_WIDTH;
        float en_w = BUTTON_WIDTH * 0.5f;
        float type_w = BUTTON_WIDTH * 1.1f;
        float btns_w = small_btn * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
        float wp_width = ImGui::GetContentRegionAvail().x - en_w - type_w - btns_w - ImGui::GetStyle().ItemSpacing.x * 3 - 8.0f;
        if (wp_width < BUTTON_WIDTH) wp_width = BUTTON_WIDTH;
        ImGui::TableSetupColumn("En", ImGuiTableColumnFlags_WidthFixed, en_w);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, type_w);
        ImGui::TableSetupColumn("WP", ImGuiTableColumnFlags_WidthFixed, wp_width);
        ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthFixed, small_btn);
        ImGui::TableSetupColumn("Eject", ImGuiTableColumnFlags_WidthFixed, small_btn);
        ImGui::TableSetupColumn("Sel", ImGuiTableColumnFlags_WidthFixed, small_btn);

        ImGui::TableNextRow();
        
        // 1. Enabled Checkbox
        ImGui::TableNextColumn();
        const floppy_options& o = options();
        const settings::context ctx{emulating != 0};
        bool drive_enabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
        char label[8];
        snprintf(label, sizeof(label), "DF%d:", i);
        if (AmigaCheckbox(label, &drive_enabled)) {
             if (drive_enabled && changed_prefs.floppyslots[i].dfxtype == DRV_NONE)
                 settings::set(*o.type[i], changed_prefs, ctx, "0");
             else if (!drive_enabled)
                 settings::set(*o.type[i], changed_prefs, ctx, "-1");
        }
        ShowHelpMarker(i == 0
            ? "Enable/disable floppy drive. DF0: is the boot drive. Changes made while emulation is running require resetting the emulated Amiga before AmigaOS can detect them."
            : "Enable/disable floppy drive. Changes made while emulation is running require resetting the emulated Amiga before AmigaOS can detect them.");

        ImGui::BeginDisabled(!drive_enabled);

        // 2. Type Dropdown
        ImGui::TableNextColumn();
        const int type_idx = drive_type_index(i);

        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x - 6.0f);
        // A disabled drive, or a type this panel does not list (a PC
        // bridgeboard drive from a WinUAE configuration), shows as such.
        char unknown_type[32];
        if (changed_prefs.floppyslots[i].dfxtype == DRV_NONE)
            std::snprintf(unknown_type, sizeof unknown_type, "Disabled");
        else
            std::snprintf(unknown_type, sizeof unknown_type, "Type %d", changed_prefs.floppyslots[i].dfxtype);
        const char* type_name = type_idx >= 0 ? drive_types[type_idx].label : unknown_type;
        if (ImGui::BeginCombo("##FloppyTypeCombo", type_name)) {
            for (int n = 0; n < IM_ARRAYSIZE(drive_types); n++) {
                const drive_type_entry& entry = drive_types[n];
                const bool is_selected = (type_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                ImGui::BeginDisabled(!settings::is_choice_available(*o.type[i], changed_prefs, ctx, entry.type));
                if (ImGui::Selectable(entry.label, is_selected)
                    && settings::set(*o.type[i], changed_prefs, ctx, entry.type) == AMIBERRY_SETTINGS_OK
                    && entry.mode) {
                    settings::set(*o.subtype[i], changed_prefs, ctx, entry.mode);
                }
                ImGui::EndDisabled();
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Drive type: 3.5\" DD (880KB standard), 3.5\" HD (1.76MB A4000), 5.25\" DD, or FB (FloppyBridge for real drives)");

        // 3. WP Checkbox
        ImGui::TableNextColumn();
        // As in Quickstart: the global read-only floppy option makes
        // disk_setwriteprotect() refuse, and without a disk there is
        // nothing to protect.
        ImGui::BeginDisabled(changed_prefs.floppy_read_only || !changed_prefs.floppyslots[i].df[0]);
        bool wp = cached_disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i);
        if (AmigaCheckbox("Write Protected", &wp)) {
            disk_setwriteprotect(&changed_prefs, i, changed_prefs.floppyslots[i].df, wp);
            invalidate_wp_cache(i);
            if (cached_disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i) != wp) {
                // Failed to change write protection -> maybe filesystem doesn't support this
                ShowMessageBox("Set/Clear write protect", "Failed to change write permission.\nMaybe underlying filesystem doesn't support this.");
            }
            DISK_reinsert(i);
        }
        ImGui::EndDisabled();
        ShowHelpMarker("Prevent writing to the disk image");

        // 4. Info Button
        ImGui::TableNextColumn();
        if (AmigaButton("?", ImVec2(-1, 0))) {
             if (strlen(changed_prefs.floppyslots[i].df) > 0)
                DisplayDiskInfo(i);
        }
        
        // 6. Eject Button
        ImGui::TableNextColumn();
        if (AmigaButton(ICON_FA_EJECT "##Eject", ImVec2(-1, 0))) {
             disk_eject(i);
             changed_prefs.floppyslots[i].df[0] = 0;
             invalidate_wp_cache(i);
        }
        
        // 7. Select Button
        ImGui::TableNextColumn();
        if (AmigaButton("...")) {
             current_floppy_dialog_mode = static_cast<FloppyDialogMode>(static_cast<int>(FloppyDialogMode::SelectDF0) + i);
             std::string startPath = changed_prefs.floppyslots[i].df;
             if (startPath.empty() && !last_floppy_dir.empty()) startPath = last_floppy_dir;
             if (startPath.empty()) startPath = get_floppy_path(); 
             OpenFileDialogKey("FLOPPY", "Select Disk Image", floppy_file_filter, startPath);
        }
        
        ImGui::EndDisabled();
        ImGui::EndTable();
    }

    // --- ROW 2: File Path Combo ---
    bool enabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
    ImGui::BeginDisabled(!enabled);
    
    ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x - 6.0f);
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
                 strncpy(changed_prefs.floppyslots[i].df, lstMRUDiskList[h].c_str(), MAX_DPATH - 1);
                 changed_prefs.floppyslots[i].df[MAX_DPATH - 1] = '\0';
                 disk_insert(i, changed_prefs.floppyslots[i].df);
                 DISK_history_add(changed_prefs.floppyslots[i].df, -1, HISTORY_FLOPPY, 0);
                 set_last_active_config_from_media(changed_prefs.floppyslots[i].df);
                 invalidate_wp_cache(i);
            }

            if (is_selected)
            {
                ImGui::PopStyleColor(1);
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ImGui::EndDisabled();
    
    ImGui::Spacing();
    ImGui::PopID(); 
}

#ifdef FLOPPYBRIDGE
// A combo over a layer CHOICE option, its static or host-enumerated list.
static void drawbridge_combo(const char* id, const settings::definition& d,
    const settings::context& ctx, const char* empty_label)
{
    char current[AMIBERRY_SETTINGS_VALUE_MAX];
    settings::get(d, changed_prefs, current, sizeof current);
    const settings::choice_list choices = settings::choices_of(d, changed_prefs);
    const char* preview = current;
    for (size_t n = 0; n < choices.count; n++) {
        if (!std::strcmp(current, choices.items[n].value))
            preview = choices.items[n].label;
    }
    if (choices.count == 0)
        preview = empty_label;
    if (ImGui::BeginCombo(id, preview)) {
        for (size_t n = 0; n < choices.count; n++) {
            const bool is_selected = !std::strcmp(current, choices.items[n].value);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            ImGui::PushID(static_cast<int>(n));
            if (ImGui::Selectable(choices.items[n].label, is_selected))
                settings::set(d, changed_prefs, ctx, choices.items[n].value);
            ImGui::PopID();
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
}

static void drawbridge_checkbox(const char* label, const settings::definition& d,
    const settings::context& ctx)
{
    ImGui::BeginDisabled(!available(d, ctx));
    bool checked = settings::get_bool(d, changed_prefs);
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked);
    ImGui::EndDisabled();
}

// The DrawBridge options; the layer enables each one by what the selected
// driver supports.
static void RenderDrawBridge()
{
    if (BeginGroupBox("DrawBridge (FloppyBridge)", true, false)) {
    const floppy_options& o = options();
    const settings::context ctx{emulating != 0};

    // --- Driver Selection ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("DrawBridge driver:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 6.0f);
    drawbridge_combo("##DBDriver", o.driver, ctx, "No drivers found");
    ShowHelpMarker("Select the DrawBridge hardware driver for connecting real Amiga floppy drives");

    ImGui::SameLine();
    if (AmigaButton(ICON_FA_ARROWS_ROTATE " Refresh")) {
        settings::refresh_dynamic_choices();
    }
    ShowHelpMarker("Refresh the list of available DrawBridge drivers");

    // --- Serial Port ---
    drawbridge_checkbox("DrawBridge: Auto-Detect serial port", o.serial_auto, ctx);
    ShowHelpMarker("Automatically detect which serial port the DrawBridge is connected to");

    ImGui::BeginDisabled(!available(o.serial_port, ctx));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    const std::string current_port = changed_prefs.drawbridge_serial_port;
    // The host's serial ports, as the Misc panel lists them.
    extern std::vector<std::string> serial_ports;
    if (ImGui::BeginCombo("##DBSerial", current_port.empty() ? "none" : current_port.c_str())) {
        const bool is_none_selected = current_port.empty();
        if (is_none_selected)
            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
        if (ImGui::Selectable("none", is_none_selected))
            settings::set(o.serial_port, changed_prefs, ctx, "");
        if (is_none_selected) {
            ImGui::PopStyleColor();
            ImGui::SetItemDefaultFocus();
        }
        for (const auto& port : serial_ports) {
            const bool is_selected = (current_port == port);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(port.c_str(), is_selected))
                settings::set(o.serial_port, changed_prefs, ctx, port);
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Select which serial port the DrawBridge hardware is connected to");
    ImGui::EndDisabled();

    // --- Smart Speed and Auto Cache ---
    drawbridge_checkbox("DrawBridge: Smart Speed (Dynamically switch on Turbo)", o.smartspeed, ctx);
    ShowHelpMarker("Automatically speed up disk access when possible without breaking compatibility");
    drawbridge_checkbox("DrawBridge: Auto-Cache (Cache disk data while drive is idle)", o.autocache, ctx);
    ShowHelpMarker("Cache track data from the real floppy when the drive is idle to improve performance");

    // --- Drive Cable ---
    ImGui::BeginDisabled(!available(o.drive_cable, ctx));
    ImGui::AlignTextToFramePadding();
    ImGui::Text("DrawBridge: Drive cable:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    drawbridge_combo("##DBCable", o.drive_cable, ctx, "");
    ShowHelpMarker("Select drive cable type: IBM PC (A/B) or SHUGART (0-3) standard");
    ImGui::EndDisabled();
    ImGui::Spacing();
    }
    EndGroupBox("DrawBridge (FloppyBridge)");
}
#endif

void render_panel_floppy()
{
    const floppy_options& o = options();
    const settings::context ctx{emulating != 0};

    ImGui::Indent(4.0f);
    
    // ---------------------------------------------------------
    // FLOPPY DRIVES GROUP
    // ---------------------------------------------------------
    BeginGroupBox("Floppy Drives");
    RenderFloppyDriveResetNotice();

    for (int i = 0; i < 4; ++i)
    {
        RenderDriveSlot(i);
    }
    EndGroupBox("Floppy Drives");

    // ---------------------------------------------------------
    // EMULATION SPEED
    // ---------------------------------------------------------
    // The slider walks the layer's speed steps. A configuration can hold
    // another speed (50% from a hand-edited file): the label shows that
    // value until the slider is moved, instead of claiming 100%.
    BeginGroupBox("Floppy Drive Emulation Speed");

    const amiberry_setting_descriptor& speed_desc = o.speed.descriptor;
    char current_speed[AMIBERRY_SETTINGS_VALUE_MAX];
    settings::get(o.speed, changed_prefs, current_speed, sizeof current_speed);
    int speed_idx = -1;
    for (size_t n = 0; n < speed_desc.choice_count; n++) {
        if (!std::strcmp(current_speed, speed_desc.choices[n].value))
            speed_idx = static_cast<int>(n);
    }
    char speed_label[32];
    if (speed_idx >= 0)
        std::snprintf(speed_label, sizeof speed_label, "%s", speed_desc.choices[speed_idx].label);
    else
        std::snprintf(speed_label, sizeof speed_label, "%s%%", current_speed);
    int slider_idx = speed_idx >= 0 ? speed_idx : 1;

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    if (ImGui::SliderInt("##SpeedSlider", &slider_idx, 0, static_cast<int>(speed_desc.choice_count) - 1, "")) {
        settings::set(o.speed, changed_prefs, ctx, speed_desc.choices[slider_idx].value);
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ShowHelpMarker("Floppy drive speed: 100% = real Amiga speed, higher = faster loading. Turbo speed may break copy protections");
    ImGui::SameLine();
    
    ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - 6.0f);
    ImGui::InputText("##SpeedLabel", speed_label, sizeof speed_label, ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();
    ImGui::Spacing();
    EndGroupBox("Floppy Drive Emulation Speed");

    // ---------------------------------------------------------
    // ACTIONS
    // ---------------------------------------------------------
    BeginGroupBox("New Floppy Disk Image");
    
    float avail_w = ImGui::GetContentRegionAvail().x - 10.0f;
    float btn_w = (avail_w - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;
    
    if (AmigaButton(ICON_FA_PLUS " 3.5'' DD", ImVec2(btn_w, 0))) {
        current_floppy_dialog_mode = FloppyDialogMode::CreateDD;
        OpenFileDialogKey("FLOPPY", "Create 3.5\" DD disk file", "Amiga Disk File (*.adf){.adf}", get_floppy_path(), true);
    }
    ShowHelpMarker("Create a new 880KB double-density floppy disk image");
    ImGui::SameLine();
    if (AmigaButton(ICON_FA_PLUS " 3.5'' HD", ImVec2(btn_w, 0))) {
        current_floppy_dialog_mode = FloppyDialogMode::CreateHD;
        OpenFileDialogKey("FLOPPY", "Create 3.5\" HD disk file", "Amiga Disk File (*.adf){.adf}", get_floppy_path(), true);
    }
    ShowHelpMarker("Create a new 1.76MB high-density floppy disk image");
    ImGui::SameLine();
    ImGui::BeginDisabled(strlen(changed_prefs.floppyslots[0].df) == 0);
    if (AmigaButton(ICON_FA_FLOPPY_DISK " Save config", ImVec2(btn_w, 0))) {
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
    ShowHelpMarker("Save current configuration with the name of the disk in DF0:");
    ImGui::EndDisabled();
    ImGui::Spacing();
    EndGroupBox("New Floppy Disk Image");

#ifdef FLOPPYBRIDGE
    RenderDrawBridge();
#endif

    std::string result_path;
    if (ConsumeFileDialogResultKey("FLOPPY", result_path)) {
       if (!result_path.empty()) {
            // Remember the directory for next time
            size_t last_sep = result_path.find_last_of("/\\");
            if (last_sep != std::string::npos)
                last_floppy_dir = result_path.substr(0, last_sep);

            if (current_floppy_dialog_mode == FloppyDialogMode::CreateDD) {
                char diskname[MAX_DPATH];
                extract_filename(result_path.c_str(), diskname);
                remove_file_extension(diskname);
                diskname[31] = '\0';
                disk_creatediskfile(&changed_prefs, result_path.c_str(), 0, DRV_35_DD, -1, diskname, false, false, nullptr);
                DISK_history_add(result_path.c_str(), -1, HISTORY_FLOPPY, 0);
                add_file_to_mru_list(lstMRUDiskList, result_path);
            }
            else if (current_floppy_dialog_mode == FloppyDialogMode::CreateHD) {
                char diskname[MAX_DPATH];
                extract_filename(result_path.c_str(), diskname);
                remove_file_extension(diskname);
                diskname[31] = '\0';
                disk_creatediskfile(&changed_prefs, result_path.c_str(), 0, DRV_35_HD, -1, diskname, false, false, nullptr);
                DISK_history_add(result_path.c_str(), -1, HISTORY_FLOPPY, 0);
                add_file_to_mru_list(lstMRUDiskList, result_path);
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
                     int archive_count = populate_diskswapper_from_archive(result_path.c_str(), &changed_prefs);
                     if (archive_count > 0) {
                         // Archive with multiple disks: insert first image into selected drive
                         strncpy(changed_prefs.floppyslots[drive_idx].df, changed_prefs.dfxlist[0], MAX_DPATH - 1);
                         changed_prefs.floppyslots[drive_idx].df[MAX_DPATH - 1] = '\0';
                         disk_insert(drive_idx, changed_prefs.floppyslots[drive_idx].df);
                         DISK_history_add(changed_prefs.floppyslots[drive_idx].df, -1, HISTORY_FLOPPY, 0);
                         add_file_to_mru_list(lstMRUDiskList, std::string(changed_prefs.dfxlist[0]));
                         set_last_active_config_from_media(changed_prefs.floppyslots[drive_idx].df);
                     } else {
                         strncpy(changed_prefs.floppyslots[drive_idx].df, result_path.c_str(), MAX_DPATH - 1);
                         changed_prefs.floppyslots[drive_idx].df[MAX_DPATH - 1] = '\0';
                         disk_insert(drive_idx, result_path.c_str());
                         DISK_history_add(result_path.c_str(), -1, HISTORY_FLOPPY, 0);
                         set_last_active_config_from_media(result_path.c_str());
                     }
                     invalidate_wp_cache(drive_idx);
                }
            }
            current_floppy_dialog_mode = FloppyDialogMode::None;
       }
    }
}
