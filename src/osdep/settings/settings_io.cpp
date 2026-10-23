// IO port options: the GUI's IO Ports panel. The parallel port (a printer,
// which can print to files, or an audio sampler), the serial port and how
// it behaves, MIDI in and out, and a game's protection dongle.
//
// The host lists (serial ports, MIDI devices, audio inputs) are dynamic
// choices, enumerated at start-up and again on a refresh.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "parser.h"
#include "sounddep/sound.h"
#include "target.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

void copy_text(TCHAR* field, size_t size, std::string_view text)
{
	std::snprintf(field, size, "%.*s", static_cast<int>(text.size()), text.data());
}

// A dynamic choice list over a host list, "none" first: values and labels
// kept until the next call of the same hook.
struct host_choices {
	std::vector<std::string> values;
	std::vector<std::string> labels;
	std::vector<amiberry_setting_choice> items;

	void reset()
	{
		values.clear();
		labels.clear();
		items.clear();
	}
	void add(std::string value, std::string label)
	{
		values.push_back(std::move(value));
		labels.push_back(std::move(label));
	}
	void publish(choice_list& out)
	{
		items.clear();
		for (size_t i = 0; i < values.size(); i++)
			items.push_back({values[i].c_str(), labels[i].c_str()});
		out.items = items.data();
		out.count = items.size();
	}
};

// ------------------------------------------------------------- printer

constexpr char print_to_file[] = "FILE:";

bool is_print_to_file(const uae_prefs& p)
{
	return !_tcsnicmp(p.prtname, print_to_file, 5);
}

// As the configuration spells it: "none", a printer, or "FILE:<folder>"
// (numbered Print_###.dat files in the folder).
void get_parallel_port(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.prtname[0] ? p.prtname : "none");
}

// The parallel port has the printer or the sampler.
void set_printer(uae_prefs& p, std::string_view text)
{
	if (text.empty() || text == "none") {
		p.prtname[0] = 0;
		return;
	}
	copy_text(p.prtname, sizeof p.prtname, text);
	p.samplersoundcard = -1;
}

void set_parallel_port(uae_prefs& p, const value& v)
{
	set_printer(p, v.text);
}

bool parallel_port_accepts(const uae_prefs& p, const value& v)
{
	return v.text.size() < sizeof p.prtname;
}

// The folder of print-to-file, "" otherwise.
void get_print_folder(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", is_print_to_file(p) ? p.prtname + 5 : "");
}

void set_print_folder(uae_prefs& p, const value& v)
{
	if (v.text.empty())
		set_printer(p, "");
	else
		set_printer(p, std::string(print_to_file) + std::string(v.text));
}

bool print_folder_accepts(const uae_prefs& p, const value& v)
{
	return v.text.size() + 5 < sizeof p.prtname;
}

// ------------------------------------------------------------- sampler

host_choices sampler;

// The host's audio inputs, by their index in record_devices[] as the
// configuration stores it; "-1" for none.
void sampler_choices(const uae_prefs&, choice_list& out)
{
	sampler.reset();
	sampler.add("-1", "None");
	enumerate_sound_devices();
	for (int i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++)
		sampler.add(std::to_string(i), record_devices[i]->name ? record_devices[i]->name : "");
	sampler.publish(out);
}

void get_sampler(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.samplersoundcard >= 0 ? p.samplersoundcard : -1);
}

void set_sampler(uae_prefs& p, const value& v)
{
	p.samplersoundcard = static_cast<int>(v.number);
	if (p.samplersoundcard >= 0)
		p.prtname[0] = 0;
}

bool sampler_selected(const uae_prefs& p, const context&)
{
	return p.samplersoundcard >= 0;
}

void get_sampler_stereo(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.sampler_stereo);
}

void set_sampler_stereo(uae_prefs& p, const value& v)
{
	p.sampler_stereo = v.number != 0;
}

// --------------------------------------------------------------- serial

host_choices serial;

void serial_choices(const uae_prefs&, choice_list& out)
{
	serial.reset();
	serial.add("none", "None");
	for (const std::string& port : serial_ports)
		serial.add(port, port);
	serial.publish(out);
}

// The configuration has no serial_port line without a port.
void get_serial_port(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.sername[0] ? p.sername : "none");
}

void set_serial_port(uae_prefs& p, const value& v)
{
	if (v.text == "none") {
		p.sername[0] = 0;
		p.use_serial = false;
	} else {
		copy_text(p.sername, sizeof p.sername, v.text);
		p.use_serial = true;
	}
}

bool serial_selected(const uae_prefs& p, const context&)
{
	return p.sername[0] != 0;
}

#define SERIAL_BOOL(name, field) \
	void get_##name(const uae_prefs& p, char* out, size_t size) { write_bool(out, size, p.field); } \
	void set_##name(uae_prefs& p, const value& v) { p.field = v.number != 0; }

