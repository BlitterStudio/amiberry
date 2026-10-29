// Input panel options: joyport devices, their modes and autofire, and
// the mouse, tablet and game controller settings.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "target.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

// ------------------------------------------------------- port device choices

struct port_device {
	int id;
	std::string token;  // .uae spelling
	std::string label;
};

std::vector<port_device> port_devices;
bool port_devices_valid = false;

// The keyboard layouts and RetroArch joysticks the input system offers on
// every host, in the order the panel lists them.
struct static_port_device {
	int id;
	const char* label;
};

constexpr static_port_device static_port_devices[] = {
	{JPORT_NONE, "None"},
	{JSEM_KBDLAYOUT, "Keyboard Layout A (Numpad, 0/5=Fire, Decimal/DEL=2nd Fire)"},
	{JSEM_KBDLAYOUT + 1, "Keyboard Layout B (Cursor, RCtrl/RAlt=Fire, RShift=2nd Fire)"},
	{JSEM_KBDLAYOUT + 2, "Keyboard Layout C (WSAD, LAlt=Fire, LShift=2nd Fire)"},
	{JSEM_KBDLAYOUT + 8, "Keyboard Layout D (Cursor, LCtrl/LAlt=Fire, LShift=2nd Fire)"},
	{JSEM_KBDLAYOUT + 3, "Keyrah Layout (Cursor, Space/RAlt=Fire, RShift=2nd Fire)"},
	{JSEM_KBDLAYOUT + 4, "RetroArch keyboard as joystick (player 1)"},
	{JSEM_KBDLAYOUT + 5, "RetroArch keyboard as joystick (player 2)"},
	{JSEM_KBDLAYOUT + 6, "RetroArch keyboard as joystick (player 3)"},
	{JSEM_KBDLAYOUT + 7, "RetroArch keyboard as joystick (player 4)"},
};

// The token the configuration writer emits for a port device id, the same
// mapping inputdevice_joyport_config() parses.
std::string port_device_token(int id)
{
	char buffer[32];
	if (id < 0)
		std::snprintf(buffer, sizeof buffer, "none");
	else if (id < JSEM_CUSTOM)
		std::snprintf(buffer, sizeof buffer, "kbd%d", id - JSEM_KBDLAYOUT + 1);
	else if (id < JSEM_JOYS)
		std::snprintf(buffer, sizeof buffer, "custom%d", id - JSEM_CUSTOM);
	else if (id < JSEM_MICE)
		std::snprintf(buffer, sizeof buffer, "joy%d", id - JSEM_JOYS);
	else if (id == JSEM_MICE)
		std::snprintf(buffer, sizeof buffer, "mouse");
	else
		std::snprintf(buffer, sizeof buffer, "mouse%d", id - JSEM_MICE);
	return buffer;
}

void build_port_devices()
{
	port_devices.clear();
	for (const static_port_device& d : static_port_devices)
		port_devices.push_back({d.id, port_device_token(d.id), d.label});
	const int num_joys = inputdevice_get_device_total(IDTYPE_JOYSTICK);
	for (int j = 0; j < num_joys; j++) {
		const char* name = inputdevice_get_device_name(IDTYPE_JOYSTICK, j);
		std::string label = std::string(
			j < MAX_INPUT_DEVICES && di_joystick[j].is_controller ? "[Gamepad] " : "[Joystick] ")
			+ (name ? name : "?");
		port_devices.push_back({JSEM_JOYS + j, port_device_token(JSEM_JOYS + j),
			std::move(label)});
	}
	const int num_mice = inputdevice_get_device_total(IDTYPE_MOUSE);
	for (int j = 0; j < num_mice; j++) {
		const char* name = inputdevice_get_device_name(IDTYPE_MOUSE, j);
		std::string label = std::string("[Mouse] ") + (name && name[0] ? name : "?");
		port_devices.push_back({JSEM_MICE + j, port_device_token(JSEM_MICE + j),
			std::move(label)});
	}
	// Disambiguate duplicate device names by appending instance numbers,
	// like the panel did.
	std::unordered_map<std::string, int> name_counts;
	for (const auto& option : port_devices)
		name_counts[option.label]++;
	std::unordered_map<std::string, int> name_seen;
	for (auto& option : port_devices) {
		if (name_counts[option.label] > 1)
			option.label += " (" + std::to_string(++name_seen[option.label]) + ")";
	}
	port_devices_valid = true;
}

