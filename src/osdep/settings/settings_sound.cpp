// Sound panel options: audio device, emulation mode, volumes, mixer
// settings and the floppy drive click emulation.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include "sounddep/sound.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

bool sound_enabled(const uae_prefs& p)
{
	return p.produce_sound > 0;
}

bool stereo_enabled(const uae_prefs& p)
{
	return sound_enabled(p) && p.sound_stereo > 0;
}

// ------------------------------------------------------------- audio device

const char* sound_device_type_name(int type)
{
	switch (type) {
	case SOUND_DEVICE_SDL: return "SDL";
	case SOUND_DEVICE_DS: return "DSOUND";
	case SOUND_DEVICE_AL: return "OpenAL";
	case SOUND_DEVICE_PA: return "PortAudio";
	case SOUND_DEVICE_WASAPI: return "WASAPI";
	default: return "WASAPI EX";
	}
}

std::vector<std::string> soundcard_choice_storage;
std::vector<std::string> soundcard_choice_values;
std::vector<amiberry_setting_choice> soundcard_choice_items;
bool soundcard_choices_valid = false;

void build_soundcard_choices()
{
	soundcard_choice_storage.clear();
	soundcard_choice_values.clear();
	soundcard_choice_items.clear();
	const int numdevs = enumerate_sound_devices();
	for (int i = 0; i < numdevs && i < MAX_SOUND_DEVICES; i++) {
		if (!sound_devices[i])
			continue;
		soundcard_choice_storage.push_back(std::string(sound_device_type_name(
			sound_devices[i]->type)) + ": " + sound_devices[i]->name);
		soundcard_choice_values.push_back(std::to_string(soundcard_choice_values.size()));
	}
	// Build the items only after every value and label has its final
	// storage, so the pointers stay valid.
	for (size_t i = 0; i < soundcard_choice_storage.size(); i++) {
		soundcard_choice_items.push_back({soundcard_choice_values[i].c_str(),
			soundcard_choice_storage[i].c_str()});
	}
	soundcard_choices_valid = true;
}

void soundcard_dynamic_choices(const uae_prefs&, choice_list& out)
{
	// Enumerated once, like the panel always did; refreshed when a front
	// end asks for it after a device change.
	if (!soundcard_choices_valid)
		build_soundcard_choices();
	out.items = soundcard_choice_items.data();
	out.count = soundcard_choice_items.size();
}

// The device a configuration selects, by name; empty for none.
std::string soundcard_name(int card)
{
	if (card < 0 || card >= MAX_SOUND_DEVICES || !sound_devices[card] || !sound_devices[card]->cfgname)
		return {};
	return sound_devices[card]->cfgname;
}

struct session_selection {
	uae_prefs* prefs;
	std::string device;
	std::string sampler; // the parallel port sampler's audio input, if any
};

// The sampler's input a configuration selects, by name; empty for none.
std::string sampler_name(int card)
{
	if (card < 0 || card >= MAX_SOUND_DEVICES || !record_devices[card] || !record_devices[card]->cfgname)
		return {};
	return record_devices[card]->cfgname;
}

void refresh_soundcard_choices()
{
	// Re-read the host device list: enumerate_sound_devices() only
	// queries while its table is empty, so the refresh clears it first.
	// Without this, a device connected after start-up stayed invisible
	// until the next restart. refresh_sound_devices() finds the pending
	// and current configurations' devices again by name; open sessions
	// select by index too, so theirs are found again the same way (a
	// device that is gone falls back to the first one, as there).
	std::vector<session_selection> selections;
	for_each_session_config([](uae_prefs& p, void* context) {
		static_cast<std::vector<session_selection>*>(context)->push_back(
			{&p, soundcard_name(p.soundcard), sampler_name(p.samplersoundcard)});
	}, &selections);
	refresh_sound_devices();
	for (const session_selection& s : selections) {
		// The sampler's input too, by name; one that is gone is no sampler.
		if (!s.sampler.empty()) {
			int input = -1;
			for (int i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++) {
				if (record_devices[i]->cfgname && s.sampler == record_devices[i]->cfgname) {
					input = i;
					break;
				}
			}
			s.prefs->samplersoundcard = input;
		}
		if (s.device.empty())
			continue;
		int card = 0;
		for (int i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
			if (sound_devices[i]->cfgname && s.device == sound_devices[i]->cfgname) {
				card = i;
				break;
			}
		}
		s.prefs->soundcard = card;
	}
	build_soundcard_choices();
}

