#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include <string>
#include <vector>

#include "imgui_panels.h"
#include "gui/gui_handling.h"

static const char* languages[] = { "US", "FR", "UK", "DE" };
static const char* styles[] = { "Original", "Warm", "Cool", "Dark" };

// Helper to find index of string in array
static int GetIndex(const char* list[], int count, const char* selection) {
    for (int i = 0; i < count; ++i) {
        if (stricmp(list[i], selection) == 0) return i;
    }
    return 0; 
}

// Hotkey Capture Popup Logic (Specific to Virtual Keyboard Toggle)
static bool open_vkbd_hotkey_popup = false;

static void ShowVkbdHotkeyPopup() {
    if (open_vkbd_hotkey_popup) {
        ImGui::OpenPopup("Set Virtual Keyboard Hotkey");
        open_vkbd_hotkey_popup = false;
    }

    if (ImGui::BeginPopupModal("Set Virtual Keyboard Hotkey", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Press a button on your controller to set it as the Hotkey button...");
        ImGui::Separator();

        // TODO: Controller button capture in ImGui needs verification.
        
        for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
            if (ImGui::IsKeyPressed((ImGuiKey)key)) {
                if (key == ImGuiKey_Escape) {
                     ImGui::CloseCurrentPopup();
                     break;
                }
                const char* key_name = ImGui::GetKeyName((ImGuiKey)key);
                if (key_name) {
                     // Set to changed_prefs.vkbd_toggle
                     strncpy(changed_prefs.vkbd_toggle, key_name, 255);
                     // Clear joy mapping for now as this is a Key
                     for (int port = 0; port < 2; port++) {
                        const auto host_joy_id = changed_prefs.jports[port].id - JSEM_JOYS;
                        if (host_joy_id >= 0 && host_joy_id < MAX_INPUT_DEVICES) {
                             di_joystick[host_joy_id].mapping.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
                        }
                     }
                     ImGui::CloseCurrentPopup();
                }
            }
        }

        if (AmigaButton("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void render_panel_virtual_keyboard()
{
    ImGui::Indent(4.0f);

    // On-screen touch joystick (D-pad + fire buttons overlay)
    AmigaCheckbox("On-screen Joystick", &changed_prefs.onscreen_joystick);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show virtual D-pad and fire buttons for touchscreen control");
    }

    ImGui::Separator();

    AmigaCheckbox("Virtual Keyboard enabled", &changed_prefs.vkbd_enabled);

    ImGui::BeginDisabled(!changed_prefs.vkbd_enabled);

    AmigaCheckbox("High-Resolution", &changed_prefs.vkbd_hires);
    AmigaCheckbox("Quit button on keyboard", &changed_prefs.vkbd_exit);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Transparency:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    int val = changed_prefs.vkbd_transparency;
    if (ImGui::SliderInt("##Transparency", &val, 0, 100, "%d%%")) {
        changed_prefs.vkbd_transparency = val;
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

    ImGui::Separator();

    // Dropdowns with Labels on LEFT
    // Layout: [Label] [Combo]
    
    // Language
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Keyboard Layout:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        int current_lang = GetIndex(languages, 4, changed_prefs.vkbd_language);
        if (ImGui::Combo("##Language", &current_lang, languages, 4)) {
            strncpy(changed_prefs.vkbd_language, languages[current_lang], 255);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    }

    // Style
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Style:          ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
        int current_style = GetIndex(styles, 4, changed_prefs.vkbd_style);
        if (ImGui::Combo("##Style", &current_style, styles, 4)) {
            strncpy(changed_prefs.vkbd_style, styles[current_style], 255);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    }

    ImGui::Separator();

    // Toggle Button
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Toggle button:");
    ImGui::SameLine();
    
    ImGui::BeginDisabled(changed_prefs.use_retroarch_vkbd); // Disable if RetroArch checked
    
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    char buf[256];
    strncpy(buf, changed_prefs.vkbd_toggle, 255);
    ImGui::InputText("##ToggleDisplay", buf, 255, ImGuiInputTextFlags_ReadOnly);
    
    ImGui::SameLine();
    if (AmigaButton("...")) {
        open_vkbd_hotkey_popup = true;
    }
    ImGui::SameLine();
    if (AmigaButton("X")) {
         changed_prefs.vkbd_toggle[0] = '\0';
         // Also clear joy mapping
         for (int port = 0; port < 2; port++) {
            const auto host_joy_id = changed_prefs.jports[port].id - JSEM_JOYS;
             if (host_joy_id >= 0 && host_joy_id < MAX_INPUT_DEVICES) {
                 di_joystick[host_joy_id].mapping.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
            }
         }
    }
    
    ImGui::EndDisabled(); // RetroArch disable

    AmigaCheckbox("Use RetroArch Vkbd button", &changed_prefs.use_retroarch_vkbd);

    ImGui::EndDisabled(); // Enabled disable

    ShowVkbdHotkeyPopup();
}