void free_port_devices()
{
	port_devices.clear();
	port_devices_valid = false;
}

// One list's storage: the strings are copied so the items stay valid
// while the device list is rebuilt, until the next call for the same list.
struct port_choice_storage {
	std::vector<std::string> tokens;
	std::vector<std::string> labels;
	std::vector<amiberry_setting_choice> items;
};

// One per port, and one for the connected devices alone.
port_choice_storage port_choice_lists[MAX_JPORTS + 1];

void remap_session_ports();

void get_joyport(const uae_prefs& p, char* out, size_t size, int port);

void update_port_devices()
{
	// The joystick list is rebuilt after a device change (the input system
	// sets joystick_refresh_needed, and this is the one consumer that
	// clears it again); the first use builds it.
	if (!port_devices_valid || joystick_refresh_needed) {
		if (port_devices_valid)
			remap_session_ports(); // the host's devices changed
		free_port_devices();
		build_port_devices();
		joystick_refresh_needed = false;
	}
}

// The connected devices, after stored when it names a device the list
// lacks (nullptr: none).
choice_list fill_port_choices(port_choice_storage& s, const char* stored)
{
	update_port_devices();
	bool listed = stored == nullptr;
	for (const auto& option : port_devices) {
		if (!listed && option.token == stored)
			listed = true;
	}
	// Fill the storage before taking pointers into it, so the pointers
	// stay valid while the vectors grow.
	s.tokens.clear();
	s.labels.clear();
	s.items.clear();
	if (!listed) {
		s.tokens.emplace_back(stored);
		s.labels.push_back(port_device_label(stored));
	}
	for (const auto& option : port_devices) {
		s.tokens.push_back(option.token);
		s.labels.push_back(option.label);
	}
	for (size_t i = 0; i < s.tokens.size(); i++)
		s.items.push_back({s.tokens[i].c_str(), s.labels[i].c_str()});
	return {s.items.data(), s.items.size()};
}

// A port's list starts with the device the configuration holds when that
// is not connected now, so a front end shows the list as it is.
void port_dynamic_choices(const uae_prefs& p, choice_list& out, int port)
{
	// A device change remaps the ports first, so the stored device read
	// next is the one the port holds after it.
	update_port_devices();
	char stored[32];
	get_joyport(p, stored, sizeof stored, port);
	out = fill_port_choices(port_choice_lists[port], stored);
}

void refresh_port_choices()
{
	remap_session_ports();
	free_port_devices();
	joystick_refresh_needed = true;
}

int decode_port_device(std::string_view text, bool& ok)
{
	ok = true;
	// At most six digits: a longer number names no device, and would
	// overflow the int.
	auto number = [&](size_t offset) {
		if (text.size() > offset + 6)
			return -1;
		int value = 0;
		for (size_t i = offset; i < text.size(); i++) {
			if (text[i] < '0' || text[i] > '9')
				return -1;
			value = value * 10 + (text[i] - '0');
		}
		return value;
	};
	if (text == "none")
		return JPORT_NONE;
	if (text.substr(0, 3) == "kbd") {
		const int layout = number(3);
		if (layout < 1 || layout > JSEM_LASTKBD)
			ok = false;
		return JSEM_KBDLAYOUT + layout - 1;
	}
	if (text.substr(0, 3) == "joy") {
		const int index = number(3);
		if (index < 0)
			ok = false;
		return JSEM_JOYS + index;
	}
	if (text == "mouse")
		return JSEM_MICE;
	if (text.substr(0, 5) == "mouse") {
		const int index = number(5);
		if (index <= 0)
			ok = false;
		return JSEM_MICE + index;
	}
	ok = false;
	return JPORT_NONE;
}