// The device count of the current list, enumerating the host's devices the
// first time, so availability and the range hold before any front end has
// listed the choices.
int soundcard_count()
{
	if (!soundcard_choices_valid)
		build_soundcard_choices();
	return static_cast<int>(soundcard_choice_items.size());
}

void soundcard_range(const uae_prefs&, double& min, double& max)
{
	min = 0;
	max = std::max(0, soundcard_count() - 1);
}

bool soundcard_available(const uae_prefs& p, const context&)
{
	return !p.soundcard_default && sound_enabled(p) && soundcard_count() > 1;
}

void get_soundcard(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.soundcard);
}

void set_soundcard(uae_prefs& p, const value& v)
{
	p.soundcard = static_cast<int>(v.number);
}

// A device other than the first one does not survive a lagless vsync mode
// (see soundcard_reset_by_vsync).
bool soundcard_accepts(const uae_prefs& p, const value& v)
{
	const int card = static_cast<int>(v.number);
	return card == 0 || !soundcard_reset_by_vsync(p, card);
}

void get_soundcard_default(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.soundcard_default);
}

void set_soundcard_default(uae_prefs& p, const value& v)
{
	p.soundcard_default = v.number != 0;
}

// --------------------------------------------------------- sound emulation

const amiberry_setting_choice sound_output_choices[] = {
	{"none", "Disabled"},
	{"interrupts", "Disabled, but emulated"},
	{"normal", "Enabled"},
	{"exact", "Enabled (exact)"},
};

void get_sound_output(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(sound_output_choices), p.produce_sound);
}

// Turning sound on makes target_fixup_options() check the device against
// the lagless vsync mode, so the setter makes the reset it would.
void set_sound_output(uae_prefs& p, const value& v)
{
	p.produce_sound = choice_index(CHOICES(sound_output_choices), v.text);
	clear_soundcard_for_vsync(p);
}

// Cycle-exact timing needs Paula emulation, so fixup_prefs() raises a
// disabled sound setting to "interrupts"; offering "none" there would
// hand the user a value the core immediately changes.
bool sound_output_choice_available(const uae_prefs& p, std::string_view choice)
{
	return choice != "none" || !p.cpu_memory_cycle_exact;
}

void get_sound_auto(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.sound_auto);
}

void set_sound_auto(uae_prefs& p, const value& v)
{
	p.sound_auto = v.number != 0;
}

bool sound_option_available(const uae_prefs& p, const context&)
{
	return sound_enabled(p);
}

bool stereo_option_available(const uae_prefs& p, const context&)
{
	return stereo_enabled(p);
}

// ----------------------------------------------------------------- volumes

void get_sound_volume(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.sound_volume_master);
}

void set_sound_volume(uae_prefs& p, const value& v)
{
	p.sound_volume_master = static_cast<int>(v.number);
}

#define VOLUME_OPTION(field, key, label, help) \
	void get_##key(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, p.field); \
	} \
	void set_##key(uae_prefs& p, const value& v) \
	{ \
		p.field = static_cast<int>(v.number); \
	}

VOLUME_OPTION(sound_volume_paula, sound_volume_paula, "Paula volume",
	"Paula (Amiga sound chip) volume.")
VOLUME_OPTION(sound_volume_cd, sound_volume_cd, "CD volume", "CD audio volume.")
VOLUME_OPTION(sound_volume_board, sound_volume_ahi, "AHI volume",
	"Retargetable (AHI) audio volume.")
VOLUME_OPTION(sound_volume_midi, sound_volume_midi, "MIDI volume", "MIDI volume.")
#undef VOLUME_OPTION

// -------------------------------------------------------------- mixer setup

const amiberry_setting_choice channel_choices[] = {
	{"mono", "Mono"},
	{"stereo", "Stereo"},
	{"clonedstereo", "Cloned stereo (4 ch)"},
	{"4ch", "4 channels"},
	{"clonedstereo6ch", "Cloned stereo (5.1)"},
	{"6ch", "5.1 channels"},
	{"clonedstereo8ch", "Cloned stereo (7.1)"},
	{"8ch", "7.1 channels"},
};

