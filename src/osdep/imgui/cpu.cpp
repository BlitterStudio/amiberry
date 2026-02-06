#include "sysdeps.h"
#include "gui/gui_handling.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "newcpu.h"
#include "options.h"
#include "cpuboard.h"
#include "memory.h"
#include "x86.h"

#ifndef MAX_JIT_CACHE
#define MAX_JIT_CACHE 16384
#endif

static float getcpufreq(int m) {
    const float f = changed_prefs.ntscmode ? 28636360.0f : 28375160.0f;
    return f * static_cast<float>(m >> 8) / 8.0f;
}

void render_panel_cpu() {
    const float slider_width = BUTTON_WIDTH * 1.5f;
    const float slider_label_width = BUTTON_WIDTH * 1.5f; // Fixed aligned width for labels
    bool settings_changed = false;

    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    // ---------------------------------------------------------
    // Logic / State Calculation (mimicking RefreshPanelCPU)
    // ---------------------------------------------------------

    bool cpu_based_enable =
            changed_prefs.cpu_model >= 68020 && changed_prefs.address_space_24 == 0;
    bool jit_enable = cpu_based_enable && !changed_prefs.mmu_model && !changed_prefs.cpu_thread; // eventually cpu_thread _should_ work with JIT, but it requires some refactoring
#ifndef JIT
    jit_enable = false;
#endif
    bool enable_jit_settings = jit_enable && changed_prefs.cachesize;

    bool enable_opt_direct = enable_jit_settings;
    bool enable_opt_indirect = enable_jit_settings;
    bool enable_chk_hardflush = enable_jit_settings;
    bool enable_chk_constjump = enable_jit_settings;
    bool enable_chk_fpujit = enable_jit_settings && changed_prefs.fpu_model > 0;
    bool enable_chk_catch = enable_jit_settings;
    bool enable_chk_noflags = enable_jit_settings;
    bool enable_jit_cache_slider = enable_jit_settings;
    bool enable_chk_jit = jit_enable;

    bool enable_chk_compatible =
            !changed_prefs.cpu_memory_cycle_exact &&
            !(changed_prefs.cachesize && changed_prefs.cpu_model >= 68040);
    bool enable_chk_fpustrict = changed_prefs.fpu_model > 0;

    bool enable_freq_combo =
            (changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) &&
            changed_prefs.m68k_speed >= 0;

    bool enable_fpu_ext =
            changed_prefs.cpu_model < 68040 &&
            (changed_prefs.cpu_model >= 68020 || !changed_prefs.cpu_compatible);
    bool enable_fpu_int = changed_prefs.cpu_model >= 68040;

    bool enable_mmu =
            changed_prefs.cpu_model >= 68030 && changed_prefs.cachesize == 0;
    bool enable_datacache = changed_prefs.cpu_model >= 68030 &&
                            changed_prefs.cachesize == 0 &&
                            changed_prefs.cpu_compatible;

    bool enable_ppc =
            changed_prefs.cpu_model >= 68040 &&
            (changed_prefs.ppc_mode == 1 ||
             (changed_prefs.ppc_mode == 0 && !is_ppc_cpu(&changed_prefs)));
    bool enable_ppc_idle =
            changed_prefs.ppc_mode != 0; // simplified based on enabled check

    bool enable_cpu_speed_slider = !changed_prefs.cpu_cycle_exact;
    bool enable_24bit =
            changed_prefs.cpu_model <= 68030 && changed_prefs.cachesize == 0;
    bool enable_cpu_idle_slider = changed_prefs.m68k_speed != 0;
    
    bool enable_x86_group = is_x86_cpu(&changed_prefs);

    bool enable_cpu_unimplemented = changed_prefs.cpu_model == 68060 && changed_prefs.cachesize == 0;

    // Calculate JIT Slider Index for display
    int jit_slider_val = 0;
    for (int i = 0; i < 6; i++) {
        // 0 to 5
        if (changed_prefs.cachesize >= (1024 << i) &&
            changed_prefs.cachesize < (1024 << i) * 2) {
            jit_slider_val = i + 1;
            break;
        }
    }
    if (changed_prefs.cachesize == 0)
        jit_slider_val = 0;

    // Calculate Frequency Combo Index
    int freq_combo_idx = 5; // Default: Fast/Max
    if (changed_prefs.cpu_clock_multiplier >= 1 << 8) {
        freq_combo_idx = 0;
        while (freq_combo_idx < 4) {
            if (changed_prefs.cpu_clock_multiplier <= (1 << 8) << freq_combo_idx)
                break;
            freq_combo_idx++;
        }
    } else if (changed_prefs.cpu_clock_multiplier == 0 &&
               changed_prefs.cpu_frequency == 0 &&
               changed_prefs.cpu_model <= 68010) {
        freq_combo_idx = 1; // A500
    } else if (changed_prefs.cpu_clock_multiplier == 0 &&
               changed_prefs.cpu_frequency == 0 &&
               changed_prefs.cpu_model >= 68020) {
        freq_combo_idx = 2; // A1200
    }

    // Calculate CPU Speed slider value
    int speed_slider_val =
            static_cast<int>(changed_prefs.m68k_speed_throttle / 100);

    // CPU Idle Slider
    int idle_slider_val =
            changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15;

    // ---------------------------------------------------------
    // Rendering & Event Handling
    // ---------------------------------------------------------

    ImGui::BeginGroup(); // Left Column
    {
        BeginGroupBox("CPU");

        int old_cpu_model = changed_prefs.cpu_model;
        if (AmigaRadioButton("68000", &changed_prefs.cpu_model, 68000))
            settings_changed = true;
        ShowHelpMarker("Original Amiga 500/1000/2000 CPU.");
        
        if (AmigaRadioButton("68010", &changed_prefs.cpu_model, 68010))
            settings_changed = true;
        ShowHelpMarker("Slightly faster version of 68000, used in some accelerators.");

        if (AmigaRadioButton("68020", &changed_prefs.cpu_model, 68020))
            settings_changed = true;
        ShowHelpMarker("Amiga 1200 CPU. 32-bit.");

        if (AmigaRadioButton("68030", &changed_prefs.cpu_model, 68030))
            settings_changed = true;
        ShowHelpMarker("Amiga 3000/4000 CPU. Includes MMU.");

        if (AmigaRadioButton("68040", &changed_prefs.cpu_model, 68040))
            settings_changed = true;
        ShowHelpMarker("Faster 32-bit CPU with integrated FPU and MMU.");

        if (AmigaRadioButton("68060", &changed_prefs.cpu_model, 68060))
            settings_changed = true;
        ShowHelpMarker("Fastest Amiga CPU.");

        const float left_group_min_width = BUTTON_WIDTH * 2.0f;
        ImGui::Dummy(ImVec2(left_group_min_width, 0.0f));

        if (settings_changed && old_cpu_model != changed_prefs.cpu_model) {
            // CPU Model Change Logic from PanelCPU.cpp
            int newcpu = changed_prefs.cpu_model;
            int newfpu = changed_prefs.fpu_model;

            // Handle FPU logic when switching CPU
            if (newcpu <= 68010)
                newfpu = 0; // Disable FPU for low end

            changed_prefs.mmu_model = 0;
            changed_prefs.mmu_ec = false;
            changed_prefs.cpu_data_cache = false;

            switch (newcpu) {
                case 68000:
                case 68010:
                    // Logic: newfpu == 0 ? 0 : (newfpu == 2 ? 68882 : 68881)
                    // But we just zeroed it above if <= 68010
                    if (changed_prefs.cpu_compatible ||
                        changed_prefs.cpu_memory_cycle_exact)
                        changed_prefs.fpu_model = 0;
                    if (newcpu != old_cpu_model)
                        changed_prefs.address_space_24 = true;
                    break;
                case 68020:
                    // Re-verify FPU. If it was internal (not possible on 020?), reset?
                    // Guisan keeps existing selection if possible.
                    if (changed_prefs.fpu_model == 0)
                        changed_prefs.fpu_model = 0;
                    else if (changed_prefs.fpu_model == 68882)
                        changed_prefs.fpu_model = 68882;
                    else
                        changed_prefs.fpu_model = 68881;
                    break;
                case 68030:
                    if (newcpu != old_cpu_model)
                        changed_prefs.address_space_24 = false;
                    // FPU check
                    if (changed_prefs.fpu_model == 0)
                        changed_prefs.fpu_model = 0;
                    else if (changed_prefs.fpu_model == 68882)
                        changed_prefs.fpu_model = 68882;
                    else
                        changed_prefs.fpu_model = 68881;

                    // MMU Check - defaults
                    if (changed_prefs.mmu_ec || changed_prefs.mmu_model)
                        changed_prefs.mmu_model = 68030;
                    break;
                case 68040:
                case 68060:
                    changed_prefs.address_space_24 = false;
                    if (changed_prefs.fpu_model)
                        changed_prefs.fpu_model = newcpu; // Internal
                    if (changed_prefs.mmu_ec || changed_prefs.mmu_model)
                        changed_prefs.mmu_model = newcpu;
                    break;
            }

            // Frequency Auto-adjust
            if (newcpu != old_cpu_model && changed_prefs.cpu_compatible) {
                if (newcpu <= 68010)
                    changed_prefs.cpu_clock_multiplier = 2 * 256;
                else if (newcpu == 68020)
                    changed_prefs.cpu_clock_multiplier = 4 * 256;
                else
                    changed_prefs.cpu_clock_multiplier = 8 * 256;
            }
            settings_changed = false; // Handled
        }

        ImGui::BeginDisabled(!enable_24bit);
        AmigaCheckbox("24-bit addressing", &changed_prefs.address_space_24);
        ShowHelpMarker("Essential for A500/A1200 compatibility. Disable for Z3 RAM usage.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_compatible);
        if (AmigaCheckbox("More compatible", &changed_prefs.cpu_compatible)) {
            // Side effects
            changed_prefs.cpu_compatible =
                    changed_prefs.cpu_memory_cycle_exact || changed_prefs.cpu_compatible;
            if (changed_prefs.cpu_compatible) // If enabled, might affect FPU
            {
                // Logic from cpuActionListener:
                if (changed_prefs.cpu_model <= 68010)
                    changed_prefs.fpu_model = 0;
            }
        }
        ShowHelpMarker("Approximates prefetch pipeline of 68000/010. Required for some timing sensitive games.");
        ImGui::EndDisabled();

        bool unimplemented_cpu = !changed_prefs.int_no_unimplemented;
        ImGui::BeginDisabled(!enable_cpu_unimplemented);
        if (AmigaCheckbox("Unimplemented CPU emu", &unimplemented_cpu)) {
            changed_prefs.int_no_unimplemented = !unimplemented_cpu;
        }
        ShowHelpMarker("Emulate unimplemented 68060 instructions in software.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_datacache);
        AmigaCheckbox("Data cache emulation", &changed_prefs.cpu_data_cache);
        ShowHelpMarker("Emulate the CPU data cache. Required for some 68030+ software.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_jit);
        bool jit_box = changed_prefs.cachesize > 0;
        if (AmigaCheckbox("JIT", &jit_box)) {
            // Toggle Logic
            if (jit_box) {
                // Enabling JIT
                if (changed_prefs.cachesize == 0)
                    changed_prefs.cachesize = MAX_JIT_CACHE; // Default to Max

                // JIT Enforced settings
                changed_prefs.cpu_cycle_exact = false;
                changed_prefs.blitter_cycle_exact = false;
                changed_prefs.cpu_memory_cycle_exact = false;

                if (changed_prefs.fpu_model > 0) {
                    changed_prefs.compfpu = true;
                }
            } else {
                changed_prefs.cachesize = 0;
                changed_prefs.compfpu = false;
            }
        }
        ShowHelpMarker("Just-In-Time compilation. Greatly speeds up CPU emulation but reduces compatibility.");
        ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("CPU");

        BeginGroupBox("MMU");
        ImGui::BeginDisabled(!enable_mmu);
        if (AmigaRadioButton("None##MMU", &changed_prefs.mmu_model, 0)) {
            changed_prefs.mmu_ec = false;
        }
        ShowHelpMarker("Disable Memory Management Unit emulation.");
        if (AmigaRadioButton("MMU", &changed_prefs.mmu_model,
                               changed_prefs.cpu_model)) {
            changed_prefs.mmu_ec = false;
        }
        ShowHelpMarker("Enable full MMU emulation. Required for some operating systems.");
        ImGui::SameLine();
        if (AmigaCheckbox("EC", &changed_prefs.mmu_ec)) {
            if (changed_prefs.mmu_ec)
                changed_prefs.mmu_model = changed_prefs.cpu_model;
        }
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(left_group_min_width, 0.0f));
        EndGroupBox("MMU");

        BeginGroupBox("FPU");
        int current_fpu_sel = 0;
        if (changed_prefs.fpu_model == 68881)
            current_fpu_sel = 1;
        else if (changed_prefs.fpu_model == 68882)
            current_fpu_sel = 2;
        else if (changed_prefs.fpu_model != 0)
            current_fpu_sel = 3;

        if (AmigaRadioButton("None##FPU", &current_fpu_sel, 0))
            changed_prefs.fpu_model = 0;
        ShowHelpMarker("Disable Floating Point Unit.");

        ImGui::BeginDisabled(!enable_fpu_ext);
        if (AmigaRadioButton("68881", &current_fpu_sel, 1))
            changed_prefs.fpu_model = 68881;
        ShowHelpMarker("External Motorola 68881 FPU. Used with 68020/68030.");
        if (AmigaRadioButton("68882", &current_fpu_sel, 2))
            changed_prefs.fpu_model = 68882;
        ShowHelpMarker("External Motorola 68882 FPU. Faster version of 68881.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_fpu_int);
        if (AmigaRadioButton("CPU internal", &current_fpu_sel, 3))
            changed_prefs.fpu_model = changed_prefs.cpu_model;
        ShowHelpMarker("Use the FPU built into 68040/68060.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_fpustrict);
        AmigaCheckbox("More compatible##FPU", &changed_prefs.fpu_strict);
        ShowHelpMarker("More accurate but slower FPU emulation.");
        
        // FPU Mode Combo
        const char* fpu_mode_items[] = { "Host (64-bit)", "Host (80-bit)", "Softfloat (80-bit)" };
        int fpu_mode_idx = changed_prefs.fpu_mode < 0 ? 1 : (changed_prefs.fpu_mode > 0 ? 2 : 0);
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::Combo("##FPUMode", &fpu_mode_idx, fpu_mode_items, IM_ARRAYSIZE(fpu_mode_items))) {
            if (fpu_mode_idx == 0) changed_prefs.fpu_mode = 0;
            else if (fpu_mode_idx == 1) changed_prefs.fpu_mode = -1;
            else if (fpu_mode_idx == 2) changed_prefs.fpu_mode = 1;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
        ShowHelpMarker("FPU precision mode. Softfloat is most accurate but slowest.");
        ImGui::EndDisabled();

        bool unimplemented_fpu = !changed_prefs.fpu_no_unimplemented;
        bool enable_unimplemented = (changed_prefs.fpu_model > 0) && (changed_prefs.cachesize == 0);
        ImGui::BeginDisabled(!enable_unimplemented);
        if (AmigaCheckbox("Unimplemented FPU emu", &unimplemented_fpu)) {
            changed_prefs.fpu_no_unimplemented = !unimplemented_fpu;
        }
        ShowHelpMarker("Emulate unimplemented FPU instructions in software.");
        ImGui::EndDisabled();
        ImGui::Dummy(ImVec2(left_group_min_width, 0.0f));
        EndGroupBox("FPU");
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(2.0f, 0.0f)); // Spacing between columns
    ImGui::SameLine();

    ImGui::BeginGroup(); // Right Column
    {
        BeginGroupBox("CPU Speed");
        // Logic: WinUAE sets m68k_speed to -1 for "Fastest Possible", 0 for "Cycle Exact/Approx"
        // Also updates throttle.
        int speed_mode = changed_prefs.m68k_speed < 0 ? -1 : 0;
        if (AmigaRadioButton("Fastest Possible", &speed_mode, -1)) {
            changed_prefs.m68k_speed = -1;
            changed_prefs.m68k_speed_throttle = 0;
        }
        ShowHelpMarker("Run the CPU as fast as your host system allows.");
        if (AmigaRadioButton("Approximate A500/A1200 or cycle-exact", &speed_mode, 0)) {
            changed_prefs.m68k_speed = 0;
        }
        ShowHelpMarker("Match original Amiga CPU speed. Required for most games.");

        ImGui::Spacing();

        ImGui::BeginDisabled(!enable_cpu_speed_slider);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Speed");
        ImGui::SameLine(slider_label_width);
        
        ImGui::SetNextItemWidth(slider_width);
        int max_speed_val = (changed_prefs.m68k_speed < 0 || (changed_prefs.cpu_memory_cycle_exact && !changed_prefs.cpu_cycle_exact)) ? 0 : 50;
        if (ImGui::SliderInt("##CPU Speed", &speed_slider_val, -9, max_speed_val, "%+d0%%")) {
             // Update throttle
            changed_prefs.m68k_speed_throttle =
                    static_cast<float>(speed_slider_val * 100);
            if (changed_prefs.m68k_speed_throttle > 0 && changed_prefs.m68k_speed < 0)
                changed_prefs.m68k_speed_throttle = 0;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_cpu_idle_slider);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Idle");
        ImGui::SameLine(slider_label_width);
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::SliderInt("##CPU Idle", &idle_slider_val, 0, 10, "%d0%%")) {
            if (idle_slider_val == 0) {
                changed_prefs.cpu_idle = 0;
            } else {
                changed_prefs.cpu_idle = (12 - idle_slider_val) * 15;
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();

        float right_group_min_width = BUTTON_WIDTH * 5.0f;
        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        EndGroupBox("CPU Speed");

        BeginGroupBox("Cycle-Exact CPU Emulation Speed");
        const char *freq_items[] = {"1x", "2x (A500)", "4x (A1200)", "8x", "16x"};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Frequency");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);

        ImGui::BeginDisabled(!enable_freq_combo);
        if (ImGui::Combo("##CPU Frequency", &freq_combo_idx, freq_items,
                         IM_ARRAYSIZE(freq_items))) {
            // Update Multiplier
            changed_prefs.cpu_frequency = 0;
            changed_prefs.cpu_clock_multiplier = 0;
            if (freq_combo_idx < 5) {
                changed_prefs.cpu_clock_multiplier = (1 << 8) << freq_combo_idx;
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
        ImGui::EndDisabled();

        ImGui::SameLine();
        // Display Frequency MHz text if applicable
        float freq_mhz = 0.0f;
        if (changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) {
            if (changed_prefs.cpu_clock_multiplier)
                freq_mhz = getcpufreq(changed_prefs.cpu_clock_multiplier);
            else if (changed_prefs.cpu_frequency)
                freq_mhz = (float) changed_prefs.cpu_frequency;
        }
        // WinUAE style readout
        char freq_label[32];
        snprintf(freq_label, sizeof(freq_label), "%.6f", freq_mhz / 1000000.0f);
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        ImGui::InputText("##FreqReadout", freq_label, sizeof(freq_label), ImGuiInputTextFlags_ReadOnly);

        const bool no_thread = (changed_prefs.cachesize > 0 || changed_prefs.cpu_compatible || changed_prefs.ppc_mode || changed_prefs.cpu_memory_cycle_exact || changed_prefs.cpu_model < 68020);
        ImGui::BeginDisabled(no_thread || emulating);
        AmigaCheckbox("Multi-threaded CPU", &changed_prefs.cpu_thread);
        ShowHelpMarker("Run CPU emulation on a separate thread. Experimental.");
        ImGui::EndDisabled();
        
        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        EndGroupBox("Cycle-Exact CPU Emulation Speed");

        BeginGroupBox("PowerPC CPU Options");
        ImGui::BeginDisabled(!enable_ppc);
        bool ppc_bool = changed_prefs.ppc_mode != 0;
        if (AmigaCheckbox("PPC CPU emulation", &ppc_bool)) {
            if (ppc_bool) {
                if (changed_prefs.ppc_mode == 0)
                    changed_prefs.ppc_mode = 1;
                // WinUAE: if mode==1 and cpu<68040, mode=0.
                if (changed_prefs.ppc_mode == 1 && changed_prefs.cpu_model < 68040)
                    changed_prefs.ppc_mode = 0;
            } else {
                changed_prefs.ppc_mode = 0;
            }
        }
        ShowHelpMarker("Enable PowerPC CPU emulation for CyberStorm PPC or Blizzard PPC boards.");
        ImGui::EndDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stopped M68K CPU idle mode");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
        ImGui::BeginDisabled(!enable_ppc_idle);
        ImGui::SliderInt("##PPC Idle", &changed_prefs.ppc_cpu_idle, 0, 10);
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        EndGroupBox("PowerPC CPU Options");

        BeginGroupBox("x86 Bridgeboard CPU options");
        ImGui::BeginDisabled(!enable_x86_group);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Speed");
        ImGui::SameLine(slider_label_width);
        
        int x86_speed_slider_val = static_cast<int>(changed_prefs.x86_speed_throttle / 100);
        
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::SliderInt("##x86Speed", &x86_speed_slider_val, 0, 1000, "%+d0%%")) {
             changed_prefs.x86_speed_throttle = static_cast<float>(x86_speed_slider_val * 100);
             if (changed_prefs.x86_speed_throttle > 0 && changed_prefs.m68k_speed < 0)
                changed_prefs.x86_speed_throttle = 0; // Mirroring safety logic? Maybe not needed for x86 but safe.
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        EndGroupBox("x86 Bridgeboard CPU options");

        // Re-organized JIT settings
        BeginGroupBox("Advanced JIT Settings");
        
        ImGui::AlignTextToFramePadding();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Cache size");
        ImGui::SameLine(slider_label_width);
        ImGui::SetNextItemWidth(slider_width);

        ImGui::BeginDisabled(!enable_jit_cache_slider);
        if (ImGui::SliderInt("##Cache size", &jit_slider_val, 0, 5)) {
            // Update cachesize
            if (jit_slider_val == 0)
                changed_prefs.cachesize = 0;
            else
                changed_prefs.cachesize =
                        1024 << (jit_slider_val - 1); // 1->1024, 2->2048... 5->16384

            bool jitex = changed_prefs.cachesize > 0;
            if (!jitex) {
                changed_prefs.compfpu = false;
            } else {
                if (jit_slider_val > 0) canbang = true;
            }

            if (changed_prefs.cachesize && changed_prefs.cpu_model >= 68040) {
                changed_prefs.cpu_compatible = false;
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("%d MB", changed_prefs.cachesize / 1024);

        ImGui::Spacing();
        
        // Use table for better organization
        if (ImGui::BeginTable("JITTable", 2, ImGuiTableFlags_None)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::BeginDisabled(!enable_chk_fpujit);
            AmigaCheckbox("FPU Support##JIT", &changed_prefs.compfpu);
            ShowHelpMarker("Enable JIT compilation for FPU instructions.");
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!enable_chk_constjump);
            AmigaCheckbox("Constant jump", &changed_prefs.comp_constjump);
            ShowHelpMarker("JIT compilation optimization.");
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!enable_chk_hardflush);
            AmigaCheckbox("Hard flush", &changed_prefs.comp_hardflush);
            ShowHelpMarker("Flush JIT cache on every resize/reset.");
            ImGui::EndDisabled();

            ImGui::TableNextColumn();

            ImGui::BeginDisabled(!enable_chk_noflags);
            AmigaCheckbox("No flags", &changed_prefs.compnf);
            ShowHelpMarker("Don't update status flags (faster but less compatible).");
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!enable_chk_catch);
            AmigaCheckbox("Catch exceptions", &changed_prefs.comp_catchfault);
            ShowHelpMarker("Catch memory access faults in compiled code.");
            ImGui::EndDisabled();

            ImGui::EndTable();
        }
        ImGui::Spacing();

        ImGui::BeginDisabled(!enable_opt_direct);
        ImGui::Text("Memory Access:");
        ImGui::SameLine();
        if (AmigaRadioButton("Direct##memaccess", &changed_prefs.comptrustbyte, 0)) {
            changed_prefs.comptrustword = 0;
            changed_prefs.comptrustlong = 0;
            changed_prefs.comptrustnaddr = 0;
        }
        ShowHelpMarker("Direct memory access. Faster but less compatible.");
        ImGui::SameLine();
        if (AmigaRadioButton("Indirect##memaccess", &changed_prefs.comptrustbyte, 1)) {
            changed_prefs.comptrustword = 1;
            changed_prefs.comptrustlong = 1;
            changed_prefs.comptrustnaddr = 1;
        }
        ShowHelpMarker("Indirect memory access. Slower but more compatible.");
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        EndGroupBox("Advanced JIT Settings");
    }
    ImGui::EndGroup();

    ImGui::Unindent(5.0f);
}