// ------------------------------------------------------------ port options

void get_joyport(const uae_prefs& p, char* out, size_t size, int port)
{
	std::snprintf(out, size, "%s",
		p.jports[port].id < 0 ? "none" : port_device_token(p.jports[port].id).c_str());
}

// The dependent changes the panel's device selection made on the
// configuration: store the device, then let the core's port validation
// resolve duplicate assignments and device identities. The prefs-only
// validation keeps session copies free of global device state.
// The host device a port id names (a joystick or a mouse); type is -1 for
// the others (keyboard layouts, custom, none).
int port_device_type(int id, int& index)
{
	if (id >= JSEM_JOYS && id < JSEM_MICE) {
		index = id - JSEM_JOYS;
		return IDTYPE_JOYSTICK;
	}
	if (id >= JSEM_MICE && id < JSEM_END) {
		index = id - JSEM_MICE;
		return IDTYPE_MOUSE;
	}
	index = -1;
	return -1;
}

// A port remembers which device it holds by name, as the configuration
// does (joyportN_friendlyname/_name), so a device list that changes order
// can be matched again (remap_session_ports).
void set_port_identity(jport& jp)
{
	int index;
	const int type = port_device_type(jp.id, index);
	const TCHAR* name = type >= 0 ? inputdevice_get_device_name(type, index) : nullptr;
	const TCHAR* unique = type >= 0 ? inputdevice_get_device_unique_name(type, index) : nullptr;
	_sntprintf(jp.idc.name, MAX_JPORT_NAME, _T("%s"), name ? name : _T(""));
	_sntprintf(jp.idc.configname, MAX_JPORT_CONFIG, _T("%s"), unique ? unique : _T(""));
	jp.idc.shortid[0] = 0;
}

void set_joyport(uae_prefs& p, const value& v, int port)
{
	bool ok = false;
	const int id = decode_port_device(v.text, ok);
	if (!ok)
		return;
	p.jports[port].id = id;
	set_port_identity(p.jports[port]);
	inputdevice_validate_jports_prefs_only(&p, port);
}

// After the host's device list changed, an open session's port id may name
// a different device at the same index. Each port holding a joystick or a
// mouse is matched again by the device's unique name; a device that is gone
// leaves the port empty rather than on whichever device took its index.
// The live configurations are matched by the input system itself.
void remap_session_ports()
{
	for_each_session_config([](uae_prefs& p, void*) {
		for (int port = 0; port < MAX_JPORTS; port++) {
			jport& jp = p.jports[port];
			int index;
			const int type = port_device_type(jp.id, index);
			if (type < 0 || !jp.idc.configname[0])
				continue;
			const int base = type == IDTYPE_JOYSTICK ? JSEM_JOYS : JSEM_MICE;
			int found = -1;
			for (int d = 0; d < inputdevice_get_device_total(type); d++) {
				const TCHAR* unique = inputdevice_get_device_unique_name(type, d);
				if (unique && !_tcscmp(unique, jp.idc.configname)) {
					found = d;
					break;
				}
			}
			if (found >= 0) {
				jp.id = base + found;
			} else {
				jp.id = JPORT_NONE;
				jp.idc = {};
			}
		}
	}, nullptr);
}

const amiberry_setting_choice autofire_choices[] = {
	{"none", "No autofire (normal)"},
	{"normal", "Autofire"},
	{"toggle", "Autofire (toggle)"},
	{"always", "Autofire (always)"},
	{"togglebutton", "No autofire (toggle)"},
};

void get_joyport_autofire(const uae_prefs& p, char* out, size_t size, int port)
{
	write_choice(out, size, CHOICES(autofire_choices), p.jports[port].autofire);
}

void set_joyport_autofire(uae_prefs& p, const value& v, int port)
{
	p.jports[port].autofire = choice_index(CHOICES(autofire_choices), v.text);
}