void get_sound_channels(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(channel_choices), p.sound_stereo);
}

void set_sound_channels(uae_prefs& p, const value& v)
{
	p.sound_stereo = choice_index(CHOICES(channel_choices), v.text);
}

// Stored as tenths of full separation, listed from 100% down like the
// panel's combo.
const amiberry_setting_choice separation_choices[] = {
	{"10", "100%"}, {"9", "90%"}, {"8", "80%"}, {"7", "70%"}, {"6", "60%"},
	{"5", "50%"}, {"4", "40%"}, {"3", "30%"}, {"2", "20%"}, {"1", "10%"}, {"0", "0%"},
};

void get_sound_stereo_separation(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.sound_stereo_separation);
}

void set_sound_stereo_separation(uae_prefs& p, const value& v)
{
	p.sound_stereo_separation = static_cast<int>(v.number);
}

const amiberry_setting_choice interpol_choices[] = {
	{"none", "Disabled"},
	{"anti", "Anti"},
	{"sinc", "Sinc"},
	{"rh", "RH"},
	{"crux", "Crux"},
};

void get_sound_interpol(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(interpol_choices), p.sound_interpol);
}

void set_sound_interpol(uae_prefs& p, const value& v)
{
	p.sound_interpol = choice_index(CHOICES(interpol_choices), v.text);
}

const amiberry_setting_choice frequency_choices[] = {
	{"11025", "11,025 Hz"},
	{"22050", "22,050 Hz"},
	{"32000", "32,000 Hz"},
	{"44100", "44,100 Hz"},
	{"48000", "48,000 Hz"},
};

void get_sound_frequency(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.sound_freq);
}

void set_sound_frequency(uae_prefs& p, const value& v)
{
	p.sound_freq = static_cast<int>(v.number);
}

void get_sound_stereo_swap_paula(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.sound_stereo_swap_paula);
}

void set_sound_stereo_swap_paula(uae_prefs& p, const value& v)
{
	p.sound_stereo_swap_paula = v.number != 0;
}

void get_sound_stereo_swap_ahi(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.sound_stereo_swap_ahi);
}

void set_sound_stereo_swap_ahi(uae_prefs& p, const value& v)
{
	p.sound_stereo_swap_ahi = v.number != 0;
}

void get_sound_stereo_mixing_delay(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.sound_mixed_stereo_delay);
}

void set_sound_stereo_mixing_delay(uae_prefs& p, const value& v)
{
	p.sound_mixed_stereo_delay = static_cast<int>(v.number);
}

// --------------------------------------------------------------- A500 filter

// "fixedonly" is a spelling the writer's table has for value 3; Amiberry's
// audio has no such filter mode, so the value is reported (a configuration
// can hold it) but not offered.
const amiberry_setting_choice filter_choices[] = {
	{"off", "Always off"},
	{"emulated", "Emulated"},
	{"on", "Always on"},
	{"fixedonly", "Fixed only (unsupported)"},
};

bool filter_choice_available(const uae_prefs&, std::string_view choice)
{
	return choice != "fixedonly";
}

void get_sound_filter(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(filter_choices), p.sound_filter);
}

void set_sound_filter(uae_prefs& p, const value& v)
{
	p.sound_filter = choice_index(CHOICES(filter_choices), v.text);
}

const amiberry_setting_choice filter_type_choices[] = {
	{"standard", "A500"},
	{"enhanced", "A1200"},
	{"fixedonly", "Fixed only"},
};

void get_sound_filter_type(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(filter_type_choices), p.sound_filter_type);
}

void set_sound_filter_type(uae_prefs& p, const value& v)
{
	p.sound_filter_type = choice_index(CHOICES(filter_type_choices), v.text);
}

// ------------------------------------------------------------ drive clicks

// 0 is off and 1 the built-in A500 samples. External samples (-1) also
// need a sample set name no option sets, so a configuration can hold them
// but they are not offered.
const amiberry_setting_choice floppy_sound_choices[] = {
	{"0", "Off"},
	{"1", "A500 drive"},
	{"-1", "External samples"},
};

bool floppy_sound_choice_available(const uae_prefs&, std::string_view choice)
{
	return choice != "-1";
}

void get_floppy_sound(int slot, const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.floppyslots[slot].dfxclick);
}

