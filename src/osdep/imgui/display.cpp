#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "amiberry_gfx.h"
#include "amiberry_gfx_mode.h"
#include "display_modes.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "play_setup.h"
#include "settings/settings_definitions.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace settings = amiberry_settings;

// The one scaling-method catalog the Display, Play and Global Settings
// pickers render. The Display panel now takes its choices from the
// settings layer; these symbols stay for the panels not migrated yet.
const ComboOption scaling_method_options[] = {
    {-1, "Auto"},
    {0, "Nearest"},
    {1, "Linear"},
    {2, "Integer"},
    {3, "Stretch"},
};
const int scaling_method_option_count = IM_ARRAYSIZE(scaling_method_options);

int scaling_method_to_index(const int scaling_method) {
    for (int i = 0; i < scaling_method_option_count; i++) {
        if (scaling_method_options[i].value == scaling_method)
            return i;
    }
    return 0; // Auto
}

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules and the dependent changes a new value implies; the
// normalization the old panel performed every render frame now lives in
// the layer's get and set functions.
struct display_options {
    const settings::definition& gfx_display = settings::require("gfx_display");
    const settings::definition& gfx_width_windowed = settings::require("gfx_width_windowed");
    const settings::definition& gfx_height_windowed = settings::require("gfx_height_windowed");
    const settings::definition& gfx_resize_windowed = settings::require("gfx_resize_windowed");
    const settings::definition& borderless = settings::require("amiberry.borderless");
    const settings::definition& gfx_fullscreen_amiga = settings::require("gfx_fullscreen_amiga");
    const settings::definition& gfx_vsync = settings::require("gfx_vsync");
    const settings::definition& gfx_vrr_monitor = settings::require("gfx_vrr_monitor");
    const settings::definition& gfx_fullscreen_picasso = settings::require("gfx_fullscreen_picasso");
    const settings::definition& gfx_vsync_picasso = settings::require("gfx_vsync_picasso");
    const settings::definition& gfx_blacker_than_black = settings::require("gfx_blacker_than_black");
    const settings::definition& gfx_flickerfixer = settings::require("gfx_flickerfixer");
    const settings::definition& gfx_monochrome = settings::require("gfx_monochrome");
    const settings::definition& gfx_lores_mode = settings::require("gfx_lores_mode");
    const settings::definition& gfx_autoresolution_vga = settings::require("gfx_autoresolution_vga");
    const settings::definition& gfx_monitorblankdelay = settings::require("gfx_monitorblankdelay");
    const settings::definition& gfx_resolution = settings::require("gfx_resolution");
    const settings::definition& gfx_overscanmode = settings::require("gfx_overscanmode");
    const settings::definition& gfx_autoresolution = settings::require("gfx_autoresolution");
    const settings::definition& ntsc = settings::require("ntsc");
    const settings::definition* displaydata[2] = {
        &settings::require("displaydata_pal"), &settings::require("displaydata_ntsc"),
    };
    const settings::definition& gfx_luminance = settings::require("gfx_luminance");
    const settings::definition& gfx_contrast = settings::require("gfx_contrast");
    const settings::definition& gfx_gamma = settings::require("gfx_gamma");
    const settings::definition* gfx_gamma_ch[3] = {
        &settings::require("gfx_gamma_r"), &settings::require("gfx_gamma_g"),
        &settings::require("gfx_gamma_b"),
    };
    const settings::definition& gfx_atari_palette_fix = settings::require("gfx_atari_palette_fix");
    const settings::definition& gfx_center_horizontal = settings::require("gfx_center_horizontal");
    const settings::definition& gfx_center_vertical = settings::require("gfx_center_vertical");
    const settings::definition& gfx_correct_aspect = settings::require("amiberry.gfx_correct_aspect");
    const settings::definition& gfx_ntscpixelspal = settings::require("gfx_ntscpixelspal");
    const settings::definition& scaling_method = settings::require("amiberry.scaling_method");
    const settings::definition& gfx_linemode = settings::require("gfx_linemode");
};

const display_options& options()
{
    static const display_options o;
    return o;
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

void option_checkbox(const char* label, const settings::definition& d, const settings::context& ctx)
{
    bool checked = settings::get_bool(d, changed_prefs);
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked);
}

