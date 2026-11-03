#include "imgui.h"
#include "sysdeps.h"
#include <string>
#include "imgui_panels.h"
#include "options.h"
#include "file_dialog.h"
#include "gui/gui_handling.h"
#include "uae.h"
#include "settings/settings_definitions.h"

namespace settings = amiberry_settings;

static constexpr const char* PRINTER_FILE_DIALOG_KEY = "IO_PRINTER_FILE";

// The settings layer owns the IO port rules: the parallel port has a
// printer or a sampler, serial options need a port, MIDI in needs MIDI
// out, and the host's ports and devices are its dynamic choices.
namespace {

const settings::definition& option(const char* id)
{
	return settings::require(id);
}

settings::context gui_context()
{
	return settings::context{emulating != 0};
}

std::string value_of(const char* id)
{
	char text[MAX_DPATH] = "";
	settings::get(option(id), changed_prefs, text, sizeof text);
	return text;
}

bool available(const char* id)
{
	return settings::is_available(option(id), changed_prefs, gui_context());
}

void put(const char* id, const std::string& value)
{
	settings::set(option(id), changed_prefs, gui_context(), value);
}

void checkbox(const char* id, const char* label)
{
	bool checked = value_of(id) == "true";
	ImGui::BeginDisabled(!available(id));
	if (AmigaCheckbox(label, &checked))
		put(id, checked ? "true" : "false");
	ImGui::EndDisabled();
}

// A combo over the option's choices, greyed out while the option is
// unavailable; a configured value the host no longer has still shows.
void combo(const char* id, const char* combo_id)
{
	const settings::definition& d = option(id);
	const std::string current = value_of(id);
	const settings::choice_list choices = settings::choices_of(d, changed_prefs);
	std::string preview = current;
	for (size_t i = 0; i < choices.count; i++) {
		if (current == choices.items[i].value)
			preview = choices.items[i].label;
	}
	ImGui::BeginDisabled(!available(id));
	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
	if (ImGui::BeginCombo(combo_id, preview.c_str())) {
		for (size_t i = 0; i < choices.count; i++) {
			const bool is_selected = current == choices.items[i].value;
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			if (ImGui::Selectable(choices.items[i].label, is_selected))
				put(id, choices.items[i].value);
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	ImGui::EndDisabled();
}

std::string print_folder_default()
{
	return home_dir.empty() ? get_data_path() : home_dir;
}

} // namespace

void render_panel_io()
{
	ImGui::Indent(4.0f);
	// ---------------------------------------------------------
	// Parallel Port
	// ---------------------------------------------------------
	BeginGroupBox("Parallel Port");

	const std::string printer = value_of("amiberry.parallel_port");
	const std::string folder = value_of("amiberry.parallel_port:folder");
	// "FILE:" alone prints to the data folder (parser.cpp).
	const bool print_to_file = printer.size() >= 5 && !_tcsnicmp(printer.c_str(), _T("FILE:"), 5);
	std::string printer_label = printer;
	if (print_to_file)
		printer_label = "Print to file (" + (folder.empty() ? get_data_path() : folder) + ")";

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Printer:"); ImGui::SameLine();
	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
	if (ImGui::BeginCombo("##Printer", printer_label.c_str())) {
		if (ImGui::Selectable("none", printer == "none"))
			put("amiberry.parallel_port", "none");
		if (ImGui::Selectable("Print to file", print_to_file))
			OpenDirDialogKey(PRINTER_FILE_DIALOG_KEY, print_to_file && !folder.empty() ? folder : print_folder_default());
		if (printer != "none" && !print_to_file)
			ImGui::Selectable(printer_label.c_str(), true);
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	ShowHelpMarker("Write Amiga parallel printer output to numbered Print_###.dat files in the selected folder");

	std::string printer_path;
	if (ConsumeDirDialogResultKey(PRINTER_FILE_DIALOG_KEY, printer_path) && !printer_path.empty())
		put("amiberry.parallel_port:folder", printer_path);
	ImGui::Spacing();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Sampler:"); ImGui::SameLine();
	combo("amiberry.samplersoundcard", "##Sampler");
	ShowHelpMarker("Audio sampling device connected to the parallel port");
	checkbox("sampler_stereo", "Stereo sampler");
	ShowHelpMarker("Enable stereo audio sampling instead of mono");
	ImGui::Spacing();
	EndGroupBox("Parallel Port");

	// ---------------------------------------------------------
	// Serial Port
	// ---------------------------------------------------------
	BeginGroupBox("Serial Port");

	combo("serial_port", "##SerialPort");
	ShowHelpMarker("Map Amiga's RS-232 serial port to host serial port");

	const float table_w = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2;
	if (ImGui::BeginTable("SerialOptsTable", 4, ImGuiTableFlags_None, ImVec2(table_w, 0))) {
		ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthStretch, 1.5f);
		ImGui::TableSetupColumn("column3", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("column4", ImGuiTableColumnFlags_WidthStretch, 1.5f);
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); checkbox("serial_on_demand", "Shared");
		ShowHelpMarker("Allow host and emulated Amiga to share the serial port");
		ImGui::TableNextColumn(); checkbox("serial_hardware_ctsrts", "Host RTS/CTS");
		ShowHelpMarker("Use hardware flow control from the host serial port");
		ImGui::TableNextColumn(); checkbox("serial_direct", "Direct");
		ShowHelpMarker("Direct serial port access bypassing emulation layer");
		ImGui::TableNextColumn(); checkbox("uaeserial", "uaeserial.device");
		ShowHelpMarker("Enable UAE's custom serial device driver");
		ImGui::EndTable();
	}

	if (ImGui::BeginTable("SerialStatusTable", 2, ImGuiTableFlags_None, ImVec2(table_w, 0))) {
		ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); checkbox("serial_status", "Serial status (RTS/CTS/...)");
		ShowHelpMarker("Emulate serial port status lines (RTS, CTS, DTR, DSR, CD)");
		ImGui::TableNextColumn(); checkbox("serial_ri", "Serial status: Ring Indicator");
		ShowHelpMarker("Emulate the RI serial line used by modems");
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		bool serial_crlf = value_of("serial_translate") == "crlf_cr";
		ImGui::BeginDisabled(!available("serial_translate"));
		if (AmigaCheckbox("CR/LF conversion", &serial_crlf))
			put("serial_translate", serial_crlf ? "crlf_cr" : "disabled");
		ImGui::EndDisabled();
		ShowHelpMarker("Translate serial line endings between Amiga and host CR/LF conventions");
		ImGui::EndTable();
	}
	ImGui::Spacing();
	EndGroupBox("Serial Port");

	// ---------------------------------------------------------
	// MIDI
	// ---------------------------------------------------------
	BeginGroupBox("MIDI");
	const float midi_table_w = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2;
	if (ImGui::BeginTable("MidiTable", 2, ImGuiTableFlags_None, ImVec2(midi_table_w, 0))) {
		ImGui::TableSetupColumn("column1", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("column2", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Out:"); ImGui::SameLine();
		combo("amiberry.midiout_device_name", "##MidiOut");
		ShowHelpMarker("Connect Amiga MIDI output to host MIDI device");
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("In:"); ImGui::SameLine();
		combo("amiberry.midiin_device_name", "##MidiIn");
		ShowHelpMarker("Connect host MIDI input to Amiga MIDI");
		ImGui::EndTable();
	}
	ImGui::Spacing();
	checkbox("amiberry.midirouter", "Route MIDI In to MIDI Out");
	ShowHelpMarker("Pass MIDI input directly through to MIDI output");
	ImGui::Spacing();
	EndGroupBox("MIDI");

	// ---------------------------------------------------------
	// Protection Dongle
	// ---------------------------------------------------------
	BeginGroupBox("Protection Dongle");
	combo("dongle", "##Dongle");
	ShowHelpMarker("Emulate copy protection dongles for specific games");
	ImGui::Spacing();
	EndGroupBox("Protection Dongle");
}