void set_floppy_sound(int slot, uae_prefs& p, const value& v)
{
	p.floppyslots[slot].dfxclick = static_cast<int>(v.number);
}

// A drive's click volumes only matter while its drive sound is on.
bool floppy_volume_available(int slot, const uae_prefs& p)
{
	return sound_enabled(p) && p.floppyslots[slot].dfxclick != 0;
}

#define FLOPPY_SOUND(n) \
	void get_floppy##n##sound(const uae_prefs& p, char* out, size_t size) \
	{ \
		get_floppy_sound(n, p, out, size); \
	} \
	void set_floppy##n##sound(uae_prefs& p, const value& v) \
	{ \
		set_floppy_sound(n, p, v); \
	} \
	bool floppy##n##_volume_available(const uae_prefs& p, const context&) \
	{ \
		return floppy_volume_available(n, p); \
	}

FLOPPY_SOUND(0)
FLOPPY_SOUND(1)
FLOPPY_SOUND(2)
FLOPPY_SOUND(3)
#undef FLOPPY_SOUND

void get_floppy_volume(int slot, bool empty, const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, empty ? p.dfxclickvolume_empty[slot] : p.dfxclickvolume_disk[slot]);
}

void set_floppy_volume(int slot, bool empty, uae_prefs& p, const value& v)
{
	if (empty)
		p.dfxclickvolume_empty[slot] = static_cast<int>(v.number);
	else
		p.dfxclickvolume_disk[slot] = static_cast<int>(v.number);
}

#define FLOPPY_VOLUME(n, empty, suffix) \
	void get_floppy##n##soundvolume_##suffix(const uae_prefs& p, char* out, size_t size) \
	{ \
		get_floppy_volume(n, empty, p, out, size); \
	} \
	void set_floppy##n##soundvolume_##suffix(uae_prefs& p, const value& v) \
	{ \
		set_floppy_volume(n, empty, p, v); \
	}

FLOPPY_VOLUME(0, false, disk)
FLOPPY_VOLUME(1, false, disk)
FLOPPY_VOLUME(2, false, disk)
FLOPPY_VOLUME(3, false, disk)
FLOPPY_VOLUME(0, true, empty)
FLOPPY_VOLUME(1, true, empty)
FLOPPY_VOLUME(2, true, empty)
FLOPPY_VOLUME(3, true, empty)
#undef FLOPPY_VOLUME

// ------------------------------------------------------------- buffer size

const amiberry_setting_choice buffer_choices[] = {
	{"0", "Minimum"},
	{"1024", "1024"},
	{"2048", "2048"},
	{"3072", "3072"},
	{"4096", "4096"},
	{"6144", "6144"},
	{"8192", "8192"},
	{"12288", "12288"},
	{"16384", "16384"},
	{"32768", "32768"},
	{"65536", "65536"},
};

void get_sound_max_buff(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.sound_maxbsiz);
}

void set_sound_max_buff(uae_prefs& p, const value& v)
{
	p.sound_maxbsiz = static_cast<int>(v.number);
}

// The buffer size and frequency a new configuration gets:
// target_default_options() applies the host defaults from amiberry_options
// (src/include/options.h), which Android raises for AAudio.
#ifdef __ANDROID__
constexpr const char* default_sound_buffer = "8192";
constexpr const char* default_sound_frequency = "48000";
#else
constexpr const char* default_sound_buffer = "3072";
constexpr const char* default_sound_frequency = "44100";
#endif

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, nullptr, 0, constraint}
// A number set by feel: a front end may show a slider (amiberry_settings.h).
#define LEVEL_OPTION(id, group, label, help, apply, min, max, step, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_INT, apply, min, max, step, def, nullptr, 0, constraint, \
		false, nullptr, 0, true}
// A volume the configuration stores as its attenuation, 0 (full volume) to
// 100 (silent): reversed, so front ends show 100 - value, as the GUI's
// sliders always have, in percent. Labels and help speak of the shown
// volume.
#define ATTENUATION_OPTION(id, label, help, def, constraint) \
	{id, "sound", label, help, AMIBERRY_SETTING_INT, AMIBERRY_APPLY_LIVE, 0, 100, 1, def, nullptr, 0, \
		constraint, true, "%", 0, true}

