#include "sysdeps.h"
#include "config.h"
#include "custom.h"
#include "gui/gui_handling.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "newcpu.h"
#include "options.h"
#include "cpuboard.h"
#include "memory.h"

#ifndef MAX_JIT_CACHE
#define MAX_JIT_CACHE 16384
#endif

static float getcpufreq(int m) {
    const float f = changed_prefs.ntscmode ? 28636360.0f : 28375160.0f;
    return f * static_cast<float>(m >> 8) / 8.0f;
}

void render_panel_cpu() {
    const float slider_width = 150.0f;
    bool settings_changed = false;

    // Global padding for the whole panel
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Indent(5.0f);

    // ---------------------------------------------------------
    // Logic / State Calculation (mimicking RefreshPanelCPU)
    // ---------------------------------------------------------

    bool cpu_based_enable =
            changed_prefs.cpu_model >= 68020 && changed_prefs.address_space_24 == 0;
    bool jit_enable = cpu_based_enable && !changed_prefs.mmu_model;
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
        if (ImGui::RadioButton("68000", &changed_prefs.cpu_model, 68000))
            settings_changed = true;
        if (ImGui::RadioButton("68010", &changed_prefs.cpu_model, 68010))
            settings_changed = true;
        if (ImGui::RadioButton("68020", &changed_prefs.cpu_model, 68020))
            settings_changed = true;
        if (ImGui::RadioButton("68030", &changed_prefs.cpu_model, 68030))
            settings_changed = true;
        if (ImGui::RadioButton("68040", &changed_prefs.cpu_model, 68040))
            settings_changed = true;
        if (ImGui::RadioButton("68060", &changed_prefs.cpu_model, 68060))
            settings_changed = true;

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
        ImGui::Checkbox("24-bit addressing", &changed_prefs.address_space_24);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_compatible);
        if (ImGui::Checkbox("More compatible", &changed_prefs.cpu_compatible)) {
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
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_datacache);
        ImGui::Checkbox("Data cache", &changed_prefs.cpu_data_cache);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_jit);
        bool jit_box = changed_prefs.cachesize > 0;
        if (ImGui::Checkbox("JIT", &jit_box)) {
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
        ImGui::EndDisabled();
        EndGroupBox("CPU");

        BeginGroupBox("MMU");
        ImGui::BeginDisabled(!enable_mmu);
        if (ImGui::RadioButton("None##MMU", &changed_prefs.mmu_model, 0)) {
            changed_prefs.mmu_ec = false;
        }
        if (ImGui::RadioButton("MMU", &changed_prefs.mmu_model,
                               changed_prefs.cpu_model)) {
            changed_prefs.mmu_ec = false;
        }
        if (ImGui::Checkbox("EC", &changed_prefs.mmu_ec)) {
            if (changed_prefs.mmu_ec)
                changed_prefs.mmu_model = changed_prefs.cpu_model;
        }
        ImGui::EndDisabled();
        EndGroupBox("MMU");


        BeginGroupBox("FPU");
        int current_fpu_sel = 0;
        if (changed_prefs.fpu_model == 68881)
            current_fpu_sel = 1;
        else if (changed_prefs.fpu_model == 68882)
            current_fpu_sel = 2;
        else if (changed_prefs.fpu_model != 0)
            current_fpu_sel = 3;

        if (ImGui::RadioButton("None##FPU", &current_fpu_sel, 0))
            changed_prefs.fpu_model = 0;

        ImGui::BeginDisabled(!enable_fpu_ext);
        if (ImGui::RadioButton("68881", &current_fpu_sel, 1))
            changed_prefs.fpu_model = 68881;
        if (ImGui::RadioButton("68882", &current_fpu_sel, 2))
            changed_prefs.fpu_model = 68882;
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_fpu_int);
        if (ImGui::RadioButton("CPU internal", &current_fpu_sel, 3))
            changed_prefs.fpu_model = changed_prefs.cpu_model;
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_fpustrict);
        ImGui::Checkbox("More compatible##FPU", &changed_prefs.fpu_strict);
        
        // FPU Mode Combo
        const char* fpu_mode_items[] = { "Host (64-bit)", "Host (80-bit)", "Softfloat (80-bit)" };
        int fpu_mode_idx = changed_prefs.fpu_mode < 0 ? 1 : (changed_prefs.fpu_mode > 0 ? 2 : 0);
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::Combo("##FPUMode", &fpu_mode_idx, fpu_mode_items, IM_ARRAYSIZE(fpu_mode_items))) {
            if (fpu_mode_idx == 0) changed_prefs.fpu_mode = 0;
            else if (fpu_mode_idx == 1) changed_prefs.fpu_mode = -1;
            else if (fpu_mode_idx == 2) changed_prefs.fpu_mode = 1;
        }
        ImGui::EndDisabled();

        bool unimplemented_fpu = !changed_prefs.fpu_no_unimplemented;
        bool enable_unimplemented = (changed_prefs.fpu_model > 0) && (changed_prefs.cachesize == 0);
        ImGui::BeginDisabled(!enable_unimplemented);
        if (ImGui::Checkbox("Unimplemented FPU emu", &unimplemented_fpu)) {
            changed_prefs.fpu_no_unimplemented = !unimplemented_fpu;
        }
        ImGui::EndDisabled();
        EndGroupBox("FPU");
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10.0f, 0.0f)); // Spacing between columns
    ImGui::SameLine();

    ImGui::BeginGroup(); // Right Column
    {
        BeginGroupBox("CPU Speed");
        // Logic: WinUAE sets m68k_speed to -1 for "Fastest Possible", 0 for "Cycle Exact/Approx"
        // Also updates throttle.
        int speed_mode = changed_prefs.m68k_speed < 0 ? -1 : 0;
        if (ImGui::RadioButton("Fastest Possible", &speed_mode, -1)) {
            changed_prefs.m68k_speed = -1;
            changed_prefs.m68k_speed_throttle = 0;
        }
        if (ImGui::RadioButton("A500/A1200 or cycle-exact", &speed_mode, 0)) {
            changed_prefs.m68k_speed = 0;
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Speed");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);

        ImGui::BeginDisabled(!enable_cpu_speed_slider);
        if (ImGui::SliderInt("##CPU Speed", &speed_slider_val, -9, 50, "%d")) {
            // Update throttle
            changed_prefs.m68k_speed_throttle =
                    static_cast<float>(speed_slider_val * 100);
            if (changed_prefs.m68k_speed_throttle > 0 && changed_prefs.m68k_speed < 0)
                changed_prefs.m68k_speed_throttle = 0;
        }
        ImGui::EndDisabled();
        // Show percentage info
        ImGui::SameLine();
        ImGui::Text("%d%%",
                    static_cast<int>(changed_prefs.m68k_speed_throttle / 10));

        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Idle ");
        ImGui::SameLine();
        ImGui::BeginDisabled(!enable_cpu_idle_slider);
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::SliderInt("##CPU Idle", &idle_slider_val, 0, 10)) {
            if (idle_slider_val == 0)
                changed_prefs.cpu_idle = 0;
            else
                changed_prefs.cpu_idle = (12 - idle_slider_val) * 15;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("%d%%", (idle_slider_val * 10));
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
        ImGui::EndDisabled();
        ImGui::SameLine();
        // Display Frequency MHz text if applicable
        if (changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) {
            float freq_mhz = 0.0f;
            if (changed_prefs.cpu_clock_multiplier)
                freq_mhz = getcpufreq(changed_prefs.cpu_clock_multiplier);
            else if (changed_prefs.cpu_frequency)
                freq_mhz = (float) changed_prefs.cpu_frequency;

            ImGui::Text("%.6f MHz", freq_mhz / 1000000.0f);
        }

        ImGui::BeginDisabled(true); // Always disabled in Guisan currently
        // ("Disabled until fixed upstream")
        ImGui::Checkbox("Multi-threaded CPU", &changed_prefs.cpu_thread);
        ImGui::EndDisabled();
        EndGroupBox("Cycle-Exact CPU Emulation Speed");

        BeginGroupBox("PowerPC CPU Options");
        ImGui::BeginDisabled(!enable_ppc);
        bool ppc_bool = changed_prefs.ppc_mode != 0;
        if (ImGui::Checkbox("PPC emulation", &ppc_bool)) {
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
        ImGui::EndDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stopped M68K CPU idle mode");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
        ImGui::BeginDisabled(!enable_ppc_idle);
        ImGui::SliderInt("##PPC Idle", &changed_prefs.ppc_cpu_idle, 0, 10);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (changed_prefs.ppc_cpu_idle == 0)
            ImGui::Text("disabled");
        else if (changed_prefs.ppc_cpu_idle == 10)
            ImGui::Text("max");
        else
            ImGui::Text("%d", changed_prefs.ppc_cpu_idle);

        EndGroupBox("PowerPC CPU Options");

        BeginGroupBox("Advanced JIT Settings");

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Cache size");
        ImGui::SameLine();
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
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("%d MB", changed_prefs.cachesize / 1024);

        ImGui::BeginDisabled(!enable_chk_fpujit);
        ImGui::Checkbox("FPU Support##JIT", &changed_prefs.compfpu);
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!enable_chk_constjump);
        ImGui::Checkbox("Constant jump", &changed_prefs.comp_constjump);
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!enable_chk_hardflush);
        ImGui::Checkbox("Hard flush", &changed_prefs.comp_hardflush);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_opt_direct);
        if (ImGui::RadioButton("Direct##memaccess", &changed_prefs.comptrustbyte,
                               0)) {
            changed_prefs.comptrustword = 0;
            changed_prefs.comptrustlong = 0;
            changed_prefs.comptrustnaddr = 0;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Indirect##memaccess", &changed_prefs.comptrustbyte,
                               1)) {
            changed_prefs.comptrustword = 1;
            changed_prefs.comptrustlong = 1;
            changed_prefs.comptrustnaddr = 1;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!enable_chk_noflags);
        ImGui::Checkbox("No flags", &changed_prefs.compnf);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!enable_chk_catch);
        ImGui::Checkbox("Catch unexpected exceptions",
                        &changed_prefs.comp_catchfault);
        ImGui::EndDisabled();
        EndGroupBox("Advanced JIT Settings");
    }
    ImGui::EndGroup();

    ImGui::Unindent(5.0f);
}
