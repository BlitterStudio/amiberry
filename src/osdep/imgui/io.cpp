#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "sounddep/sound.h"
#include "parser.h"
#include "gui/gui_handling.h"

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
	sampler_names.emplace_back("none");
	for (int card = 0; card < MAX_SOUND_DEVICES && record_devices[card]; card++) {
		int type = record_devices[card]->type;
		TCHAR tmp[MAX_DPATH];
		_sntprintf(tmp, MAX_DPATH, _T("%s: %s"),
			type == SOUND_DEVICE_SDL2 ? _T("SDL2") : type == SOUND_DEVICE_DS ? _T("DSOUND") : type == SOUND_DEVICE_AL ? _T("OpenAL") : type == SOUND_DEVICE_PA ? _T("PortAudio") : _T("WASAPI"),
			record_devices[card]->name);
		// Filter for SDL2 as per Guisan implementation?
		if (type == SOUND_DEVICE_SDL2) {
			char* s = ua(tmp);
			sampler_names.emplace_back(s);
			xfree(s);
		}
	}

	serial_port_names.clear();
	serial_port_names.emplace_back("none");
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
	midi_in_names.emplace_back("none");
	for (const auto& i : midi_in_ports) {
		midi_in_names.push_back(i);
	}

	midi_out_names.clear();
	midi_out_names.emplace_back("none");
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
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	return changed;
}