const amiberry_setting_choice port_mode_choices[] = {
	{"", "Default"},
	{"mouse", "Wheel mouse"},
	{"mousenowheel", "Mouse"},
	{"djoy", "Joystick"},
	{"gamepad", "Gamepad"},
	{"ajoy", "Analog joystick"},
	{"cdtvjoy", "CDTV remote mouse"},
	{"cd32joy", "CD32 pad"},
	{"lightpen", "Light pen"},
};

void get_joyport_mode(const uae_prefs& p, char* out, size_t size, int port)
{
	write_choice(out, size, CHOICES(port_mode_choices), p.jports[port].mode);
}

void set_joyport_mode(uae_prefs& p, const value& v, int port)
{
	p.jports[port].mode = choice_index(CHOICES(port_mode_choices), v.text);
}

// Whether the left analog stick of the port's device moves the mouse. The
// stored value is a bitmask the mouse-map hotkeys toggle per stick
// (inputdevice.cpp AKS_MOUSEMAP_*), but input routing (amiberry_input.cpp)
// maps only the left stick, for any value other than 0: the right stick has
// no choice until it is routed. Another value reads back as stored.
const amiberry_setting_choice mouse_map_choices[] = {
	{"0", "None"},
	{"1", "Left stick"},
};

// The configuration reports the bitmask as-is.
void get_joyport_mousemap(const uae_prefs& p, char* out, size_t size, int port)
{
	write_int(out, size, p.jports[port].mousemap);
}

void set_joyport_mousemap(uae_prefs& p, const value& v, int port)
{
	p.jports[port].mousemap = static_cast<int>(v.number);
}

bool mousemap_available(const uae_prefs& p, const context&, int port)
{
	return p.jports[port].mode != 0;
}

#define PORT_DEVICE_OPTIONS(n) \
	void get_joyport##n(const uae_prefs& p, char* out, size_t size) \
	{ \
		get_joyport(p, out, size, n); \
	} \
	void set_joyport##n(uae_prefs& p, const value& v) { set_joyport(p, v, n); } \
	void joyport##n##_choices(const uae_prefs& p, choice_list& out) \
	{ \
		port_dynamic_choices(p, out, n); \
	} \
	void get_joyport##n##autofire(const uae_prefs& p, char* out, size_t size) \
	{ \
		get_joyport_autofire(p, out, size, n); \
	} \
	void set_joyport##n##autofire(uae_prefs& p, const value& v) \
	{ \
		set_joyport_autofire(p, v, n); \
	}

// The configuration stores a port mode only for ports 1 and 2 (joyport0mode
// and joyport1mode) and reads the mouse map back for those two only, so the
// parallel-port adapter ports 3 and 4 have neither option.
#define PORT_MODE_OPTIONS(n) \
	void get_joyport##n##mode(const uae_prefs& p, char* out, size_t size) \
	{ \
		get_joyport_mode(p, out, size, n); \
	} \
	void set_joyport##n##mode(uae_prefs& p, const value& v) { set_joyport_mode(p, v, n); } \
	void get_joyport##n##mousemap(const uae_prefs& p, char* out, size_t size) \
	{ \
		get_joyport_mousemap(p, out, size, n); \
	} \
	void set_joyport##n##mousemap(uae_prefs& p, const value& v) \
	{ \
		set_joyport_mousemap(p, v, n); \
	} \
	bool joyport##n##_mousemap_available(const uae_prefs& p, const context& ctx) \
	{ \
		return mousemap_available(p, ctx, n); \
	}

PORT_DEVICE_OPTIONS(0)
PORT_DEVICE_OPTIONS(1)
PORT_DEVICE_OPTIONS(2)
PORT_DEVICE_OPTIONS(3)
PORT_MODE_OPTIONS(0)
PORT_MODE_OPTIONS(1)

// -------------------------------------------------------- controller inputs

void get_input_autoswitch(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.input_autoswitch);
}

void set_input_autoswitch(uae_prefs& p, const value& v)
{
	p.input_autoswitch = v.number != 0;
}

void get_input_multi_mouse(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.input_multi_mouse);
}

void set_input_multi_mouse(uae_prefs& p, const value& v)
{
	p.input_multi_mouse = v.number != 0;
}

