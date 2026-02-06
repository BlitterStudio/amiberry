#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "imgui_panels.h"
#include "sounddep/sound.h"
#include <vector>
#include <string>
#include <cstdio>

#include "gui/gui_handling.h"

static std::vector<std::string> sound_device_names;
static std::vector<const char *> sound_device_items;
static int selected_floppy_drive = 0;
static int master_volume = 100;
static int selected_volume_type = 0; // 0=Paula, 1=CD, 2=AHI, 3=MIDI
static constexpr int sndbufsizes[] = {1024, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 32768, 65536, -1};

static int getsoundbufsizeindex(int size) {
    int idx;
    if (size < sndbufsizes[0])
        return 0;
    for (idx = 0; sndbufsizes[idx] < size && sndbufsizes[idx + 1] >= 0; idx++);
    return idx + 1;
}

void render_panel_sound() {
    // Global padding for the whole panel
    ImGui::Indent(4.0f);

    // Initialize Audio Devices
    if (sound_device_names.empty()) {
        int numdevs = enumerate_sound_devices();
        for (int i = 0; i < numdevs; ++i) {
            char tmp[256];
            int type = sound_devices[i]->type;
            snprintf(tmp, sizeof(tmp), "%s: %s",
                     type == SOUND_DEVICE_SDL2
                         ? "SDL2"
                         : (type == SOUND_DEVICE_DS
                                ? "DSOUND"
                                : (type == SOUND_DEVICE_AL
                                       ? "OpenAL"
                                       : (type == SOUND_DEVICE_PA
                                              ? "PortAudio"
                                              : (type == SOUND_DEVICE_WASAPI ? "WASAPI" : "WASAPI EX")))),
                     sound_devices[i]->name);
            sound_device_names.push_back(tmp);
        }
        if (sound_device_names.empty()) sound_device_names.push_back("None");
        for (const auto &name: sound_device_names) sound_device_items.push_back(name.c_str());
    }

    // ---------------------------------------------------------
    // Audio Device
    // ---------------------------------------------------------
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Audio Device:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2);

    // Logic: Disabled if Default checked OR Sound Disabled OR No devices
    bool disable_device = changed_prefs.soundcard_default || changed_prefs.produce_sound == 0 || sound_device_names.
                          size() <= 1;
    ImGui::BeginDisabled(disable_device);
    if (ImGui::BeginCombo("##AudioDevice",
                          (changed_prefs.soundcard >= 0 && changed_prefs.soundcard < static_cast<int>(sound_device_items
                               .size()))
                              ? sound_device_items[changed_prefs.soundcard]
                              : "Unknown")) {
        for (int n = 0; n < static_cast<int>(sound_device_items.size()); n++) {
            const bool is_selected = (changed_prefs.soundcard == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(sound_device_items[n], is_selected)) {
                changed_prefs.soundcard = n;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
    ImGui::EndDisabled();
    ShowHelpMarker("Select which audio device to use for sound output");

    AmigaCheckbox("System default", &changed_prefs.soundcard_default);
    ShowHelpMarker("Use the operating system's default audio device");

    ImGui::Spacing();

    // ---------------------------------------------------------
    // Emulation | Volume
    // ---------------------------------------------------------
    if (ImGui::BeginTable("TopRowTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        BeginGroupBox("Sound Emulation");

        if (AmigaRadioButton("Disabled", changed_prefs.produce_sound == 0)) changed_prefs.produce_sound = 0;
        ShowHelpMarker("No sound processing or output");
        if (AmigaRadioButton("Disabled, but emulated", changed_prefs.produce_sound == 1))
            changed_prefs.produce_sound = 1;
        ShowHelpMarker("Paula sound chip is emulated for timing accuracy but no audio output");
        // Treat >= 2 as Enabled. If switching to Enabled, default to 2. If already >= 2, keep current value.
        if (AmigaRadioButton("Enabled", changed_prefs.produce_sound >= 2)) {
            if (changed_prefs.produce_sound < 2) changed_prefs.produce_sound = 2;
        }
        ShowHelpMarker("Full sound emulation with audio output");

        ImGui::Spacing();

        AmigaCheckbox("Automatic switching", &changed_prefs.sound_auto);
        ShowHelpMarker("Automatically adjust sound emulation quality based on performance");
        ImGui::Spacing();
        EndGroupBox("Sound Emulation");

        ImGui::TableNextColumn();

        // ---------------------------------------------------------
        // Volume (WinUAE Style)
        // ---------------------------------------------------------
        BeginGroupBox("Volume");

        // Row 1: Master Slider
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Master");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
        if (ImGui::SliderInt("##MasterVol", &master_volume, 0, 100, "")) {
            // Assuming master_sound_volume takes 0-100 or inverted?
            // Standard uae volume seems to be 0=silent, 100=max.
            master_sound_volume(100 - master_volume); // WinUAE code often inverts UI volume (0=max attenuation?)
            // Let's assume standard behavior first. If master_volume logic is reversed, verified later.
            // Actually typical WinUAE: 100% is max. Logic internally might be attenuation.
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::Text("%d%%", master_volume);
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

        int *current_vol_ptr = &changed_prefs.sound_volume_paula;
        if (selected_volume_type == 1) current_vol_ptr = &changed_prefs.sound_volume_cd;
        else if (selected_volume_type == 2) current_vol_ptr = &changed_prefs.sound_volume_board;
        else if (selected_volume_type == 3) current_vol_ptr = &changed_prefs.sound_volume_midi;

        // Note: Amiberry usually stores volume as 0-100? or inverted?
        // Existing code: newvol = 100 - slider. So stored value is inverted (attenuation).
        // Display should be 100%.
        int display_vol = 100 - *current_vol_ptr;
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
        if (ImGui::SliderInt("##SubVol", &display_vol, 0, 100, "")) {
            *current_vol_ptr = 100 - display_vol;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::Text("%d%%", display_vol);
        ShowHelpMarker("Individual volume controls - Paula: Amiga sound chip, CD: CD audio, AHI: retargetable audio, MIDI: external MIDI");
        ImGui::Spacing();
        EndGroupBox("Volume");

        ImGui::EndTable();
    }

    // ---------------------------------------------------------
    // Settings
    // ---------------------------------------------------------
    BeginGroupBox("Settings");

    if (ImGui::BeginTable("SettingsRow1Table", 3, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Channel mode:");
        const char *channel_mode_items[] = {
            "Mono", "Stereo", "Cloned stereo (4 ch)", "4 Channels", "Cloned stereo (5.1)", "5.1 Channels",
            "Cloned stereo (7.1)", "7.1 channels"
        };
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##ChanMode", channel_mode_items[changed_prefs.sound_stereo])) {
            for (int n = 0; n < IM_ARRAYSIZE(channel_mode_items); n++) {
                const bool is_selected = (changed_prefs.sound_stereo == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(channel_mode_items[n], is_selected)) {
                    changed_prefs.sound_stereo = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Audio channel mode: Mono=single channel, Stereo=2ch, 4/5.1/7.1=multi-channel");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stereo separation:");
        const char *separation_items[] = {"100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%"};
        int sep_idx = 10 - changed_prefs.sound_stereo_separation;
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);

        // Logic: Disable if Mono
        ImGui::BeginDisabled(changed_prefs.sound_stereo == 0);
        if (ImGui::BeginCombo("##Sep", separation_items[sep_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(separation_items); n++) {
                const bool is_selected = (sep_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(separation_items[n], is_selected)) {
                    sep_idx = n;
                    changed_prefs.sound_stereo_separation = 10 - sep_idx;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ImGui::EndDisabled();
        ShowHelpMarker("Controls the amount of stereo separation between left and right channels (100%=full separation, 0%=mono)");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Interpolation:");
        const char *interpolation_items[] = {"Disabled", "Anti", "Sinc", "RH", "Crux"};
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##Interp", interpolation_items[changed_prefs.sound_interpol])) {
            for (int n = 0; n < IM_ARRAYSIZE(interpolation_items); n++) {
                const bool is_selected = (changed_prefs.sound_interpol == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(interpolation_items[n], is_selected)) {
                    changed_prefs.sound_interpol = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Audio quality filter: Disabled=raw output, Anti=anti-aliasing, Sinc=high quality resampling");

        ImGui::EndTable();
    }
    ImGui::Spacing();

    if (ImGui::BeginTable("SettingsRow2Table", 4, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Frequency:");
        const char *frequency_items[] = {"11025", "22050", "32000", "44100", "48000"};
        int freq_idx = 0;
        if (changed_prefs.sound_freq == 11025) freq_idx = 0;
        else if (changed_prefs.sound_freq == 22050) freq_idx = 1;
        else if (changed_prefs.sound_freq == 32000) freq_idx = 2;
        else if (changed_prefs.sound_freq == 44100) freq_idx = 3;
        else if (changed_prefs.sound_freq == 48000) freq_idx = 4;
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##Freq", frequency_items[freq_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(frequency_items); n++) {
                const bool is_selected = (freq_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(frequency_items[n], is_selected)) {
                    const int freqs[] = {11025, 22050, 32000, 44100, 48000};
                    changed_prefs.sound_freq = freqs[n];
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Audio output sample rate in Hz (44100 or 48000 recommended for modern systems)");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Swap channels:");
        const char *swap_channels_items[] = {"-", "Paula only", "AHI only", "Both"};
        int swap_idx = 0;
        if (changed_prefs.sound_stereo_swap_paula && changed_prefs.sound_stereo_swap_ahi) swap_idx = 3;
        else if (changed_prefs.sound_stereo_swap_ahi) swap_idx = 2;
        else if (changed_prefs.sound_stereo_swap_paula) swap_idx = 1;
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##Swap", swap_channels_items[swap_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(swap_channels_items); n++) {
                const bool is_selected = (swap_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(swap_channels_items[n], is_selected)) {
                    changed_prefs.sound_stereo_swap_paula = (n == 1 || n == 3);
                    changed_prefs.sound_stereo_swap_ahi = (n == 2 || n == 3);
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Swap left and right audio channels for Paula (Amiga sound chip) or AHI (retargetable audio)");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stereo delay:");
        const char *stereo_delay_items[] = {"-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
        int delay_idx = changed_prefs.sound_mixed_stereo_delay > 0 ? changed_prefs.sound_mixed_stereo_delay : 0;
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);

        // Logic: Disable if Mono
        ImGui::BeginDisabled(changed_prefs.sound_stereo == 0);
        if (ImGui::BeginCombo("##Delay", stereo_delay_items[delay_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(stereo_delay_items); n++) {
                const bool is_selected = (delay_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(stereo_delay_items[n], is_selected)) {
                    changed_prefs.sound_mixed_stereo_delay = n == 0 ? -1 : n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ImGui::EndDisabled();
        ShowHelpMarker("Adds a small delay between stereo channels for a wider soundstage effect");

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Audio filter:");
        const char *filter_items[] = {
            "Always off", "Emulated (A500)", "Emulated (A1200)", "Always on (A500)", "Always on (A1200)",
            "Always on (Fixed only)"
        };
        int filter_idx = 0;
        if (changed_prefs.sound_filter == FILTER_SOUND_OFF) filter_idx = 0;
        else if (changed_prefs.sound_filter == FILTER_SOUND_EMUL) filter_idx = (changed_prefs.sound_filter_type == 1)
                                                                                   ? 2
                                                                                   : 1;
        else if (changed_prefs.sound_filter == FILTER_SOUND_ON) {
            if (changed_prefs.sound_filter_type == 2) filter_idx = 5;
            else if (changed_prefs.sound_filter_type == 1) filter_idx = 4;
            else filter_idx = 3;
        }
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##Filter", filter_items[filter_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(filter_items); n++) {
                const bool is_selected = (filter_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(filter_items[n], is_selected)) {
                    if (n == 0) changed_prefs.sound_filter = FILTER_SOUND_OFF;
                    else if (n == 1) {
                        changed_prefs.sound_filter = FILTER_SOUND_EMUL;
                        changed_prefs.sound_filter_type = 0;
                    } else if (n == 2) {
                        changed_prefs.sound_filter = FILTER_SOUND_EMUL;
                        changed_prefs.sound_filter_type = 1;
                    } else if (n == 3) {
                        changed_prefs.sound_filter = FILTER_SOUND_ON;
                        changed_prefs.sound_filter_type = 0;
                    } else if (n == 4) {
                        changed_prefs.sound_filter = FILTER_SOUND_ON;
                        changed_prefs.sound_filter_type = 1;
                    } else if (n == 5) {
                        changed_prefs.sound_filter = FILTER_SOUND_ON;
                        changed_prefs.sound_filter_type = 2;
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
        ShowHelpMarker("Emulates the Amiga's hardware low-pass audio filter (A500=sharper, A1200=softer)");

        ImGui::EndTable();
    }
    ImGui::Spacing();
    EndGroupBox("Settings");

    // ---------------------------------------------------------
    // Floppy | Buffer
    // ---------------------------------------------------------
    if (ImGui::BeginTable("BottomRowTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        BeginGroupBox("Floppy Drive Sound Emulation");

        int &vol_empty = changed_prefs.dfxclickvolume_empty[selected_floppy_drive];
        int &vol_disk = changed_prefs.dfxclickvolume_disk[selected_floppy_drive];

        // Row 1: Slider | Value | Label
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        int dis_vol_empty = 100 - vol_empty; // Assuming inversion based on previous patterns
        if (ImGui::SliderInt("##Empty", &dis_vol_empty, 0, 100, "")) vol_empty = 100 - dis_vol_empty;
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
        int dis_vol_disk = 100 - vol_disk;
        if (ImGui::SliderInt("##Disk", &dis_vol_disk, 0, 100, "")) vol_disk = 100 - dis_vol_disk;
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d%%", dis_vol_disk);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Disk in drive");
        ShowHelpMarker("Volume for floppy drive sounds when a disk is being read");

        // Row 3: Sound Dropdown | Drive Selector
        ImGui::Dummy(ImVec2(0, 10)); // Spacer

        const char *sound_types[] = {"No sound", "A500 (built-in)"}; // 0, 1
        int sound_type = changed_prefs.floppyslots[selected_floppy_drive].dfxclick;
        if (sound_type > 1) sound_type = 1; // Clamp if unknown
        ImGui::SetNextItemWidth(BUTTON_WIDTH * 1.5f);
        if (ImGui::BeginCombo("##SoundType", sound_types[sound_type])) {
            for (int n = 0; n < IM_ARRAYSIZE(sound_types); n++) {
                const bool is_selected = (sound_type == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(sound_types[n], is_selected)) {
                    changed_prefs.floppyslots[selected_floppy_drive].dfxclick = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Enable realistic floppy drive sound emulation using A500 drive samples");

        ImGui::SameLine();
        const char *drive_items[] = {"DF0:", "DF1:", "DF2:", "DF3:"};
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
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

        int buf_idx = getsoundbufsizeindex(changed_prefs.sound_maxbsiz);
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 6);
        if (ImGui::SliderInt("##BufSize", &buf_idx, 0, 10, "")) {
            if (buf_idx == 0) changed_prefs.sound_maxbsiz = 0;
            else changed_prefs.sound_maxbsiz = sndbufsizes[buf_idx - 1];
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        if (buf_idx == 0) ImGui::Text("Min");
        else ImGui::Text("%d", changed_prefs.sound_maxbsiz);
        ShowHelpMarker("Audio buffer size in bytes - smaller values reduce latency but may cause audio glitches");

        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Audio Pull/Push:");
        AmigaRadioButton("Pull", &changed_prefs.sound_pullmode, 1);
        ShowHelpMarker("Pull mode: emulator requests audio data when needed (recommended)");
        ImGui::SameLine();
        AmigaRadioButton("Push", &changed_prefs.sound_pullmode, 0);
        ShowHelpMarker("Push mode: emulator continuously sends audio data to output device");
        ImGui::Spacing();
        EndGroupBox("Sound Buffer Size");

        ImGui::EndTable();
    }
}
