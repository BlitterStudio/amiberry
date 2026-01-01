#include "imgui.h"
#include "imgui_panels.h"
#include "sysdeps.h"
#include "config.h"
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
                        if (ImGui::Checkbox(field->label_bit_pairs[j].first.c_str(), &v)) {
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
                         if (ImGui::Checkbox(field->caption.c_str(), &v)) {
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
                    }
                    break;
                default:
                    break;
            }
            ImGui::PopID();
            ImGui::Separator();
        }

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            create_startup_sequence(); // Apply changes
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void render_panel_whdload()
{
    // Top Section: WHDLoad auto-config
    ImGui::Text("WHDLoad auto-config:");
    
    ImGui::SameLine();
    
    const float eject_width = 80.0f;
    const float select_width = 100.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    
    float current_x = ImGui::GetCursorPosX();
    float available_w = ImGui::GetContentRegionAvail().x;
    float spacer = available_w - eject_width - select_width - spacing;
    if (spacer > 0) ImGui::SetCursorPosX(current_x + spacer);

    if (ImGui::Button("Eject", ImVec2(eject_width, 0))) {
        whdload_prefs.whdload_filename = "";
        clear_whdload_prefs();
    }
    ImGui::SameLine();
    if (ImGui::Button("Select file", ImVec2(select_width, 0))) {
         std::string startPath = whdload_prefs.whdload_filename;
         if (startPath.empty()) startPath = get_whdload_arch_path();
         const char *filter = "LHA Files (*.lha,*.lzh){.lha,.lzh},All Files (*){.*}";
         OpenFileDialog("Select WHDLoad LHA file", filter, startPath);
    }
    
    // Process File Dialog Result
    std::string result_path;
    if (ConsumeFileDialogResult(result_path)) {
        if (!result_path.empty()) {
             whdload_prefs.whdload_filename = result_path;
             add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
             whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
             if (!last_loaded_config[0])
                 set_last_active_config(whdload_prefs.whdload_filename.c_str());
        }
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

    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##WhdFileCombo", &current_whd_idx, whd_files.data(), (int)whd_files.size())) {
        if (current_whd_idx >= 0 && current_whd_idx < (int)whd_paths.size()) {
             whdload_prefs.whdload_filename = whd_paths[current_whd_idx];
             whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
             if (!last_loaded_config[0])
                 set_last_active_config(whdload_prefs.whdload_filename.c_str());
        }
    }
    
    ImGui::Separator();
    
    auto DrawReadOnlyInput = [](const char* label, const std::string& value) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", label);
        ImGui::SameLine(120.0f);
        ImGui::SetNextItemWidth(-1);
        char buf[256];
        strncpy(buf, value.c_str(), 255);
        ImGui::BeginDisabled();
        ImGui::InputText(label, buf, 255, ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();
    };

    DrawReadOnlyInput("Game Name", whdload_prefs.game_name);
    DrawReadOnlyInput("UUID", whdload_prefs.variant_uuid);
    DrawReadOnlyInput("Slave Default", whdload_prefs.slave_default);
    
    ImGui::Checkbox("Slave Libraries", &whdload_prefs.slave_libraries);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Slaves:");
    ImGui::SameLine(120.0f);
    
    std::vector<const char*> slave_names;
    for(const auto& s : whdload_prefs.slaves) slave_names.push_back(s.filename.c_str());
    
    int current_slave_idx = -1;
    for(size_t i=0; i<whdload_prefs.slaves.size(); ++i) {
        if(whdload_prefs.slaves[i].filename == whdload_prefs.selected_slave.filename) {
            current_slave_idx = (int)i;
            break;
        }
    }
    
    ImGui::SetNextItemWidth(-1);
    if(ImGui::Combo("##SlavesCombo", &current_slave_idx, slave_names.data(), (int)slave_names.size())) {
        if(current_slave_idx >= 0) {
            whdload_prefs.selected_slave = whdload_prefs.slaves[current_slave_idx];
            create_startup_sequence();
        }
    }
    
    DrawReadOnlyInput("Slave Data path", whdload_prefs.selected_slave.data_path);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom:");
    ImGui::SameLine(120.0f);
    ImGui::SetNextItemWidth(-1);
    char custom_buf[1024];
    strncpy(custom_buf, whdload_prefs.custom.c_str(), 1023);
    if(ImGui::InputText("##CustomInput", custom_buf, 1024)) {
        whdload_prefs.custom = custom_buf;
        create_startup_sequence();
    }
    
    ImGui::SetCursorPosX(120.0f);
    bool custom_enabled = !whdload_prefs.whdload_filename.empty();
    ImGui::BeginDisabled(!custom_enabled);
    if(ImGui::Button("Custom Fields", ImVec2(200.0f, 0.0f))) {
            show_custom_fields = true; 
    }
    ImGui::EndDisabled();
    
    ImGui::Separator();
    
    ImGui::Text("Global options");
    ImGui::BeginChild("GlobalOptions", ImVec2(0, 0), true); 
    
    if(ImGui::Checkbox("Button Wait", &whdload_prefs.button_wait)) create_startup_sequence();
    if(ImGui::Checkbox("Show Splash", &whdload_prefs.show_splash)) create_startup_sequence();
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Config Delay:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if(ImGui::InputInt("##ConfigDelay", &whdload_prefs.config_delay)) {
        if(whdload_prefs.config_delay < 0) whdload_prefs.config_delay = 0;
        create_startup_sequence();
    }
    
    if(ImGui::Checkbox("Write Cache", &whdload_prefs.write_cache)) create_startup_sequence();
    if(ImGui::Checkbox("Quit on Exit", &whdload_prefs.quit_on_exit)) create_startup_sequence();

    ImGui::EndChild();
    
    RenderCustomFieldsPopup();
}
