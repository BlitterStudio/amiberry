#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "imgui_panels.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "sounddep/sound.h"
#include "parser.h"

static const char* dongle_items[] = {
	"none", "RoboCop 3", "Leader Board", "B.A.T. II", "Italy '90 Soccer",
	"Dames Grand-Maitre", "Rugby Coach", "Cricket Captain", "Leviathan",
	"Music Master", "Logistics/SuperBase", "Scala MM (Red)", "Scala MM (Green)",
	"Striker Manager", "Multi-player Soccer Manager", "Football Director 2"
};



static bool initialized = false;
static std::vector<std::string> sampler_names;
static std::vector<std::string> serial_port_names;
static std::vector<std::string> midi_in_names;
static std::vector<std::string> midi_out_names;

static void RefreshDevices()
{
	enumerate_sound_devices();

	sampler_names.clear();
	sampler_names.push_back("none");
	for (int card = 0; card < MAX_SOUND_DEVICES && record_devices[card]; card++) {
		int type = record_devices[card]->type;
		TCHAR tmp[MAX_DPATH];
		_sntprintf(tmp, MAX_DPATH, _T("%s: %s"),
			type == SOUND_DEVICE_SDL2 ? _T("SDL2") : type == SOUND_DEVICE_DS ? _T("DSOUND") : type == SOUND_DEVICE_AL ? _T("OpenAL") : type == SOUND_DEVICE_PA ? _T("PortAudio") : _T("WASAPI"),
			record_devices[card]->name);
		// Filter for SDL2 as per Guisan implementation?
		if (type == SOUND_DEVICE_SDL2) {
			char* s = ua(tmp);
			sampler_names.push_back(s);
			xfree(s);
		}
	}

	serial_port_names.clear();
	serial_port_names.push_back("none");
	for (const auto& i : serial_ports)
	{
		if (i.find(SERIAL_INTERNAL) != std::string::npos)
		{
			std::string tmp = i;
			// logic from PanelIOPorts
			/*
			if (!shmem_serial_state()) shmem_serial_create();
			switch (shmem_serial_state()) { ... }
			*/
			serial_port_names.push_back(tmp);
		}
		else {
			serial_port_names.push_back(i);
		}
	}

	midi_in_names.clear();
	midi_in_names.push_back("none");
	for (const auto& i : midi_in_ports) {
		midi_in_names.push_back(i);
	}

	midi_out_names.clear();
	midi_out_names.push_back("none");
	for (const auto& i : midi_out_ports) {
		midi_out_names.push_back(i);
	}

	initialized = true;
}

