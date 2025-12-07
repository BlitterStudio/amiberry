#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_sound()
{
	ImGui::Text("Sound Emulation");
	ImGui::RadioButton("Disabled", &changed_prefs.produce_sound, 0);
	ImGui::RadioButton("Disabled, but emulated", &changed_prefs.produce_sound, 1);
	ImGui::RadioButton("Enabled", &changed_prefs.produce_sound, 2);
	ImGui::Checkbox("Automatic switching", &changed_prefs.sound_auto);

	ImGui::Separator();
	ImGui::Text("Volume");
	ImGui::SliderInt("Paula Volume", &changed_prefs.sound_volume_paula, 0, 100);
	ImGui::SliderInt("CD Volume", &changed_prefs.sound_volume_cd, 0, 100);
	ImGui::SliderInt("AHI Volume", &changed_prefs.sound_volume_board, 0, 100);
	ImGui::SliderInt("MIDI Volume", &changed_prefs.sound_volume_midi, 0, 100);

	ImGui::Separator();
	ImGui::Text("Floppy Drive Sound Emulation");
	ImGui::Checkbox("Enable floppy drive sound", (bool*)&changed_prefs.floppyslots[0].dfxclick);
	ImGui::SliderInt("Empty drive", &changed_prefs.dfxclickvolume_empty[0], 0, 100);
	ImGui::SliderInt("Disk in drive", &changed_prefs.dfxclickvolume_disk[0], 0, 100);

	ImGui::Separator();
	ImGui::Text("Sound Buffer Size");
	ImGui::SliderInt("##SoundBufferSize", &changed_prefs.sound_maxbsiz, 0, 65536);
	ImGui::RadioButton("Pull audio", &changed_prefs.sound_pullmode, 1);
	ImGui::RadioButton("Push audio", &changed_prefs.sound_pullmode, 0);

	ImGui::Separator();
	ImGui::Text("Options");
	const char* soundcard_items[] = { "-" };
	ImGui::Combo("Device", &changed_prefs.soundcard, soundcard_items, IM_ARRAYSIZE(soundcard_items));
	ImGui::Checkbox("System default", &changed_prefs.soundcard_default);
	const char* channel_mode_items[] = { "Mono", "Stereo", "Cloned stereo (4 channels)", "4 Channels", "Cloned stereo (5.1)", "5.1 Channels", "Cloned stereo (7.1)", "7.1 channels" };
	ImGui::Combo("Channel mode", &changed_prefs.sound_stereo, channel_mode_items, IM_ARRAYSIZE(channel_mode_items));
	const char* frequency_items[] = { "11025", "22050", "32000", "44100", "48000" };
	ImGui::Combo("Frequency", &changed_prefs.sound_freq, frequency_items, IM_ARRAYSIZE(frequency_items));
	const char* interpolation_items[] = { "Disabled", "Anti", "Sinc", "RH", "Crux" };
	ImGui::Combo("Interpolation", &changed_prefs.sound_interpol, interpolation_items, IM_ARRAYSIZE(interpolation_items));
	const char* filter_items[] = { "Always off", "Emulated (A500)", "Emulated (A1200)", "Always on (A500)", "Always on (A1200)", "Always on (Fixed only)" };
	ImGui::Combo("Filter", (int*)&changed_prefs.sound_filter, filter_items, IM_ARRAYSIZE(filter_items));
	const char* separation_items[] = { "100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%" };
	ImGui::Combo("Stereo separation", &changed_prefs.sound_stereo_separation, separation_items, IM_ARRAYSIZE(separation_items));
	const char* stereo_delay_items[] = { "-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
	ImGui::Combo("Stereo delay", &changed_prefs.sound_mixed_stereo_delay, stereo_delay_items, IM_ARRAYSIZE(stereo_delay_items));
	const char* swap_channels_items[] = { "-", "Paula only", "AHI only", "Both" };
	ImGui::Combo("Swap channels", (int*)&changed_prefs.sound_stereo_swap_paula, swap_channels_items, IM_ARRAYSIZE(swap_channels_items));
}