// A combo over an option's choice list; unavailable choices are greyed
// out (or left out with hide_unavailable, for values the layer reports but
// never offers here), and a value from a configuration outside the list
// shows as-is. Returns whether a new value was stored.
bool option_combo(const char* id, const settings::definition& d, const settings::context& ctx,
    float width, bool hide_unavailable = false)
{
    char current[AMIBERRY_SETTINGS_VALUE_MAX];
    settings::get(d, changed_prefs, current, sizeof current);
    const settings::choice_list choices = settings::choices_of(d, changed_prefs);
    const char* preview = current;
    int selected = -1;
    for (size_t i = 0; i < choices.count; i++) {
        if (!std::strcmp(current, choices.items[i].value)) {
            selected = static_cast<int>(i);
            preview = choices.items[i].label;
            break;
        }
    }
    bool changed = false;
    if (width > 0.0f)
        ImGui::SetNextItemWidth(width);
    if (ImGui::BeginCombo(id, preview)) {
        for (size_t n = 0; n < choices.count; n++) {
            const bool is_selected = selected == static_cast<int>(n);
            const bool choice_available = settings::is_choice_available(d, changed_prefs, ctx,
                choices.items[n].value);
            if (!choice_available && !is_selected && hide_unavailable)
                continue;
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            ImGui::BeginDisabled(!choice_available);
            ImGui::PushID(static_cast<int>(n));
            if (ImGui::Selectable(choices.items[n].label, is_selected)
                && settings::set(d, changed_prefs, ctx, choices.items[n].value) == AMIBERRY_SETTINGS_OK)
                changed = !is_selected;
            ImGui::PopID();
            ImGui::EndDisabled();
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    return changed;
}

// A typed integer, stored when editing ends (Enter or focus loss) rather
// than on every keystroke: the partial numbers of a value being typed
// ("1", "13" on the way to 1366) are outside the range. The value is
// clamped to the range and rounded up to the option's step, as
// fixup_prefs_dim2() rounds the window size.
void option_input_int(const char* id, const settings::definition& d, const settings::context& ctx)
{
    static ImGuiID editing = 0;
    static int edited_value = 0;
    const ImGuiID item = ImGui::GetID(id);
    int value = editing == item ? edited_value
        : static_cast<int>(settings::get_number(d, changed_prefs));
    ImGui::InputInt(id, &value, 0, 0);
    if (ImGui::IsItemActive()) {
        editing = item;
        edited_value = value;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        double min, max;
        settings::get_range(d, changed_prefs, min, max);
        const int step = d.descriptor.step >= 1 ? static_cast<int>(d.descriptor.step) : 1;
        value = std::max(static_cast<int>(min), std::min(static_cast<int>(max), value));
        value = (value + step - 1) / step * step;
        if (value > max)
            value -= step;
        settings::set_number(d, changed_prefs, ctx, value);
    }
    if (!ImGui::IsItemActive() && editing == item)
        editing = 0;
}

// KMSDRM owns the whole display, Android the device screen.
bool fullscreen_only_display()
{
#ifdef __ANDROID__
    return true;
#else
    return kmsdrm_detected;
#endif
}

// The composite linemode value for a (doubled, progressive-scanline,
// interlace-scanline) tuple, in the writer's spelling.
const char* linemode_value(int doubled, int progressive, int interlaced)
{
    const amiberry_setting_descriptor& desc = options().gfx_linemode.descriptor;
    const int pscan = std::max(0, std::min(3, progressive));
    const int iscan = std::max(0, std::min(2, interlaced));
    const int index = doubled ? iscan * 4 + pscan + 1 : 0;
    return desc.choices[index].value;
}

// A displaydata_pal/_ntsc value is the refresh entry's CSV: the rate, then
// the entry's other fields; "0" while unlocked. The FPS adjustment reads
// the rate and stores a bare rate, which the layer applies to the entry
// without touching its other fields.
double displaydata_rate(const settings::definition& d)
{
    char text[MAX_DPATH] = "0"; // an entry with commands is longer than VALUE_MAX
    settings::get(d, changed_prefs, text, sizeof text);
    return std::strtod(text, nullptr);
}

void set_displaydata_rate(const settings::definition& d, const settings::context& ctx, double rate)
{
    char value[32];
    std::snprintf(value, sizeof value, "%f", rate);
    settings::set(d, changed_prefs, ctx, value);
}

} // namespace

void render_panel_display() {
    ImGui::Indent(4.0f);

    const display_options& o = options();
    const settings::context ctx{emulating != 0};

    const bool kmsdrm = kmsdrm_detected;

    // ---------------------------------------------------------
    // Host Display Selection
    // ---------------------------------------------------------
    if (available(o.gfx_display, ctx)) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Display:");
        ImGui::SameLine(BUTTON_WIDTH);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 5.0f);
        option_combo("##HostDisplay", o.gfx_display, ctx, 0.0f);
        ImGui::Spacing();
    }

    // ---------------------------------------------------------
    // Screen Group (Top)
    // ---------------------------------------------------------
    BeginGroupBox("Screen Settings");

    if (kmsdrm) {
        ImGui::TextDisabled("KMSDRM uses the active console display mode");
        ImGui::SameLine();
        ShowHelpMarker("Host resolution and refresh rate are controlled by the console configuration. Amiberry uses console Full-window mode; blocking OpenGL/GLES with matching console and emulated refresh uses hardware/vblank pacing, otherwise software timing is used.");
        ImGui::Spacing();
    }

    // Windowed & Buffering
    ImGui::BeginDisabled(!available(o.gfx_width_windowed, ctx));
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Windowed:");
    ImGui::SameLine(BUTTON_WIDTH);
    ImGui::SetNextItemWidth(BUTTON_WIDTH / 2);
    option_input_int("##WinW", o.gfx_width_windowed, ctx);
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH / 2);
    option_input_int("##WinH", o.gfx_height_windowed, ctx);
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
    ImGui::SameLine();
    option_checkbox("Window resize", o.gfx_resize_windowed, ctx);
    ShowHelpMarker("Allow resizing the emulator window by dragging the window edges");

    // Borderless (Enable only if Windowed)
    ImGui::SameLine();
    ImGui::BeginDisabled(!available(o.borderless, ctx));
    option_checkbox("Borderless", o.borderless, ctx);
    ImGui::EndDisabled();
    ShowHelpMarker("Remove window decorations (title bar and borders) while in windowed mode");
    ImGui::EndDisabled();

    ImGui::Spacing();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Native:");
    ImGui::SameLine(BUTTON_WIDTH);
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
    ImGui::BeginDisabled(!available(o.gfx_fullscreen_amiga, ctx));
    if (option_combo("##NativeMode", o.gfx_fullscreen_amiga, ctx, 0.0f, true)) {
        char applied[AMIBERRY_SETTINGS_VALUE_MAX];
        settings::get(o.gfx_fullscreen_amiga, changed_prefs, applied, sizeof applied);
        play_sync_screen_mode_cache(std::strcmp(applied, "false") == 0
            ? GFX_WINDOW : GFX_FULLWINDOW);
    }
    ImGui::EndDisabled();
    ShowHelpMarker(fullscreen_only_display()
        ? (kmsdrm
            ? "KMSDRM always uses the active console display in Full-window mode"
            : "Android always uses the device display in Full-window mode")
        : "Native chipset screen mode: Windowed or Full-window (borderless desktop mode)");

    ImGui::SameLine();

    // VSync Mapping: one combo over the vsync value plus Adaptive, which
    // is the VRR monitor option; the layer's setter keeps both in step.
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 5.0f);
    if (kmsdrm) ImGui::BeginDisabled();
    {
        const amiberry_setting_descriptor& desc = o.gfx_vsync.descriptor;
        const bool adaptive = settings::get_bool(o.gfx_vrr_monitor, changed_prefs);
        char current[AMIBERRY_SETTINGS_VALUE_MAX];
        settings::get(o.gfx_vsync, changed_prefs, current, sizeof current);
        const char* preview = "-";
        int vsync_idx = 0;
        for (size_t n = 0; n < desc.choice_count; n++) {
            if (!std::strcmp(current, desc.choices[n].value)) {
                vsync_idx = static_cast<int>(n);
                preview = desc.choices[n].label;
            }
        }
        if (adaptive)
            preview = "Adaptive";
        if (ImGui::BeginCombo("##NativeVSync", preview)) {
            for (int n = 0; n < 4; n++) {
                const char* label = n == 3 ? "Adaptive" : desc.choices[n].label;
                const bool is_selected = (n == 3 && adaptive)
                    || (n < 3 && !adaptive && vsync_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(label, is_selected)) {
                    if (n == 3) {
                        settings::set_bool(o.gfx_vrr_monitor, changed_prefs, ctx, true);
                    } else {
                        settings::set_bool(o.gfx_vrr_monitor, changed_prefs, ctx, false);
                        settings::set(o.gfx_vsync, changed_prefs, ctx, desc.choices[n].value);
                    }
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    if (kmsdrm) ImGui::EndDisabled();
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ShowHelpMarker(kmsdrm
        ? "Legacy KMSDRM uses software timing with drained presentation; blocking presentation is used for pacing only when console and emulated refresh match. VSync controls, refresh switching, and Adaptive/VRR modes are not available"
        : "VSync mode: Standard waits for vertical blank, 50/60Hz enforces refresh rate, Adaptive uses VRR (FreeSync/G-SYNC) with late-swap tearing fallback");

    ImGui::Spacing();

    // The RTG screen mode is fixed on full-window-only displays (Android,
    // KMSDRM) while RTG vsync stays selectable there; each control follows
    // its own option's availability.
    ImGui::AlignTextToFramePadding();
    ImGui::Text("RTG:");
    ImGui::SameLine(BUTTON_WIDTH);
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
    ImGui::BeginDisabled(!available(o.gfx_fullscreen_picasso, ctx));
    option_combo("##RTGMode", o.gfx_fullscreen_picasso, ctx, 0.0f, true);
    ImGui::EndDisabled();
    ShowHelpMarker(fullscreen_only_display()
        ? (kmsdrm
            ? "KMSDRM always uses the active console display in Full-window mode"
            : "Android always uses the device display in Full-window mode")
        : "RTG (Picasso96) screen mode when using graphics card emulation");

    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 5.0f);
    ImGui::BeginDisabled(kmsdrm || !available(o.gfx_vsync_picasso, ctx));
    option_combo("##RTGVSync", o.gfx_vsync_picasso, ctx, 0.0f, true);
    ImGui::EndDisabled();
    ShowHelpMarker(kmsdrm
        ? "KMSDRM presentation timing depends on the SDL driver path; RTG remains software-paced"
        : "VSync mode for RTG display");

    ImGui::Spacing();

    // Checkboxes
    if (ImGui::BeginTable("ChkTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        option_checkbox("Blacker than black", o.gfx_blacker_than_black, ctx);
        ShowHelpMarker("Enable proper black levels for displays that support it");
        option_checkbox("Remove interlace artifacts", o.gfx_flickerfixer, ctx);
        ShowHelpMarker("Apply scanline doubling to reduce flickering from interlaced video modes");
        option_checkbox("Monochrome video out", o.gfx_monochrome, ctx);
        ShowHelpMarker("Render display in grayscale, simulating a monochrome monitor");
        ImGui::TableNextColumn();
        {
            bool filtered = has_value(o.gfx_lores_mode, "filtered");
            if (AmigaCheckbox("Filtered low resolution", &filtered))
                settings::set(o.gfx_lores_mode, changed_prefs, ctx,
                    filtered ? "filtered" : "normal");
        }
        ShowHelpMarker("Apply filtering to low resolution modes for smoother appearance");

        ImGui::BeginDisabled(!available(o.gfx_autoresolution_vga, ctx));
        option_checkbox("VGA mode res. autoswitch", o.gfx_autoresolution_vga, ctx);
        ShowHelpMarker("Automatically switch to VGA resolution when VGA modes are detected (requires HighRes and Double line mode)");
        ImGui::EndDisabled();

        {
            bool resync_blank = settings::get_number(o.gfx_monitorblankdelay, changed_prefs) > 0;
            if (AmigaCheckbox("Display resync blanking", &resync_blank)) {
                settings::set_number(o.gfx_monitorblankdelay, changed_prefs, ctx,
                    resync_blank ? 1000 : 0);
            }
        }
        ShowHelpMarker("Blank the screen briefly when display mode changes to prevent visual artifacts");
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Resolution row
    if (ImGui::BeginTable("ResTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("col1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("col2", ImGuiTableColumnFlags_WidthStretch, 1.0f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Amiga resolution:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 5.0f);
        ImGui::BeginDisabled(!available(o.gfx_resolution, ctx));
        option_combo("##Resolution", o.gfx_resolution, ctx, 0.0f);
        ImGui::EndDisabled();
        ShowHelpMarker("Sets the internal rendering pipeline's pixel mapping relative to the Amiga's video output.\n"
            "LoRes \xE2\x80\x94 every rendered pixel equals 1 Amiga lores pixel (fastest; hires content is rendered at half resolution).\n"
            "HiRes \xE2\x80\x94 lores pixels are rendered as 2 hires pixels, hires pixels map 1:1 (full quality; recommended).\n"
            "SuperHiRes \xE2\x80\x94 rarely needed; used mainly for doubled-scanned AGA modes.\n"
            "This does not change the host display mode. Disabled when resolution autoswitch is enabled.");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Overscan:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 5.0f);
        option_combo("##Overscan", o.gfx_overscanmode, ctx, 0.0f);
        ShowHelpMarker("Amount of border area to display. Higher values show more of the display area including normally hidden borders");
        ImGui::EndTable();
    }

    // Autoswitch
    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Resolution autoswitch:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
    option_combo("##ResAutoswitch", o.gfx_autoresolution, ctx, 0.0f);
    ShowHelpMarker("Automatically match the internal render resolution to the Amiga's current screen mode each frame.\n"
        "Always On \xE2\x80\x94 immediately follow whichever resolution appears in the frame.\n"
        "Percentage thresholds (10%/33%/66%) \xE2\x80\x94 only switch when the dominant resolution covers at least that share of the frame; lower values switch more readily.\n"
        "Caveats: mode switches take 1 frame (brief visual glitch), AGA can have \xE2\x80\x9Chires\xE2\x80\x9D pixels even in lores mode,\n"
        "screenshots capture whichever resolution was active, and shader effects may behave differently between resolutions.");

    ImGui::Spacing();

    // Video standard + FPS adj
    // WinUAE's "Refresh" (frameskip) slider is not shown: since the WinUAE 6 core, gfx_framerate
    // no longer skips any drawing or presentation, so the control had no visible effect.
    if (ImGui::BeginTable("RefreshRowTable", 4, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 0.7f);
        ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        // Own column for the lock checkbox: placed after the value field, it fell outside the
        // fixed-width column and was clipped away.
        ImGui::TableSetupColumn("lock", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Standard:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        {
            bool ntsc = settings::get_bool(o.ntsc, changed_prefs);
            const char *video_standards[] = {"PAL", "NTSC"};
            if (ImGui::BeginCombo("##VideoStandard", video_standards[ntsc ? 1 : 0])) {
                for (int n = 0; n < 2; n++) {
                    const bool is_selected = ntsc == (n == 1);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(video_standards[n], is_selected))
                        settings::set_bool(o.ntsc, changed_prefs, ctx, n == 1);
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Video standard: PAL (50Hz, European) or NTSC (60Hz, American/Japanese)");

        // The FPS adjustment edits the selected standard's entry; 0 means
        // the entry follows the standard rate and is unlocked.
        const settings::definition& displaydata = *o.displaydata[changed_prefs.ntscmode ? 1 : 0];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("FPS adj.:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        const double entry_rate = displaydata_rate(displaydata);
        const bool fps_adj_enabled = entry_rate > 0;
        ImGui::BeginDisabled(!fps_adj_enabled);
        {
            int rate_pos = std::max(1, std::min(99, static_cast<int>(entry_rate + 0.5)));
            if (ImGui::SliderInt("##FPSAdjSld", &rate_pos, 1, 99, "", ImGuiSliderFlags_AlwaysClamp))
                set_displaydata_rate(displaydata, ctx, rate_pos);
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        }
        ImGui::EndDisabled();
        ShowHelpMarker("Emulated refresh rate in Hz. Only used when the checkbox is enabled");
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        ImGui::BeginDisabled(!fps_adj_enabled);
        {
            float rate = static_cast<float>(entry_rate);
            if (ImGui::InputFloat("##FPSVal", &rate, 0.0f, 0.0f, "%.6f")) {
                if (rate < 1.0f || rate > 300.0f) {
                    // WinUAE resets out-of-range rates to the standard.
                    rate = changed_prefs.ntscmode ? 60.0f : 50.0f;
                }
                set_displaydata_rate(displaydata, ctx, rate);
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        ImGui::EndDisabled();
        ImGui::TableNextColumn();
        {
            bool locked = fps_adj_enabled;
            if (AmigaCheckbox("##FPSLocked", &locked)) {
                // Locking keeps the standard rate until it is edited;
                // unlocking follows the standard again.
                set_displaydata_rate(displaydata, ctx,
                    locked ? (changed_prefs.ntscmode ? 60 : 50) : 0);
            }
        }
        ShowHelpMarker("Use the FPS adj. rate instead of the standard PAL (50 Hz) or NTSC (60 Hz) rate");
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Display Attributes (Brightness, Contrast, Gamma, etc.)
    static int current_attr_idx = 0;
    const char *attr_items[] = {
        "Brightness", "Contrast", "Gamma", "Gamma [R]", "Gamma [G]", "Gamma [B]", "Dark palette fix"
    };
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
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

    {
        // The six color attributes work in tenths (slider -200..200,
        // input clamped to the same range); the palette fix is a choice.
        if (current_attr_idx <= 5) {
            const settings::definition* attr_d = current_attr_idx == 0 ? &o.gfx_luminance
                : current_attr_idx == 1 ? &o.gfx_contrast
                : current_attr_idx == 2 ? &o.gfx_gamma
                : o.gfx_gamma_ch[current_attr_idx - 3];
            ImGui::SameLine();
            const int stored = static_cast<int>(settings::get_number(*attr_d, changed_prefs));
            int ui_val = stored / 10;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - BUTTON_WIDTH
                - ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::SliderInt("##AttrSlider", &ui_val, -200, 200, "",
                    ImGuiSliderFlags_AlwaysClamp))
                settings::set_number(*attr_d, changed_prefs, ctx, ui_val * 10);
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
            ImGui::SameLine();
            // WinUAE displays value / 10; the input is clamped to the
            // slider's range instead of accepting any typed value.
            double disp_val = static_cast<double>(stored) / 10.0;
            ImGui::SetNextItemWidth(BUTTON_WIDTH);
            if (ImGui::InputDouble("##AttrVal", &disp_val, 0, 0, "%.1f")) {
                disp_val = std::max(-200.0, std::min(200.0, disp_val));
                settings::set_number(*attr_d, changed_prefs, ctx, disp_val * 10);
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        } else {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - BUTTON_WIDTH
                - ImGui::GetStyle().ItemSpacing.x);
            option_combo("##AttrFix", o.gfx_atari_palette_fix, ctx, 0.0f);
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        }
    }
    ImGui::Spacing();
    EndGroupBox("Screen Settings");

    ImGui::Spacing();

    // ---------------------------------------------------------
    // Right Column
    // ---------------------------------------------------------
    if (ImGui::BeginTable("BottomGrid", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthStretch, 1.12f);
        ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthStretch, 0.88f);
        ImGui::TableSetupColumn("column3", ImGuiTableColumnFlags_WidthStretch, 1.06f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        BeginGroupBox("Centering");
        ImGui::BeginDisabled(!available(o.gfx_center_horizontal, ctx));
        {
            bool h_center = has_value(o.gfx_center_horizontal, "smart");
            if (AmigaCheckbox("Horizontal", &h_center))
                settings::set(o.gfx_center_horizontal, changed_prefs, ctx,
                    h_center ? "smart" : "none");
        }
        ShowHelpMarker("Automatically center the display horizontally on screen");
        {
            bool v_center = has_value(o.gfx_center_vertical, "smart");
            if (AmigaCheckbox("Vertical", &v_center))
                settings::set(o.gfx_center_vertical, changed_prefs, ctx,
                    v_center ? "smart" : "none");
        }
        ShowHelpMarker("Automatically center the display vertically on screen");
        ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("Centering");

        BeginGroupBox("Aspect and scaling");
        ImGui::BeginDisabled(!available(o.gfx_correct_aspect, ctx));
        {
            bool correct_aspect = settings::get_number(o.gfx_correct_aspect, changed_prefs) != 0;
            if (AmigaCheckbox("Correct aspect ratio", &correct_aspect))
                settings::set_number(o.gfx_correct_aspect, changed_prefs, ctx,
                    correct_aspect ? 1 : 0);
        }
        ImGui::EndDisabled();
        ShowHelpMarker(changed_prefs.scaling_method == 3
            ? "Not available with the Stretch scaling method, which always fills the window."
            : "Apply Amiga display aspect correction to native display modes.");
        option_checkbox("Always stretch PAL as NTSC", o.gfx_ntscpixelspal, ctx);
        ShowHelpMarker("Show PAL modes with NTSC vertical proportions. The bottom part of the PAL display is cut.\nExperimental: vertical positioning may need adjustment.");
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Scaling method:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        option_combo("##ScalingMethod", o.scaling_method, ctx, 0.0f);
        ShowHelpMarker("How the display is scaled to fit the window.\nAuto: uses pixel-perfect integer scaling when it fills the same area as best fit, otherwise smooth scaling.\nNearest: sharp pixels, Linear: smooth, Integer: always pixel-perfect.\nStretch: fills the whole window, ignoring the aspect ratio.");
        ImGui::Spacing();
        EndGroupBox("Aspect and scaling");

        ImGui::TableNextColumn();

        // Line Mode Group
        constexpr float line_mode_group_indent = 16.0f;
        ImGui::Indent(line_mode_group_indent);
        BeginGroupBox("Line Mode");
        ImGui::BeginDisabled(!available(o.gfx_linemode, ctx));
        {
            // The five progressive-line radios; the interlaced group
            // below switches the other half of the composite value.
            static const char* const line_labels[] = {
                "Single", "Double", "Scanlines", "Double, fields", "Double, fields+"
            };
            static const char* const line_help[] = {
                "Single scan: display one line per scanline (200 lines for NTSC, 256 for PAL)",
                "Line doubling: display each line twice for better visibility (400/512 lines)",
                "Simulate scanlines with black lines between display lines for CRT effect",
                "Interlaced field rendering: alternates between even and odd lines",
                "Enhanced interlaced field rendering with improved blending",
            };
            const int pscan = changed_prefs.gfx_pscanlines >= 0
                && changed_prefs.gfx_pscanlines <= 3 ? changed_prefs.gfx_pscanlines : 0;
            const int iscan = changed_prefs.gfx_iscanlines >= 0
                && changed_prefs.gfx_iscanlines <= 2 ? changed_prefs.gfx_iscanlines : 0;
            const int selected = changed_prefs.gfx_vresolution != 0 ? pscan + 1 : 0;
            for (int n = 0; n < 5; n++) {
                if (AmigaRadioButton(line_labels[n], selected == n)) {
                    settings::set(o.gfx_linemode, changed_prefs, ctx,
                        linemode_value(n > 0, n > 0 ? n - 1 : 0, iscan));
                }
                ShowHelpMarker(line_help[n]);
            }
        }
        ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("Line Mode");
        ImGui::Unindent(line_mode_group_indent);

        ImGui::TableNextColumn();

        // Interlaced Line Mode Group
        BeginGroupBox("Interlaced line mode");
        ImGui::BeginDisabled(!available(o.gfx_linemode, ctx));
        {
            const bool is_double = changed_prefs.gfx_vresolution != 0;
            static const char* const interlaced_labels[] = {
                "Single##I", "Double, frames##I", "Double, fields##I", "Double, fields+##I"
            };
            static const char* const interlaced_help[] = {
                "Single line mode for interlaced screens (available only in single line mode)",
                "Double frames mode for interlaced screens (available only in double line mode)",
                "Interlaced field rendering: alternates between even and odd lines",
                "Enhanced interlaced field rendering with improved blending",
            };
            const int pscan = changed_prefs.gfx_pscanlines >= 0
                && changed_prefs.gfx_pscanlines <= 3 ? changed_prefs.gfx_pscanlines : 0;
            const int iscan = changed_prefs.gfx_iscanlines >= 0
                && changed_prefs.gfx_iscanlines <= 2 ? changed_prefs.gfx_iscanlines : 0;
            const int selected = is_double ? iscan + 1 : 0;
            // Radio 0 is the single-line mode; radios 1-3 are the three
            // double-line interlace modes (iscanlines 0-2).
            for (int n = 0; n < 4; n++) {
                ImGui::BeginDisabled(n == 0 ? is_double : !is_double);
                if (AmigaRadioButton(interlaced_labels[n], selected == n)) {
                    settings::set(o.gfx_linemode, changed_prefs, ctx,
                        linemode_value(is_double, pscan, n == 0 ? 0 : n - 1));
                }
                ImGui::EndDisabled();
                ShowHelpMarker(interlaced_help[n]);
            }
        }
        ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("Interlaced line mode");

        ImGui::EndTable();
    }
}