// Generic Combo for vectors
static bool VectorCombo(const char* label, int* current_item, const std::vector<std::string>& items)
{
	bool changed = false;
	if (ImGui::BeginCombo(label, items[*current_item].c_str()))
	{
		for (int n = 0; n < static_cast<int>(items.size()); n++)
		{
			const bool is_selected = (*current_item == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(items[n].c_str(), is_selected))
			{
				*current_item = n;
				changed = true;
			}
			if (is_selected)
			{
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

void render_panel_io()
{
	if (!initialized)
		RefreshDevices();

	// ---------------------------------------------------------
	// Parallel Port
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("ParallelPort", ImVec2(0, 100), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Parallel Port"); ImGui::EndMenuBar(); }

	// Sampler
	int sampler_idx = changed_prefs.samplersoundcard + 1;
	// Safety check
	if (sampler_idx < 0 || sampler_idx >= (int)sampler_names.size()) sampler_idx = 0;
	
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Sampler:"); ImGui::SameLine();
	ImGui::SetNextItemWidth(-1);
	if (VectorCombo("##Sampler", &sampler_idx, sampler_names)) {
		changed_prefs.samplersoundcard = sampler_idx - 1;
		if (sampler_idx > 0)
			changed_prefs.prtname[0] = 0;
	}

	bool stereo_enabled = (sampler_idx > 0);
	if (!stereo_enabled) ImGui::BeginDisabled();
	AmigaCheckbox("Stereo sampler", &changed_prefs.sampler_stereo);
	if (!stereo_enabled) ImGui::EndDisabled();

	ImGui::EndChild();
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// Serial Port
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("SerialPort", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Serial Port"); ImGui::EndMenuBar(); }

	int serial_idx = 0;
	if (changed_prefs.sername[0]) {
		char* current_ser = ua(changed_prefs.sername);
		std::string s_curr(current_ser);
		xfree(current_ser);
		
		for (size_t i = 0; i < serial_port_names.size(); ++i) {
			if (serial_port_names[i] == s_curr) { serial_idx = i; break; }
		}
	}

	ImGui::SetNextItemWidth(-1);
	if (VectorCombo("##SerialPort", &serial_idx, serial_port_names)) {
		if (serial_idx == 0) {
			changed_prefs.sername[0] = 0;
			changed_prefs.use_serial = false;
		} else {
			// serial_port_names is 1:1 with serial_ports offset by 1
			if (serial_idx - 1 < (int)serial_ports.size()) {
				au_copy(changed_prefs.sername, 256, serial_ports[serial_idx - 1].c_str());
				changed_prefs.use_serial = true;
			}
		}
	}

	bool use_serial = (serial_idx > 0);
	if (!use_serial) ImGui::BeginDisabled();

	if (ImGui::BeginTable("SerialOptsTable", 4, ImGuiTableFlags_None)) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); AmigaCheckbox("Shared", &changed_prefs.serial_demand);
		ImGui::TableNextColumn(); AmigaCheckbox("Host RTS/CTS", &changed_prefs.serial_hwctsrts);
		ImGui::TableNextColumn(); AmigaCheckbox("Direct", &changed_prefs.serial_direct);
		ImGui::TableNextColumn(); AmigaCheckbox("uaeserial.device", &changed_prefs.uaeserial);
		ImGui::EndTable();
	}

	if (ImGui::BeginTable("SerialStatusTable", 2, ImGuiTableFlags_None)) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); AmigaCheckbox("Serial status (RTS/...)", &changed_prefs.serial_rtsctsdtrdtecd);
		ImGui::TableNextColumn(); AmigaCheckbox("Serial status: Ring Ind.", &changed_prefs.serial_ri);
		ImGui::EndTable();
	}

	if (!use_serial) ImGui::EndDisabled();

	ImGui::EndChild();
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// MIDI
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("MIDI", ImVec2(0, 100), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("MIDI"); ImGui::EndMenuBar(); }

	int midi_out_idx = 0;
	int midi_in_idx = 0;

	if (ImGui::BeginTable("MidiTable", 2, ImGuiTableFlags_None)) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		// Out
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Out:"); ImGui::SameLine();
		if (changed_prefs.midioutdev[0]) {
			char* out_name = ua(changed_prefs.midioutdev);
			std::string s_out(out_name);
			xfree(out_name);

			for(size_t i=0; i<midi_out_names.size(); ++i) if(midi_out_names[i] == s_out) { midi_out_idx = i; break; }
		}
		ImGui::SetNextItemWidth(-1);
		if (VectorCombo("##MidiOut", &midi_out_idx, midi_out_names)) {
			if (midi_out_idx == 0) {
				changed_prefs.midioutdev[0] = 0;
				changed_prefs.midiindev[0] = 0; // Clear IN if OUT is cleared
			} else {
				if (midi_out_idx - 1 < (int)midi_out_ports.size())
					au_copy(changed_prefs.midioutdev, 256, midi_out_ports[midi_out_idx - 1].c_str());
			}
		}

		ImGui::TableNextColumn();

		// In
		ImGui::AlignTextToFramePadding();
		ImGui::Text("In:"); ImGui::SameLine();
		bool midi_in_enabled = (midi_out_idx > 0);
		if (changed_prefs.midiindev[0]) {
			char* in_name = ua(changed_prefs.midiindev);
			std::string s_in(in_name);
			xfree(in_name);

			for(size_t i=0; i<midi_in_names.size(); ++i) if(midi_in_names[i] == s_in) { midi_in_idx = i; break; }
		}

		if (!midi_in_enabled) ImGui::BeginDisabled();
		ImGui::SetNextItemWidth(-1);
		if (VectorCombo("##MidiIn", &midi_in_idx, midi_in_names)) {
			if (midi_in_idx == 0) changed_prefs.midiindev[0] = 0;
			else if (midi_in_idx - 1 < (int)midi_in_ports.size())
				au_copy(changed_prefs.midiindev, 256, midi_in_ports[midi_in_idx - 1].c_str());
		}
		if (!midi_in_enabled) ImGui::EndDisabled();

		ImGui::EndTable();
	}

	// Route
	bool route_enabled = (midi_in_idx > 0);
	if (!route_enabled) ImGui::BeginDisabled();
	AmigaCheckbox("Route MIDI In to MIDI Out", &changed_prefs.midirouter);
	if (!route_enabled) ImGui::EndDisabled();

	ImGui::EndChild();
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// Protection Dongle
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("Dongle", ImVec2(0, 60), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Protection Dongle"); ImGui::EndMenuBar(); }

	ImGui::SetNextItemWidth(-1);
	if (ImGui::BeginCombo("##Dongle", dongle_items[changed_prefs.dongle])) {
		for (int n = 0; n < IM_ARRAYSIZE(dongle_items); n++) {
			const bool is_selected = (changed_prefs.dongle == n);
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(dongle_items[n], is_selected)) {
				changed_prefs.dongle = n;
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
}

