#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "imgui_panels.h"
#include "sounddep/sound.h"
#include "settings/settings_definitions.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gui/gui_handling.h"

namespace settings = amiberry_settings;

static std::vector<std::string> sound_device_names;

// The device names the Global Settings panel lists, in the layer's device
// order (the indices the configuration stores). They follow the layer's
// list, which the Sound panel refreshes after a device change, and are
// only rebuilt when it changed.
const std::vector<std::string>& get_sound_device_names()
{
    static const settings::definition& soundcard = settings::require("amiberry.soundcard");
    const settings::choice_list devices = settings::choices_of(soundcard, changed_prefs);
    if (devices.count == 0) {
        if (sound_device_names.size() != 1 || sound_device_names[0] != "None")
            sound_device_names.assign(1, "None");
        return sound_device_names;
    }
    bool same = devices.count == sound_device_names.size();
    for (size_t i = 0; same && i < devices.count; i++)
        same = sound_device_names[i] == devices.items[i].label;
    if (!same) {
        sound_device_names.clear();
        for (size_t i = 0; i < devices.count; i++)
            sound_device_names.emplace_back(devices.items[i].label);
    }
    return sound_device_names;
}

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules, their device lists and the dependent changes a new
// value implies.
struct sound_options {
    const settings::definition& soundcard = settings::require("amiberry.soundcard");
    const settings::definition& soundcard_default = settings::require("amiberry.soundcard_default");
    const settings::definition& sound_output = settings::require("sound_output");
    const settings::definition& sound_auto = settings::require("sound_auto");
    const settings::definition& sound_volume = settings::require("sound_volume");
    const settings::definition& sound_volume_paula = settings::require("sound_volume_paula");
    const settings::definition& sound_volume_cd = settings::require("sound_volume_cd");
    const settings::definition& sound_volume_ahi = settings::require("sound_volume_ahi");
    const settings::definition& sound_volume_midi = settings::require("sound_volume_midi");
    const settings::definition& sound_channels = settings::require("sound_channels");
    const settings::definition& sound_stereo_separation = settings::require("sound_stereo_separation");
    const settings::definition& sound_interpol = settings::require("sound_interpol");
    const settings::definition& sound_frequency = settings::require("sound_frequency");
    const settings::definition& sound_stereo_swap_paula = settings::require("sound_stereo_swap_paula");
    const settings::definition& sound_stereo_swap_ahi = settings::require("sound_stereo_swap_ahi");
    const settings::definition& sound_stereo_mixing_delay = settings::require("sound_stereo_mixing_delay");
    const settings::definition& sound_filter = settings::require("sound_filter");
    const settings::definition& sound_filter_type = settings::require("sound_filter_type");
    const settings::definition* floppy_sound[4] = {
        &settings::require("floppy0sound"), &settings::require("floppy1sound"),
        &settings::require("floppy2sound"), &settings::require("floppy3sound"),
    };
    const settings::definition* floppy_volume_disk[4] = {
        &settings::require("floppy0soundvolume_disk"), &settings::require("floppy1soundvolume_disk"),
        &settings::require("floppy2soundvolume_disk"), &settings::require("floppy3soundvolume_disk"),
    };
    const settings::definition* floppy_volume_empty[4] = {
        &settings::require("floppy0soundvolume_empty"), &settings::require("floppy1soundvolume_empty"),
        &settings::require("floppy2soundvolume_empty"), &settings::require("floppy3soundvolume_empty"),
    };
    const settings::definition& sound_max_buff = settings::require("sound_max_buff");
};

