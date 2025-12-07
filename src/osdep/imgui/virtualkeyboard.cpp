#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "gui/gui_handling.h"

void render_panel_virtual_keyboard()
{
    ImGui::Checkbox("Virtual Keyboard enabled", &changed_prefs.vkbd_enabled);

    if (!changed_prefs.vkbd_enabled)
        ImGui::BeginDisabled();

    ImGui::Checkbox("High-Resolution", &changed_prefs.vkbd_hires);
    ImGui::Checkbox("Quit button on keyboard", &changed_prefs.vkbd_exit);

    ImGui::Text("Transparency:");
    ImGui::SameLine();
    ImGui::SliderInt("##VkTransparency", &changed_prefs.vkbd_transparency, 0, 100, "%d%%");

    const char* languages[] = { "US", "FR", "UK", "DE" };
    int current_lang = 0;
    if (strcmp(changed_prefs.vkbd_language, "FR") == 0) current_lang = 1;
    else if (strcmp(changed_prefs.vkbd_language, "UK") == 0) current_lang = 2;
    else if (strcmp(changed_prefs.vkbd_language, "DE") == 0) current_lang = 3;

    if (ImGui::Combo("Keyboard Layout", &current_lang, languages, IM_ARRAYSIZE(languages))) {
        strcpy(changed_prefs.vkbd_language, languages[current_lang]);
    }

    const char* styles[] = { "Original", "Warm", "Cool", "Dark" };
    int current_style = 0;
    if (strcmp(changed_prefs.vkbd_style, "Warm") == 0) current_style = 1;
    else if (strcmp(changed_prefs.vkbd_style, "Cool") == 0) current_style = 2;
    else if (strcmp(changed_prefs.vkbd_style, "Dark") == 0) current_style = 3;

    if (ImGui::Combo("Style", &current_style, styles, IM_ARRAYSIZE(styles))) {
        strcpy(changed_prefs.vkbd_style, styles[current_style]);
    }

    ImGui::Checkbox("Use RetroArch Vkbd button", &changed_prefs.use_retroarch_vkbd);

    if (changed_prefs.use_retroarch_vkbd)
        ImGui::BeginDisabled();

    ImGui::Text("Toggle button:");
    ImGui::SameLine();
    ImGui::InputText("##VkSetHotkey", changed_prefs.vkbd_toggle, sizeof(changed_prefs.vkbd_toggle), ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("..."))
    {
        // TODO: Implement input selection dialog
    }
    ImGui::SameLine();
    if (ImGui::Button("X##VkSetHotkeyClear"))
    {
        changed_prefs.vkbd_toggle[0] = 0;
        for (int port = 0; port < 2; port++)
        {
            const auto host_joy_id = changed_prefs.jports[port].id - JSEM_JOYS;
            if (host_joy_id >= 0 && host_joy_id < MAX_INPUT_DEVICES)
            {
                didata* did = &di_joystick[host_joy_id];
                did->mapping.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
            }
        }
    }

    if (changed_prefs.use_retroarch_vkbd)
        ImGui::EndDisabled();

    if (!changed_prefs.vkbd_enabled)
        ImGui::EndDisabled();
}
