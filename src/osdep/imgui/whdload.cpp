#include "imgui.h"
#include "imgui_panels.h"
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include <string>
#include <vector>

static bool show_custom_fields = false;

static int set_bit(const int value, const int bit_position) {
    return value | (1 << bit_position);
}

static int clear_bit(const int value, const int bit_position) {
    return value & ~(1 << bit_position);
}

static bool is_bit_set(const int num, const int bit) {
    return (num & (1 << bit)) != 0;
}

static void RenderCustomFieldsPopup() 
{
    if (show_custom_fields) {
        ImGui::OpenPopup("Custom Fields");
        show_custom_fields = false;
    }

    if (ImGui::BeginPopupModal("Custom Fields", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        whdload_custom* fields[] = {
            &whdload_prefs.selected_slave.custom1,
            &whdload_prefs.selected_slave.custom2,
            &whdload_prefs.selected_slave.custom3,
            &whdload_prefs.selected_slave.custom4,
            &whdload_prefs.selected_slave.custom5
        };

        for (int i = 0; i < 5; ++i) {
            whdload_custom* field = fields[i];
            if (field->type == 0) continue; // None/Invalid? Assuming 0 is default/none if check needed.
            // Actually ShowCustomFields just loops. If vectors are empty it does nothing.
            
            // Guisan logic loops `custom_number` times.
            
            ImGui::PushID(i);
            
            switch (field->type) {
                case bit_type:
                    ImGui::Text("Custom%d:", i + 1);
                    for (size_t j = 0; j < field->label_bit_pairs.size(); ++j) {
                        bool v = is_bit_set(field->value, field->label_bit_pairs[j].second);
                        if (AmigaCheckbox(field->label_bit_pairs[j].first.c_str(), &v)) {
                            field->value = v ? set_bit(field->value, field->label_bit_pairs[j].second)
                                             : clear_bit(field->value, field->label_bit_pairs[j].second);
                        }
                    }
                    break;
                case bool_type:
                    // Single boolean usually? Guisan code loops 'custom_number'.
                    // If bool_type, custom_number is usually 1?
                    // Check logic: if !boolean.empty() -> it renders checkboxes.
                    // For bool_type, custom_field.caption is used.
                    ImGui::Text("Custom%d:", i + 1);
                    {
                         bool v = field->value != 0;
                         if (AmigaCheckbox(field->caption.c_str(), &v)) {
                             field->value = v ? 1 : 0;
                         }
                    }
                    break;
                case list_type:
                    ImGui::Text("%s:", field->caption.c_str());
                    // Dropdown
                    if (!field->labels.empty()) {
                        int current_item = field->value;
                        if (current_item < 0) current_item = 0;
                        if (current_item >= (int)field->labels.size()) current_item = 0;
                        
                        // Vector of pointers for ImGui
                        std::vector<const char*> items;
                        for(const auto& s : field->labels) items.push_back(s.c_str());
                        
                        if (ImGui::Combo("##Combo", &current_item, items.data(), (int)items.size())) {
                            field->value = current_item;
                        }
                        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
                    }
                    break;
                default:
                    break;
            }
            ImGui::PopID();
            ImGui::Separator();
        }

        if (AmigaButton("OK", ImVec2(BUTTON_WIDTH, 0))) {
            create_startup_sequence(); // Apply changes
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void render_panel_whdload()
{
    ImGui::Indent(4.0f);

    // Top Section: WHDLoad auto-config
    ImGui::AlignTextToFramePadding();
    ImGui::Text("WHDLoad auto-config:");

    ImGui::SameLine();
    if (AmigaButton("Select file", ImVec2(BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT))) {
        std::string startPath = whdload_prefs.whdload_filename;
        if (startPath.empty()) startPath = get_whdload_arch_path();
        const auto filter = "LHA Files (*.lha,*.lzh){.lha,.lzh},All Files (*){.*}";
        OpenFileDialogKey("WHDLOAD", "Select WHDLoad LHA file", filter, startPath);
    }

    // Process File Dialog Result
    std::string result_path;
    if (ConsumeFileDialogResultKey("WHDLOAD", result_path)) {
        if (!result_path.empty()) {
             whdload_prefs.whdload_filename = result_path;
             add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
             whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
             if (!last_loaded_config[0])
                 set_last_active_config(whdload_prefs.whdload_filename.c_str());
        }
    }

    ImGui::SameLine();
    if (AmigaButton("Eject", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
        whdload_prefs.whdload_filename = "";
        clear_whdload_prefs();
    }

    // WHDLoad File Dropdown
    std::vector<const char*> whd_files;
    std::vector<std::string> whd_paths; 
    
    for(const auto& str : lstMRUWhdloadList) {
        whd_paths.push_back(str);
    }
    
    std::vector<std::string> display_names;
    for(const auto& path : whd_paths) {
         std::string fname = path.substr(path.find_last_of("/\\") + 1);
         display_names.push_back(fname);
    }
    
    for(const auto& name : display_names) whd_files.push_back(name.c_str());

    int current_whd_idx = -1;
    if (!whdload_prefs.whdload_filename.empty()) {
        for(size_t i=0; i<whd_paths.size(); ++i) {
            if(whd_paths[i] == whdload_prefs.whdload_filename) {
                current_whd_idx = (int)i;
                break;
            }
        }
    }

    ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
    if (ImGui::Combo("##WhdFileCombo", &current_whd_idx, whd_files.data(), (int)whd_files.size())) {
        if (current_whd_idx >= 0 && current_whd_idx < (int)whd_paths.size()) {
             whdload_prefs.whdload_filename = whd_paths[current_whd_idx];
             whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
             if (!last_loaded_config[0])
                 set_last_active_config(whdload_prefs.whdload_filename.c_str());
        }
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    
    ImGui::Separator();
    
    auto DrawReadOnlyInput = [](const char* label, const std::string& value) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        char buf[256];
        strncpy(buf, value.c_str(), 255);
        ImGui::BeginDisabled();
        const auto hidden_label = "##" + std::string(label);
        ImGui::InputText(hidden_label.c_str(), buf, 255, ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();
    };

    DrawReadOnlyInput("Game Name", whdload_prefs.game_name);
    DrawReadOnlyInput("UUID", whdload_prefs.variant_uuid);
    DrawReadOnlyInput("Slave Default", whdload_prefs.slave_default);

    AmigaCheckbox("Slave Libraries", &whdload_prefs.slave_libraries);
    ShowHelpMarker("Use slave-specific libraries");
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Slaves:");
    ImGui::SameLine();
    
    std::vector<const char*> slave_names;
    for(const auto& s : whdload_prefs.slaves) slave_names.push_back(s.filename.c_str());
    
    int current_slave_idx = -1;
    for(size_t i=0; i<whdload_prefs.slaves.size(); ++i) {
        if(whdload_prefs.slaves[i].filename == whdload_prefs.selected_slave.filename) {
            current_slave_idx = (int)i;
            break;
        }
    }
    
    ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
    if(ImGui::Combo("##SlavesCombo", &current_slave_idx, slave_names.data(), (int)slave_names.size())) {
        if(current_slave_idx >= 0) {
            whdload_prefs.selected_slave = whdload_prefs.slaves[current_slave_idx];
            create_startup_sequence();
        }
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    
    DrawReadOnlyInput("Slave Data path", whdload_prefs.selected_slave.data_path);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
    char custom_buf[1024];
    strncpy(custom_buf, whdload_prefs.custom.c_str(), 1023);
    if(AmigaInputText("##CustomInput", custom_buf, 1024)) {
        whdload_prefs.custom = custom_buf;
        create_startup_sequence();
    }

    ImGui::Spacing();
    bool custom_enabled = !whdload_prefs.whdload_filename.empty();
    ImGui::BeginDisabled(!custom_enabled);
    if(AmigaButton("Custom Fields", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT))) {
            show_custom_fields = true; 
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    BeginGroupBox("Global options");

    if(AmigaCheckbox("Button Wait", &whdload_prefs.button_wait)) create_startup_sequence();
    ShowHelpMarker("Wait for button press before starting the game");
    if(AmigaCheckbox("Show Splash", &whdload_prefs.show_splash)) create_startup_sequence();
    ShowHelpMarker("Show WHDLoad splash screen on startup");

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Config Delay:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    if(ImGui::InputInt("##ConfigDelay", &whdload_prefs.config_delay)) {
        if(whdload_prefs.config_delay < 0) whdload_prefs.config_delay = 0;
        create_startup_sequence();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    ShowHelpMarker("Delay in seconds before starting the game");

    if(AmigaCheckbox("Write Cache", &whdload_prefs.write_cache)) create_startup_sequence();
    ShowHelpMarker("Cache writes for better performance (less safe)");
    if(AmigaCheckbox("Quit on Exit", &whdload_prefs.quit_on_exit)) create_startup_sequence();
    ShowHelpMarker("Return to emulator when the game exits");

    EndGroupBox("Global options");
    
    RenderCustomFieldsPopup();
}
