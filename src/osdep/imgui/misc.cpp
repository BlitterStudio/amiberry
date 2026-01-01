#include "imgui.h"
#include "imgui_internal.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "statusline.h"

// Helper for bitmask checkboxes
static bool CheckboxFlags(const char* label, int* flags, int mask)
{
    bool v = ((*flags & mask) == mask);
    bool pressed = ImGui::Checkbox(label, &v);
    if (pressed)
    {
        if (v)
            *flags |= mask;
        else
            *flags &= ~mask;
    }
    return pressed;
}

static char* target_hotkey_string = nullptr;
static bool open_hotkey_popup = false;

static void ShowHotkeyPopup() {
    if (open_hotkey_popup) {
        ImGui::OpenPopup("Set Hotkey");
        open_hotkey_popup = false;
    }

    if (ImGui::BeginPopupModal("Set Hotkey", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Press a key to map...");
        ImGui::Separator();

        // Check for key presses
        for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
            if (ImGui::IsKeyPressed((ImGuiKey)key)) {
                // Ignore modifier keys alone if desired, or allow them
                // Get Key Name
                const char* key_name = ImGui::GetKeyName((ImGuiKey)key);
                if (key_name && target_hotkey_string) {
                    // Check modifiers
                    std::string full_key;
                    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) full_key += "Ctrl+";
                    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) full_key += "Shift+";
                    if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt)) full_key += "Alt+";
                    if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) || ImGui::IsKeyDown(ImGuiKey_RightSuper)) full_key += "Super+";
                    
                    full_key += key_name;
                    strncpy(target_hotkey_string, full_key.c_str(), 255);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        // Escape to cancel
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();

        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void HotkeyControl(const char* label, char* config_val) {
    ImGui::PushID(label);
    ImGui::Text("%s", label); 
    ImGui::SameLine();
    
    ImGui::BeginDisabled();
    ImGui::Unindent(); // Align text field slightly?
    ImGui::InputText("##Display", config_val, 256, ImGuiInputTextFlags_ReadOnly);
    ImGui::Indent();
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    if (ImGui::Button("...")) {
        target_hotkey_string = config_val;
        open_hotkey_popup = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("X")) {
        config_val[0] = '\0';
    }
    ImGui::PopID();
}

void render_panel_misc()
{
    // Use a table for better column control
    if (ImGui::BeginTable("MiscPanelTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Checkboxes", ImGuiTableColumnFlags_WidthStretch, 0.6f); // 60% width
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 0.4f);   // 40% width
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        // --- LEFT COLUMN: Checkboxes ---
        CheckboxFlags("Untrap = middle button", &changed_prefs.input_mouse_untrap, MOUSEUNTRAP_MIDDLEBUTTON);
        ImGui::Checkbox("Show GUI on startup", &changed_prefs.start_gui);
        ImGui::Checkbox("Always on top", &changed_prefs.main_alwaysontop);
        ImGui::Checkbox("GUI Always on top", &changed_prefs.gui_alwaysontop);
        ImGui::Checkbox("Synchronize clock", &changed_prefs.tod_hack);
        ImGui::Checkbox("One second reboot pause", &changed_prefs.reset_delay);
        ImGui::Checkbox("Faster RTG", &changed_prefs.picasso96_nocustom);
        ImGui::Checkbox("Clipboard sharing", &changed_prefs.clipboard_sharing);
        ImGui::Checkbox("Allow native code", &changed_prefs.native_code);
        CheckboxFlags("Status Line native", &changed_prefs.leds_on_screen, STATUSLINE_CHIPSET);
        CheckboxFlags("Status Line RTG", &changed_prefs.leds_on_screen, STATUSLINE_RTG);
        ImGui::Checkbox("Log illegal memory accesses", &changed_prefs.illegal_mem);
        ImGui::Checkbox("Minimize when focus is lost", &changed_prefs.minimize_inactive);
        ImGui::Checkbox("Master floppy write protection", &changed_prefs.floppy_read_only);
        ImGui::Checkbox("Master harddrive write protection", &changed_prefs.harddrive_read_only);
        ImGui::Checkbox("Hide all UAE autoconfig boards", &changed_prefs.uae_hide_autoconfig);
        ImGui::Checkbox("RCtrl = RAmiga", &changed_prefs.right_control_is_right_win_key);
        ImGui::Checkbox("Capture mouse when window is activated", &changed_prefs.capture_always);
        ImGui::Checkbox("A600/A1200/A4000 IDE scsi.device disable", &changed_prefs.scsidevicedisable);
        ImGui::Checkbox("Alt-Tab releases control", &changed_prefs.alt_tab_release);
        ImGui::Checkbox("Warp mode reset", &changed_prefs.turbo_boot);
        ImGui::Checkbox("Use RetroArch Quit Button", &changed_prefs.use_retroarch_quit);
        ImGui::Checkbox("Use RetroArch Menu Button", &changed_prefs.use_retroarch_menu);
        ImGui::Checkbox("Use RetroArch Reset Button", &changed_prefs.use_retroarch_reset);

        ImGui::TableNextColumn();

        // --- RIGHT COLUMN: Controls & LEDs ---

        auto HotkeyRow = [&](const char* label, char* config_val) {
            ImGui::PushID(label);
            ImGui::Text("%s", label);
            
            // Nested table for Input + Buttons on the next line
            if (ImGui::BeginTable("##HotkeyRow", 2, ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Btns", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::BeginDisabled();
                ImGui::InputText("##Display", config_val, 256, ImGuiInputTextFlags_ReadOnly);
                ImGui::EndDisabled();
                
                ImGui::TableNextColumn();
                if (ImGui::Button("...")) {
                    target_hotkey_string = config_val;
                    open_hotkey_popup = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    config_val[0] = '\0';
                }
                ImGui::EndTable();
            }
            ImGui::PopID();
        };

        HotkeyRow("Open GUI:", changed_prefs.open_gui);
        HotkeyRow("Quit Key:", changed_prefs.quit_amiberry);
        HotkeyRow("Action Replay:", changed_prefs.action_replay);
        HotkeyRow("FullScreen:", changed_prefs.fullscreen_toggle);
        HotkeyRow("Minimize:", changed_prefs.minimize);
        HotkeyRow("Left Amiga:", changed_prefs.left_amiga);
        HotkeyRow("Right Amiga:", changed_prefs.right_amiga);

        ImGui::Separator();
        
        // LEDs Table (2 columns)
        if (ImGui::BeginTable("MiscLEDTable", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Combo", ImGuiTableColumnFlags_WidthStretch);
            
            const char* led_items[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD" };
            
            auto LedRow = [&](const char* label, int* val) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::Combo(label, val, led_items, IM_ARRAYSIZE(led_items));
            };

            LedRow("NumLock:", &changed_prefs.kbd_led_num);
            LedRow("ScrollLock:", &changed_prefs.kbd_led_scr);
            LedRow("CapsLock:", &changed_prefs.kbd_led_cap);
            
            ImGui::EndTable();
        }

        ImGui::EndTable();
    }
    
    ShowHotkeyPopup();
}
