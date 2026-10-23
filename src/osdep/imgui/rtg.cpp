#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "gfxboard.h"
#include "ethernet.h"
#include "../amiberry_gfx.h"
#include "../display_modes.h"
#include "../target.h"
#include <vector>
#include <string>

#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "settings/settings_definitions.h"

namespace settings = amiberry_settings;

void render_panel_rtg() {
    ImGui::Indent(4.0f);

    const settings::context ctx{emulating != 0};
    const settings::definition& size_option = settings::require("gfxcard_size");
    const settings::definition& type_option = settings::require("gfxcard_type");

    // --- RTG Board Selection ---
    // The settings layer owns the board rules: which boards fit the machine
    // (Zorro III needs 32-bit addressing, Zorro II at most 2 MB chip RAM),
    // their memory ranges, and what turning a board on changes.
    ImGui::Text("RTG Graphics Card");
    ImGui::Separator();

    struct rtgboardconfig *rbc = &changed_prefs.rtgboards[0];
    const settings::choice_list boards = settings::choices_of(type_option, changed_prefs);
    char current_type[64] = "";
    settings::get(type_option, changed_prefs, current_type, sizeof current_type);
    const bool board_on = rbc->rtgmem_size > 0;
    const char* preview = "-";
    for (size_t i = 0; board_on && i < boards.count; i++) {
        if (!strcmp(boards.items[i].value, current_type))
            preview = boards.items[i].label;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    if (ImGui::BeginCombo("##RTGBoardCombo", preview)) {
        if (ImGui::Selectable("-", !board_on)) {
            settings::set(size_option, changed_prefs, ctx, "0");
            // A second board, if any, moves into slot 0.
            cfgfile_compatibility_rtg(&changed_prefs);
        }
        for (size_t i = 0; i < boards.count; i++) {
            const bool fits = settings::is_choice_available(type_option, changed_prefs, ctx, boards.items[i].value);
            ImGui::BeginDisabled(!fits);
            if (ImGui::Selectable(boards.items[i].label, board_on && !strcmp(boards.items[i].value, current_type))) {
                // Turns the board on too, with its default memory.
                settings::set(type_option, changed_prefs, ctx, boards.items[i].value);
                cfgfile_compatibility_rtg(&changed_prefs);
            }
            ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker("Select RTG graphics card to emulate (Picasso96, CyberGraphX compatible). Zorro III boards need "
        "32-bit addressing; Zorro II boards, at most 2 MB chip RAM.");
    const int current_board_idx = board_on ? 1 : 0;

    if (rbc->rtgmem_type == GFXBOARD_ID_ZZ9000_Z2 ||
        rbc->rtgmem_type == GFXBOARD_ID_ZZ9000_Z3) {
        ImGui::Spacing();
        ImGui::Text("ZZ9000 Ethernet");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
        auto **netdrivers = target_ethernet_enumerate();
        const char *selection = "Unavailable";
        if (!_tcsicmp(changed_prefs.zz9000net_name, _T("none")))
            selection = "Disconnected";
        for (int i = 0; netdrivers && netdrivers[i]; ++i) {
            if (!_tcsicmp(changed_prefs.zz9000net_name, netdrivers[i]->name)) {
                selection = netdrivers[i]->desc;
                break;
            }
        }
        if (ImGui::BeginCombo("##ZZ9000Ethernet", selection)) {
            if (ImGui::Selectable("Disconnected", !_tcsicmp(changed_prefs.zz9000net_name, _T("none"))))
                _tcscpy(changed_prefs.zz9000net_name, _T("none"));
            for (int i = 0; netdrivers && netdrivers[i]; ++i) {
                if (ImGui::Selectable(netdrivers[i]->desc,
                                      !_tcsicmp(changed_prefs.zz9000net_name, netdrivers[i]->name)))
                    _tcscpy(changed_prefs.zz9000net_name, netdrivers[i]->name);
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Host network backend for the installed ZZ9000Net.device");
        if (zz9000_net_board_present())
            ImGui::TextDisabled("Running host backend: %s",
                                zz9000_net_host_active() ? "Active" : "Disconnected");
        AmigaCheckbox("ZZ9000 INT2", &changed_prefs.zz9000_int2);
        ShowHelpMarker("Match this to the real drive's ZZ9K_INT2 setting; default is INT6");
    }

    // WinUAE enable_for_expansiondlg() logic
    bool has_memory = rbc->rtgmem_size > 0;
    bool is_uae_rtg = rbc->rtgmem_type < GFXBOARD_HARDWARE;
    bool is_hw_board = rbc->rtgmem_type >= GFXBOARD_HARDWARE;

    bool color_modes_en = has_memory && is_uae_rtg;
    bool display_opts_en = has_memory || is_hw_board;
    bool sw_rtg_opts_en = has_memory && is_uae_rtg;
    bool autoswitch_en = has_memory && (!is_hw_board || !gfxboard_get_switcher(rbc));

    ImGui::BeginDisabled(!has_memory);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Monitor:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
    {
        int mon_id = rbc->monitor_id;
        const char* mon_labels[MAX_AMIGAMONITORS];
        char mon_buf[MAX_AMIGAMONITORS][32];
        for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
            snprintf(mon_buf[i], sizeof(mon_buf[i]), "Monitor %d", i + 1);
            mon_labels[i] = mon_buf[i];
        }
        if (ImGui::Combo("##MonitorCombo", &mon_id, mon_labels, MAX_AMIGAMONITORS)) {
            rbc->monitor_id = mon_id;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    }

    bool override_native = rbc->initial_active;
    if (AmigaCheckbox("Override initial native chipset display", &override_native)) {
        if (override_native) {
            for (int i = 0; i < MAX_RTG_BOARDS; ++i)
                changed_prefs.rtgboards[i].initial_active = false;
        }
        rbc->initial_active = override_native;
    }
    ShowHelpMarker("Start with RTG display active instead of native Amiga chipset");
    ImGui::EndDisabled();

    ImGui::Spacing();

    // Table: VRAM (Left) vs Color Modes (Right)
    if (ImGui::BeginTable("rtg_main_table", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // --- VRAM Section ---
        // The sizes the board takes, from the settings layer.
        std::vector<int> sizes_mb;
        for (int mb = 1; mb <= 256; mb *= 2) {
            if (settings::is_choice_available(size_option, changed_prefs, ctx, std::to_string(mb).c_str()))
                sizes_mb.push_back(mb);
        }
        const int current_mb = rbc->rtgmem_size / (1024 * 1024);
        int slider_val = 0;
        for (size_t i = 0; i < sizes_mb.size(); i++) {
            if (sizes_mb[i] <= current_mb)
                slider_val = static_cast<int>(i);
        }
        const int max_slider_val = sizes_mb.empty() ? 0 : static_cast<int>(sizes_mb.size()) - 1;

        char vram_label[32];
        snprintf(vram_label, sizeof(vram_label), "%d MB", sizes_mb.empty() ? 0 : sizes_mb[slider_val]);

        ImGui::BeginDisabled(current_board_idx == 0 || sizes_mb.empty());
        ImGui::Text("VRAM size:");
        if (ImGui::SliderInt("##VRAMsizeSlider", &slider_val, 0, max_slider_val, vram_label) && !sizes_mb.empty())
            settings::set(size_option, changed_prefs, ctx, std::to_string(sizes_mb[slider_val]).c_str());
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        if (!sizes_mb.empty()) {
            char help[128];
            snprintf(help, sizeof(help), "Video memory for this card (%d MB - %d MB)", sizes_mb.front(), sizes_mb.back());
            ShowHelpMarker(help);
        }
        ImGui::EndDisabled();

        ImGui::TableNextColumn();

        // Right Column: Color Modes (only for UAE RTG)
        ImGui::BeginDisabled(!color_modes_en);
        ImGui::Text("Color modes:");

        // 8-bit Mode
        const char *rtg_8bit_modes[] = {"8-bit", "All", "CLUT (*)"};
        int idx8 = 0;
        uae_u32 mask = changed_prefs.picasso96_modeflags;
        if (mask & RGBFF_CLUT) idx8 = 2;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##8bit", &idx8, rtg_8bit_modes, IM_ARRAYSIZE(rtg_8bit_modes))) {
            mask &= ~RGBFF_CLUT;
            if (idx8 == 1 || idx8 == 2) mask |= RGBFF_CLUT;
            changed_prefs.picasso96_modeflags = mask;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("8-bit color mode settings. CLUT = Color LookUp Table (palette-based)");

        // 16-bit Mode
        const char *rtg_16bit_modes[] = {
            "15/16-bit", "All", "R5G6B5PC (*)", "R5G5B5PC", "R5G6B5", "R5G5B5", "B5G6R5PC", "B5G5R5PC"
        };
        int idx16 = 0;
        if (mask & RGBFF_R5G6B5PC) idx16 = 2;
        else if (mask & RGBFF_R5G5B5PC) idx16 = 3;
        else if (mask & RGBFF_R5G6B5) idx16 = 4;
        else if (mask & RGBFF_R5G5B5) idx16 = 5;
        else if (mask & RGBFF_B5G6R5PC) idx16 = 6;
        else if (mask & RGBFF_B5G5R5PC) idx16 = 7;

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##16bit", &idx16, rtg_16bit_modes, IM_ARRAYSIZE(rtg_16bit_modes))) {
            mask &= ~(RGBFF_R5G6B5PC | RGBFF_R5G5B5PC | RGBFF_R5G6B5 | RGBFF_R5G5B5 | RGBFF_B5G6R5PC | RGBFF_B5G5R5PC);
            if (idx16 == 1) mask |= RGBFF_R5G6B5PC | RGBFF_R5G5B5PC | RGBFF_R5G6B5 | RGBFF_R5G5B5 | RGBFF_B5G6R5PC |
                            RGBFF_B5G5R5PC;
            else if (idx16 == 2) mask |= RGBFF_R5G6B5PC;
            else if (idx16 == 3) mask |= RGBFF_R5G5B5PC;
            else if (idx16 == 4) mask |= RGBFF_R5G6B5;
            else if (idx16 == 5) mask |= RGBFF_R5G5B5;
            else if (idx16 == 6) mask |= RGBFF_B5G6R5PC;
            else if (idx16 == 7) mask |= RGBFF_B5G5R5PC;
            changed_prefs.picasso96_modeflags = mask;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("15/16-bit color format. R=Red, G=Green, B=Blue, PC=PC byte order");

        // 24-bit Mode
        const char *rtg_24bit_modes[] = {"24-bit", "All", "R8G8B8", "B8G8R8"};
        int idx24 = 0;
        if (mask & RGBFF_R8G8B8) idx24 = 2;
        else if (mask & RGBFF_B8G8R8) idx24 = 3;

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##24bit", &idx24, rtg_24bit_modes, IM_ARRAYSIZE(rtg_24bit_modes))) {
            mask &= ~(RGBFF_R8G8B8 | RGBFF_B8G8R8);
            if (idx24 == 1) mask |= RGBFF_R8G8B8 | RGBFF_B8G8R8;
            else if (idx24 == 2) mask |= RGBFF_R8G8B8;
            else if (idx24 == 3) mask |= RGBFF_B8G8R8;
            changed_prefs.picasso96_modeflags = mask;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("24-bit true color format (8 bits per RGB channel)");

        // 32-bit Mode
        const char *rtg_32bit_modes[] = {"32-bit", "All", "A8R8G8B8", "A8B8G8R8", "R8G8B8A8 (*)", "B8G8R8A8"};
        int idx32 = 0;
        if (mask & RGBFF_A8R8G8B8) idx32 = 2;
        else if (mask & RGBFF_A8B8G8R8) idx32 = 3;
        else if (mask & RGBFF_R8G8B8A8) idx32 = 4;
        else if (mask & RGBFF_B8G8R8A8) idx32 = 5;

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##32bit", &idx32, rtg_32bit_modes, IM_ARRAYSIZE(rtg_32bit_modes))) {
            mask &= ~(RGBFF_A8R8G8B8 | RGBFF_A8B8G8R8 | RGBFF_R8G8B8A8 | RGBFF_B8G8R8A8);
            if (idx32 == 1) mask |= RGBFF_A8R8G8B8 | RGBFF_A8B8G8R8 | RGBFF_R8G8B8A8 | RGBFF_B8G8R8A8;
            else if (idx32 == 2) mask |= RGBFF_A8R8G8B8;
            else if (idx32 == 3) mask |= RGBFF_A8B8G8R8;
            else if (idx32 == 4) mask |= RGBFF_R8G8B8A8;
            else if (idx32 == 5) mask |= RGBFF_B8G8R8A8;
            changed_prefs.picasso96_modeflags = mask;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("32-bit color with alpha channel (A=Alpha for transparency)");
        ImGui::EndDisabled(); // color_modes_en

        ImGui::EndTable();
    }
    ImGui::Spacing();

    // --- Settings Checkboxes (2 Columns) ---
    if (ImGui::BeginTable("rtg_settings_table", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Left Column: Scaling options (enabled when any RTG is active)
        ImGui::BeginDisabled(!display_opts_en);

        int current_autoscale_mode = changed_prefs.gf[GF_RTG].gfx_filter_autoscale;

        bool scale_smaller = (current_autoscale_mode == RTG_MODE_SCALE);
        if (AmigaCheckbox("Scale if smaller than display size", &scale_smaller)) {
            changed_prefs.gf[GF_RTG].gfx_filter_autoscale = scale_smaller ? RTG_MODE_SCALE : 0;
        }
        ShowHelpMarker("Automatically scale RTG output if resolution is smaller than display");

        AmigaCheckbox("Always scale in windowed mode", &changed_prefs.rtgallowscaling);
        ShowHelpMarker("Scale RTG output even when running in windowed mode");

        bool always_center = (current_autoscale_mode == RTG_MODE_CENTER);
        if (AmigaCheckbox("Always center", &always_center)) {
            changed_prefs.gf[GF_RTG].gfx_filter_autoscale = always_center ? RTG_MODE_CENTER : 0;
        }
        ShowHelpMarker("Center RTG display on screen without scaling");

        bool int_scale = (current_autoscale_mode == RTG_MODE_INTEGER_SCALE);
        if (AmigaCheckbox("Integer scaling", &int_scale)) {
            changed_prefs.gf[GF_RTG].gfx_filter_autoscale = int_scale ? RTG_MODE_INTEGER_SCALE : 0;
        }
        ShowHelpMarker("Scale by whole number multiples only (2x, 3x, etc.) for sharp pixels");

        int_scale = changed_prefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE;
        ImGui::BeginDisabled(!int_scale);
        ImGui::Text("Integer scale limit:");
        ImGui::SameLine();
        static const char *rtg_scale_limits[] = {"1/1", "1/2", "1/4", "1/8"};
        int rtg_scale_limit = changed_prefs.gf[GF_RTG].gfx_filter_integerscalelimit;
        if (rtg_scale_limit < 0 || rtg_scale_limit >= IM_ARRAYSIZE(rtg_scale_limits)) {
            rtg_scale_limit = 0;
        }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##RTGIntegerScaleLimit", &rtg_scale_limit, rtg_scale_limits, IM_ARRAYSIZE(rtg_scale_limits))) {
            changed_prefs.gf[GF_RTG].gfx_filter_integerscalelimit = rtg_scale_limit;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ImGui::EndDisabled();
        ShowHelpMarker("Allow RTG integer scaling to use fractional 1/x steps when an exact whole-number scale does not fit.");

        bool zero_copy_enabled = is_uae_rtg && changed_prefs.rtg_zerocopy;
        ImGui::BeginDisabled(!is_uae_rtg);
        if (AmigaCheckbox("Zero Copy (Buffer sharing)", &zero_copy_enabled)) {
            changed_prefs.rtg_zerocopy = zero_copy_enabled;
        }
        ImGui::EndDisabled();
        ShowHelpMarker("Share UAE RTG buffers directly between emulation and display for better performance");

        ImGui::EndDisabled(); // display_opts_en

        ImGui::TableNextColumn();

        // Right Column: Hardware/Misc Checkboxes
        bool has_hw_switcher = is_hw_board && gfxboard_get_switcher(rbc);
        bool autoswitch_display = has_memory && (rbc->autoswitch || has_hw_switcher || is_uae_rtg);
        ImGui::BeginDisabled(!autoswitch_en);
        if (AmigaCheckbox("Native/RTG autoswitch", &autoswitch_display)) {
            rbc->autoswitch = autoswitch_display;
        }
        ImGui::EndDisabled();
        ShowHelpMarker("Automatically switch between native chipset and RTG display");

        ImGui::BeginDisabled(!sw_rtg_opts_en);
        AmigaCheckbox("Multithreaded", &changed_prefs.rtg_multithread);
        ShowHelpMarker("Use multiple CPU threads for RTG rendering (better performance)");

        AmigaCheckbox("Hardware sprite emulation", &changed_prefs.rtg_hardwaresprite);
        ShowHelpMarker("Use RTG card's hardware sprite engine for mouse pointer");

        AmigaCheckbox("Hardware vertical blank interrupt", &changed_prefs.rtg_hardwareinterrupt);
        ShowHelpMarker("Emulate hardware VBlank interrupt for RTG display timing");
        ImGui::EndDisabled(); // sw_rtg_opts_en

        ImGui::EndTable();
    }
    ImGui::Spacing();

    // --- Bottom Row: Screen Mode Details ---
    ImGui::BeginDisabled(!display_opts_en);

    ImGui::Text("Screen Mode settings");
    ImGui::Separator();

    {
        int display_count = 0;
        while (display_count < MAX_DISPLAYS && Displays[display_count].monitorname)
            display_count++;
        if (display_count > 1) {
            ImGui::Text("Display:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            int current_display = changed_prefs.gfx_apmode[APMODE_RTG].gfx_display;
            if (ImGui::BeginCombo("##RTGHostDisplay", current_display > 0 && current_display <= display_count
                ? Displays[current_display - 1].monitorname : "Primary")) {
                for (int i = 0; i < display_count; i++) {
                    char label[256];
                    snprintf(label, sizeof(label), "%s%s##disp%d", Displays[i].monitorname,
                        Displays[i].primary ? " (Primary)" : "", i);
                    bool is_selected = (current_display == i + 1);
                    if (ImGui::Selectable(label, is_selected)) {
                        changed_prefs.gfx_apmode[APMODE_RTG].gfx_display = i + 1;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ImGui::Spacing();
        }
    }

    if (ImGui::BeginTable("rtg_bottom_table", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const char *rtg_refreshrates[] = {"Chipset", "Default", "50", "60", "70", "75"};
        int current_rate_idx = 0;
        if (changed_prefs.rtgvblankrate == 0) current_rate_idx = 0;
        else if (changed_prefs.rtgvblankrate == -1) current_rate_idx = 1;
        else if (changed_prefs.rtgvblankrate == 50) current_rate_idx = 2;
        else if (changed_prefs.rtgvblankrate == 60) current_rate_idx = 3;
        else if (changed_prefs.rtgvblankrate == 70) current_rate_idx = 4;
        else if (changed_prefs.rtgvblankrate == 75) current_rate_idx = 5;

        ImGui::AlignTextToFramePadding(); ImGui::Text("Refresh rate:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##RefreshRateCombo", &current_rate_idx, rtg_refreshrates, IM_ARRAYSIZE(rtg_refreshrates))) {
            switch (current_rate_idx) {
                case 0: changed_prefs.rtgvblankrate = 0;
                    break;
                case 1: changed_prefs.rtgvblankrate = -1;
                    break;
                case 2: changed_prefs.rtgvblankrate = 50;
                    break;
                case 3: changed_prefs.rtgvblankrate = 60;
                    break;
                case 4: changed_prefs.rtgvblankrate = 70;
                    break;
                case 5: changed_prefs.rtgvblankrate = 75;
                    break;
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("RTG display refresh rate in Hz. Chipset uses native Amiga timing");

        ImGui::TableNextColumn();

        const char *rtg_buffermodes[] = {"Double buffering", "Triple buffering"};
        int current_buffer = (changed_prefs.gfx_apmode[1].gfx_backbuffers > 0)
                                 ? changed_prefs.gfx_apmode[1].gfx_backbuffers - 1
                                 : 0;
        ImGui::AlignTextToFramePadding(); ImGui::Text("Buffer mode:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##BufferModeCombo", &current_buffer, rtg_buffermodes, IM_ARRAYSIZE(rtg_buffermodes))) {
            changed_prefs.gfx_apmode[1].gfx_backbuffers = current_buffer + 1;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Number of display buffers. Triple buffering reduces tearing but uses more VRAM");

        ImGui::TableNextColumn();

        const char *rtg_aspectratios[] = {"Disabled", "Automatic"};
        int current_aspect = (changed_prefs.rtgscaleaspectratio == 0) ? 0 : 1;
        ImGui::AlignTextToFramePadding(); ImGui::Text("Aspect ratio:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##AspectRatioCombo", &current_aspect, rtg_aspectratios, IM_ARRAYSIZE(rtg_aspectratios))) {
            changed_prefs.rtgscaleaspectratio = (current_aspect == 0) ? 0 : -1;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Correct aspect ratio for square pixels when scaling RTG display");

        ImGui::EndTable();
    }

    ImGui::EndDisabled(); // display_opts_en
}
