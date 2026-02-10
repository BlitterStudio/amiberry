#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "amiberry_gfx.h" // For getdisplay and MultiDisplay
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "gfxboard.h"

void render_panel_display() {
    ImGui::Indent(4.0f);

    // Logic Check: RTG Enabled
    // WinUAE: ((!address_space_24 || configtype==2) && size) || type >= HARDWARE
    bool rtg_enabled = false;
#ifdef PICASSO96
    bool addr_ok = !changed_prefs.address_space_24 ||
                   gfxboard_get_configtype(&changed_prefs.rtgboards[0]) == 2;
    rtg_enabled = (addr_ok && changed_prefs.rtgboards[0].rtgmem_size) ||
                  changed_prefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE;
#endif

    // Logic Check: Refresh Rate Slider / FPS Adj
    bool refresh_enabled = !changed_prefs.cpu_memory_cycle_exact;

    // Logic Check: Resolution & Line Mode Groups
    bool autoresol_enabled = changed_prefs.gfx_autoresolution > 0;
    bool linemode_enabled = !autoresol_enabled;
    bool resolution_enabled = !autoresol_enabled;

    // Logic Check: VGA Autoswitch
    // RES_HIRES=1, VRES_DOUBLE=1.
    bool vga_autoswitch_enabled = (changed_prefs.gfx_resolution >= 1 && changed_prefs.gfx_vresolution >= 1);
    if (!vga_autoswitch_enabled && changed_prefs.gfx_autoresolution_vga) {
        changed_prefs.gfx_autoresolution_vga = false;
    }

    // ---------------------------------------------------------
    // Screen Group (Top)
    // ---------------------------------------------------------
    BeginGroupBox("Screen Settings");

    // Windowed & Buffering
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Windowed:");
    ImGui::SameLine(BUTTON_WIDTH);
    ImGui::SetNextItemWidth(BUTTON_WIDTH / 2);
    ImGui::InputInt("##WinW", &changed_prefs.gfx_monitor[0].gfx_size_win.width, 0, 0);
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH / 2);
    ImGui::InputInt("##WinH", &changed_prefs.gfx_monitor[0].gfx_size_win.height, 0, 0);
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    ImGui::SameLine();
    AmigaCheckbox("Window resize", &changed_prefs.gfx_windowed_resize);
    ShowHelpMarker("Allow resizing the emulator window by dragging the window edges");

   // Borderless (Enable only if Windowed)
    ImGui::SameLine();
    bool is_windowed = changed_prefs.gfx_apmode[0].gfx_fullscreen == 0;
    if (!is_windowed) ImGui::BeginDisabled();
    AmigaCheckbox("Borderless", &changed_prefs.borderless);
    ShowHelpMarker("Remove window decorations (title bar and borders) while in windowed mode");
    if (!is_windowed) ImGui::EndDisabled();

    ImGui::Spacing();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Native:");
    ImGui::SameLine(BUTTON_WIDTH);
    const char *screenmode_items[] = {"Windowed", "Fullscreen", "Full-window"};
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    if (ImGui::BeginCombo("##NativeMode", screenmode_items[changed_prefs.gfx_apmode[0].gfx_fullscreen])) {
        for (int n = 0; n < IM_ARRAYSIZE(screenmode_items); n++) {
            const bool is_selected = (changed_prefs.gfx_apmode[0].gfx_fullscreen == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(screenmode_items[n], is_selected)) {
                changed_prefs.gfx_apmode[0].gfx_fullscreen = n;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Native chipset screen mode: Windowed, Fullscreen or Full-window (borderless fullscreen)");

    ImGui::SameLine();

    // VSync Mapping
    const char *vsync_items[] = {"-", "Lagless", "Lagless 50/60Hz", "Standard", "Standard 50/60Hz"};
    // Calc current index
    int vsync_idx = 0;
    if (changed_prefs.gfx_apmode[0].gfx_vsync == 1 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 1) vsync_idx = 1;
    else if (changed_prefs.gfx_apmode[0].gfx_vsync == 2 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 1)
        vsync_idx = 2;
    else if (changed_prefs.gfx_apmode[0].gfx_vsync == 1 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 0)
        vsync_idx = 3;
    else if (changed_prefs.gfx_apmode[0].gfx_vsync == 2 && changed_prefs.gfx_apmode[0].gfx_vsyncmode == 0)
        vsync_idx = 4;

    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    if (ImGui::BeginCombo("##NativeVSync", vsync_items[vsync_idx])) {
        for (int n = 0; n < IM_ARRAYSIZE(vsync_items); n++) {
            const bool is_selected = (vsync_idx == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(vsync_items[n], is_selected)) {
                vsync_idx = n;
                switch (vsync_idx) {
                    case 0: changed_prefs.gfx_apmode[0].gfx_vsync = 0;
                        changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
                        break;
                    case 1: changed_prefs.gfx_apmode[0].gfx_vsync = 1;
                        changed_prefs.gfx_apmode[0].gfx_vsyncmode = 1;
                        break;
                    case 2: changed_prefs.gfx_apmode[0].gfx_vsync = 2;
                        changed_prefs.gfx_apmode[0].gfx_vsyncmode = 1;
                        break;
                    case 3: changed_prefs.gfx_apmode[0].gfx_vsync = 1;
                        changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
                        break;
                    case 4: changed_prefs.gfx_apmode[0].gfx_vsync = 2;
                        changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
                        break;
                }
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("VSync mode: Lagless minimizes input lag, Standard waits for vertical blank, 50/60Hz variants enforce specific refresh rates");

    ImGui::Spacing();

    if (!rtg_enabled) ImGui::BeginDisabled();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("RTG:");
    ImGui::SameLine(BUTTON_WIDTH);
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    if (ImGui::BeginCombo("##RTGMode", screenmode_items[changed_prefs.gfx_apmode[1].gfx_fullscreen])) {
        for (int n = 0; n < IM_ARRAYSIZE(screenmode_items); n++) {
            const bool is_selected = (changed_prefs.gfx_apmode[1].gfx_fullscreen == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(screenmode_items[n], is_selected)) {
                changed_prefs.gfx_apmode[1].gfx_fullscreen = n;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("RTG (Picasso96) screen mode when using graphics card emulation");

    ImGui::SameLine();

    const char *vsync_rtg_items[] = {"-", "Lagless"};
    // Calculate RTG VSync index from gfx_vsync value
    int rtg_vsync_idx = 0;
    if (changed_prefs.gfx_apmode[1].gfx_vsync > 0)
        rtg_vsync_idx = 1;
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    if (ImGui::BeginCombo("##RTGVSync", vsync_rtg_items[rtg_vsync_idx])) {
        for (int n = 0; n < IM_ARRAYSIZE(vsync_rtg_items); n++) {
            const bool is_selected = (rtg_vsync_idx == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(vsync_rtg_items[n], is_selected)) {
                if (n == 0) {
                    changed_prefs.gfx_apmode[1].gfx_vsync = 0;
                    changed_prefs.gfx_apmode[1].gfx_vsyncmode = 0;
                } else if (n == 1) {
                    changed_prefs.gfx_apmode[1].gfx_vsync = 1;
                    changed_prefs.gfx_apmode[1].gfx_vsyncmode = 1;
                }
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("VSync mode for RTG display");
    if (!rtg_enabled) ImGui::EndDisabled();

    ImGui::Spacing();

    // Checkboxes
    if (ImGui::BeginTable("ChkTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.5f);
        ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.5f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        AmigaCheckbox("Blacker than black", &changed_prefs.gfx_blackerthanblack);
        ShowHelpMarker("Enable proper black levels for displays that support it");
        AmigaCheckbox("Remove interlace artifacts", &changed_prefs.gfx_scandoubler);
        ShowHelpMarker("Apply scanline doubling to reduce flickering from interlaced video modes");
        AmigaCheckbox("Monochrome video out", &changed_prefs.gfx_grayscale);
        ShowHelpMarker("Render display in grayscale, simulating a monochrome monitor");
        ImGui::TableNextColumn();
        AmigaCheckbox("Filtered low resolution", (bool *) &changed_prefs.gfx_lores_mode);
        ShowHelpMarker("Apply filtering to low resolution modes for smoother appearance");

        if (!vga_autoswitch_enabled) ImGui::BeginDisabled();
        AmigaCheckbox("VGA mode res. autoswitch", &changed_prefs.gfx_autoresolution_vga);
        ShowHelpMarker("Automatically switch to VGA resolution when VGA modes are detected (requires HighRes and Double line mode)");
        if (!vga_autoswitch_enabled) ImGui::EndDisabled();

        bool resync_blank = changed_prefs.gfx_monitorblankdelay > 0;
        if (AmigaCheckbox("Display resync blanking", &resync_blank)) {
            changed_prefs.gfx_monitorblankdelay = resync_blank ? 1000 : 0;
        }
        ShowHelpMarker("Blank the screen briefly when display mode changes to prevent visual artifacts");
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Resolution row
    if (ImGui::BeginTable("ResTable", 4, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("label1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.5f);
        ImGui::TableSetupColumn("combo1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.5f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Resolution:");
        ImGui::SameLine();
        const char *resolution_items[] = {"LowRes", "HighRes (normal)", "SuperHighRes"};
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (!resolution_enabled) ImGui::BeginDisabled();
        if (ImGui::BeginCombo("##Resolution", resolution_items[changed_prefs.gfx_resolution])) {
            for (int n = 0; n < IM_ARRAYSIZE(resolution_items); n++) {
                const bool is_selected = (changed_prefs.gfx_resolution == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(resolution_items[n], is_selected)) {
                    changed_prefs.gfx_resolution = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Horizontal resolution: LowRes (320px), HighRes (640px), or SuperHighRes (1280px). Disabled when resolution autoswitch is enabled");
        if (!resolution_enabled) ImGui::EndDisabled();

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Overscan:");
        ImGui::SameLine();
        const char *overscan_items[] = {"Standard", "Overscan", "Broadcast", "Extreme", "Ultra"};
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::BeginCombo("##Overscan", overscan_items[changed_prefs.gfx_overscanmode])) {
            for (int n = 0; n < IM_ARRAYSIZE(overscan_items); n++) {
                const bool is_selected = (changed_prefs.gfx_overscanmode == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(overscan_items[n], is_selected)) {
                    changed_prefs.gfx_overscanmode = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Amount of border area to display. Higher values show more of the display area including normally hidden borders");
        ImGui::EndTable();
    }

    // Autoswitch
    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Resolution autoswitch:");
    ImGui::SameLine();
    const char *res_autoswitch_items[] = {"Disabled", "Always On", "10%", "33%", "66%"};
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    // Convert int value to index (0, 1, 10, 33, 66 -> 0, 1, 2, 3, 4)
    int autoswitch_idx = 0;
    if (changed_prefs.gfx_autoresolution == 1) autoswitch_idx = 1;
    else if (changed_prefs.gfx_autoresolution >= 2 && changed_prefs.gfx_autoresolution <= 10) autoswitch_idx = 2;
    else if (changed_prefs.gfx_autoresolution > 10 && changed_prefs.gfx_autoresolution <= 33) autoswitch_idx = 3;
    else if (changed_prefs.gfx_autoresolution > 33) autoswitch_idx = 4;

    if (ImGui::BeginCombo("##ResAutoswitch", res_autoswitch_items[autoswitch_idx])) {
        for (int n = 0; n < IM_ARRAYSIZE(res_autoswitch_items); n++) {
            const bool is_selected = (autoswitch_idx == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(res_autoswitch_items[n], is_selected)) {
                autoswitch_idx = n;
                switch (autoswitch_idx) {
                    case 0: changed_prefs.gfx_autoresolution = 0;
                        break;
                    case 1: changed_prefs.gfx_autoresolution = 1;
                        break;
                    case 2: changed_prefs.gfx_autoresolution = 10;
                        break;
                    case 3: changed_prefs.gfx_autoresolution = 33;
                        break;
                    case 4: changed_prefs.gfx_autoresolution = 66;
                        break;
                }
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Automatically switch resolution based on Amiga display mode. Percentage values control sensitivity threshold");

    ImGui::Spacing();

    // Refresh Slider + PAL/NTSC
    // WinUAE: Label "Refresh:" -> Slider -> Dropdown "PAL"
    if (ImGui::BeginTable("RefreshRowTable", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("combo", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Refresh:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (!refresh_enabled) ImGui::BeginDisabled();
        ImGui::SliderInt("##RefreshSld", &changed_prefs.gfx_framerate, 1, 10, ""); // 1=Every, 2=Every 2nd
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ShowHelpMarker("Frame skip rate: 1=every frame (no skip), 2=every other frame, etc. Disabled in cycle-exact mode");
        if (!refresh_enabled) ImGui::EndDisabled();

        ImGui::TableNextColumn();
        const char *video_standards[] = {"PAL", "NTSC"};
        int current_vid_Standard = changed_prefs.ntscmode ? 1 : 0;
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::BeginCombo("##VideoStandard", video_standards[current_vid_Standard])) {
            for (int n = 0; n < IM_ARRAYSIZE(video_standards); n++) {
                const bool is_selected = (current_vid_Standard == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(video_standards[n], is_selected)) {
                    current_vid_Standard = n;
                    changed_prefs.ntscmode = (current_vid_Standard == 1);
                    if (changed_prefs.ntscmode) {
                        changed_prefs.chipset_refreshrate = 60;
                    } else {
                        changed_prefs.chipset_refreshrate = 50;
                    }
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Video standard: PAL (50Hz, European) or NTSC (60Hz, American/Japanese)");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("FPS adj.:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        // WinUAE: FPS adj slider/text enabled by cr->locked, not by cpu_memory_cycle_exact
        bool fps_adj_enabled = changed_prefs.cr[changed_prefs.cr_selected].locked;
        if (!fps_adj_enabled) ImGui::BeginDisabled();
        ImGui::SliderFloat("##FPSAdjSld", &changed_prefs.cr[changed_prefs.cr_selected].rate, 1.0f, 100.0f, "");
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ShowHelpMarker("Adjust frame rate adjustment percentage. Only active when locked checkbox is enabled");
        if (!fps_adj_enabled) ImGui::EndDisabled();
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.6f", changed_prefs.cr[changed_prefs.cr_selected].rate);
        if (!fps_adj_enabled) ImGui::BeginDisabled();
        ImGui::InputText("##FPSVal", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        if (!fps_adj_enabled) ImGui::EndDisabled();
        ImGui::SameLine();
        AmigaCheckbox("##FPSLocked", (bool *) &changed_prefs.cr[changed_prefs.cr_selected].locked);
        ShowHelpMarker("Lock the FPS adjustment to a fixed value. When unlocked, automatic adjustment is used");
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Display Attributes (Brightness, Contrast, Gamma, etc.)
    static int current_attr_idx = 0;
    const char *attr_items[] = {
        "Brightness", "Contrast", "Gamma", "Gamma [R]", "Gamma [G]", "Gamma [B]", "Dark palette fix"
    };
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
    if (ImGui::BeginCombo("##AttrCombo", attr_items[current_attr_idx])) {
        for (int n = 0; n < IM_ARRAYSIZE(attr_items); n++) {
            const bool is_selected = (current_attr_idx == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(attr_items[n], is_selected)) {
                current_attr_idx = n;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Display color adjustment attribute. Adjust using slider and numeric input below");

    int *val_ptr = nullptr;
    int min_val = -200;
    int max_val = 200;
    int multiplier = 10;

    switch (current_attr_idx) {
        case 0: val_ptr = &changed_prefs.gfx_luminance;
            break;
        case 1: val_ptr = &changed_prefs.gfx_contrast;
            break;
        case 2: val_ptr = &changed_prefs.gfx_gamma;
            break;
        case 3: val_ptr = &changed_prefs.gfx_gamma_ch[0];
            break;
        case 4: val_ptr = &changed_prefs.gfx_gamma_ch[1];
            break;
        case 5: val_ptr = &changed_prefs.gfx_gamma_ch[2];
            break;
        case 6:
            val_ptr = &changed_prefs.gfx_threebitcolors;
            min_val = 0;
            max_val = 3;
            multiplier = 1;
            break;
    }

    if (val_ptr) {
        ImGui::SameLine();
        // Convert stored value to UI value
        int ui_val = *val_ptr / multiplier;
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::SliderInt("##AttrSlider", &ui_val, min_val, max_val, "")) {
            *val_ptr = ui_val * multiplier;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        // Input as float-like display to match WinUAE style logic "%.1f"
        // WinUAE displays (value / multiplier)
        // For editability, InputDouble or InputFloat is good.
        double disp_val = (double) *val_ptr / (double) multiplier;
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::InputDouble("##AttrVal", &disp_val, 0, 0, "%.1f")) {
            *val_ptr = (int) (disp_val * multiplier);
            if (current_attr_idx == 6) {
                // clamp special case
                if (*val_ptr < 0) *val_ptr = 0;
                if (*val_ptr > 3) *val_ptr = 3;
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    }
    ImGui::Spacing();
    EndGroupBox("Screen Settings");

    ImGui::Spacing();

    // ---------------------------------------------------------
    // Right Column
    // ---------------------------------------------------------
    if (ImGui::BeginTable("BottomGrid", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2);
        ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2);
        ImGui::TableSetupColumn("column3", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        BeginGroupBox("Centering");
        bool h_center = changed_prefs.gfx_xcenter == 2;
        if (AmigaCheckbox("Horizontal", &h_center)) changed_prefs.gfx_xcenter = h_center ? 2 : 0;
        ShowHelpMarker("Automatically center the display horizontally on screen");
        bool v_center = changed_prefs.gfx_ycenter == 2;
        if (AmigaCheckbox("Vertical", &v_center)) changed_prefs.gfx_ycenter = v_center ? 2 : 0;
        ShowHelpMarker("Automatically center the display vertically on screen");
        ImGui::Spacing();
        EndGroupBox("Centering");

        ImGui::TableNextColumn();

        // Line Mode Group
        BeginGroupBox("Line Mode");
        if (!linemode_enabled) ImGui::BeginDisabled();
        if (AmigaRadioButton("Single", changed_prefs.gfx_vresolution == 0 && changed_prefs.gfx_pscanlines == 0)) {
            changed_prefs.gfx_vresolution = 0;
            changed_prefs.gfx_pscanlines = 0;
        }
        ShowHelpMarker("Single scan: display one line per scanline (200 lines for NTSC, 256 for PAL)");
        if (AmigaRadioButton("Double", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 0)) {
            changed_prefs.gfx_vresolution = 1;
            changed_prefs.gfx_pscanlines = 0;
        }
        ShowHelpMarker("Line doubling: display each line twice for better visibility (400/512 lines)");
        if (AmigaRadioButton("Scanlines", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 1)) {
            changed_prefs.gfx_vresolution = 1;
            changed_prefs.gfx_pscanlines = 1;
        }
        ShowHelpMarker("Simulate scanlines with black lines between display lines for CRT effect");
        if (AmigaRadioButton("Double, fields", changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 2)) {
            changed_prefs.gfx_vresolution = 1;
            changed_prefs.gfx_pscanlines = 2;
        }
        ShowHelpMarker("Interlaced field rendering: alternates between even and odd lines");
        if (AmigaRadioButton("Double, fields+",
                               changed_prefs.gfx_vresolution == 1 && changed_prefs.gfx_pscanlines == 3)) {
            changed_prefs.gfx_vresolution = 1;
            changed_prefs.gfx_pscanlines = 3;
                               }
        ShowHelpMarker("Enhanced interlaced field rendering with improved blending");
        if (!linemode_enabled) ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("Line Mode");

        ImGui::TableNextColumn();

        // Interlaced Line Mode Group
        BeginGroupBox("Interlaced line mode");
        bool is_double = changed_prefs.gfx_vresolution > 0;
        if (!linemode_enabled || is_double) ImGui::BeginDisabled();
        if (AmigaRadioButton("Single##I", changed_prefs.gfx_iscanlines == 0 && !is_double)) {
            changed_prefs.gfx_iscanlines = 0;
        }
        ShowHelpMarker("Single line mode for interlaced screens (available only in single line mode)");
        if (!linemode_enabled || is_double) ImGui::EndDisabled();

        if (!linemode_enabled || !is_double) ImGui::BeginDisabled();
        // WinUAE has logic mapping these to 0, 1, 2 depending on values.
        // IDC_LM_IDOUBLED -> Frames? Is enabled if Double is selected.
        // My previous assumption regarding Double/Fields/Fields+ was iscanlines=1,2 etc.
        // Wait, CheckRadioButton says:
        // IDC_LM_INORMAL + (gfx_iscanlines ? gfx_iscanlines + 1: (gfx_vresolution ? 1 : 0))
        // If IDoubled (Double frames) is selected (val=1?), iscanlines must be...
        // If ISingle selected (val=0), iscanlines=0.
        // We need to match this.
        // Let's rely on radio button variable binding.
        // We need 4 options here.
        // Single, Double frames, Double fields, Double fields+
        // If Single##I is selected (RadioButton 0), it sets iscanlines = 0.
        // If Double frames##I (RadioButton 1) ?
        // If Double fields##I (RadioButton 2) ?
        // If Double fields+##I (RadioButton 3) ?
        // Mapping:
        // If ISingle: iscan=0.
        // If IDouble frames: iscan=0? No, that would overlap.
        // In WinUAE, if Double is selected, vres>0.
        // The check code: INORMAL + (iscan? iscan+1 : (vres?1:0))
        // If vres=1, scan=0 -> INORMAL + 1 -> IDOUBLED.
        // So IDOUBLED (Double frames) maps to iscan=0 when vres=1.
        // IDOUBLED2 (Double fields) maps to iscan=1.
        // IDOUBLED3 (Double fields+) maps to iscan=2.

        // So:
        // Single##I -> enabled if !is_double. iscan=0.
        // Double Frames##I -> enabled if is_double. iscan=0.
        // Double Fields##I -> enabled if is_double. iscan=1.
        // Double Fields+##I -> enabled if is_double. iscan=2.

        // Implementation:
        // Toggle radio button for "Double frames" manually if iscan==0.
        bool dbl_frames_active = (changed_prefs.gfx_iscanlines == 0 && is_double);
        if (AmigaRadioButton("Double, frames##I", dbl_frames_active)) { changed_prefs.gfx_iscanlines = 0; }
        ShowHelpMarker("Double frames mode for interlaced screens (available only in double line mode)");
        AmigaRadioButton("Double, fields##I", &changed_prefs.gfx_iscanlines, 1);
        ShowHelpMarker("Interlaced field rendering: alternates between even and odd lines");
        AmigaRadioButton("Double, fields+##I", &changed_prefs.gfx_iscanlines, 2);
        ShowHelpMarker("Enhanced interlaced field rendering with improved blending");
        if (!linemode_enabled || !is_double) ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("Interlaced line mode");

        ImGui::EndTable();
    }
}