const char* const drive_sound_constraint = "Needs sound emulation and the drive sound on.";

const definition definitions[] = {
	{DYNAMIC_CHOICE_OPTION("amiberry.soundcard", "sound", "Audio device",
			"Which host audio device carries the emulator's output.",
			AMIBERRY_APPLY_LIVE, "0",
			"Needs the system default to be off, sound emulation on and more than one device "
			"from the last enumeration."),
		get_soundcard, set_soundcard, soundcard_available, nullptr, soundcard_range,
		soundcard_accepts, soundcard_dynamic_choices},
	{BOOL_OPTION("amiberry.soundcard_default", "sound", "System default",
			"Use the operating system's default audio device.",
			AMIBERRY_APPLY_LIVE, "true", nullptr),
		get_soundcard_default, set_soundcard_default, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("sound_output", "sound", "Sound emulation",
			"None turns all sound off; Emulated keeps Paula timing without output; Exact is "
			"most accurate.",
			AMIBERRY_APPLY_LIVE, "exact", sound_output_choices,
			"Cycle-exact modes need at least emulated sound, so Disabled is not available in them."),
		get_sound_output, set_sound_output, nullptr, sound_output_choice_available, nullptr,
		nullptr},
	{BOOL_OPTION("sound_auto", "sound", "Automatic switching",
			"Automatically adjust the sound emulation quality based on performance.",
			AMIBERRY_APPLY_LIVE, "false", "Needs sound emulation to be on."),
		get_sound_auto, set_sound_auto, sound_option_available, nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("sound_volume", "Master volume",
			"Master output volume; 100% is full volume and 0% silent.", "0", nullptr),
		get_sound_volume, set_sound_volume, nullptr, nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("sound_volume_paula", "Paula volume",
			"Paula (Amiga sound chip) volume; 100% is full volume.", "0",
			"Needs sound emulation to be on."),
		get_sound_volume_paula, set_sound_volume_paula, sound_option_available, nullptr, nullptr,
		nullptr},
	{ATTENUATION_OPTION("sound_volume_cd", "CD volume",
			"CD audio volume; 100% is full volume.", "20", "Needs sound emulation to be on."),
		get_sound_volume_cd, set_sound_volume_cd, sound_option_available, nullptr, nullptr,
		nullptr},
	{ATTENUATION_OPTION("sound_volume_ahi", "AHI volume",
			"Retargetable (AHI) audio volume; 100% is full volume.", "0",
			"Needs sound emulation to be on."),
		get_sound_volume_ahi, set_sound_volume_ahi, sound_option_available, nullptr, nullptr,
		nullptr},
	{ATTENUATION_OPTION("sound_volume_midi", "MIDI volume",
			"MIDI volume; 100% is full volume.", "0", "Needs sound emulation to be on."),
		get_sound_volume_midi, set_sound_volume_midi, sound_option_available, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("sound_channels", "sound", "Channel mode",
			"Stereo, multi-channel and cloned-stereo output modes.",
			AMIBERRY_APPLY_LIVE, "stereo", channel_choices, "Needs sound emulation to be on."),
		get_sound_channels, set_sound_channels, sound_option_available, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("sound_stereo_separation", "sound", "Stereo separation",
			"How far apart the left and right channels are; 0% is mono.",
			AMIBERRY_APPLY_LIVE, "7", separation_choices,
			"Needs a stereo (or multi-channel) output mode."),
		get_sound_stereo_separation, set_sound_stereo_separation, stereo_option_available,
		nullptr, nullptr, nullptr},
	{CHOICE_OPTION("sound_interpol", "sound", "Interpolation",
			"Resampling filter between Paula and the output device.",
			AMIBERRY_APPLY_LIVE, "anti", interpol_choices, "Needs sound emulation to be on."),
		get_sound_interpol, set_sound_interpol, sound_option_available, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("sound_frequency", "sound", "Frequency",
			"Output sample rate; 44100 or 48000 is recommended for modern systems.",
			AMIBERRY_APPLY_LIVE, default_sound_frequency, frequency_choices, "Needs sound emulation to be on."),
		get_sound_frequency, set_sound_frequency, sound_option_available, nullptr, nullptr,
		nullptr},
	{BOOL_OPTION("sound_stereo_swap_paula", "sound", "Swap Paula channels",
			"Swap the left and right Paula channels.",
			AMIBERRY_APPLY_LIVE, "false", "Needs sound emulation to be on."),
		get_sound_stereo_swap_paula, set_sound_stereo_swap_paula, sound_option_available,
		nullptr, nullptr, nullptr},
	{BOOL_OPTION("sound_stereo_swap_ahi", "sound", "Swap AHI channels",
			"Swap the left and right AHI channels.",
			AMIBERRY_APPLY_LIVE, "false", "Needs sound emulation to be on."),
		get_sound_stereo_swap_ahi, set_sound_stereo_swap_ahi, sound_option_available, nullptr,
		nullptr, nullptr},
	{LEVEL_OPTION("sound_stereo_mixing_delay", "sound", "Stereo delay",
			"Delay between the stereo channels for a wider soundstage; 0 disables it.",
			AMIBERRY_APPLY_LIVE, 0, 10, 1, "0",
			"Needs a stereo (or multi-channel) output mode."),
		get_sound_stereo_mixing_delay, set_sound_stereo_mixing_delay, stereo_option_available,
		nullptr, nullptr, nullptr},
	{CHOICE_OPTION("sound_filter", "sound", "Audio filter",
			"The Amiga's hardware low-pass filter.",
			AMIBERRY_APPLY_LIVE, "emulated", filter_choices, "Needs sound emulation to be on."),
		get_sound_filter, set_sound_filter, sound_option_available, filter_choice_available,
		nullptr, nullptr},
	{CHOICE_OPTION("sound_filter_type", "sound", "Audio filter type",
			"The filter curve: sharper A500 or softer A1200.",
			AMIBERRY_APPLY_LIVE, "standard", filter_type_choices,
			"Needs sound emulation to be on."),
		get_sound_filter_type, set_sound_filter_type, sound_option_available, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("floppy0sound", "sound", "DF0 drive sound",
			"Mechanical drive click emulation for DF0 with the built-in A500 drive samples.",
			AMIBERRY_APPLY_LIVE, "0", floppy_sound_choices, "Needs sound emulation to be on."),
		get_floppy0sound, set_floppy0sound, sound_option_available, floppy_sound_choice_available,
		nullptr, nullptr},
	{CHOICE_OPTION("floppy1sound", "sound", "DF1 drive sound",
			"Mechanical drive click emulation for DF1 with the built-in A500 drive samples.",
			AMIBERRY_APPLY_LIVE, "0", floppy_sound_choices, "Needs sound emulation to be on."),
		get_floppy1sound, set_floppy1sound, sound_option_available, floppy_sound_choice_available,
		nullptr, nullptr},
	{CHOICE_OPTION("floppy2sound", "sound", "DF2 drive sound",
			"Mechanical drive click emulation for DF2 with the built-in A500 drive samples.",
			AMIBERRY_APPLY_LIVE, "0", floppy_sound_choices, "Needs sound emulation to be on."),
		get_floppy2sound, set_floppy2sound, sound_option_available, floppy_sound_choice_available,
		nullptr, nullptr},
	{CHOICE_OPTION("floppy3sound", "sound", "DF3 drive sound",
			"Mechanical drive click emulation for DF3 with the built-in A500 drive samples.",
			AMIBERRY_APPLY_LIVE, "0", floppy_sound_choices, "Needs sound emulation to be on."),
		get_floppy3sound, set_floppy3sound, sound_option_available, floppy_sound_choice_available,
		nullptr, nullptr},
	{ATTENUATION_OPTION("floppy0soundvolume_disk", "DF0 disk volume",
			"DF0 click volume with a disk in the drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy0soundvolume_disk, set_floppy0soundvolume_disk, floppy0_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy1soundvolume_disk", "DF1 disk volume",
			"DF1 click volume with a disk in the drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy1soundvolume_disk, set_floppy1soundvolume_disk, floppy1_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy2soundvolume_disk", "DF2 disk volume",
			"DF2 click volume with a disk in the drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy2soundvolume_disk, set_floppy2soundvolume_disk, floppy2_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy3soundvolume_disk", "DF3 disk volume",
			"DF3 click volume with a disk in the drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy3soundvolume_disk, set_floppy3soundvolume_disk, floppy3_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy0soundvolume_empty", "DF0 empty drive volume",
			"DF0 click volume with an empty drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy0soundvolume_empty, set_floppy0soundvolume_empty, floppy0_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy1soundvolume_empty", "DF1 empty drive volume",
			"DF1 click volume with an empty drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy1soundvolume_empty, set_floppy1soundvolume_empty, floppy1_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy2soundvolume_empty", "DF2 empty drive volume",
			"DF2 click volume with an empty drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy2soundvolume_empty, set_floppy2soundvolume_empty, floppy2_volume_available,
		nullptr, nullptr, nullptr},
	{ATTENUATION_OPTION("floppy3soundvolume_empty", "DF3 empty drive volume",
			"DF3 click volume with an empty drive; 100% is full volume.", "33", drive_sound_constraint),
		get_floppy3soundvolume_empty, set_floppy3soundvolume_empty, floppy3_volume_available,
		nullptr, nullptr, nullptr},
	{CHOICE_OPTION("sound_max_buff", "sound", "Buffer size",
			"Audio buffer size; smaller values reduce latency but may glitch on slow systems.",
			AMIBERRY_APPLY_LIVE, default_sound_buffer, buffer_choices, "Needs sound emulation to be on."),
		get_sound_max_buff, set_sound_max_buff, sound_option_available, nullptr, nullptr,
		nullptr},
};

bool commit_sound(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(soundcard);
	FIELD(soundcard_default);
	FIELD(produce_sound);
	FIELD(sound_auto);
	FIELD(sound_volume_master);
	FIELD(sound_volume_paula);
	FIELD(sound_volume_cd);
	FIELD(sound_volume_board);
	FIELD(sound_volume_midi);
	FIELD(sound_stereo);
	FIELD(sound_stereo_separation);
	FIELD(sound_mixed_stereo_delay);
	FIELD(sound_interpol);
	FIELD(sound_freq);
	FIELD(sound_maxbsiz);
	FIELD(sound_filter);
	FIELD(sound_filter_type);
	FIELD(sound_stereo_swap_paula);
	FIELD(sound_stereo_swap_ahi);
	// One option per drive, merged one by one.
	for (int i = 0; i < 4; i++) {
		FIELD(dfxclickvolume_disk[i]);
		FIELD(dfxclickvolume_empty[i]);
	}
#undef FIELD
	for (int i = 0; i < 4; i++)
		wrote |= commit_field(base.floppyslots[i].dfxclick, from.floppyslots[i].dfxclick,
			to.floppyslots[i].dfxclick);
	return wrote;
}

#undef BOOL_OPTION
#undef CHOICE_OPTION
#undef DYNAMIC_CHOICE_OPTION
#undef LEVEL_OPTION
#undef ATTENUATION_OPTION
#undef CHOICES

const domain sound = {
	definitions, sizeof definitions / sizeof definitions[0], commit_sound,
	refresh_soundcard_choices,
};

} // namespace