void get_input_joystick_up_button(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.input_joystick_up_button);
}

void set_input_joystick_up_button(uae_prefs& p, const value& v)
{
	p.input_joystick_up_button = v.number != 0;
}

void get_input_joystick_deadzone(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.input_joystick_deadzone);
}

void set_input_joystick_deadzone(uae_prefs& p, const value& v)
{
	p.input_joystick_deadzone = static_cast<int>(v.number);
}

void get_input_autofire_speed(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.input_autofire_linecnt);
}

void set_input_autofire_speed(uae_prefs& p, const value& v)
{
	p.input_autofire_linecnt = static_cast<int>(v.number);
}

const amiberry_setting_choice digital_speed_choices[] = {
	{"2", "2"},
	{"5", "5"},
	{"10", "10"},
	{"15", "15"},
	{"20", "20"},
};

void get_input_joymouse_speed_digital(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.input_joymouse_speed);
}

void set_input_joymouse_speed_digital(uae_prefs& p, const value& v)
{
	p.input_joymouse_speed = static_cast<int>(v.number);
}

const amiberry_setting_choice analog_speed_choices[] = {
	{"5", "5"},
	{"10", "10"},
	{"15", "15"},
	{"20", "20"},
	{"25", "25"},
	{"30", "30"},
	{"35", "35"},
	{"40", "40"},
	{"45", "45"},
	{"50", "50"},
	{"75", "75"},
	{"100", "100"},
	{"125", "125"},
	{"150", "150"},
};

void get_input_joymouse_speed_analog(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.input_joymouse_multiplier);
}

void set_input_joymouse_speed_analog(uae_prefs& p, const value& v)
{
	p.input_joymouse_multiplier = static_cast<int>(v.number);
}

void get_input_mouse_speed(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.input_mouse_speed);
}

void set_input_mouse_speed(uae_prefs& p, const value& v)
{
	p.input_mouse_speed = static_cast<int>(v.number);
}

// ----------------------------------------------------------- mouse handling

// One field, two keys: magic_mouse carries the Magic Mouse bit in the
// WinUAE section and middle_mouse the middle button bit in Amiberry's.
void get_magic_mouse(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, (p.input_mouse_untrap & MOUSEUNTRAP_MAGIC) != 0);
}

void set_magic_mouse(uae_prefs& p, const value& v)
{
	if (v.number)
		p.input_mouse_untrap |= MOUSEUNTRAP_MAGIC;
	else
		p.input_mouse_untrap &= ~MOUSEUNTRAP_MAGIC;
}

void get_middle_mouse(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, (p.input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON) != 0);
}

void set_middle_mouse(uae_prefs& p, const value& v)
{
	if (v.number)
		p.input_mouse_untrap |= MOUSEUNTRAP_MIDDLEBUTTON;
	else
		p.input_mouse_untrap &= ~MOUSEUNTRAP_MIDDLEBUTTON;
}

const amiberry_setting_choice magic_cursor_choices[] = {
	{"both", "Both"},
	{"native", "Native only"},
	{"host", "Host only"},
};

void get_magic_mousecursor(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(magic_cursor_choices), p.input_magic_mouse_cursor);
}

void set_magic_mousecursor(uae_prefs& p, const value& v)
{
	p.input_magic_mouse_cursor = choice_index(CHOICES(magic_cursor_choices), v.text);
}

void get_tablet_library(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.tablet_library);
}

void set_tablet_library(uae_prefs& p, const value& v)
{
	p.tablet_library = v.number != 0;
}

const amiberry_setting_choice tablet_choices[] = {
	{"none", "Disabled"},
	{"mousehack", "MouseHack"},
	{"tablet", "Real Tablet"},
};

void get_absolute_mouse(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, CHOICES(tablet_choices), p.input_tablet);
}

void set_absolute_mouse(uae_prefs& p, const value& v)
{
	p.input_tablet = choice_index(CHOICES(tablet_choices), v.text);
}

bool tablet_installed(const uae_prefs& p)
{
	return p.input_tablet != 0;
}