const sound_options& options()
{
    static const sound_options o;
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

// A combo over an option's choice list. The preview shows the current
// value even when it is not one of the choices (a frequency or a device
// from a configuration), instead of snapping to a nearby label.
void option_combo(const char* id, const settings::definition& d, const settings::context& ctx,
    bool use_label_for_preview = true)
{
    char current[AMIBERRY_SETTINGS_VALUE_MAX];
    settings::get(d, changed_prefs, current, sizeof current);
    const settings::choice_list choices = settings::choices_of(d, changed_prefs);
    const char* preview = current;
    int selected = -1;
    for (size_t i = 0; i < choices.count; i++) {
        if (!std::strcmp(current, choices.items[i].value)) {
            selected = static_cast<int>(i);
            preview = use_label_for_preview ? choices.items[i].label : current;
            break;
        }
    }
    if (ImGui::BeginCombo(id, preview)) {
        for (size_t n = 0; n < choices.count; n++) {
            const bool is_selected = selected == static_cast<int>(n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            ImGui::PushID(static_cast<int>(n));
            if (ImGui::Selectable(choices.items[n].label, is_selected))
                settings::set(d, changed_prefs, ctx, choices.items[n].value);
            ImGui::PopID();
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
}

} // namespace

static int selected_floppy_drive = 0;
static int selected_volume_type = 0; // 0=Paula, 1=CD, 2=AHI, 3=MIDI

void render_panel_sound() {
    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    const sound_options& o = options();
    const settings::context ctx{emulating != 0};

    // Pick up audio devices connected after start-up whenever the GUI
    // window opens again; the old panel enumerated once per process.
    if (ImGui::IsWindowAppearing())
        settings::refresh_dynamic_choices();
    // Build the device list before the availability checks read it.
    settings::choices_of(o.soundcard, changed_prefs);

    // ---------------------------------------------------------
    // Audio Device
    // ---------------------------------------------------------
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Audio Device:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2);

    ImGui::BeginDisabled(!available(o.soundcard, ctx));
    option_combo("##AudioDevice", o.soundcard, ctx);
    ImGui::EndDisabled();
    ShowHelpMarker("Select which audio device to use for sound output");

    option_checkbox("System default", o.soundcard_default, ctx);
    ShowHelpMarker("Use the operating system's default audio device");

    ImGui::Spacing();

    const bool sound_en = available(o.sound_auto, ctx);
    const bool stereo_en = available(o.sound_stereo_separation, ctx);

    if (ImGui::BeginTable("TopRowTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        BeginGroupBox("Sound Emulation");

        // WinUAE's four modes, including sample-exact, which the old
        // three-radio group could never re-select once it was left.
        const amiberry_setting_descriptor& output_desc = o.sound_output.descriptor;
        static const char* const output_help[] = {
            "No sound processing or output",
            "Paula sound chip is emulated for timing accuracy but no audio output",
            "Full sound emulation with audio output",
            "Full sound emulation with sample-exact timing; most accurate but slower"
        };
        for (size_t n = 0; n < output_desc.choice_count; n++) {
            const bool is_off = !std::strcmp(output_desc.choices[n].value, "none");
            if (is_off)
                ImGui::BeginDisabled(!settings::is_choice_available(o.sound_output, changed_prefs,
                    ctx, output_desc.choices[n].value));
            if (AmigaRadioButton(output_desc.choices[n].label,
                has_value(o.sound_output, output_desc.choices[n].value)))
                settings::set(o.sound_output, changed_prefs, ctx, output_desc.choices[n].value);
            if (is_off)
                ImGui::EndDisabled();
            ShowHelpMarker(output_help[n]);
        }

        ImGui::Spacing();

        ImGui::BeginDisabled(!sound_en);
        option_checkbox("Automatic switching", o.sound_auto, ctx);
        ShowHelpMarker("Automatically adjust sound emulation quality based on performance");
        ImGui::EndDisabled();
        ImGui::Spacing();
        EndGroupBox("Sound Emulation");

        ImGui::TableNextColumn();

        // ---------------------------------------------------------
        // Volume (WinUAE Style)
        // ---------------------------------------------------------
        BeginGroupBox("Volume");
        // The volumes are stored as attenuation (0 is full), so the panel
        // shows 100 - value. The master volume is persisted and applied
        // by the audio path like every other volume; the old slider drove
        // a relative host-mixer step that also toggled mute at 100%.
        ImGui::BeginDisabled(!sound_en);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Master");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - BUTTON_WIDTH * 0.5f);
        int master_display = 100 - static_cast<int>(settings::get_number(o.sound_volume,
            changed_prefs));
        if (ImGui::SliderInt("##MasterVol", &master_display, 0, 100, "",
            ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(o.sound_volume, changed_prefs, ctx, 100 - master_display);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::Text("%d%%", master_display);
        ShowHelpMarker("Master volume control for all audio output");

        ImGui::Spacing();

        // Row 2: Type Dropdown + Slider
        const char *vol_types[] = {"Paula", "CD", "AHI", "MIDI"};
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        if (ImGui::BeginCombo("##VolType", vol_types[selected_volume_type])) {
            for (int n = 0; n < IM_ARRAYSIZE(vol_types); n++) {
                const bool is_selected = (selected_volume_type == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(vol_types[n], is_selected)) {
                    selected_volume_type = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ImGui::SameLine();

        const settings::definition* current_vol = &o.sound_volume_paula;
        if (selected_volume_type == 1) current_vol = &o.sound_volume_cd;
        else if (selected_volume_type == 2) current_vol = &o.sound_volume_ahi;
        else if (selected_volume_type == 3) current_vol = &o.sound_volume_midi;

        int display_vol = 100 - static_cast<int>(settings::get_number(*current_vol, changed_prefs));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - BUTTON_WIDTH * 0.5f);
        if (ImGui::SliderInt("##SubVol", &display_vol, 0, 100, "", ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(*current_vol, changed_prefs, ctx, 100 - display_vol);
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::Text("%d%%", display_vol);
        ShowHelpMarker("Individual volume controls - Paula: Amiga sound chip, CD: CD audio, AHI: retargetable audio, MIDI: external MIDI");
        ImGui::EndDisabled(); // sound_en
        ImGui::Spacing();
        EndGroupBox("Volume");

        ImGui::EndTable();
    }

    BeginGroupBox("Settings");
    ImGui::BeginDisabled(!sound_en);

    if (ImGui::BeginTable("SettingsRow1Table", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Channel mode:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        option_combo("##ChanMode", o.sound_channels, ctx);
        ShowHelpMarker("Audio channel mode: Mono=single channel, Stereo=2ch, 4/5.1/7.1=multi-channel");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stereo separation:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);

        ImGui::BeginDisabled(!stereo_en);
        // The combo lists 100%..0% while the stored value runs 0..10.
        {
            const int separation = static_cast<int>(settings::get_number(
                o.sound_stereo_separation, changed_prefs));
            int sep_idx = 10 - separation;
            if (sep_idx < 0)
                sep_idx = 0;
            char label[16];
            snprintf(label, sizeof label, "%d%%", sep_idx == 10 ? 0 : (10 - sep_idx) * 10);
            if (ImGui::BeginCombo("##Sep", label)) {
                for (int n = 0; n <= 10; n++) {
                    const bool is_selected = (sep_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    char item[16];
                    snprintf(item, sizeof item, "%d%%", (10 - n) * 10);
                    if (ImGui::Selectable(item, is_selected))
                        settings::set_number(o.sound_stereo_separation, changed_prefs, ctx, 10 - n);
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        }
        ImGui::EndDisabled();
        ShowHelpMarker("Controls the amount of stereo separation between left and right channels (100%=full separation, 0%=mono)");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Interpolation:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        option_combo("##Interp", o.sound_interpol, ctx);
        ShowHelpMarker("Audio quality filter: Disabled=raw output, Anti=anti-aliasing, Sinc=high quality resampling");

        ImGui::EndTable();
    }
    ImGui::Spacing();

    if (ImGui::BeginTable("SettingsRow2Table", 4, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c4", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Frequency:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        // An off-list frequency from a configuration shows as its own
        // value instead of a label that no longer matches it.
        option_combo("##Freq", o.sound_frequency, ctx, false);
        ShowHelpMarker("Audio output sample rate in Hz (44100 or 48000 recommended for modern systems)");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Swap channels:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        {
            const char *swap_channels_items[] = {"-", "Paula only", "AHI only", "Both"};
            const bool swap_paula = settings::get_bool(o.sound_stereo_swap_paula, changed_prefs);
            const bool swap_ahi = settings::get_bool(o.sound_stereo_swap_ahi, changed_prefs);
            int swap_idx = swap_paula && swap_ahi ? 3 : swap_ahi ? 2 : swap_paula ? 1 : 0;
            if (ImGui::BeginCombo("##Swap", swap_channels_items[swap_idx])) {
                for (int n = 0; n < IM_ARRAYSIZE(swap_channels_items); n++) {
                    const bool is_selected = (swap_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(swap_channels_items[n], is_selected)) {
                        settings::set_bool(o.sound_stereo_swap_paula, changed_prefs, ctx,
                            n == 1 || n == 3);
                        settings::set_bool(o.sound_stereo_swap_ahi, changed_prefs, ctx,
                            n == 2 || n == 3);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        }
        ShowHelpMarker("Swap left and right audio channels for Paula (Amiga sound chip) or AHI (retargetable audio)");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stereo delay:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);

        ImGui::BeginDisabled(!stereo_en);
        {
            const char *stereo_delay_items[] = {"-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
            const int delay = static_cast<int>(settings::get_number(o.sound_stereo_mixing_delay,
                changed_prefs));
            int delay_idx = delay > 0 ? delay : 0;
            if (delay_idx > 10)
                delay_idx = 10;
            if (ImGui::BeginCombo("##Delay", stereo_delay_items[delay_idx])) {
                for (int n = 0; n < IM_ARRAYSIZE(stereo_delay_items); n++) {
                    const bool is_selected = (delay_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(stereo_delay_items[n], is_selected)) {
                        // "-" is no delay (0): the writer saves -1 as 0
                        // anyway, and the audio treats both alike.
                        settings::set_number(o.sound_stereo_mixing_delay, changed_prefs, ctx, n);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        }
        ImGui::EndDisabled();
        ShowHelpMarker("Adds a small delay between stereo channels for a wider soundstage effect");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Audio filter:");
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        {
            // One combo over the filter and its type: the six WinUAE
            // combinations of Emulated/Always on with the A500/A1200/
            // Fixed-only curves.
            const char *filter_items[] = {
                "Always off", "Emulated (A500)", "Emulated (A1200)", "Always on (A500)",
                "Always on (A1200)", "Always on (Fixed only)"
            };
            char filter[AMIBERRY_SETTINGS_VALUE_MAX];
            char type[AMIBERRY_SETTINGS_VALUE_MAX];
            settings::get(o.sound_filter, changed_prefs, filter, sizeof filter);
            settings::get(o.sound_filter_type, changed_prefs, type, sizeof type);
            int filter_idx;
            if (!std::strcmp(filter, "off"))
                filter_idx = 0;
            else if (!std::strcmp(filter, "emulated"))
                filter_idx = !std::strcmp(type, "enhanced") ? 2 : 1;
            else if (!std::strcmp(type, "fixedonly"))
                filter_idx = 5;
            else
                filter_idx = !std::strcmp(type, "enhanced") ? 4 : 3;
            if (ImGui::BeginCombo("##Filter", filter_items[filter_idx])) {
                for (int n = 0; n < IM_ARRAYSIZE(filter_items); n++) {
                    const bool is_selected = (filter_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(filter_items[n], is_selected)) {
                        static const char *const filters[] = {
                            "off", "emulated", "emulated", "on", "on", "on"
                        };
                        static const char *const types[] = {
                            nullptr, "standard", "enhanced", "standard", "enhanced", "fixedonly"
                        };
                        settings::set(o.sound_filter, changed_prefs, ctx, filters[n]);
                        if (types[n])
                            settings::set(o.sound_filter_type, changed_prefs, ctx, types[n]);
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        }
        ShowHelpMarker("Emulates the Amiga's hardware low-pass audio filter (A500=sharper, A1200=softer)");

        ImGui::EndTable();
    }
    ImGui::EndDisabled(); // sound_en
    ImGui::Spacing();
    EndGroupBox("Settings");

    ImGui::BeginDisabled(!sound_en);
    if (ImGui::BeginTable("BottomRowTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        BeginGroupBox("Floppy Drive Sound Emulation");

        const settings::definition& empty_volume = *o.floppy_volume_empty[selected_floppy_drive];
        const settings::definition& disk_volume = *o.floppy_volume_disk[selected_floppy_drive];
        const settings::definition& drive_sound = *o.floppy_sound[selected_floppy_drive];

        // The volumes need the drive's sound on (the settings layer refuses
        // them otherwise), so they grey out with it.
        ImGui::BeginDisabled(!available(empty_volume, ctx));
        // Row 1: Slider | Value | Label
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        int dis_vol_empty = 100 - static_cast<int>(settings::get_number(empty_volume,
            changed_prefs));
        if (ImGui::SliderInt("##Empty", &dis_vol_empty, 0, 100, "", ImGuiSliderFlags_AlwaysClamp))
            settings::set_number(empty_volume, changed_prefs, ctx, 100 - dis_vol_empty);
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d%%", dis_vol_empty);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Empty drive");
        ShowHelpMarker("Volume for floppy drive head movement sounds when no disk is inserted");

        // Row 2: Slider | Value | Label
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        int dis_vol_disk = 100 - static_cast<int>(settings::get_number(disk_volume, changed_prefs));
        if (ImGui::SliderInt("##Disk", &dis_vol_disk, 0, 100, "", ImGuiSliderFlags_AlwaysClamp))
            settings::set_number(disk_volume, changed_prefs, ctx, 100 - dis_vol_disk);
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d%%", dis_vol_disk);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Disk in drive");
        ShowHelpMarker("Volume for floppy drive sounds when a disk is being read");
        ImGui::EndDisabled();

        // Row 3: Sound Dropdown | Drive Selector
        ImGui::Dummy(ImVec2(0, 10)); // Spacer

        {
            // A negative value means external click samples configured
            // outside the panel; show it instead of hiding it behind the
            // built-in label.
            const int sound_type = static_cast<int>(settings::get_number(drive_sound,
                changed_prefs));
            const char* preview = sound_type < 0 ? "External samples"
                : sound_type > 0 ? "A500 (built-in)" : "No sound";
            const char *sound_types[] = {"No sound", "A500 (built-in)"};
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            if (ImGui::BeginCombo("##SoundType", preview)) {
                for (int n = 0; n < IM_ARRAYSIZE(sound_types); n++) {
                    const bool is_selected = (sound_type == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(sound_types[n], is_selected))
                        settings::set_number(drive_sound, changed_prefs, ctx, n);
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ShowHelpMarker("Enable realistic floppy drive sound emulation using A500 drive samples");
        }

        ImGui::SameLine();
        const char *drive_items[] = {"DF0:", "DF1:", "DF2:", "DF3:"};
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##FloppyDrive", drive_items[selected_floppy_drive])) {
            for (int n = 0; n < IM_ARRAYSIZE(drive_items); n++) {
                const bool is_selected = (selected_floppy_drive == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(drive_items[n], is_selected)) {
                    selected_floppy_drive = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ImGui::Spacing();
        EndGroupBox("Floppy Drive Sound Emulation");

        ImGui::TableNextColumn();

        // --- Buffer Size ---
        BeginGroupBox("Sound Buffer Size");

        {
            const settings::choice_list choices = settings::choices_of(o.sound_max_buff,
                changed_prefs);
            char current[AMIBERRY_SETTINGS_VALUE_MAX];
            settings::get(o.sound_max_buff, changed_prefs, current, sizeof current);
            int buf_idx = 0;
            const char* preview = current;
            for (size_t i = 0; i < choices.count; i++) {
                if (!std::strcmp(current, choices.items[i].value)) {
                    buf_idx = static_cast<int>(i);
                    preview = i == 0 ? "Min" : current;
                    break;
                }
            }
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            if (ImGui::SliderInt("##BufSize", &buf_idx, 0, static_cast<int>(choices.count) - 1, "",
                ImGuiSliderFlags_AlwaysClamp)) {
                settings::set(o.sound_max_buff, changed_prefs, ctx, choices.items[buf_idx].value);
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
            const int size = std::atoi(current);
            if (size <= 0) {
                ImGui::Text("Min");
            } else {
                const int native_channels = get_audio_nativechannels(changed_prefs.sound_stereo);
                const int sample_rate = changed_prefs.sound_freq > 0 ? changed_prefs.sound_freq : 44100;
                const float buffer_ms = native_channels > 0
                    ? (static_cast<float>(size >> 2) * 1000.0f) / static_cast<float>(sample_rate)
                    : 0.0f;
                ImGui::Text("%d (~%.1f ms/buffer)", size, buffer_ms);
            }
        }
        ShowHelpMarker("Emulator audio buffer setting. Smaller values usually reduce latency but may cause glitches on slower systems. Actual output latency also depends on host audio queueing and the selected audio device.");

        ImGui::Spacing();

        ImGui::Spacing();
        EndGroupBox("Sound Buffer Size");

        ImGui::EndTable();
    }
    ImGui::EndDisabled(); // sound_en
}