const domain& sound_domain()
{
	return sound;
}

namespace {

// The vsync mode fixup_prefs_dimensions() leaves for one screen: RTG vsync
// always turns into the lagless mode, and vsync in a window turns both off.
bool lagless_vsync(const struct apmode& ap, bool rtg)
{
	const bool mode = ap.gfx_vsyncmode != 0 || (rtg && ap.gfx_vsync > 0);
	if (ap.gfx_vsync && ap.gfx_fullscreen == GFX_WINDOW)
		return false;
	return mode;
}

} // namespace

// target_fixup_options() puts an SDL sound device back to the first one
// while either screen uses the lagless vsync mode (every Amiberry device is
// an SDL one).
bool soundcard_reset_by_vsync(const uae_prefs& p, int card)
{
	if (!p.produce_sound || card < 0 || card >= MAX_SOUND_DEVICES || !sound_devices[card]
		|| sound_devices[card]->type != SOUND_DEVICE_SDL)
		return false;
	return lagless_vsync(p.gfx_apmode[APMODE_NATIVE], false)
		|| lagless_vsync(p.gfx_apmode[APMODE_RTG], true);
}

void clear_soundcard_for_vsync(uae_prefs& p)
{
	if (soundcard_reset_by_vsync(p, p.soundcard))
		p.soundcard = 0;
}

} // namespace amiberry_settings