bool tablet_option_available(const uae_prefs& p, const context&)
{
	return tablet_installed(p);
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, nullptr, 0, constraint}
// A number shown with its unit; level: set by feel, so a front end may show
// a slider.
#define MEASURED_OPTION(id, group, label, help, apply, min, max, step, def, constraint, unit, level) \
	{id, group, label, help, AMIBERRY_SETTING_INT, apply, min, max, step, def, nullptr, 0, constraint, \
		false, unit, 0, level}

const definition definitions[] = {
	{DYNAMIC_CHOICE_OPTION("joyport0", "input", "Port 1 device",
			"Input device for joyport 1, the mouse port.",
			AMIBERRY_APPLY_LIVE, "mouse", nullptr),
		get_joyport0, set_joyport0, nullptr, nullptr, nullptr, nullptr, joyport0_choices},
	{DYNAMIC_CHOICE_OPTION("joyport1", "input", "Port 2 device",
			"Input device for joyport 2, the joystick port.",
			AMIBERRY_APPLY_LIVE, "joy0", nullptr),
		get_joyport1, set_joyport1, nullptr, nullptr, nullptr, nullptr, joyport1_choices},
	{DYNAMIC_CHOICE_OPTION("joyport2", "input", "Port 3 device",
			"Input device for the parallel port adapter's first joyport.",
			AMIBERRY_APPLY_LIVE, "none", nullptr),
		get_joyport2, set_joyport2, nullptr, nullptr, nullptr, nullptr, joyport2_choices},
	{DYNAMIC_CHOICE_OPTION("joyport3", "input", "Port 4 device",
			"Input device for the parallel port adapter's second joyport.",
			AMIBERRY_APPLY_LIVE, "none", nullptr),
		get_joyport3, set_joyport3, nullptr, nullptr, nullptr, nullptr, joyport3_choices},
	{CHOICE_OPTION("joyport0autofire", "input", "Port 1 autofire",
			"Autofire for the Port 1 device.",
			AMIBERRY_APPLY_LIVE, "none", autofire_choices, nullptr),
		get_joyport0autofire, set_joyport0autofire, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("joyport1autofire", "input", "Port 2 autofire",
			"Autofire for the Port 2 device.",
			AMIBERRY_APPLY_LIVE, "none", autofire_choices, nullptr),
		get_joyport1autofire, set_joyport1autofire, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("joyport2autofire", "input", "Port 3 autofire",
			"Autofire for the Port 3 device.",
			AMIBERRY_APPLY_LIVE, "none", autofire_choices, nullptr),
		get_joyport2autofire, set_joyport2autofire, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("joyport3autofire", "input", "Port 4 autofire",
			"Autofire for the Port 4 device.",
			AMIBERRY_APPLY_LIVE, "none", autofire_choices, nullptr),
		get_joyport3autofire, set_joyport3autofire, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("joyport0mode", "input", "Port 1 mode",
			"Emulation mode of the Port 1 device.",
			AMIBERRY_APPLY_LIVE, "", port_mode_choices, nullptr),
		get_joyport0mode, set_joyport0mode, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("joyport1mode", "input", "Port 2 mode",
			"Emulation mode of the Port 2 device.",
			AMIBERRY_APPLY_LIVE, "", port_mode_choices, nullptr),
		get_joyport1mode, set_joyport1mode, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("joyport0mousemap", "input", "Port 1 left stick as mouse",
			"Move the mouse with the left analog stick of the Port 1 device.",
			AMIBERRY_APPLY_LIVE, "0", mouse_map_choices,
			"Only in a non-default port mode."),
		get_joyport0mousemap, set_joyport0mousemap, joyport0_mousemap_available, nullptr,
		nullptr, nullptr},
	{CHOICE_OPTION("joyport1mousemap", "input", "Port 2 left stick as mouse",
			"Move the mouse with the left analog stick of the Port 2 device.",
			AMIBERRY_APPLY_LIVE, "0", mouse_map_choices,
			"Only in a non-default port mode."),
		get_joyport1mousemap, set_joyport1mousemap, joyport1_mousemap_available, nullptr,
		nullptr, nullptr},
	{BOOL_OPTION("input.autoswitch", "input", "Mouse/Joystick autoswitching",
			"Switch the port devices automatically based on input activity.",
			AMIBERRY_APPLY_LIVE, "true", nullptr),
		get_input_autoswitch, set_input_autoswitch, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("input.multi_mouse", "input", "Multi-mouse support",
			"Treat each physical mouse as a separate device for two-player mouse games.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_input_multi_mouse, set_input_multi_mouse, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("input.joystick_up_button", "input", "Left face button as Joy Up",
			"Bind the left face button (Xbox X / PS Square) as an extra Joy Up.",
			AMIBERRY_APPLY_LIVE, "true", nullptr),
		get_input_joystick_up_button, set_input_joystick_up_button, nullptr, nullptr, nullptr,
		nullptr},
	{MEASURED_OPTION("input.joystick_deadzone", "input", "Joystick dead zone",
			"Analog stick dead zone as a percentage of full deflection.",
			AMIBERRY_APPLY_LIVE, 0, 100, 1, "33", nullptr, "%", true),
		get_input_joystick_deadzone, set_input_joystick_deadzone, nullptr, nullptr, nullptr,
		nullptr},
	{MEASURED_OPTION("input.autofire_speed", "input", "Autofire rate",
			"Autofire period in scanlines; 1248 is fast, 3744 is slow, 0 is off.",
			AMIBERRY_APPLY_LIVE, 0, 3744, 1, "600", nullptr, "scanlines", false),
		get_input_autofire_speed, set_input_autofire_speed, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("input.joymouse_speed_digital", "input", "Digital joy-mouse speed",
			"Mouse cursor speed when digital joystick directions drive the mouse.",
			AMIBERRY_APPLY_LIVE, "10", digital_speed_choices, nullptr),
		get_input_joymouse_speed_digital, set_input_joymouse_speed_digital, nullptr, nullptr,
		nullptr, nullptr},
	{CHOICE_OPTION("input.joymouse_speed_analog", "input", "Analog joy-mouse speed",
			"Mouse pointer speed when an analog stick drives the mouse; lower is slower.",
			AMIBERRY_APPLY_LIVE, "25", analog_speed_choices, nullptr),
		get_input_joymouse_speed_analog, set_input_joymouse_speed_analog, nullptr, nullptr,
		nullptr, nullptr},
	{MEASURED_OPTION("input.mouse_speed", "input", "Mouse speed",
			"Speed of the real mouse and touch pointer; 100% is normal. The core resets "
			"values outside 1 to 1000 to 100.",
			AMIBERRY_APPLY_LIVE, 1, 1000, 1, "100",
			"Between 1 and 1000; the core resets anything else to 100.", "%", false),
		get_input_mouse_speed, set_input_mouse_speed, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("magic_mouse", "input", "Magic Mouse untrap",
			"Release the mouse from the emulator window with the Magic Mouse button.",
			AMIBERRY_APPLY_LIVE, "false", nullptr),
		get_magic_mouse, set_magic_mouse, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.middle_mouse", "input", "Middle button untrap",
			"Release the mouse from the emulator window with the middle button.",
			AMIBERRY_APPLY_LIVE, "true", nullptr),
		get_middle_mouse, set_middle_mouse, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("magic_mousecursor", "input", "Magic Mouse cursor",
			"Which cursor the Magic Mouse shows.",
			AMIBERRY_APPLY_LIVE, "both", magic_cursor_choices,
			"Needs the virtual mouse driver installed."),
		get_magic_mousecursor, set_magic_mousecursor, tablet_option_available, nullptr, nullptr,
		nullptr},
	{BOOL_OPTION("tablet_library", "input", "Tablet.library emulation",
			"Emulate the Amiga tablet.library for graphics tablets.",
			AMIBERRY_APPLY_RESET, "false",
			"Needs the virtual mouse driver installed. AmigaOS sees the library after a reset."),
		get_tablet_library, set_tablet_library, tablet_option_available, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("absolute_mouse", "input", "Tablet mode",
			"MouseHack maps absolute pointers (touch, light pens); Real Tablet supports "
			"pressure-sensitive tablets.",
			AMIBERRY_APPLY_LIVE, "none", tablet_choices, nullptr),
		get_absolute_mouse, set_absolute_mouse, nullptr, nullptr, nullptr, nullptr},
};

