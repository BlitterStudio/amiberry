#include "sysdeps.h"
#include "gui/gui_handling.h"
#include "imgui.h"
#include "imgui_panels.h"
#include "newcpu.h"
#include "options.h"
#include "settings/settings_definitions.h"

#include <cstdio>
#include <cstring>

namespace settings = amiberry_settings;

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules and the dependent changes a new value implies.
struct cpu_options {
    const settings::definition& cpu_model = settings::require("cpu_model");
    const settings::definition& address_space_24 = settings::require("cpu_24bit_addressing");
    const settings::definition& cpu_compatible = settings::require("cpu_compatible");
    const settings::definition& cpu_no_unimplemented = settings::require("cpu_no_unimplemented");
    const settings::definition& cpu_data_cache = settings::require("cpu_data_cache");
    const settings::definition& mmu_model = settings::require("mmu_model");
    const settings::definition& fpu_model = settings::require("fpu_model");
    const settings::definition& fpu_strict = settings::require("fpu_strict");
    const settings::definition& fpu_softfloat = settings::require("fpu_softfloat");
    const settings::definition& fpu_no_unimplemented = settings::require("fpu_no_unimplemented");
    const settings::definition& cpu_speed = settings::require("cpu_speed");
    const settings::definition& cpu_throttle = settings::require("cpu_throttle");
    const settings::definition& cpu_idle = settings::require("amiberry.cpu_idle");
    const settings::definition& cpu_multiplier = settings::require("cpu_multiplier");
    const settings::definition& cpu_threaded = settings::require("cpu_threaded");
#ifdef WITH_PPC
    const settings::definition& ppc_model = settings::require("ppc_model");
    const settings::definition& ppc_implementation = settings::require("ppc_implementation");
    const settings::definition& ppc_cpu_idle = settings::require("ppc_cpu_idle");
#endif
    const settings::definition& x86_throttle = settings::require("cpu_x86_throttle");
    const settings::definition& cachesize = settings::require("cachesize");
    const settings::definition& compfpu = settings::require("compfpu");
    const settings::definition& comp_constjump = settings::require("comp_constjump");
    const settings::definition& comp_flushmode = settings::require("comp_flushmode");
    const settings::definition& comp_nf = settings::require("comp_nf");
    const settings::definition& comp_catchfault = settings::require("comp_catchfault");
    const settings::definition& comp_trustbyte = settings::require("comp_trustbyte");
};

const cpu_options& options()
{
    static const cpu_options o;
    return o;
}

float getcpufreq(int m)
{
    const float f = changed_prefs.ntscmode ? 28636360.0f : 28375160.0f;
    return f * static_cast<float>(m >> 8) / 8.0f;
}

bool available(const settings::definition& d, const settings::context& ctx)
{
    return settings::is_available(d, changed_prefs, ctx);
}

bool has_value(const settings::definition& d, const char* v)
{
    char current[AMIBERRY_SETTINGS_VALUE_MAX];
    settings::get(d, changed_prefs, current, sizeof current);
    return std::strcmp(current, v) == 0;
}

// The choice of an option that names the current CPU model, for example
// "68040" or "68ec040".
void cpu_choice(char* out, size_t size, const char* prefix)
{
    std::snprintf(out, size, "%s%d", prefix, changed_prefs.cpu_model % 100);
}

// A checkbox for a BOOL option. inverted shows the opposite of the stored
// value, for options whose key is a negative ("no_...").
void option_checkbox(const char* label, const settings::definition& d,
    const settings::context& ctx, bool inverted = false)
{
    bool checked = settings::get_bool(d, changed_prefs) != inverted;
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked != inverted);
}

} // namespace

