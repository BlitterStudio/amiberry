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
        ShowHelpMarker("Use middle mouse button to release mouse capture");
        AmigaCheckbox("Show GUI on startup", &changed_prefs.start_gui);
        ShowHelpMarker("Show the configuration GUI when Amiberry starts");
        AmigaCheckbox("Always on top", &changed_prefs.main_alwaysontop);
        ShowHelpMarker("Keep the emulator window on top of other windows");
        AmigaCheckbox("GUI Always on top", &changed_prefs.gui_alwaysontop);
        ShowHelpMarker("Keep the configuration GUI on top of other windows");
        AmigaCheckbox("Synchronize clock", &changed_prefs.tod_hack);
        ShowHelpMarker("Sync Amiga's clock with host system time");
        AmigaCheckbox("One second reboot pause", &changed_prefs.reset_delay);
        ShowHelpMarker("Add a 1 second delay when resetting the emulated Amiga");
        AmigaCheckbox("Faster RTG", &changed_prefs.picasso96_nocustom);
        ShowHelpMarker("Speed up RTG (graphics card) operations by disabling custom chipset emulation");
        AmigaCheckbox("Clipboard sharing", &changed_prefs.clipboard_sharing);
        ShowHelpMarker("Share clipboard between Amiga and host system");
        AmigaCheckbox("Allow native code", &changed_prefs.native_code);
        ShowHelpMarker("Allow execution of native code (JIT compiler required)");
        CheckboxFlags("Status Line native", &changed_prefs.leds_on_screen, STATUSLINE_CHIPSET);
        ShowHelpMarker("Show status line with LED indicators in native chipset modes");
        CheckboxFlags("Status Line RTG", &changed_prefs.leds_on_screen, STATUSLINE_RTG);
        ShowHelpMarker("Show status line with LED indicators in RTG modes");
        AmigaCheckbox("Log illegal memory accesses", &changed_prefs.illegal_mem);
        ShowHelpMarker("Log attempts to access invalid memory addresses to console");
        AmigaCheckbox("Minimize when focus is lost", &changed_prefs.minimize_inactive);
        ShowHelpMarker("Automatically minimize the emulator window when it loses focus");
        AmigaCheckbox("Master floppy write protection", &changed_prefs.floppy_read_only);
        ShowHelpMarker("Enable write protection for all floppy disk images");
        AmigaCheckbox("Master harddrive write protection", &changed_prefs.harddrive_read_only);
        ShowHelpMarker("Enable write protection for all hard drive images");
        AmigaCheckbox("Hide all UAE autoconfig boards", &changed_prefs.uae_hide_autoconfig);
        ShowHelpMarker("Hide UAE expansion boards from Amiga's autoconfig system");
        AmigaCheckbox("RCtrl = RAmiga", &changed_prefs.right_control_is_right_win_key);
        ShowHelpMarker("Map Right Control key to Right Amiga key");
        AmigaCheckbox("Capture mouse when window is activated", &changed_prefs.capture_always);
        ShowHelpMarker("Automatically capture mouse when clicking on the emulator window");
        AmigaCheckbox("A600/A1200/A4000 IDE scsi.device disable", &changed_prefs.scsidevicedisable);
        ShowHelpMarker("Disable scsi.device for built-in IDE controllers on these models");
        AmigaCheckbox("Alt-Tab releases control", &changed_prefs.alt_tab_release);
        ShowHelpMarker("Release mouse and keyboard capture when Alt-Tab is pressed");
        AmigaCheckbox("Warp mode reset", &changed_prefs.turbo_boot);
        ShowHelpMarker("Enable turbo/warp mode during reset for faster booting");
        AmigaCheckbox("Use RetroArch Quit Button", &changed_prefs.use_retroarch_quit);
        ShowHelpMarker("Enable RetroArch-style quit button mapping");
        AmigaCheckbox("Use RetroArch Menu Button", &changed_prefs.use_retroarch_menu);
        ShowHelpMarker("Enable RetroArch-style menu button mapping");
        AmigaCheckbox("Use RetroArch Reset Button", &changed_prefs.use_retroarch_reset);
        ShowHelpMarker("Enable RetroArch-style reset button mapping");

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
        ShowHelpMarker("Keyboard shortcut to open the configuration GUI");
        HotkeyRow("Quit Key:", changed_prefs.quit_amiberry);
        ShowHelpMarker("Keyboard shortcut to quit the emulator");
        HotkeyRow("Action Replay:", changed_prefs.action_replay);
        ShowHelpMarker("Keyboard shortcut to activate Action Replay cartridge");
        HotkeyRow("FullScreen:", changed_prefs.fullscreen_toggle);
        ShowHelpMarker("Keyboard shortcut to toggle fullscreen mode");
        HotkeyRow("Minimize:", changed_prefs.minimize);
        ShowHelpMarker("Keyboard shortcut to minimize the emulator window");
        HotkeyRow("Left Amiga:", changed_prefs.left_amiga);
        ShowHelpMarker("Keyboard shortcut to simulate Left Amiga key press");
        HotkeyRow("Right Amiga:", changed_prefs.right_amiga);
        ShowHelpMarker("Keyboard shortcut to simulate Right Amiga key press");

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
            ShowHelpMarker("Map Amiga status LED to keyboard NumLock indicator");
            LedRow("ScrollLock:", &changed_prefs.kbd_led_scr);
            ShowHelpMarker("Map Amiga status LED to keyboard ScrollLock indicator");
            LedRow("CapsLock:", &changed_prefs.kbd_led_cap);
            ShowHelpMarker("Map Amiga status LED to keyboard CapsLock indicator");
            
            ImGui::EndTable();
        }

        ImGui::EndTable();
    }
    
    ShowHotkeyPopup();
}