bool commit_input(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	// Each port's settings are separate options, so they are merged one by
	// one: a session that changed a port's autofire keeps a mode changed
	// elsewhere meanwhile. The device is its id with its identity.
	for (int i = 0; i < MAX_JPORTS; i++) {
		const jport& b = base.jports[i];
		const jport& f = from.jports[i];
		jport& t = to.jports[i];
		if (b.id != f.id || std::memcmp(&b.idc, &f.idc, sizeof b.idc) != 0) {
			wrote |= t.id != f.id || std::memcmp(&t.idc, &f.idc, sizeof t.idc) != 0;
			t.id = f.id;
			t.idc = f.idc;
		}
		wrote |= commit_field(b.mode, f.mode, t.mode);
		wrote |= commit_field(b.submode, f.submode, t.submode);
		wrote |= commit_field(b.autofire, f.autofire, t.autofire);
		wrote |= commit_field(b.mousemap, f.mousemap, t.mousemap);
		wrote |= commit_field(b.nokeyboardoverride, f.nokeyboardoverride, t.nokeyboardoverride);
	}
	FIELD(input_autoswitch);
	FIELD(input_multi_mouse);
	FIELD(input_joystick_up_button);
	FIELD(input_joystick_deadzone);
	FIELD(input_autofire_linecnt);
	FIELD(input_joymouse_speed);
	FIELD(input_joymouse_multiplier);
	FIELD(input_mouse_speed);
	// Independent flags of two options (magic_mouse, amiberry.middle_mouse):
	// only the bits the session changed are merged, so a flag changed
	// elsewhere meanwhile is kept.
	if (const int changed = base.input_mouse_untrap ^ from.input_mouse_untrap) {
		const int merged = (to.input_mouse_untrap & ~changed) | (from.input_mouse_untrap & changed);
		wrote |= merged != to.input_mouse_untrap;
		to.input_mouse_untrap = merged;
	}
	FIELD(input_magic_mouse_cursor);
	FIELD(tablet_library);
	FIELD(input_tablet);
#undef FIELD
	return wrote;
}