void render_panel_cpu() {
    const float slider_width = BUTTON_WIDTH * 1.5f;
    const float slider_label_width = BUTTON_WIDTH * 1.5f; // Fixed aligned width for labels
    const cpu_options& o = options();
    const settings::context ctx{emulating != 0};

    // Global padding for the whole panel
    ImGui::Indent(4.0f);

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

    ImGui::BeginGroup(); // Left Column
    {
        const float left_group_min_width = BUTTON_WIDTH * 2.0f;

        if (BeginGroupBox("CPU", true)) {

        const amiberry_setting_descriptor& cpu_desc = o.cpu_model.descriptor;
        int cpu_model_idx = 0;
        const char* cpu_model_labels[8];
        for (size_t i = 0; i < cpu_desc.choice_count; i++) {
            cpu_model_labels[i] = cpu_desc.choices[i].label;
            if (has_value(o.cpu_model, cpu_desc.choices[i].value))
                cpu_model_idx = static_cast<int>(i);
        }
        // Size the closed combo to the longest descriptive label so it isn't
        // clipped; the dropdown list auto-sizes regardless.
        const float cpu_combo_width = ImGui::CalcTextSize(cpu_desc.choices[0].label).x +
            ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetNextItemWidth(cpu_combo_width);
        if (ImGui::Combo("##CPUModel", &cpu_model_idx, cpu_model_labels, static_cast<int>(cpu_desc.choice_count))) {
            settings::set(o.cpu_model, changed_prefs, ctx, cpu_desc.choices[cpu_model_idx].value);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
        ShowHelpMarker(cpu_desc.help);

        ImGui::Dummy(ImVec2(left_group_min_width, 0.0f));

        ImGui::BeginDisabled(!available(o.address_space_24, ctx));
        option_checkbox("24-bit addressing", o.address_space_24, ctx);
        ShowHelpMarker(o.address_space_24.descriptor.help);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.cpu_compatible, ctx));
        option_checkbox("More compatible", o.cpu_compatible, ctx);
        ShowHelpMarker(o.cpu_compatible.descriptor.help);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.cpu_no_unimplemented, ctx));
        option_checkbox("Unimplemented CPU emu", o.cpu_no_unimplemented, ctx, true);
        ShowHelpMarker("Emulate unimplemented 68060 instructions in software.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.cpu_data_cache, ctx));
        option_checkbox("Data cache emulation", o.cpu_data_cache, ctx);
        ShowHelpMarker(o.cpu_data_cache.descriptor.help);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.cachesize, ctx));
        bool jit_box = changed_prefs.cachesize > 0;
        if (AmigaCheckbox("JIT", &jit_box)) {
            settings::set_number(o.cachesize, changed_prefs, ctx, jit_box ? o.cachesize.descriptor.max : 0);
        }
        ShowHelpMarker("Just-In-Time compilation. Greatly speeds up CPU emulation but reduces compatibility.");
        ImGui::EndDisabled();
        ImGui::Spacing();
        }
        EndGroupBox("CPU");

        if (BeginGroupBox("MMU", true)) {
        char full_mmu[AMIBERRY_SETTINGS_VALUE_MAX];
        char ec_mmu[AMIBERRY_SETTINGS_VALUE_MAX];
        cpu_choice(full_mmu, sizeof full_mmu, "680");
        cpu_choice(ec_mmu, sizeof ec_mmu, "68ec0");
        ImGui::BeginDisabled(!available(o.mmu_model, ctx));
        if (AmigaRadioButton("None##MMU", changed_prefs.mmu_model == 0)) {
            settings::set(o.mmu_model, changed_prefs, ctx, "0");
        }
        ShowHelpMarker("Disable Memory Management Unit emulation.");
        if (AmigaRadioButton("MMU", changed_prefs.mmu_model != 0 && changed_prefs.mmu_model == changed_prefs.cpu_model)) {
            settings::set(o.mmu_model, changed_prefs, ctx, full_mmu);
        }
        ShowHelpMarker("Enable full MMU emulation. Required for some operating systems.");
        ImGui::SameLine();
        // EC only means something with an MMU model selected; a stale flag
        // left by a configuration without one shows as unticked.
        bool mmu_ec = changed_prefs.mmu_model != 0 && changed_prefs.mmu_ec;
        if (AmigaCheckbox("EC", &mmu_ec)) {
            settings::set(o.mmu_model, changed_prefs, ctx, mmu_ec ? ec_mmu : full_mmu);
        }
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(left_group_min_width, 0.0f));
        }
        EndGroupBox("MMU");

        if (BeginGroupBox("FPU", true)) {
        char internal_fpu[AMIBERRY_SETTINGS_VALUE_MAX];
        cpu_choice(internal_fpu, sizeof internal_fpu, "680");
        int current_fpu_sel = 0;
        if (changed_prefs.fpu_model == 68881)
            current_fpu_sel = 1;
        else if (changed_prefs.fpu_model == 68882)
            current_fpu_sel = 2;
        else if (changed_prefs.fpu_model != 0)
            current_fpu_sel = 3;

        if (AmigaRadioButton("None##FPU", &current_fpu_sel, 0))
            settings::set(o.fpu_model, changed_prefs, ctx, "0");
        ShowHelpMarker("Disable Floating Point Unit.");

        ImGui::BeginDisabled(!settings::is_choice_available(o.fpu_model, changed_prefs, ctx, "68881"));
        if (AmigaRadioButton("68881", &current_fpu_sel, 1))
            settings::set(o.fpu_model, changed_prefs, ctx, "68881");
        ShowHelpMarker("External Motorola 68881 FPU. Used with 68020/68030.");
        if (AmigaRadioButton("68882", &current_fpu_sel, 2))
            settings::set(o.fpu_model, changed_prefs, ctx, "68882");
        ShowHelpMarker("External Motorola 68882 FPU. Faster version of 68881.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!settings::is_choice_available(o.fpu_model, changed_prefs, ctx, internal_fpu));
        if (AmigaRadioButton("CPU internal", &current_fpu_sel, 3))
            settings::set(o.fpu_model, changed_prefs, ctx, internal_fpu);
        ShowHelpMarker("Use the FPU built into 68040/68060.");
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.fpu_strict, ctx));
        option_checkbox("More compatible##FPU", o.fpu_strict, ctx);
        ShowHelpMarker(o.fpu_strict.descriptor.help);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.fpu_softfloat, ctx));
