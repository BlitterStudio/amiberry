#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_io()
{
	ImGui::Text("Parallel Port");
	const char* sampler_items[] = { "none" };
	// Future: populate sampler_items dynamically from detected devices
	static int sampler_idx = 0;
	ImGui::Combo("Sampler", &sampler_idx, sampler_items, IM_ARRAYSIZE(sampler_items));
	ImGui::Checkbox("Stereo sampler", &changed_prefs.sampler_stereo);

	ImGui::Separator();
	ImGui::Text("Serial Port");
	// CRITICAL FIX: previously used ImGui::Combo with (int*)(void*)changed_prefs.sername (unsafe cast).
	// We only have a single placeholder option right now; present it read-only until real enumeration exists.
	const char* serial_port_items[] = { "none" };
	static int serial_port_idx = 0; // always 0 with current placeholder list
	ImGui::BeginDisabled();
	ImGui::Combo("##SerialPort", &serial_port_idx, serial_port_items, IM_ARRAYSIZE(serial_port_items));
	ImGui::EndDisabled();
	ImGui::Checkbox("Shared", &changed_prefs.serial_demand);
	ImGui::Checkbox("Direct", &changed_prefs.serial_direct);
	ImGui::Checkbox("Host RTS/CTS", &changed_prefs.serial_hwctsrts);
	ImGui::Checkbox("uaeserial.device", &changed_prefs.uaeserial);
	ImGui::Checkbox("Serial status (RTS/CTS/DTR/DTE/CD)", &changed_prefs.serial_rtsctsdtrdtecd);
	ImGui::Checkbox("Serial status: Ring Indicator", &changed_prefs.serial_ri);

	ImGui::Separator();
	ImGui::Text("MIDI");
	// CRITICAL FIX: remove unsafe casts for midioutdev/midiindev; use indices instead.
	const char* midi_out_items[] = { "none" };
	const char* midi_in_items[] = { "none" };
	static int midi_out_idx = 0;
	static int midi_in_idx = 0;
	ImGui::BeginDisabled();
	ImGui::Combo("Out", &midi_out_idx, midi_out_items, IM_ARRAYSIZE(midi_out_items));
	ImGui::Combo("In", &midi_in_idx, midi_in_items, IM_ARRAYSIZE(midi_in_items));
	ImGui::EndDisabled();
	ImGui::Checkbox("Route MIDI In to MIDI Out", &changed_prefs.midirouter);

	ImGui::Separator();
	ImGui::Text("Protection Dongle");
	const char* dongle_items[] = { "none", "RoboCop 3", "Leader Board", "B.A.T. II", "Italy '90 Soccer", "Dames Grand-Maitre", "Rugby Coach", "Cricket Captain", "Leviathan", "Music Master", "Logistics/SuperBase", "Scala MM (Red)", "Scala MM (Green)", "Striker Manager", "Multi-player Soccer Manager", "Football Director 2" };
	ImGui::Combo("##ProtectionDongle", &changed_prefs.dongle, dongle_items, IM_ARRAYSIZE(dongle_items));
}
