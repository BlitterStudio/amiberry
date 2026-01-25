#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui.h"
#include "gui/gui_handling.h"
#include "savestate.h"
#include <sys/stat.h>
#include <ctime>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>

#include "imgui_panels.h"

static std::string get_file_timestamp(const char* filename)
{
    struct stat st;
    if (stat(filename, &st) == -1) {
        return "ERROR";
    }
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &st.st_mtime);
#else
    localtime_r(&st.st_mtime, &tm);
#endif
    char date_string[256];
    strftime(date_string, sizeof(date_string), "%c", &tm);
    return std::string(date_string);
}

static bool show_delete_confirm_popup = false;
static bool show_alert_popup = false;
static std::string alert_title;
static std::string alert_message;

static void ShowAlert(const char* title, const char* message)
{
    alert_title = title;
    alert_message = message;
    show_alert_popup = true;
}

static SDL_Texture* savestate_screenshot_tex = nullptr;
static int savestate_screenshot_slot = -1;
static SDL_Renderer* cached_renderer = nullptr;

static void ClearScreenshotTexture() {
    if (savestate_screenshot_tex) {
        SDL_DestroyTexture(savestate_screenshot_tex);
        savestate_screenshot_tex = nullptr;
    }
    savestate_screenshot_slot = -1;
}

static void UpdateScreenshotTexture(int slot) {
    if (AMonitors[0].gui_renderer != cached_renderer) {
        ClearScreenshotTexture();
        cached_renderer = AMonitors[0].gui_renderer;
    }

    if (slot == savestate_screenshot_slot && savestate_screenshot_tex) return; 
    
    if (slot != savestate_screenshot_slot) {
        ClearScreenshotTexture();
    }
    savestate_screenshot_slot = slot;

    if (!screenshot_filename.empty()) {
        SDL_Surface* surface = IMG_Load(screenshot_filename.c_str());
        if (surface) {
            if (cached_renderer) {
                savestate_screenshot_tex = SDL_CreateTextureFromSurface(cached_renderer, surface);
            }
            SDL_FreeSurface(surface);
        }
    }
}

void render_panel_savestates()
{
    static int last_state_num = -1;
    bool force_reload = false;

    if (current_state_num != last_state_num) {
        gui_update();
        last_state_num = current_state_num;
        force_reload = true;
    }

    if (force_reload) {
        ClearScreenshotTexture();
    }
    
    UpdateScreenshotTexture(current_state_num);

    if (ImGui::BeginTable("SavestatesTable", 2, ImGuiTableFlags_SizingStretchProp))
    {
        // --- Left Column: Slots ---
        ImGui::TableSetupColumn("Slots", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::TextDisabled("Slot");
        if (ImGui::BeginChild("SlotList", ImVec2(0, 360), true))
        {
            for (int i = 0; i < 15; ++i)
            {
                if (ImGui::RadioButton(std::to_string(i).c_str(), &current_state_num, i)) {
                    gui_update();
                    last_state_num = current_state_num;
                    ClearScreenshotTexture();
                }
            }
        }
        ImGui::EndChild();

        // --- Right Column: Details & Actions ---
        ImGui::TableNextColumn();
        ImGui::TextDisabled("State screenshot");
        if (ImGui::BeginChild("ScreenshotArea", ImVec2(0, 360), true))
        {
            if (savestate_screenshot_tex) {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImGui::Image((ImTextureID)savestate_screenshot_tex, region);
            } else {
                 ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - 50.0f, ImGui::GetWindowHeight() * 0.5f));
                 ImGui::Text("(Preview)");
            }
        }
        ImGui::EndChild();

        ImGui::Spacing();

        bool has_state = strlen(savestate_fname) > 0;
        if (has_state) {
            struct stat st{};
            if (stat(savestate_fname, &st) == 0) {
                 ImGui::Text("Filename: %s", extract_filename(std::string(savestate_fname)).c_str());
                 ImGui::Text("%s", get_file_timestamp(savestate_fname).c_str());
            } else {
                 ImGui::Text("Filename: No savestate found");
                 ImGui::Text("%s", extract_filename(std::string(savestate_fname)).c_str());
            }
        } else {
             ImGui::Text("No savestate loaded");
             ImGui::TextUnformatted("\n");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Buttons
        ImGui::BeginDisabled(!has_state);
        if (AmigaButton("Load from Slot", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
        {
            if (emulating && strlen(savestate_fname) > 0) {
                FILE* f = fopen(savestate_fname, "rbe");
                if (f) {
                    fclose(f);
                    savestate_initsave(savestate_fname, 1, 1, true);
                    savestate_state = STATE_DORESTORE;
                    gui_running = false;
                } else {
                    ShowAlert("Loading savestate", "Statefile doesn't exist.");
                }
            } else if (!emulating) {
                 ShowAlert("Loading savestate", "Emulation hasn't started yet.");
            }
        }
        ImGui::SameLine();
        if (AmigaButton("Save to Slot", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
        {
             if (emulating) {
                savestate_initsave(savestate_fname, 1, 0, true);
                save_state(savestate_fname, "...");
                if (create_screenshot()) {
                    save_thumb(screenshot_filename);
                    ClearScreenshotTexture();
                }
             } else {
                 ShowAlert("Saving state", "Emulation hasn't started yet.");
             }
        }
        ImGui::SameLine();
        if (AmigaButton("Delete", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
        {
             if (strlen(savestate_fname) > 0) {
                 show_delete_confirm_popup = true;
             }
        }
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (AmigaButton("Load state...", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
        {
             disk_selection(4, &changed_prefs);
             current_state_num = 99;
             last_state_num = 99;
             gui_update();
             ClearScreenshotTexture();
        }
        ImGui::SameLine();
        if (AmigaButton("Save state...", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
        {
             if (emulating) {
                 disk_selection(5, &changed_prefs);
             } else {
                 ShowAlert("Saving state", "Emulation hasn't started yet.");
             }
        }

        ImGui::EndTable();
    }

    // --- Popups ---
    if (show_alert_popup) {
        ImGui::OpenPopup(alert_title.c_str());
        show_alert_popup = false;
    }
    if (ImGui::BeginPopupModal(alert_title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", alert_message.c_str());
        ImGui::Separator();
        if (AmigaButton("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (show_delete_confirm_popup) {
        ImGui::OpenPopup("Delete Configuration");
        show_delete_confirm_popup = false;
    }
    if (ImGui::BeginPopupModal("Delete Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Do you really want to delete the statefile?");
        ImGui::Text("This will also delete the corresponding screenshot.");
        ImGui::Separator();
        if (AmigaButton("Yes", ImVec2(120, 0))) {
            remove(savestate_fname);
            if (!screenshot_filename.empty()) remove(screenshot_filename.c_str());
            ClearScreenshotTexture();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (AmigaButton("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