#ifdef MSVC_LONG_DOUBLE
        static const settings::definition& fpu_long_double = settings::require("fpu_msvc_long_double");
        const char* fpu_mode_items[] = { "Host (64-bit)", "Host (80-bit)", "Softfloat (80-bit)" };
        const int softfloat_idx = 2;
        int fpu_mode_idx = settings::get_bool(o.fpu_softfloat, changed_prefs) ? softfloat_idx
            : settings::get_bool(fpu_long_double, changed_prefs) ? 1 : 0;
#else
        const char* fpu_mode_items[] = { "Host (64-bit)", "Softfloat (80-bit)" };
        const int softfloat_idx = 1;
        int fpu_mode_idx = settings::get_bool(o.fpu_softfloat, changed_prefs) ? softfloat_idx : 0;
#endif
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::Combo("##FPUMode", &fpu_mode_idx, fpu_mode_items, IM_ARRAYSIZE(fpu_mode_items))) {
            settings::set_bool(o.fpu_softfloat, changed_prefs, ctx, fpu_mode_idx == softfloat_idx);
#ifdef MSVC_LONG_DOUBLE
            if (fpu_mode_idx != softfloat_idx)
                settings::set_bool(fpu_long_double, changed_prefs, ctx, fpu_mode_idx == 1);
#endif
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
        ShowHelpMarker(o.fpu_softfloat.descriptor.help);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.fpu_no_unimplemented, ctx));
        option_checkbox("Unimplemented FPU emu", o.fpu_no_unimplemented, ctx, true);
        ShowHelpMarker("Emulate unimplemented FPU instructions in software.");
        ImGui::EndDisabled();
        ImGui::Dummy(ImVec2(left_group_min_width, 0.0f));
        }
        EndGroupBox("FPU");
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(2.0f, 0.0f)); // Spacing between columns
    ImGui::SameLine();

    ImGui::BeginGroup(); // Right Column
    {
        const float right_group_min_width = BUTTON_WIDTH * 5.0f;

        if (BeginGroupBox("CPU Speed", true)) {
        if (AmigaRadioButton("Fastest Possible", has_value(o.cpu_speed, "max"))) {
            settings::set(o.cpu_speed, changed_prefs, ctx, "max");
        }
        ShowHelpMarker("Run the CPU as fast as your host system allows.");
        if (AmigaRadioButton("Approximate A500/A1200 or cycle-exact", !has_value(o.cpu_speed, "max"))) {
            settings::set(o.cpu_speed, changed_prefs, ctx, "real");
        }
        ShowHelpMarker("Match original Amiga CPU speed. Required for most games.");

        ImGui::Spacing();

        ImGui::BeginDisabled(!available(o.cpu_throttle, ctx));
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Speed");
        ImGui::SameLine(slider_label_width);

        ImGui::SetNextItemWidth(slider_width);
        double throttle_min, throttle_max;
        settings::get_range(o.cpu_throttle, changed_prefs, throttle_min, throttle_max);
        if (ImGui::SliderInt("##CPU Speed", &speed_slider_val, static_cast<int>(throttle_min / 100),
                static_cast<int>(throttle_max / 100), "%+d0%%", ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(o.cpu_throttle, changed_prefs, ctx, speed_slider_val * 100.0);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!available(o.cpu_idle, ctx));
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Idle");
        ImGui::SameLine(slider_label_width);
        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::SliderInt("##CPU Idle", &idle_slider_val, 0, 10, "%d0%%", ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(o.cpu_idle, changed_prefs, ctx,
                idle_slider_val == 0 ? 0 : (12 - idle_slider_val) * 15);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        }
        EndGroupBox("CPU Speed");

        if (BeginGroupBox("Cycle-Exact CPU Emulation Speed", true)) {
        const char *freq_items[] = {"1x", "2x (A500)", "4x (A1200)", "8x", "16x"};
        const char *freq_values[] = {"1", "2", "4", "8", "16"};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Frequency");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);

        ImGui::BeginDisabled(!available(o.cpu_multiplier, ctx));
        if (ImGui::Combo("##CPU Frequency", &freq_combo_idx, freq_items,
                         IM_ARRAYSIZE(freq_items))) {
            settings::set(o.cpu_multiplier, changed_prefs, ctx, freq_values[freq_combo_idx]);
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

        ImGui::BeginDisabled(!available(o.cpu_threaded, ctx));
        option_checkbox("Multi-threaded CPU", o.cpu_threaded, ctx);
        ShowHelpMarker(o.cpu_threaded.descriptor.help);
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        }
        EndGroupBox("Cycle-Exact CPU Emulation Speed");

#ifdef WITH_PPC
        if (BeginGroupBox("PowerPC CPU Options", true, false)) {
        ImGui::BeginDisabled(!available(o.ppc_model, ctx));
        bool ppc_bool = !has_value(o.ppc_model, "");
        if (AmigaCheckbox("PPC CPU emulation", &ppc_bool)) {
            settings::set(o.ppc_model, changed_prefs, ctx, ppc_bool ? "automatic" : "");
        }
        ShowHelpMarker(o.ppc_model.descriptor.help);
        ImGui::EndDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("PPC implementation");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
        const amiberry_setting_descriptor& impl_desc = o.ppc_implementation.descriptor;
        const char* ppc_impl_names[4];
        int ppc_impl = 0;
        for (size_t i = 0; i < impl_desc.choice_count; i++) {
            ppc_impl_names[i] = impl_desc.choices[i].label;
            if (has_value(o.ppc_implementation, impl_desc.choices[i].value))
                ppc_impl = static_cast<int>(i);
        }
        ImGui::BeginDisabled(!available(o.ppc_implementation, ctx));
        if (ImGui::Combo("##PPCImplementation", &ppc_impl, ppc_impl_names, static_cast<int>(impl_desc.choice_count))) {
            settings::set(o.ppc_implementation, changed_prefs, ctx, impl_desc.choices[ppc_impl].value);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();
#ifndef WITH_QEMU_PPC
        ImGui::TextDisabled("QEMU PPC plugin support is not compiled into this build.");
#endif
        ShowHelpMarker(impl_desc.help);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stopped M68K CPU idle mode");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
        ImGui::BeginDisabled(!available(o.ppc_cpu_idle, ctx));
        const amiberry_setting_descriptor& idle_desc = o.ppc_cpu_idle.descriptor;
        int ppc_idle = changed_prefs.ppc_cpu_idle;
        if (ImGui::SliderInt("##PPC Idle", &ppc_idle, 0, static_cast<int>(idle_desc.choice_count) - 1, "%d",
                ImGuiSliderFlags_AlwaysClamp)) {
            settings::set(o.ppc_cpu_idle, changed_prefs, ctx, idle_desc.choices[ppc_idle].value);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();
        ShowHelpMarker(idle_desc.help);

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        }
        EndGroupBox("PowerPC CPU Options");
#endif

        if (BeginGroupBox("x86 Bridgeboard CPU options", true, false)) {
        ImGui::BeginDisabled(!available(o.x86_throttle, ctx));
        ImGui::AlignTextToFramePadding();
        ImGui::Text("CPU Speed");
        ImGui::SameLine(slider_label_width);

        int x86_speed_slider_val = static_cast<int>(changed_prefs.x86_speed_throttle / 100);

        ImGui::SetNextItemWidth(slider_width);
        if (ImGui::SliderInt("##x86Speed", &x86_speed_slider_val, 0, 1000, "%+d0%%", ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(o.x86_throttle, changed_prefs, ctx, x86_speed_slider_val * 100.0);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ShowHelpMarker(o.x86_throttle.descriptor.help);
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        }
        EndGroupBox("x86 Bridgeboard CPU options");

        if (BeginGroupBox("Advanced JIT Settings", true)) {

        ImGui::AlignTextToFramePadding();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Cache size");
        ImGui::SameLine(slider_label_width);
        ImGui::SetNextItemWidth(slider_width);

        // The checkbox in the CPU group turns the JIT on; the slider only
        // sizes a running JIT's cache.
        ImGui::BeginDisabled(!available(o.cachesize, ctx) || changed_prefs.cachesize == 0);
        if (ImGui::SliderInt("##Cache size", &jit_slider_val, 0, 5, "%d", ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(o.cachesize, changed_prefs, ctx,
                jit_slider_val == 0 ? 0 : 1024 << (jit_slider_val - 1)); // 1->1024, 2->2048... 5->16384

            if (changed_prefs.cachesize > 0 && jit_slider_val > 0)
                canbang = true;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("%d MB", changed_prefs.cachesize / 1024);

        ImGui::Spacing();

        // Use table for better organization
        if (ImGui::BeginTable("JITTable", 2, ImGuiTableFlags_None,
            ImVec2(right_group_min_width, 0.0f))) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::BeginDisabled(!available(o.compfpu, ctx));
            option_checkbox("FPU Support##JIT", o.compfpu, ctx);
            ShowHelpMarker(o.compfpu.descriptor.help);
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!available(o.comp_constjump, ctx));
            option_checkbox("Constant jump", o.comp_constjump, ctx);
            ShowHelpMarker(o.comp_constjump.descriptor.help);
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!available(o.comp_flushmode, ctx));
            bool hard_flush = has_value(o.comp_flushmode, "hard");
            if (AmigaCheckbox("Hard flush", &hard_flush)) {
                settings::set(o.comp_flushmode, changed_prefs, ctx, hard_flush ? "hard" : "soft");
            }
            ShowHelpMarker("Flush JIT cache on every resize/reset.");
            ImGui::EndDisabled();

            ImGui::TableNextColumn();

            ImGui::BeginDisabled(!available(o.comp_nf, ctx));
            option_checkbox("No flags", o.comp_nf, ctx);
            ShowHelpMarker(o.comp_nf.descriptor.help);
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!available(o.comp_catchfault, ctx));
            option_checkbox("Catch exceptions", o.comp_catchfault, ctx);
            ShowHelpMarker(o.comp_catchfault.descriptor.help);
            ImGui::EndDisabled();

            ImGui::EndTable();
        }
        ImGui::Spacing();

        ImGui::BeginDisabled(!available(o.comp_trustbyte, ctx));
        ImGui::Text("Memory Access:");
        ImGui::SameLine();
        if (AmigaRadioButton("Direct##memaccess", changed_prefs.comptrustbyte == 0)) {
            settings::set(o.comp_trustbyte, changed_prefs, ctx, "direct");
        }
        ShowHelpMarker("Direct memory access. Faster but less compatible.");
        ImGui::SameLine();
        if (AmigaRadioButton("Indirect##memaccess", changed_prefs.comptrustbyte == 1)) {
            settings::set(o.comp_trustbyte, changed_prefs, ctx, "indirect");
        }
        ShowHelpMarker("Indirect memory access. Slower but more compatible.");
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(right_group_min_width, 0.0f));
        }
        EndGroupBox("Advanced JIT Settings");
    }
    ImGui::EndGroup();

    ImGui::Unindent(5.0f);
}