SERIAL_BOOL(serial_on_demand, serial_demand)
SERIAL_BOOL(serial_hardware_ctsrts, serial_hwctsrts)
SERIAL_BOOL(serial_status, serial_rtsctsdtrdtecd)
SERIAL_BOOL(serial_ri, serial_ri)
SERIAL_BOOL(serial_direct, serial_direct)
SERIAL_BOOL(uaeserial, uaeserial)
#undef SERIAL_BOOL

const amiberry_setting_choice serial_translate_choices[] = {
	{"disabled", "No conversion"},
	{"crlf_cr", "CR/LF to CR"},
};

void get_serial_translate(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", serial_translate_choices[p.serial_crlf == 1 ? 1 : 0].value);
}

void set_serial_translate(uae_prefs& p, const value& v)
{
	p.serial_crlf = v.text == "crlf_cr" ? 1 : 0;
}

// ----------------------------------------------------------------- MIDI

host_choices midi_out;
host_choices midi_in;

void midi_out_choices(const uae_prefs&, choice_list& out)
{
	midi_out.reset();
	midi_out.add("none", "None");
	for (const std::string& port : midi_out_ports)
		midi_out.add(port, port);
	midi_out.publish(out);
}

void midi_in_choices(const uae_prefs&, choice_list& out)
{
	midi_in.reset();
	midi_in.add("none", "None");
	for (const std::string& port : midi_in_ports)
		midi_in.add(port, port);
	midi_in.publish(out);
}

void get_midi_out(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.midioutdev[0] ? p.midioutdev : "none");
}

// MIDI in goes with MIDI out (the panel cleared it with it).
void set_midi_out(uae_prefs& p, const value& v)
{
	if (v.text == "none") {
		p.midioutdev[0] = 0;
		p.midiindev[0] = 0;
	} else {
		copy_text(p.midioutdev, sizeof p.midioutdev, v.text);
	}
}

bool midi_out_selected(const uae_prefs& p, const context&)
{
	return p.midioutdev[0] != 0;
}

void get_midi_in(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.midiindev[0] ? p.midiindev : "none");
}

void set_midi_in(uae_prefs& p, const value& v)
{
	if (v.text == "none")
		p.midiindev[0] = 0;
	else
		copy_text(p.midiindev, sizeof p.midiindev, v.text);
}

bool midi_in_selected(const uae_prefs& p, const context&)
{
	return p.midiindev[0] != 0;
}

void get_midirouter(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.midirouter);
}

void set_midirouter(uae_prefs& p, const value& v)
{
	p.midirouter = v.number != 0;
}

// --------------------------------------------------------------- dongle

const amiberry_setting_choice dongle_choices[] = {
	{"none", "None"},
	{"robocop 3", "RoboCop 3"},
	{"leaderboard", "Leader Board"},
	{"b.a.t. ii", "B.A.T. II"},
	{"italy'90 soccer", "Italy '90 Soccer"},
	{"dames grand maitre", "Dames Grand-Maitre"},
	{"rugby coach", "Rugby Coach"},
	{"cricket captain", "Cricket Captain"},
	{"leviathan", "Leviathan"},
	{"musicmaster", "Music Master"},
	{"logistics", "Logistics/SuperBase"},
	{"scala red", "Scala MM (Red)"},
	{"scala green", "Scala MM (Green)"},
	{"strikermanager", "Striker Manager"},
	{"multi-player soccer manager", "Multi-player Soccer Manager"},
	{"football director 2", "Football Director 2"},
};

void get_dongle(const uae_prefs& p, char* out, size_t size)
{
	const size_t count = sizeof dongle_choices / sizeof dongle_choices[0];
	if (p.dongle >= 0 && static_cast<size_t>(p.dongle) < count)
		std::snprintf(out, size, "%s", dongle_choices[p.dongle].value);
	else
		write_int(out, size, p.dongle); // as the writer spells one it has no name for
}

