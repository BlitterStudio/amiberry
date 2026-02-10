#include "imgui.h"
#include "sysdeps.h"
#include "custom.h"
#include "options.h"
#include "imgui_panels.h"
#include "rommgr.h"
#include "specialmonitors.h"

#include <vector>
#include <string>

#include "gui/gui_handling.h"

static bool initialized = false;
static std::vector<std::string> keyboard_items_strs;
static std::vector<const char *> keyboard_items_ptr;
static std::vector<std::string> special_monitor_strs;
static std::vector<const char *> special_monitor_ptr;

static void appendkbmcurom(std::string &s, bool hasrom) {
    if (!hasrom) {
        s += " [ROM not found]";
    }
}

static void init_lists() {
    keyboard_items_strs.clear();
    keyboard_items_strs.emplace_back("Keyboard disconnected");
    keyboard_items_strs.emplace_back("UAE High level emulation");

    int ids1[] = {321, -1};
    int ids2[] = {322, -1};
    int ids3[] = {323, -1};
    struct romlist *has65001 = nullptr;
    struct romlist *has657036 = getromlistbyids(ids1, nullptr);
    struct romlist *has6805 = getromlistbyids(ids2, nullptr);
    struct romlist *has8039 = getromlistbyids(ids3, nullptr);

    std::string tmp = "A500 / A500 + (6500 - 1 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    tmp = "A600 (6570 - 036 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    tmp = "A1000 (6500 - 1 MCU. ROM not yet dumped)";
    appendkbmcurom(tmp, has65001);
    keyboard_items_strs.push_back(tmp);

    tmp = "A1000 (6570 - 036 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    tmp = "A1200 (68HC05C MCU)";
    appendkbmcurom(tmp, has6805);
    keyboard_items_strs.push_back(tmp);

    tmp = "A2000 (Cherry, 8039 MCU)";
    appendkbmcurom(tmp, has8039);
    keyboard_items_strs.push_back(tmp);

    tmp = "A2000/A3000/A4000 (6570-036 MCU)";
    appendkbmcurom(tmp, has657036);
    keyboard_items_strs.push_back(tmp);

    keyboard_items_ptr.clear();
    for (const auto &s: keyboard_items_strs)
        keyboard_items_ptr.push_back(s.c_str());

    special_monitor_strs.clear();
    special_monitor_strs.emplace_back("-");
    special_monitor_strs.emplace_back("Autodetect");

    for (int i = 0; specialmonitorfriendlynames[i]; ++i)
        special_monitor_strs.emplace_back(specialmonitorfriendlynames[i]);

    special_monitor_ptr.clear();
    for (const auto &s: special_monitor_strs)
        special_monitor_ptr.push_back(s.c_str());
}


void render_panel_chipset() {
    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    if (!initialized) init_lists();

    // Determine Chipset Selection Index (Logic from WinUAE values_to_chipsetdlg)
    int chipset_selection = 0; // Default to OCS
    switch (changed_prefs.chipset_mask) {
        case CSMASK_A1000_NOEHB:
            chipset_selection = 6;
            break;
        case CSMASK_A1000:
            chipset_selection = 5;
            break;
        case CSMASK_OCS:
            chipset_selection = 0;
            break;
        case CSMASK_ECS_AGNUS:
            chipset_selection = 1;
            break;
        case CSMASK_ECS_DENISE:
            chipset_selection = 2;
            break;
        case CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE:
            chipset_selection = 3;
            break;
        case CSMASK_AGA:
        case CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA:
            chipset_selection = 4;
            break;
    }

    if (ImGui::BeginTable("chipset_table", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.56f);
        ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.44f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // LEFT COLUMN
        BeginGroupBox("Chipset");
        {
            // Chipset radios
            ImGui::BeginGroup();
            if (AmigaRadioButton("A1000 (No EHB)", &chipset_selection, 6))
                changed_prefs.chipset_mask = CSMASK_A1000_NOEHB;
            ShowHelpMarker("Original A1000 chipset without Extra Half-Brite mode.");
            if (AmigaRadioButton("OCS + OCS Denise", &chipset_selection, 0)) changed_prefs.chipset_mask = CSMASK_OCS;
            ShowHelpMarker("Original Chip Set with original Denise. Standard A500/A2000.");
            if (AmigaRadioButton("ECS + OCS Denise", &chipset_selection, 1))
                changed_prefs.chipset_mask = CSMASK_ECS_AGNUS;
            ShowHelpMarker("Enhanced Chip Set Agnus with original Denise.");
            if (AmigaRadioButton("AGA", &chipset_selection, 4))
                changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
            ShowHelpMarker("Advanced Graphics Architecture. A1200/A4000 chipset.");
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            if (AmigaRadioButton("A1000", &chipset_selection, 5)) changed_prefs.chipset_mask = CSMASK_A1000;
            ShowHelpMarker("Original A1000 chipset with EHB support.");
            if (AmigaRadioButton("OCS + ECS Denise", &chipset_selection, 2))
                changed_prefs.chipset_mask = CSMASK_ECS_DENISE;
            ShowHelpMarker("Original Agnus with Enhanced Denise chip.");
            if (AmigaRadioButton("Full ECS", &chipset_selection, 3))
                changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
            ShowHelpMarker("Full Enhanced Chip Set. A600/A3000.");

            AmigaCheckbox("NTSC", &changed_prefs.ntscmode);
            ShowHelpMarker("Enable NTSC mode (60Hz)");
            ImGui::EndGroup();

            ImGui::Spacing();

            bool cycle_exact = changed_prefs.cpu_cycle_exact;
            if (AmigaCheckbox("Cycle Exact (Full)", &cycle_exact)) {
                changed_prefs.cpu_cycle_exact = cycle_exact;
                if (cycle_exact) {
                    changed_prefs.cpu_memory_cycle_exact = true;
                    changed_prefs.blitter_cycle_exact = true;
                } else {
                    if (changed_prefs.cpu_model < 68020) {
                        changed_prefs.cpu_memory_cycle_exact = false;
                        changed_prefs.blitter_cycle_exact = false;
                    }
                }
            }
            ShowHelpMarker("Bit-perfect emulation. Requires fast CPU.");

            bool memory_exact = changed_prefs.cpu_memory_cycle_exact;
            // Disabled if CPU < 68020 (WinUAE logic)
            ImGui::BeginDisabled(changed_prefs.cpu_model < 68020);
            if (AmigaCheckbox("Cycle Exact (DMA/Memory)", &memory_exact)) {
                changed_prefs.cpu_memory_cycle_exact = memory_exact;
                changed_prefs.blitter_cycle_exact = memory_exact;
                if (memory_exact) {
                    // WinUAE logic: If Memory Exact enabled and CPU < 68020, force Full Exact
                    if (changed_prefs.cpu_model < 68020) {
                        changed_prefs.cpu_cycle_exact = true;
                    }

                    if (changed_prefs.cpu_model <= 68030) {
                        changed_prefs.m68k_speed = 0;
                        changed_prefs.cpu_compatible = 1;
                    }
                    changed_prefs.immediate_blits = false;
                    changed_prefs.cachesize = 0;
                    changed_prefs.gfx_framerate = 1;
                } else {
                    // WinUAE logic: If Memory Exact disabled, force Full Exact disabled
                    changed_prefs.cpu_cycle_exact = false;
                }
            }
            ShowHelpMarker("Cycle-exact DMA and memory access emulation.");
            ImGui::EndDisabled();
            ImGui::Spacing();
        }
        EndGroupBox("Chipset");

        BeginGroupBox("Keyboard");
        {
            ImGui::SetNextItemWidth(BUTTON_WIDTH * 3.0f);
            int kb_mode = changed_prefs.keyboard_mode + 1;
            if (ImGui::Combo("##Keyboard Layout", &kb_mode, keyboard_items_ptr.data(), keyboard_items_ptr.size())) {
                changed_prefs.keyboard_mode = kb_mode - 1;
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
            ShowHelpMarker("Select keyboard controller emulation mode.");

            ImGui::Spacing();
            bool disable_nkro = (changed_prefs.keyboard_mode == KB_UAE || changed_prefs.keyboard_mode == KB_A2000_8039);
            if (disable_nkro) changed_prefs.keyboard_nkro = true;

            ImGui::BeginDisabled(disable_nkro);
            AmigaCheckbox("Keyboard N-key rollover", &changed_prefs.keyboard_nkro);
            ShowHelpMarker("Allow multiple simultaneous key presses.");
            ImGui::Spacing();
            ImGui::EndDisabled();
        }
        EndGroupBox("Keyboard");

        ImGui::TableNextColumn();

        // RIGHT COLUMN
        BeginGroupBox("Options");
        {
            ImGui::BeginDisabled(changed_prefs.cpu_cycle_exact);
            if (AmigaCheckbox("Immediate Blitter", &changed_prefs.immediate_blits)) {
                if (changed_prefs.immediate_blits) changed_prefs.waiting_blits = false;
            }
            ShowHelpMarker("Blitter operations complete instantly. Faster but less accurate.");
            ImGui::EndDisabled();

            bool disable_wait_blits = (changed_prefs.immediate_blits || (
                                           changed_prefs.cpu_memory_cycle_exact && changed_prefs.cpu_model <= 68010));
            if (disable_wait_blits) changed_prefs.waiting_blits = 0;
            ImGui::BeginDisabled(disable_wait_blits);
            bool wait_blits = changed_prefs.waiting_blits;
            if (AmigaCheckbox("Wait for Blitter", &wait_blits)) {
                changed_prefs.waiting_blits = wait_blits ? 1 : 0;
                if (changed_prefs.waiting_blits) changed_prefs.immediate_blits = false;
            }
            ShowHelpMarker("CPU waits for Blitter to finish. More compatible than Immediate Blitter.");
            ImGui::EndDisabled();

            bool mt = multithread_enabled != 0;
            if (AmigaCheckbox("Multithreaded Denise", &mt)) {
                multithread_enabled = mt ? 1 : 0;
            }
            ShowHelpMarker("Run Denise graphics chip emulation on a separate thread.");

            ImGui::Spacing();
            const char *optimization_items[] = {"Full", "Partial", "None"};
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Optimizations");
            ShowHelpMarker("Keep at 'Full' for best compatibility.");

            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            ImGui::Combo("##Optimizations", &changed_prefs.cs_optimizations, optimization_items,
                         IM_ARRAYSIZE(optimization_items));
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            const char *chipset_items[] = {
                "Custom", "Generic", "CDTV", "CDTV-CR", "CD32", "A500", "A500+", "A600", "A1000", "A1200", "A2000",
                "A3000", "A3000T", "A4000", "A4000T", "Velvet", "Casablanca", "DraCo"
            };

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Chipset Extra");
            ShowHelpMarker("Select specialized chipset behavior.");

            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);

            // Map cs_compatible value to index
            int cs_compatible_idx = changed_prefs.cs_compatible;
            if (cs_compatible_idx < 0 || cs_compatible_idx >= IM_ARRAYSIZE(chipset_items)) {
                cs_compatible_idx = 0; // Default to Custom
            }

            if (ImGui::BeginCombo("##Chipset Extra",
                                  (cs_compatible_idx >= 0 && cs_compatible_idx < IM_ARRAYSIZE(chipset_items))
                                      ? chipset_items[cs_compatible_idx]
                                      : "Unknown")) {
                for (int n = 0; n < IM_ARRAYSIZE(chipset_items); n++) {
                    const bool is_selected = (cs_compatible_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(chipset_items[n], is_selected)) {
                        changed_prefs.cs_compatible = n;
                        built_in_chipset_prefs(&changed_prefs);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Monitor sync source:");

            const char *sync_items[] = {"Combined", "CSync", "H/VSync"};
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            if (ImGui::BeginCombo("##SyncSource", sync_items[changed_prefs.cs_hvcsync])) {
                for (int n = 0; n < IM_ARRAYSIZE(sync_items); n++) {
                    const bool is_selected = (changed_prefs.cs_hvcsync == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(sync_items[n], is_selected)) {
                        changed_prefs.cs_hvcsync = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Video port display hardware");
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            if (ImGui::BeginCombo("##VideoPort", special_monitor_ptr[changed_prefs.monitoremu])) {
                for (int n = 0; n < static_cast<int>(special_monitor_ptr.size()); n++) {
                    const bool is_selected = (changed_prefs.monitoremu == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(special_monitor_ptr[n], is_selected)) {
                        changed_prefs.monitoremu = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
            ImGui::Spacing();
        }
        EndGroupBox("Options");

        ImGui::EndTable();
    }

    ImGui::Indent(4.0f);

    BeginGroupBox("Collision Level");
    {
        ImGui::BeginGroup();
        AmigaRadioButton("None##Collision", &changed_prefs.collision_level, 0);
        ShowHelpMarker("Disable all collision detection. Fastest.");
        ImGui::Spacing();
        AmigaRadioButton("Sprites only", &changed_prefs.collision_level, 1);
        ShowHelpMarker("Detect sprite-to-sprite collisions only.");
        ImGui::Spacing();
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(BUTTON_WIDTH / 2, 0));
        ImGui::SameLine();

        ImGui::BeginGroup();
        AmigaRadioButton("Sprites and Sprites vs. Playfield", &changed_prefs.collision_level, 2);
        ShowHelpMarker("Detect sprite-to-sprite and sprite-to-playfield collisions.");
        ImGui::Spacing();
        AmigaRadioButton("Full (rarely needed)", &changed_prefs.collision_level, 3);
        ShowHelpMarker("Full collision detection including playfield-to-playfield. Slowest.");
        ImGui::Spacing();
        ImGui::EndGroup();
    }
    EndGroupBox("Collision Level");

    // Genlock section
    BeginGroupBox("Genlock");
    {
        AmigaCheckbox("Genlock connected", &changed_prefs.genlock);
        ShowHelpMarker("Connect a genlock device for video overlay mixing.");
        ImGui::SameLine();

        bool genlock_enable = changed_prefs.genlock || changed_prefs.genlock_effects;
        ImGui::BeginDisabled(!genlock_enable);

        const char *genlock_items[] = {
            "-",
            "Noise (built-in)",
            "Test card (built-in)",
            "Image file (png)",
            "Video file",
            "Capture device",
            "American Laser Games/Picmatic LaserDisc Player",
            "Sony LaserDisc Player",
            "Pioneer LaserDisc Player"
        };
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 3.5f);
        ImGui::Combo("##Genlock", &changed_prefs.genlock_image, genlock_items, IM_ARRAYSIZE(genlock_items));
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
        ImGui::SameLine();

        const char *genlock_mix_items[] = {"100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%"};
        int genlock_mix_index = changed_prefs.genlock_mix / 25;
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::Combo("##GenlockMix", &genlock_mix_index, genlock_mix_items, IM_ARRAYSIZE(genlock_mix_items))) {
            changed_prefs.genlock_mix = genlock_mix_index * 25;
            if (changed_prefs.genlock_mix > 255) changed_prefs.genlock_mix = 255;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

        AmigaCheckbox("Include alpha channels in screenshots and video captures.", &changed_prefs.genlock_alpha);
        bool genlock_aspect = changed_prefs.genlock_aspect > 0;
        if (AmigaCheckbox("Keep aspect ratio", &genlock_aspect)) {
            changed_prefs.genlock_aspect = genlock_aspect ? 1 : 0;
        }

        if (changed_prefs.genlock_image == 3 || changed_prefs.genlock_image == 4 || changed_prefs.genlock_image >= 6)
        // Image, Video or LaserDisc
        {
            // Show File selector
            // Index 3 is Image, 4 and >= 6 are Video
            char *filename_ptr = (changed_prefs.genlock_image == 3)
                                     ? changed_prefs.genlock_image_file
                                     : changed_prefs.genlock_video_file;
            AmigaInputText("Genlock File", filename_ptr, MAX_DPATH);
            ImGui::SameLine();
            if (AmigaButton("...##GenlockFile")) {
                const char *filter = (changed_prefs.genlock_image == 3) ? ".png,.jpg,.jpeg,.bmp" : ".avi,.mp4,.mkv";
                std::string title = (changed_prefs.genlock_image == 3)
                                        ? "Select Genlock Image"
                                        : "Select Genlock Video";
                OpenFileDialogKey("CHIPSET_FILE", title.c_str(), filter, filename_ptr);
            }

            std::string filePath;
            if (ConsumeFileDialogResultKey("CHIPSET_FILE", filePath)) {
                strncpy(filename_ptr, filePath.c_str(), MAX_DPATH);
            }
        }
        ImGui::EndDisabled();
        ImGui::Spacing();
    }
    EndGroupBox("Genlock");
}
