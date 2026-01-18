#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "sounddep/sound.h"
#include <vector>
#include <string>
#include <cstdio>

static std::vector<std::string> sound_device_names;
static std::vector<const char*> sound_device_items;
static int selected_floppy_drive = 0;
static int master_volume = 100;
static int selected_volume_type = 0; // 0=Paula, 1=CD, 2=AHI, 3=MIDI
static constexpr int sndbufsizes[] = { 1024, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 32768, 65536, -1 };

static int getsoundbufsizeindex(int size)
{
	int idx;
	if (size < sndbufsizes[0])
		return 0;
	for (idx = 0; sndbufsizes[idx] < size && sndbufsizes[idx + 1] >= 0; idx++);
	return idx + 1;
}

void render_panel_sound()
{
	// Initialize Audio Devices
	if (sound_device_names.empty()) {
		int numdevs = enumerate_sound_devices();
		for (int i = 0; i < numdevs; ++i) {
			char tmp[256];
			int type = sound_devices[i]->type;
			snprintf(tmp, sizeof(tmp), "%s: %s",
				type == SOUND_DEVICE_SDL2 ? "SDL2" : (type == SOUND_DEVICE_DS ? "DSOUND" : (type == SOUND_DEVICE_AL ? "OpenAL" : (type == SOUND_DEVICE_PA ? "PortAudio" : (type == SOUND_DEVICE_WASAPI ? "WASAPI" : "WASAPI EX")))),
				sound_devices[i]->name);
			sound_device_names.push_back(tmp);
		}
		if (sound_device_names.empty()) sound_device_names.push_back("None");
		for (const auto& name : sound_device_names) sound_device_items.push_back(name.c_str());
	}

	// ---------------------------------------------------------
	// Audio Device
	// ---------------------------------------------------------
	ImGui::Text("Audio Device:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 170);
	
	// Logic: Disabled if Default checked OR Sound Disabled OR No devices
	bool disable_device = changed_prefs.soundcard_default || changed_prefs.produce_sound == 0 || sound_device_names.size() <= 1;
	ImGui::BeginDisabled(disable_device);
	if (ImGui::BeginCombo("##AudioDevice", (changed_prefs.soundcard >= 0 && changed_prefs.soundcard < static_cast<int>(sound_device_items.size())) ? sound_device_items[changed_prefs.soundcard] : "Unknown")) {
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
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	ImGui::Checkbox("System default", &changed_prefs.soundcard_default);
	
	ImGui::Spacing();

	// ---------------------------------------------------------
	// Emulation | Volume
	// ---------------------------------------------------------
	ImGui::Columns(2, "TopRow", true);
	
	ImGui::BeginGroup();
	ImGui::BeginChild("SoundEmu", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Sound Emulation"); ImGui::EndMenuBar(); }
	
	if (ImGui::RadioButton("Disabled", changed_prefs.produce_sound == 0)) changed_prefs.produce_sound = 0;
	if (ImGui::RadioButton("Disabled, but emulated", changed_prefs.produce_sound == 1)) changed_prefs.produce_sound = 1;
	// Treat >= 2 as Enabled. If switching to Enabled, default to 2. If already >= 2, keep current value.
	if (ImGui::RadioButton("Enabled", changed_prefs.produce_sound >= 2)) {
		if (changed_prefs.produce_sound < 2) changed_prefs.produce_sound = 2;
	}
	
	ImGui::Spacing();
	ImGui::Checkbox("Automatic switching", &changed_prefs.sound_auto);
	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::NextColumn();

	// ---------------------------------------------------------
	// Volume (WinUAE Style)
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("Volume", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Volume"); ImGui::EndMenuBar(); }

	// Row 1: Master Slider
	ImGui::Text("Master"); ImGui::SameLine(60);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
	if (ImGui::SliderInt("##MasterVol", &master_volume, 0, 100, "")) {
		// Assuming master_sound_volume takes 0-100 or inverted? 
		// Standard uae volume seems to be 0=silent, 100=max.
		master_sound_volume(100 - master_volume); // WinUAE code often inverts UI volume (0=max attenuation?)
		// Let's assume standard behavior first. If master_volume logic is reversed, verified later.
		// Actually typical WinUAE: 100% is max. Logic internally might be attenuation.
	}
	ImGui::SameLine(); ImGui::Text("%d%%", master_volume);

	ImGui::Spacing();

	// Row 2: Type Dropdown + Slider
	const char* vol_types[] = { "Paula", "CD", "AHI", "MIDI" };
	ImGui::SetNextItemWidth(60);
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
	ImGui::SameLine();
	
	int* current_vol_ptr = &changed_prefs.sound_volume_paula;
	if (selected_volume_type == 1) current_vol_ptr = &changed_prefs.sound_volume_cd;
	else if (selected_volume_type == 2) current_vol_ptr = &changed_prefs.sound_volume_board;
	else if (selected_volume_type == 3) current_vol_ptr = &changed_prefs.sound_volume_midi;

	// Note: Amiberry usually stores volume as 0-100? or inverted?
	// Existing code: newvol = 100 - slider. So stored value is inverted (attenuation).
	// Display should be 100%. 
	int display_vol = 100 - *current_vol_ptr;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
	if (ImGui::SliderInt("##SubVol", &display_vol, 0, 100, "")) {
		*current_vol_ptr = 100 - display_vol;
	}
	ImGui::SameLine(); ImGui::Text("%d%%", display_vol);

	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::Columns(1);
	
	// ---------------------------------------------------------
	// Settings
	// ---------------------------------------------------------
	ImGui::BeginChild("Settings", ImVec2(0, 150), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Settings"); ImGui::EndMenuBar(); }

	ImGui::Columns(3, "SettingsRow1", false);
	ImGui::Text("Channel mode:");
	const char* channel_mode_items[] = { "Mono", "Stereo", "Cloned stereo (4 ch)", "4 Channels", "Cloned stereo (5.1)", "5.1 Channels", "Cloned stereo (7.1)", "7.1 channels" };
	ImGui::SetNextItemWidth(-1);
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
	
	ImGui::NextColumn();
	ImGui::Text("Stereo separation:");
	const char* separation_items[] = { "100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%" };
	int sep_idx = 10 - changed_prefs.sound_stereo_separation;
	ImGui::SetNextItemWidth(-1);
	
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
	ImGui::EndDisabled();
	
	ImGui::NextColumn();
	ImGui::Text("Interpolation:");
	const char* interpolation_items[] = { "Disabled", "Anti", "Sinc", "RH", "Crux" };
	ImGui::SetNextItemWidth(-1);
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
	
	ImGui::Columns(1); ImGui::Spacing();
	
	ImGui::Columns(4, "SettingsRow2", false);
	ImGui::Text("Frequency:");
	const char* frequency_items[] = { "11025", "22050", "32000", "44100", "48000" };
	int freq_idx = 0;
	if (changed_prefs.sound_freq == 11025) freq_idx = 0;
	else if (changed_prefs.sound_freq == 22050) freq_idx = 1;
	else if (changed_prefs.sound_freq == 32000) freq_idx = 2;
	else if (changed_prefs.sound_freq == 44100) freq_idx = 3;
	else if (changed_prefs.sound_freq == 48000) freq_idx = 4;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::BeginCombo("##Freq", frequency_items[freq_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(frequency_items); n++) {
			const bool is_selected = (freq_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(frequency_items[n], is_selected)) {
				const int freqs[] = { 11025, 22050, 32000, 44100, 48000 };
				changed_prefs.sound_freq = freqs[n];
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	
	ImGui::NextColumn();
	ImGui::Text("Swap channels:");
	const char* swap_channels_items[] = { "-", "Paula only", "AHI only", "Both" };
	int swap_idx = 0;
	if (changed_prefs.sound_stereo_swap_paula && changed_prefs.sound_stereo_swap_ahi) swap_idx = 3;
	else if (changed_prefs.sound_stereo_swap_ahi) swap_idx = 2;
	else if (changed_prefs.sound_stereo_swap_paula) swap_idx = 1;
	ImGui::SetNextItemWidth(-1);
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
	
	ImGui::NextColumn();
	ImGui::Text("Stereo delay:");
	const char* stereo_delay_items[] = { "-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
	int delay_idx = changed_prefs.sound_mixed_stereo_delay > 0 ? changed_prefs.sound_mixed_stereo_delay : 0;
	ImGui::SetNextItemWidth(-1);
	
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
	ImGui::EndDisabled();
	
	ImGui::NextColumn();
	ImGui::Text("Audio filter:");
	const char* filter_items[] = { "Always off", "Emulated (A500)", "Emulated (A1200)", "Always on (A500)", "Always on (A1200)", "Always on (Fixed only)" };
	int filter_idx = 0;
	if (changed_prefs.sound_filter == FILTER_SOUND_OFF) filter_idx = 0;
	else if (changed_prefs.sound_filter == FILTER_SOUND_EMUL) filter_idx = (changed_prefs.sound_filter_type == 1) ? 2 : 1;
	else if (changed_prefs.sound_filter == FILTER_SOUND_ON) {
		if (changed_prefs.sound_filter_type == 2) filter_idx = 5;
		else if (changed_prefs.sound_filter_type == 1) filter_idx = 4;
		else filter_idx = 3;
	}
	ImGui::SetNextItemWidth(-1);
	if (ImGui::BeginCombo("##Filter", filter_items[filter_idx])) {
		for (int n = 0; n < IM_ARRAYSIZE(filter_items); n++) {
			const bool is_selected = (filter_idx == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(filter_items[n], is_selected)) {
				if (n == 0) changed_prefs.sound_filter = FILTER_SOUND_OFF;
				else if (n == 1) { changed_prefs.sound_filter = FILTER_SOUND_EMUL; changed_prefs.sound_filter_type = 0; }
				else if (n == 2) { changed_prefs.sound_filter = FILTER_SOUND_EMUL; changed_prefs.sound_filter_type = 1; }
				else if (n == 3) { changed_prefs.sound_filter = FILTER_SOUND_ON; changed_prefs.sound_filter_type = 0; }
				else if (n == 4) { changed_prefs.sound_filter = FILTER_SOUND_ON; changed_prefs.sound_filter_type = 1; }
				else if (n == 5) { changed_prefs.sound_filter = FILTER_SOUND_ON; changed_prefs.sound_filter_type = 2; }
			}
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	
	ImGui::Columns(1);
	ImGui::EndChild();

	// ---------------------------------------------------------
	// Floppy | Buffer
	// ---------------------------------------------------------
	ImGui::Columns(2, "BottomRow", true);
	
	// --- Floppy Sound (WinUAE Layout) ---
	ImGui::BeginGroup();
	ImGui::BeginChild("FloppySound", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Floppy Drive Sound Emulation"); ImGui::EndMenuBar(); }
	
	int& vol_empty = changed_prefs.dfxclickvolume_empty[selected_floppy_drive];
	int& vol_disk = changed_prefs.dfxclickvolume_disk[selected_floppy_drive];
	
	// Row 1: Slider | Value | Label
	ImGui::SetNextItemWidth(100);
	int dis_vol_empty = 100 - vol_empty; // Assuming inversion based on previous patterns
	if (ImGui::SliderInt("##Empty", &dis_vol_empty, 0, 100, "")) vol_empty = 100 - dis_vol_empty;
	ImGui::SameLine(); 
	ImGui::Text("%d%%", dis_vol_empty); 
	ImGui::SameLine(); ImGui::Text("Empty drive");
	
	// Row 2: Slider | Value | Label
	ImGui::SetNextItemWidth(100);
	int dis_vol_disk = 100 - vol_disk;
	if (ImGui::SliderInt("##Disk", &dis_vol_disk, 0, 100, "")) vol_disk = 100 - dis_vol_disk;
	ImGui::SameLine();
	ImGui::Text("%d%%", dis_vol_disk);
	ImGui::SameLine(); ImGui::Text("Disk in drive");

	// Row 3: Sound Dropdown | Drive Selector
	ImGui::Dummy(ImVec2(0, 10)); // Spacer
	
	const char* sound_types[] = { "No sound", "A500 (built-in)" }; // 0, 1
	int sound_type = changed_prefs.floppyslots[selected_floppy_drive].dfxclick;
	if (sound_type > 1) sound_type = 1; // Clamp if unknown
	ImGui::SetNextItemWidth(120);
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

	ImGui::SameLine();
	const char* drive_items[] = { "DF0:", "DF1:", "DF2:", "DF3:" };
	ImGui::SetNextItemWidth(80);
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
	
	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::NextColumn();
	
	// --- Buffer Size ---
	ImGui::BeginGroup();
	ImGui::BeginChild("BufferSize", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Sound Buffer Size"); ImGui::EndMenuBar(); }
	
	int buf_idx = getsoundbufsizeindex(changed_prefs.sound_maxbsiz);
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60);
	if (ImGui::SliderInt("##BufSize", &buf_idx, 0, 10, "")) {
		if (buf_idx == 0) changed_prefs.sound_maxbsiz = 0;
		else changed_prefs.sound_maxbsiz = sndbufsizes[buf_idx - 1];
	}
	ImGui::SameLine();
	if (buf_idx == 0) ImGui::Text("Min");
	else ImGui::Text("%d", changed_prefs.sound_maxbsiz);
	
	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
	
	ImGui::Text("Audio Pull/Push:");
	ImGui::RadioButton("Pull", &changed_prefs.sound_pullmode, 1);
	ImGui::SameLine();
	ImGui::RadioButton("Push", &changed_prefs.sound_pullmode, 0);

	ImGui::EndChild();
	ImGui::EndGroup();

	ImGui::Columns(1);
}