void render_panel_io()
{
	if (!initialized)
		RefreshDevices();

	ImGui::Indent(4.0f);
	// ---------------------------------------------------------
	// Parallel Port
	// ---------------------------------------------------------
	BeginGroupBox("Parallel Port");

	// Sampler
	int sampler_idx = changed_prefs.samplersoundcard + 1;
	// Safety check
	if (sampler_idx < 0 || sampler_idx >= (int)sampler_names.size()) sampler_idx = 0;
	
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Sampler:"); ImGui::SameLine();
	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
	if (VectorCombo("##Sampler", &sampler_idx, sampler_names)) {
		changed_prefs.samplersoundcard = sampler_idx - 1;
		if (sampler_idx > 0)
			changed_prefs.prtname[0] = 0;
	}
	ShowHelpMarker("Audio sampling device connected to the parallel port");

	bool stereo_enabled = (sampler_idx > 0);
	if (!stereo_enabled) ImGui::BeginDisabled();
	AmigaCheckbox("Stereo sampler", &changed_prefs.sampler_stereo);
	ShowHelpMarker("Enable stereo audio sampling instead of mono");
	if (!stereo_enabled) ImGui::EndDisabled();
	ImGui::Spacing();
	EndGroupBox("Parallel Port");

	// ---------------------------------------------------------
	// Serial Port
	// ---------------------------------------------------------
	BeginGroupBox("Serial Port");

	int serial_idx = 0;
	if (changed_prefs.sername[0]) {
		char* current_ser = ua(changed_prefs.sername);
		std::string s_curr(current_ser);
		xfree(current_ser);
		
		for (size_t i = 0; i < serial_port_names.size(); ++i) {
			if (serial_port_names[i] == s_curr) { serial_idx = i; break; }
		}
	}

	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
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
	ShowHelpMarker("Map Amiga's RS-232 serial port to host serial port");

	bool use_serial = (serial_idx > 0);
	if (!use_serial) ImGui::BeginDisabled();

	if (ImGui::BeginTable("SerialOptsTable", 4, ImGuiTableFlags_None)) {
		ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
		ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 1.8f);
		ImGui::TableSetupColumn("column3", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
		ImGui::TableSetupColumn("column4", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 1.8f);
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); AmigaCheckbox("Shared", &changed_prefs.serial_demand);
		ShowHelpMarker("Allow host and emulated Amiga to share the serial port");
		ImGui::TableNextColumn(); AmigaCheckbox("Host RTS/CTS", &changed_prefs.serial_hwctsrts);
		ShowHelpMarker("Use hardware flow control from the host serial port");
		ImGui::TableNextColumn(); AmigaCheckbox("Direct", &changed_prefs.serial_direct);
		ShowHelpMarker("Direct serial port access bypassing emulation layer");
		ImGui::TableNextColumn(); AmigaCheckbox("uaeserial.device", &changed_prefs.uaeserial);
		ShowHelpMarker("Enable UAE's custom serial device driver");
		ImGui::EndTable();
	}

	if (ImGui::BeginTable("SerialStatusTable", 2, ImGuiTableFlags_None)) {
		ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.9f);
		ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.9f);
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); AmigaCheckbox("Serial status (RTS/CTS/...)", &changed_prefs.serial_rtsctsdtrdtecd);
		ShowHelpMarker("Emulate serial port status lines (RTS, CTS, DTR, DSR, CD)");
		ImGui::TableNextColumn(); AmigaCheckbox("Serial status: Ring Indicator", &changed_prefs.serial_ri);
		ShowHelpMarker("Emulate the RI serial line used by modems");
		ImGui::EndTable();
	}

	if (!use_serial) ImGui::EndDisabled();
	ImGui::Spacing();
	EndGroupBox("Serial Port");

	// ---------------------------------------------------------
	// MIDI
	// ---------------------------------------------------------
	BeginGroupBox("MIDI");

	int midi_out_idx = 0;
	int midi_in_idx = 0;

	if (ImGui::BeginTable("MidiTable", 2, ImGuiTableFlags_None)) {
		ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 3.0f);
		ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 3.0f);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		// Out
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Out:"); ImGui::SameLine();
		if (changed_prefs.midioutdev[0]) {
			char* out_name = ua(changed_prefs.midioutdev);
			const std::string s_out(out_name);
			xfree(out_name);

			for(size_t i=0; i<midi_out_names.size(); ++i) if(midi_out_names[i] == s_out) { midi_out_idx = i; break; }
		}
		ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
		if (VectorCombo("##MidiOut", &midi_out_idx, midi_out_names)) {
			if (midi_out_idx == 0) {
				changed_prefs.midioutdev[0] = 0;
				changed_prefs.midiindev[0] = 0; // Clear IN if OUT is cleared
			} else {
				if (midi_out_idx - 1 < (int)midi_out_ports.size())
					au_copy(changed_prefs.midioutdev, 256, midi_out_ports[midi_out_idx - 1].c_str());
			}
		}
		ShowHelpMarker("Connect Amiga MIDI output to host MIDI device");

		ImGui::TableNextColumn();

		// In
		ImGui::AlignTextToFramePadding();
		ImGui::Text("In:"); ImGui::SameLine();
		bool midi_in_enabled = (midi_out_idx > 0);
		if (changed_prefs.midiindev[0]) {
			char* in_name = ua(changed_prefs.midiindev);
			const std::string s_in(in_name);
			xfree(in_name);

			for(size_t i=0; i<midi_in_names.size(); ++i) if(midi_in_names[i] == s_in) { midi_in_idx = i; break; }
		}

		if (!midi_in_enabled) ImGui::BeginDisabled();
		ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
		if (VectorCombo("##MidiIn", &midi_in_idx, midi_in_names)) {
			if (midi_in_idx == 0) changed_prefs.midiindev[0] = 0;
			else if (midi_in_idx - 1 < (int)midi_in_ports.size())
				au_copy(changed_prefs.midiindev, 256, midi_in_ports[midi_in_idx - 1].c_str());
		}
		ShowHelpMarker("Connect host MIDI input to Amiga MIDI");
		if (!midi_in_enabled) ImGui::EndDisabled();

		ImGui::EndTable();
	}
	ImGui::Spacing();
	// Route
	bool route_enabled = (midi_in_idx > 0);
	if (!route_enabled) ImGui::BeginDisabled();
	AmigaCheckbox("Route MIDI In to MIDI Out", &changed_prefs.midirouter);
	ShowHelpMarker("Pass MIDI input directly through to MIDI output");
	if (!route_enabled) ImGui::EndDisabled();
	ImGui::Spacing();
	EndGroupBox("MIDI");

	// ---------------------------------------------------------
	// Protection Dongle
	// ---------------------------------------------------------
	BeginGroupBox("Protection Dongle");

	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
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
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	ShowHelpMarker("Emulate copy protection dongles for specific games");
	ImGui::Spacing();
	EndGroupBox("Protection Dongle");
}

