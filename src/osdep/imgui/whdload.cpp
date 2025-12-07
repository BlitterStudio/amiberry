#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include <string>
#include <vector>

void render_panel_whdload()
{
    ImGui::Text("WHDLoad auto-config:");
    ImGui::SameLine(ImGui::GetWindowWidth() - 180);
    if (ImGui::Button("Eject")) {
        whdload_prefs.whdload_filename = "";
        clear_whdload_prefs();
    }

    ImGui::SameLine();
    if (ImGui::Button("Select file")) {
        // TODO: Implement file selector.
    }

    // WHDLoad file dropdown
    std::vector<const char*> whdload_display_items;
    std::vector<std::string> whdload_filenames;
    for (const auto& full_path : lstMRUWhdloadList) {
        whdload_filenames.push_back(full_path.substr(full_path.find_last_of("/\\") + 1));
    }
    for (const auto& filename : whdload_filenames) {
        whdload_display_items.push_back(filename.c_str());
    }

    int current_whd = -1;
    if (!whdload_prefs.whdload_filename.empty()) {
        for (size_t i = 0; i < lstMRUWhdloadList.size(); ++i) {
            if (lstMRUWhdloadList[i] == whdload_prefs.whdload_filename) {
                current_whd = static_cast<int>(i);
                break;
            }
        }
    }
    ImGui::PushItemWidth(-1);
    if (ImGui::Combo("##WHDLoadFile", &current_whd, whdload_display_items.data(), static_cast<int>(whdload_display_items.size()))) {
        if (current_whd >= 0) {
            const auto& selected_path = lstMRUWhdloadList[static_cast<size_t>(current_whd)];
            if (selected_path != whdload_prefs.whdload_filename) {
                whdload_prefs.whdload_filename = selected_path;
                add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
                whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
                if (!last_loaded_config[0])
                    set_last_active_config(whdload_prefs.whdload_filename.c_str());
            }
        }
    }
    ImGui::PopItemWidth();

    ImGui::Separator();

    // WHDLoad game options
    ImGui::InputText("Game Name", whdload_prefs.game_name.data(), whdload_prefs.game_name.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputText("UUID", whdload_prefs.variant_uuid.data(), whdload_prefs.variant_uuid.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputText("Slave Default", whdload_prefs.slave_default.data(), whdload_prefs.slave_default.size(), ImGuiInputTextFlags_ReadOnly);

    ImGui::Checkbox("Slave Libraries", &whdload_prefs.slave_libraries);

    // Slaves dropdown
    std::vector<const char*> slave_items;
    for (const auto& slave : whdload_prefs.slaves) {
        slave_items.push_back(slave.filename.c_str());
    }
    int current_slave = -1;
    if (!whdload_prefs.selected_slave.filename.empty()) {
        for (size_t i = 0; i < whdload_prefs.slaves.size(); ++i) {
            if (whdload_prefs.slaves[i].filename == whdload_prefs.selected_slave.filename) {
                current_slave = static_cast<int>(i);
                break;
            }
        }
    }
    if (ImGui::Combo("Slaves", &current_slave, slave_items.data(), static_cast<int>(slave_items.size()))) {
        if (current_slave >= 0) {
            whdload_prefs.selected_slave = whdload_prefs.slaves[static_cast<size_t>(current_slave)];
            create_startup_sequence();
        }
    }

    char data_path_buf[1024];
    strncpy(data_path_buf, whdload_prefs.selected_slave.data_path.c_str(), sizeof(data_path_buf));
    data_path_buf[sizeof(data_path_buf) - 1] = 0;
    ImGui::InputText("Slave Data path", data_path_buf, sizeof(data_path_buf), ImGuiInputTextFlags_ReadOnly);

    if (ImGui::InputText("Custom", whdload_prefs.custom.data(), whdload_prefs.custom.size())) {
        create_startup_sequence();
    }

    if (whdload_prefs.whdload_filename.empty()) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Custom Fields", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
    {
        // TODO: Implement custom fields dialog
    }
    if (whdload_prefs.whdload_filename.empty()) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    ImGui::Text("Global options");

    if (ImGui::Checkbox("Button Wait", &whdload_prefs.button_wait)) {
        create_startup_sequence();
    }
    if (ImGui::Checkbox("Show Splash", &whdload_prefs.show_splash)) {
        create_startup_sequence();
    }

    if (ImGui::InputInt("Config Delay", &whdload_prefs.config_delay)) {
        create_startup_sequence();
    }

    if (ImGui::Checkbox("Write Cache", &whdload_prefs.write_cache)) {
        create_startup_sequence();
    }
    if (ImGui::Checkbox("Quit on Exit", &whdload_prefs.quit_on_exit)) {
        create_startup_sequence();
    }
}
