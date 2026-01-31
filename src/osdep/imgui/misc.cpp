#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "statusline.h"
#include "gui/gui_handling.h"

// Helper for bitmask checkboxes
static bool CheckboxFlags(const char* label, int* flags, int mask)
{
    bool v = ((*flags & mask) == mask);
    bool pressed = AmigaCheckbox(label, &v);
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

        if (AmigaButton("Cancel")) {
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
    if (AmigaButton("...")) {
        target_hotkey_string = config_val;
        open_hotkey_popup = true;
    }
    ImGui::SameLine();
    if (AmigaButton("X")) {
        config_val[0] = '\0';
    }
    ImGui::PopID();
}

void render_panel_misc()
{
    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    // Use a table for better column control
    if (ImGui::BeginTable("MiscPanelTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Checkboxes", ImGuiTableColumnFlags_WidthStretch, 0.6f); // 60% width
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 0.4f);   // 40% width
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        // --- LEFT COLUMN: Checkboxes ---
        CheckboxFlags("Untrap = middle button", &changed_prefs.input_mouse_untrap, MOUSEUNTRAP_MIDDLEBUTTON);
        AmigaCheckbox("Show GUI on startup", &changed_prefs.start_gui);
        AmigaCheckbox("Always on top", &changed_prefs.main_alwaysontop);
        AmigaCheckbox("GUI Always on top", &changed_prefs.gui_alwaysontop);
        AmigaCheckbox("Synchronize clock", &changed_prefs.tod_hack);
        AmigaCheckbox("One second reboot pause", &changed_prefs.reset_delay);
        AmigaCheckbox("Faster RTG", &changed_prefs.picasso96_nocustom);
        AmigaCheckbox("Clipboard sharing", &changed_prefs.clipboard_sharing);
        AmigaCheckbox("Allow native code", &changed_prefs.native_code);
        CheckboxFlags("Status Line native", &changed_prefs.leds_on_screen, STATUSLINE_CHIPSET);
        CheckboxFlags("Status Line RTG", &changed_prefs.leds_on_screen, STATUSLINE_RTG);
        AmigaCheckbox("Log illegal memory accesses", &changed_prefs.illegal_mem);
        AmigaCheckbox("Minimize when focus is lost", &changed_prefs.minimize_inactive);
        AmigaCheckbox("Master floppy write protection", &changed_prefs.floppy_read_only);
        AmigaCheckbox("Master harddrive write protection", &changed_prefs.harddrive_read_only);
        AmigaCheckbox("Hide all UAE autoconfig boards", &changed_prefs.uae_hide_autoconfig);
        AmigaCheckbox("RCtrl = RAmiga", &changed_prefs.right_control_is_right_win_key);
        AmigaCheckbox("Capture mouse when window is activated", &changed_prefs.capture_always);
        AmigaCheckbox("A600/A1200/A4000 IDE scsi.device disable", &changed_prefs.scsidevicedisable);
        AmigaCheckbox("Alt-Tab releases control", &changed_prefs.alt_tab_release);
        AmigaCheckbox("Warp mode reset", &changed_prefs.turbo_boot);
        AmigaCheckbox("Use RetroArch Quit Button", &changed_prefs.use_retroarch_quit);
        AmigaCheckbox("Use RetroArch Menu Button", &changed_prefs.use_retroarch_menu);
        AmigaCheckbox("Use RetroArch Reset Button", &changed_prefs.use_retroarch_reset);

        ImGui::TableNextColumn();

        // --- RIGHT COLUMN: Controls & LEDs ---

        auto HotkeyRow = [&](const char* label, char* config_val) {
            ImGui::PushID(label);
            ImGui::Text("%s", label);
            
            // Nested table for Input + Buttons on the next line
            if (ImGui::BeginTable("##HotkeyRow", 2, ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 1.8f);
                ImGui::TableSetupColumn("Btns", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::BeginDisabled();
                ImGui::InputText("##Display", config_val, 256, ImGuiInputTextFlags_ReadOnly);
                ImGui::EndDisabled();
                
                ImGui::TableNextColumn();
                if (AmigaButton("...")) {
                    target_hotkey_string = config_val;
                    open_hotkey_popup = true;
                }
                ImGui::SameLine();
                if (AmigaButton("X")) {
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
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
            ImGui::TableSetupColumn("Combo", ImGuiTableColumnFlags_WidthStretch);
            
            const char* led_items[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD" };
            
            auto LedRow = [&](const char* label, int* val) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
                const std::string combo_label = "##" + std::string(label);
                ImGui::Combo(combo_label.c_str(), val, led_items, IM_ARRAYSIZE(led_items));
                AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
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