void set_dongle(uae_prefs& p, const value& v)
{
	for (size_t i = 0; i < sizeof dongle_choices / sizeof dongle_choices[0]; i++) {
		if (v.text == dongle_choices[i].value)
			p.dongle = static_cast<int>(i);
	}
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define PATH_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_PATH, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define SERIAL_OPTION(id, label, help, apply, def, name) \
	{BOOL_OPTION(id, "io", label, help, apply, def, "Only with a serial port."), \
		get_##name, set_##name, serial_selected, nullptr, nullptr}

const definition definitions[] = {
	{STRING_OPTION("amiberry.parallel_port", "io", "Printer",
			"The parallel port printer: none, a host printer, or \"FILE:<folder>\" to print to numbered Print_###.dat files.",
			AMIBERRY_APPLY_LIVE, "none", "The parallel port has the printer or the sampler."),
		get_parallel_port, set_parallel_port, nullptr, nullptr, nullptr, parallel_port_accepts},
	{PATH_OPTION("amiberry.parallel_port:folder", "io", "Print to folder",
			"Print to numbered Print_###.dat files in this folder; empty for no printer.",
			AMIBERRY_APPLY_LIVE, "", "The parallel port has the printer or the sampler."),
		get_print_folder, set_print_folder, nullptr, nullptr, nullptr, print_folder_accepts},
	{DYNAMIC_CHOICE_OPTION("amiberry.samplersoundcard", "io", "Sampler",
			"An audio input the Amiga samples through the parallel port.",
			AMIBERRY_APPLY_LIVE, "-1", "The parallel port has the printer or the sampler."),
		get_sampler, set_sampler, nullptr, nullptr, nullptr, nullptr, sampler_choices},
	{BOOL_OPTION("sampler_stereo", "io", "Stereo sampler",
			"Sample in stereo instead of mono.",
			AMIBERRY_APPLY_LIVE, "false", "Only with a sampler."),
		get_sampler_stereo, set_sampler_stereo, sampler_selected, nullptr, nullptr},
	{DYNAMIC_CHOICE_OPTION("serial_port", "io", "Serial port",
			"The host serial port (or network connection) the Amiga's RS-232 port goes to.",
			AMIBERRY_APPLY_LIVE, "none", nullptr),
		get_serial_port, set_serial_port, nullptr, nullptr, nullptr, nullptr, serial_choices},
	SERIAL_OPTION("serial_on_demand", "Shared", "Open the host port only while the Amiga uses it, so others can share it.",
		AMIBERRY_APPLY_LIVE, "false", serial_on_demand),
	SERIAL_OPTION("serial_hardware_ctsrts", "Host RTS/CTS", "Hardware flow control on the host port.",
		AMIBERRY_APPLY_LIVE, "true", serial_hardware_ctsrts),
	SERIAL_OPTION("serial_status", "Serial status (RTS/CTS/DTR/DSR/CD)", "Pass the status lines through.",
		AMIBERRY_APPLY_RESTART, "true", serial_status),
	SERIAL_OPTION("serial_ri", "Ring indicator", "Pass the ring indicator line through (for modems).",
		AMIBERRY_APPLY_RESTART, "false", serial_ri),
	SERIAL_OPTION("serial_direct", "Direct", "Bypass the emulated serial timing.", AMIBERRY_APPLY_LIVE, "false", serial_direct),
	{CHOICE_OPTION("serial_translate", "io", "Line ending conversion",
			"Convert the line endings the Amiga sends.",
			AMIBERRY_APPLY_RESTART, "disabled", serial_translate_choices, "Only with a serial port."),
		get_serial_translate, set_serial_translate, serial_selected, nullptr, nullptr},
	SERIAL_OPTION("uaeserial", "uaeserial.device", "A serial device for Amiga programs on the host's ports.",
		AMIBERRY_APPLY_RESTART, "false", uaeserial),
	{DYNAMIC_CHOICE_OPTION("amiberry.midiout_device_name", "io", "MIDI out",
			"The host MIDI device the Amiga's MIDI out (through the serial port) plays on.",
			AMIBERRY_APPLY_LIVE, "none", nullptr),
		get_midi_out, set_midi_out, nullptr, nullptr, nullptr, nullptr, midi_out_choices},
	{DYNAMIC_CHOICE_OPTION("amiberry.midiin_device_name", "io", "MIDI in",
			"The host MIDI device the Amiga receives MIDI from.",
			AMIBERRY_APPLY_LIVE, "none", "Only with MIDI out."),
		get_midi_in, set_midi_in, midi_out_selected, nullptr, nullptr, nullptr, midi_in_choices},
	{BOOL_OPTION("amiberry.midirouter", "io", "Route MIDI in to MIDI out",
			"Pass MIDI input straight through to the MIDI output.",
			AMIBERRY_APPLY_LIVE, "false", "Only with MIDI in."),
		get_midirouter, set_midirouter, midi_in_selected, nullptr, nullptr},
	{CHOICE_OPTION("dongle", "io", "Protection dongle",
			"The copy protection dongle a game checks for.",
			AMIBERRY_APPLY_LIVE, "none", dongle_choices, nullptr),
		get_dongle, set_dongle, nullptr, nullptr, nullptr},
};

#undef SERIAL_OPTION

bool commit_io(const uae_prefs& base, const uae_prefs& from, uae_prefs& to, const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(prtname);
	FIELD(samplersoundcard);
	FIELD(sampler_stereo);
	FIELD(sername);
	FIELD(use_serial);
	FIELD(serial_demand);
	FIELD(serial_hwctsrts);
	FIELD(serial_rtsctsdtrdtecd);
	FIELD(serial_ri);
	FIELD(serial_direct);
	FIELD(serial_crlf);
	FIELD(uaeserial);
	FIELD(midioutdev);
	FIELD(midiindev);
	FIELD(midirouter);
	FIELD(dongle);
#undef FIELD
	return wrote;
}

// The host's ports and MIDI devices, listed again (audio inputs are
// listed with the audio devices).
void refresh_io_choices()
{
	enumserialports();
	enummidiports();
}

const domain io = {
	definitions, sizeof definitions / sizeof definitions[0], commit_io, refresh_io_choices,
};

} // namespace

const domain& io_domain()
{
	return io;
}

} // namespace amiberry_settings