const domain input = {
	definitions, sizeof definitions / sizeof definitions[0], commit_input,
	refresh_port_choices,
};

} // namespace

const domain& input_domain()
{
	return input;
}

bool is_port_device_token(std::string_view text)
{
	bool ok = false;
	decode_port_device(text, ok);
	return ok;
}

choice_list connected_port_devices()
{
	return fill_port_choices(port_choice_lists[MAX_JPORTS], nullptr);
}

namespace {

// The number a token ends in, from offset; 0 when it has none.
int token_number(std::string_view token, size_t offset)
{
	int number = 0;
	for (size_t i = offset; i < token.size() && number < 100000; i++)
		number = number * 10 + (token[i] - '0');
	return number;
}

} // namespace

std::string port_device_label(std::string_view token)
{
	if (token == "joydefault")
		return "Default joystick";
	if (token == "mousedefault")
		return "Default mouse";
	if (token == "custom")
		return "Custom input mapping";
	if (token.substr(0, 6) == "custom")
		return "Custom input mapping " + std::to_string(token_number(token, 6) + 1);
	if (token.substr(0, 3) == "joy")
		return "Joystick " + std::to_string(token_number(token, 3) + 1) + " (not connected)";
	if (token.substr(0, 5) == "mouse")
		return "Mouse " + std::to_string(token_number(token, 5) + 1) + " (not connected)";
	if (token.substr(0, 3) == "kbd")
		return "Keyboard layout " + std::to_string(token_number(token, 3));
	return std::string(token);
}

} // namespace amiberry_settings
