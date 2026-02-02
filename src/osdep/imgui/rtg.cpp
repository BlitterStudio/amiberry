#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "gfxboard.h"
#include "../amiberry_gfx.h" // For RTG_MODE constants
#include <vector>
#include <string>

#include "imgui_panels.h"
#include "gui/gui_handling.h"

static std::vector<std::string> rtg_board_names;
static bool rtg_boards_initialized = false;

void render_panel_rtg() {
    ImGui::Indent(4.0f);

    if (!rtg_boards_initialized) {
        rtg_board_names.clear();
        rtg_board_names.emplace_back("-");
        int v = 0;
        TCHAR tmp[256];
        while (true) {
            int index = gfxboard_get_id_from_index(v);
            if (index < 0) break;
            const TCHAR *n1 = gfxboard_get_name(index);
            const TCHAR *n2 = gfxboard_get_manufacturername(index);
            v++;
            _tcscpy(tmp, n1);
            if (n2) {
                _tcscat(tmp, " (");
                _tcscat(tmp, n2);
                _tcscat(tmp, ")");
            }
            rtg_board_names.emplace_back(tmp);
        }
        rtg_boards_initialized = true;
    }

    // Prepare Vector for Combo
    std::vector<const char *> rtg_boards_c;
    for (const auto &name: rtg_board_names) rtg_boards_c.push_back(name.c_str());

    // --- RTG Board Selection ---
    ImGui::Text("RTG Graphics Card");
    ImGui::Separator();

    // Board Dropdown
    int current_board_idx = 0;
    if (changed_prefs.rtgboards[0].rtgmem_size > 0) {
        // Find index that matches current type
        int type = changed_prefs.rtgboards[0].rtgmem_type;
        // Search in the list (index map: list_idx-1 == gfxboard_v_index)
        // We need to reverse map ID -> v_index.
        // Helper: gfxboard_get_index_from_id(type) -> returns v_index.
        // List index is v_index + 1 (because of "-").
        int v_idx = gfxboard_get_index_from_id(type);
        if (v_idx >= 0) current_board_idx = v_idx + 1;
    }

    if (ImGui::Combo("##RTGBoardCombo", &current_board_idx, rtg_boards_c.data(), (int) rtg_boards_c.size())) {
        if (current_board_idx == 0) {
            changed_prefs.rtgboards[0].rtgmem_type = 0;
            changed_prefs.rtgboards[0].rtgmem_size = 0;
        } else {
            // Get ID from v_index (current_board_idx - 1)
            int v_idx = current_board_idx - 1;
            int new_type = gfxboard_get_id_from_index(v_idx);
            changed_prefs.rtgboards[0].rtgmem_type = new_type;

            if (changed_prefs.rtgboards[0].rtgmem_size == 0) {
                // Default sizes based on type (simplification: 4MB default)
                // If Z2 (GFXBOARD_UAE_Z2 == 0?? Check header again. header says 0. But ID from index might return correct ID)
                // Let's rely on standard default.
                changed_prefs.rtgboards[0].rtgmem_size = 4 * 1024 * 1024;
                if (new_type == GFXBOARD_UAE_Z2) changed_prefs.rtgboards[0].rtgmem_size = 2 * 1024 * 1024;
            }
        }
        cfgfile_compatibility_rtg(&changed_prefs);
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

    // Override initial native chipset display
    // WinUAE logic: Exclusive across boards
    bool override_native = changed_prefs.rtgboards[0].initial_active;
    ImGui::BeginDisabled(current_board_idx == 0); // Disable everything if no board selected
    if (AmigaCheckbox("Override initial native chipset display", &override_native)) {
        if (override_native) {
            for (int i = 0; i < MAX_RTG_BOARDS; ++i)
                changed_prefs.rtgboards[i].initial_active = false;
        }
        changed_prefs.rtgboards[0].initial_active = override_native;
    }

    ImGui::Spacing();

    // Table: VRAM (Left) vs Color Modes (Right)
    if (ImGui::BeginTable("rtg_main_table", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // VRAM Section
        int vram_bytes = changed_prefs.rtgboards[0].rtgmem_size;
        int vram_mb = vram_bytes >> 20;
        int slider_val = 0;
        // Calculate log2 of MB to get slider index. 1MB=0, 2MB=1, 4MB=2...
        if (vram_mb > 0) {
            int temp = vram_mb;
            while (temp >>= 1) slider_val++;
        }

        // Zorro II limit is 8MB (index 3: 2^3=8).
        // Zorro III / PCI usually up to 256MB or 512MB.
        // WinUAE max suggests 256MB or similar. Let's limit to 256MB (index 8) for now.
        int max_slider_val = 8;
        if (changed_prefs.rtgboards[0].rtgmem_type == GFXBOARD_UAE_Z2) max_slider_val = 3;

        // Clamp if switching boards caused invalid val
        if (slider_val > max_slider_val) slider_val = max_slider_val;

        // Display textual representation of the slider value
        char vram_label[32];
        snprintf(vram_label, sizeof(vram_label), "%d MB", 1 << slider_val);

        ImGui::Text("VRAM size:");
        if (ImGui::SliderInt("##VRAMsizeSlider", &slider_val, 0, max_slider_val, vram_label)) {
            changed_prefs.rtgboards[0].rtgmem_size = (1 << slider_val) * 1024 * 1024;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

        ImGui::TableNextColumn();

        // Right Column: Color Modes
        ImGui::Text("Color modes:");

        // 8-bit Mode
        const char *rtg_8bit_modes[] = {"8-bit", "All", "CLUT (*)"};
        int idx8 = 0;
        uae_u32 mask = changed_prefs.picasso96_modeflags;
        if (mask & RGBFF_CLUT) idx8 = 2; // Simple assumption
        if (ImGui::Combo("##8bit", &idx8, rtg_8bit_modes, IM_ARRAYSIZE(rtg_8bit_modes))) {
            mask &= ~RGBFF_CLUT; // Clear 8bit
            if (idx8 == 1 || idx8 == 2) mask |= RGBFF_CLUT;
            changed_prefs.picasso96_modeflags = mask;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

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

        // 24-bit Mode
        const char *rtg_24bit_modes[] = {"24-bit", "All", "R8G8B8", "B8G8R8"};
        int idx24 = 0;
        if (mask & RGBFF_R8G8B8) idx24 = 2;
        else if (mask & RGBFF_B8G8R8) idx24 = 3;

        if (ImGui::Combo("##24bit", &idx24, rtg_24bit_modes, IM_ARRAYSIZE(rtg_24bit_modes))) {
            mask &= ~(RGBFF_R8G8B8 | RGBFF_B8G8R8);
            if (idx24 == 1) mask |= RGBFF_R8G8B8 | RGBFF_B8G8R8;
            else if (idx24 == 2) mask |= RGBFF_R8G8B8;
            else if (idx24 == 3) mask |= RGBFF_B8G8R8;
            changed_prefs.picasso96_modeflags = mask;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        // 32-bit Mode
        const char *rtg_32bit_modes[] = {"32-bit", "All", "A8R8G8B8", "A8B8G8R8", "R8G8B8A8 (*)", "B8G8R8A8"};
        int idx32 = 0;
        if (mask & RGBFF_A8R8G8B8) idx32 = 2;
        else if (mask & RGBFF_A8B8G8R8) idx32 = 3;
        else if (mask & RGBFF_R8G8B8A8) idx32 = 4;
        else if (mask & RGBFF_B8G8R8A8) idx32 = 5;

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

        ImGui::EndTable();
    }
    ImGui::Spacing();

    // --- Settings Checkboxes (2 Columns) ---
    if (ImGui::BeginTable("rtg_settings_table", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Left Column: Scaling options
        // Autoscale modes are mutually exclusive in valid RTG filter:
        // RTG_MODE_SCALE (1), RTG_MODE_CENTER (2), RTG_MODE_INTEGER_SCALE (3)
        int current_autoscale_mode = changed_prefs.gf[1].gfx_filter_autoscale;

        bool scale_smaller = (current_autoscale_mode == RTG_MODE_SCALE);
        if (AmigaCheckbox("Scale if smaller than display size", &scale_smaller)) {
            changed_prefs.gf[1].gfx_filter_autoscale = scale_smaller ? RTG_MODE_SCALE : 0;
        }

        AmigaCheckbox("Always scale in windowed mode", &changed_prefs.rtgallowscaling);

        bool always_center = (current_autoscale_mode == RTG_MODE_CENTER);
        if (AmigaCheckbox("Always center", &always_center)) {
            changed_prefs.gf[1].gfx_filter_autoscale = always_center ? RTG_MODE_CENTER : 0;
        }

        bool int_scale = (current_autoscale_mode == RTG_MODE_INTEGER_SCALE);
        if (AmigaCheckbox("Integer scaling", &int_scale)) {
            changed_prefs.gf[1].gfx_filter_autoscale = int_scale ? RTG_MODE_INTEGER_SCALE : 0;
        }

        AmigaCheckbox("Zero Copy (Buffer sharing)", &changed_prefs.rtg_zerocopy);

        ImGui::TableNextColumn();

        // Right Column: Hardware/Misc Checkboxes
        AmigaCheckbox("Native/RTG autoswitch", &changed_prefs.rtgboards[0].autoswitch);
        AmigaCheckbox("Multithreaded", &changed_prefs.rtg_multithread);

        bool hwsprite_enabled = changed_prefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE;
        ImGui::BeginDisabled(!hwsprite_enabled);
        AmigaCheckbox("Hardware sprite emulation", &changed_prefs.rtg_hardwaresprite);
        ImGui::EndDisabled();

        AmigaCheckbox("Hardware vertical blank interrupt", &changed_prefs.rtg_hardwareinterrupt);

        ImGui::EndTable();
    }
    ImGui::Spacing();

    // --- Bottom Row: Screen Mode Details ---
    ImGui::Text("Screen Mode settings");
    ImGui::Separator();

    if (ImGui::BeginTable("rtg_bottom_table", 3, ImGuiTableFlags_None)) {
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

        ImGui::Text("Refresh rate:");
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
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

        ImGui::TableNextColumn();

        const char *rtg_buffermodes[] = {"Double buffering", "Triple buffering"};
        int current_buffer = (changed_prefs.gfx_apmode[1].gfx_backbuffers > 0)
                                 ? changed_prefs.gfx_apmode[1].gfx_backbuffers - 1
                                 : 0;
        ImGui::Text("Buffer mode:");
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
        if (ImGui::Combo("##BufferModeCombo", &current_buffer, rtg_buffermodes, IM_ARRAYSIZE(rtg_buffermodes))) {
            changed_prefs.gfx_apmode[1].gfx_backbuffers = current_buffer + 1;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        ImGui::TableNextColumn();

        const char *rtg_aspectratios[] = {"Disabled", "Automatic"};
        int current_aspect = (changed_prefs.rtgscaleaspectratio == 0) ? 0 : 1;
        ImGui::Text("Aspect ratio:");
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
        if (ImGui::Combo("##AspectRatioCombo", &current_aspect, rtg_aspectratios, IM_ARRAYSIZE(rtg_aspectratios))) {
            changed_prefs.rtgscaleaspectratio = (current_aspect == 0) ? 0 : -1;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        ImGui::EndTable();
    }
    ImGui::EndDisabled();
}
