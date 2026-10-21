// Floppy options: the four drives (image, type, FloppyBridge mode), the
// drive speed, the disk swapper list and the DrawBridge (FloppyBridge)
// hardware settings.
//
// The image options are the configuration's floppy0..3 and diskimage0..19
// paths. Inserting a disk into a running emulation is an action
// (amiberry_actions_insert_floppy); these options are what a configuration
// holds, which is also what the Floppy panel edits.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "disk.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

void write_path(char* out, size_t size, const TCHAR* path)
{
	std::snprintf(out, size, "%s", path);
}

void set_path(TCHAR* field, size_t size, const value& v)
{
	std::snprintf(field, size, "%.*s", static_cast<int>(v.text.size()), v.text.data());
}

// ------------------------------------------------------------------ drives

// floppyNtype is the raw drive type. The PC drive types WinUAE offers for
// bridgeboards (4-6) are reported but not offered: the Floppy panel never
// listed them.
const amiberry_setting_choice drive_type_choices[] = {
	{"-1", "Disabled"},
	{"0", "3.5\" DD"},
	{"1", "3.5\" HD"},
	{"2", "5.25\" (40)"},
	{"7", "5.25\" (80)"},
	{"3", "3.5\" ESCOM"},
	{"8", "FloppyBridge"},
};

// The FloppyBridge profile a drive uses; profiles 1-4 are the ones
// drawbridge_update_profiles() builds, in this order.
const amiberry_setting_choice bridge_mode_choices[] = {
	{"0", "Normal"},
	{"1", "Compatible"},
	{"2", "Turbo"},
	{"3", "Stalling"},
};

bool floppybridge_built()
{
#ifdef FLOPPYBRIDGE
	return true;
#else
	return false;
#endif
}

void set_bridge_mode(floppyslot& slot, int mode)
{
	slot.dfxsubtype = mode;
	std::snprintf(slot.dfxsubtypeid, sizeof slot.dfxsubtypeid, "%d:%s", mode + 1,
		bridge_mode_choices[mode].label);
}

template <int N>
void get_floppy(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.floppyslots[N].df);
}

template <int N>
void set_floppy(uae_prefs& p, const value& v)
{
	set_path(p.floppyslots[N].df, sizeof p.floppyslots[N].df, v);
}

template <int N>
bool drive_enabled(const uae_prefs& p, const context&)
{
	return p.floppyslots[N].dfxtype != DRV_NONE;
}

template <int N>
void get_floppy_type(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.floppyslots[N].dfxtype);
}

// Leaving FloppyBridge clears its profile, as the panel did; switching to
// it selects the Normal profile.
template <int N>
void set_floppy_type(uae_prefs& p, const value& v)
{
	floppyslot& slot = p.floppyslots[N];
	const int type = static_cast<int>(v.number);
	if (type == DRV_FB && slot.dfxtype != DRV_FB) {
		set_bridge_mode(slot, 0);
	} else if (type != DRV_FB) {
		slot.dfxsubtype = 0;
		slot.dfxsubtypeid[0] = 0;
	}
	slot.dfxtype = type;
}

bool floppy_type_choice_available(const uae_prefs&, std::string_view choice)
{
	return choice != "8" || floppybridge_built();
}

template <int N>
void get_floppy_subtype(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.floppyslots[N].dfxsubtype);
}

template <int N>
void set_floppy_subtype(uae_prefs& p, const value& v)
{
	set_bridge_mode(p.floppyslots[N], static_cast<int>(v.number));
}

template <int N>
bool bridge_drive(const uae_prefs& p, const context&)
{
	return floppybridge_built() && p.floppyslots[N].dfxtype == DRV_FB;
}

// ------------------------------------------------------------------ speed

const amiberry_setting_choice floppy_speed_choices[] = {
	{"0", "Turbo"},
	{"100", "100% (compatible)"},
	{"200", "200%"},
	{"400", "400%"},
	{"800", "800%"},
};

void get_floppy_speed(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.floppy_speed);
}

void set_floppy_speed(uae_prefs& p, const value& v)
{
	p.floppy_speed = static_cast<int>(v.number);
}

// ---------------------------------------------------------------- swapper

template <int N>
void get_diskimage(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.dfxlist[N]);
}

template <int N>
void set_diskimage(uae_prefs& p, const value& v)
{
	set_path(p.dfxlist[N], sizeof p.dfxlist[N], v);
}

// -------------------------------------------------------------- DrawBridge

// The driver list comes from the FloppyBridge library. Without it (or
// without drivers) the list holds the default driver 0, so the option
// keeps a value every configuration can hold.
std::vector<std::string> driver_values;
std::vector<std::string> driver_labels;
std::vector<amiberry_setting_choice> driver_items;
bool driver_choices_valid = false;

void build_driver_choices(bool refresh)
{
	driver_values.clear();
	driver_labels.clear();
	driver_items.clear();
	const int count = drawbridge_driver_count(refresh);
	for (int i = 0; i < count; i++) {
		driver_values.push_back(std::to_string(i));
		driver_labels.emplace_back(drawbridge_driver_name(i));
	}
	if (driver_values.empty()) {
		driver_values.emplace_back("0");
		driver_labels.emplace_back("No drivers found");
	}
	for (size_t i = 0; i < driver_values.size(); i++)
		driver_items.push_back({driver_values[i].c_str(), driver_labels[i].c_str()});
	driver_choices_valid = true;
}

void driver_dynamic_choices(const uae_prefs&, choice_list& out)
{
	if (!driver_choices_valid)
		build_driver_choices(false);
	out.items = driver_items.data();
	out.count = driver_items.size();
}

void refresh_floppy_choices()
{
	build_driver_choices(true);
}

unsigned int driver_options(const uae_prefs& p)
{
	return drawbridge_driver_options(p.drawbridge_driver);
}

bool drawbridge_available(const uae_prefs&, const context&)
{
	return floppybridge_built();
}

bool serial_auto_available(const uae_prefs& p, const context&)
{
	return (driver_options(p) & DRAWBRIDGE_OPTION_AUTODETECT_COMPORT) != 0;
}

// The port is chosen by hand while detection is off (or not supported).
bool serial_port_available(const uae_prefs& p, const context&)
{
	const unsigned int options = driver_options(p);
	if (!(options & DRAWBRIDGE_OPTION_COMPORT))
		return false;
	return !p.drawbridge_serial_auto || !(options & DRAWBRIDGE_OPTION_AUTODETECT_COMPORT);
}

bool smartspeed_available(const uae_prefs& p, const context&)
{
	return (driver_options(p) & DRAWBRIDGE_OPTION_SMARTSPEED) != 0;
}

bool autocache_available(const uae_prefs& p, const context&)
{
	return (driver_options(p) & DRAWBRIDGE_OPTION_AUTOCACHE) != 0;
}

bool drive_cable_available(const uae_prefs& p, const context&)
{
	return (driver_options(p) & DRAWBRIDGE_OPTION_DRIVE_CABLE) != 0;
}

void get_drawbridge_driver(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.drawbridge_driver);
}

void set_drawbridge_driver(uae_prefs& p, const value& v)
{
	p.drawbridge_driver = static_cast<int>(v.number);
}

void get_drawbridge_serial_auto(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.drawbridge_serial_auto);
}

void set_drawbridge_serial_auto(uae_prefs& p, const value& v)
{
	p.drawbridge_serial_auto = v.number != 0;
}

void get_drawbridge_serial_port(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.drawbridge_serial_port);
}

void set_drawbridge_serial_port(uae_prefs& p, const value& v)
{
	set_path(p.drawbridge_serial_port, sizeof p.drawbridge_serial_port, v);
}

bool serial_port_fits(const uae_prefs& p, const value& v)
{
	return v.text.size() < sizeof p.drawbridge_serial_port;
}

void get_drawbridge_smartspeed(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.drawbridge_smartspeed);
}

void set_drawbridge_smartspeed(uae_prefs& p, const value& v)
{
	p.drawbridge_smartspeed = v.number != 0;
}

void get_drawbridge_autocache(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.drawbridge_autocache);
}

void set_drawbridge_autocache(uae_prefs& p, const value& v)
{
	p.drawbridge_autocache = v.number != 0;
}

// drawbridge_update_profiles() resets cables outside 0-5 to 0.
const amiberry_setting_choice drive_cable_choices[] = {
	{"0", "Drive A (IBM PC)"},
	{"1", "Drive B (IBM PC)"},
	{"2", "Drive 0 (SHUGART)"},
	{"3", "Drive 1 (SHUGART)"},
	{"4", "Drive 2 (SHUGART)"},
	{"5", "Drive 3 (SHUGART)"},
};

// The IBM PC drive A/B lines work with any driver that has a cable option;
// the Shugart drive 0-3 lines need a driver that supports Shugart mode
// (FloppyBridge refuses them otherwise).
bool drive_cable_choice_available(const uae_prefs& p, std::string_view choice)
{
	return choice == "0" || choice == "1" || (driver_options(p) & DRAWBRIDGE_OPTION_SHUGART) != 0;
}

void get_drawbridge_drive_cable(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.drawbridge_drive_cable);
}

void set_drawbridge_drive_cable(uae_prefs& p, const value& v)
{
	p.drawbridge_drive_cable = static_cast<int>(v.number);
}

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define PATH_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_PATH, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, 0, 0, 0, def, nullptr, 0, constraint}

const char* const drive_constraint = "Needs the drive enabled.";
const char* const bridge_constraint = "Needs a FloppyBridge drive type.";
const char* const driver_constraint = "Needs a DrawBridge driver that supports it.";

#define DRIVE(n, type_default) \
	{PATH_OPTION("floppy" #n, "floppy", "DF" #n ": image", \
			"Disk image in drive DF" #n ":, or empty for no disk.", \
			AMIBERRY_APPLY_LIVE, "", drive_constraint), \
		get_floppy<n>, set_floppy<n>, drive_enabled<n>, nullptr, nullptr, nullptr, nullptr}, \
	{CHOICE_OPTION("floppy" #n "type", "floppy", "DF" #n ": drive type", \
			"Drive type of DF" #n ":, or Disabled. AmigaOS sees a change after a reset.", \
			AMIBERRY_APPLY_RESET, type_default, drive_type_choices, \
			"FloppyBridge needs a build with FloppyBridge support."), \
		get_floppy_type<n>, set_floppy_type<n>, nullptr, floppy_type_choice_available, nullptr, \
		nullptr, nullptr}, \
	{CHOICE_OPTION("floppy" #n "subtype", "floppy", "DF" #n ": FloppyBridge mode", \
			"How the real drive behind DF" #n ": is accessed.", \
			AMIBERRY_APPLY_RESET, "0", bridge_mode_choices, bridge_constraint), \
		get_floppy_subtype<n>, set_floppy_subtype<n>, bridge_drive<n>, nullptr, nullptr, nullptr, \
		nullptr}

#define DISKIMAGE(n) \
	{PATH_OPTION("diskimage" #n, "diskswapper", "Disk swapper entry " #n, \
			"Disk image in entry " #n " of the disk swapper list, or empty.", \
			AMIBERRY_APPLY_LIVE, "", nullptr), \
		get_diskimage<n>, set_diskimage<n>, nullptr, nullptr, nullptr, nullptr, nullptr}

const definition definitions[] = {
	DRIVE(0, "0"),
	DRIVE(1, "0"),
	DRIVE(2, "-1"),
	DRIVE(3, "-1"),
	{CHOICE_OPTION("floppy_speed", "floppy", "Floppy drive emulation speed",
			"Drive speed in percent of a real drive; Turbo transfers without delays.",
			AMIBERRY_APPLY_LIVE, "100", floppy_speed_choices, nullptr),
		get_floppy_speed, set_floppy_speed, nullptr, nullptr, nullptr, nullptr, nullptr},
	DISKIMAGE(0), DISKIMAGE(1), DISKIMAGE(2), DISKIMAGE(3), DISKIMAGE(4),
	DISKIMAGE(5), DISKIMAGE(6), DISKIMAGE(7), DISKIMAGE(8), DISKIMAGE(9),
	DISKIMAGE(10), DISKIMAGE(11), DISKIMAGE(12), DISKIMAGE(13), DISKIMAGE(14),
	DISKIMAGE(15), DISKIMAGE(16), DISKIMAGE(17), DISKIMAGE(18), DISKIMAGE(19),
	{DYNAMIC_CHOICE_OPTION("amiberry.drawbridge_driver", "drawbridge", "DrawBridge driver",
			"The FloppyBridge driver for the hardware that connects real floppy drives.",
			AMIBERRY_APPLY_LIVE, "0", "Needs a build with FloppyBridge support."),
		get_drawbridge_driver, set_drawbridge_driver, drawbridge_available, nullptr, nullptr,
		nullptr, driver_dynamic_choices},
	{BOOL_OPTION("amiberry.drawbridge_serial_autodetect", "drawbridge", "Auto-detect serial port",
			"Find the serial port the hardware is connected to automatically.",
			AMIBERRY_APPLY_LIVE, "true", driver_constraint),
		get_drawbridge_serial_auto, set_drawbridge_serial_auto, serial_auto_available, nullptr,
		nullptr, nullptr, nullptr},
	{STRING_OPTION("amiberry.drawbridge_serial_port", "drawbridge", "Serial port",
			"The serial port the hardware is connected to; empty for none.",
			AMIBERRY_APPLY_LIVE, "",
			"Needs a DrawBridge driver that uses a serial port, with auto-detection off."),
		get_drawbridge_serial_port, set_drawbridge_serial_port, serial_port_available, nullptr,
		nullptr, serial_port_fits, nullptr},
	{BOOL_OPTION("amiberry.drawbridge_smartspeed", "drawbridge", "Smart Speed",
			"Switch to Turbo dynamically when that does not break compatibility.",
			AMIBERRY_APPLY_LIVE, "false", driver_constraint),
		get_drawbridge_smartspeed, set_drawbridge_smartspeed, smartspeed_available, nullptr,
		nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.drawbridge_autocache", "drawbridge", "Auto-Cache",
			"Cache disk data while the drive is idle.",
			AMIBERRY_APPLY_LIVE, "false", driver_constraint),
		get_drawbridge_autocache, set_drawbridge_autocache, autocache_available, nullptr,
		nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.drawbridge_drive_cable", "drawbridge", "Drive cable",
			"Which drive select line of the cable the real drive is on.",
			AMIBERRY_APPLY_LIVE, "0", drive_cable_choices, driver_constraint),
		get_drawbridge_drive_cable, set_drawbridge_drive_cable, drive_cable_available,
		drive_cable_choice_available, nullptr, nullptr, nullptr},
};

bool commit_floppy(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	for (int i = 0; i < 4; i++) {
		FIELD(floppyslots[i].df);
		FIELD(floppyslots[i].dfxtype);
		FIELD(floppyslots[i].dfxsubtype);
		FIELD(floppyslots[i].dfxsubtypeid);
	}
	FIELD(floppy_speed);
	for (int i = 0; i < MAX_SPARE_DRIVES; i++)
		FIELD(dfxlist[i]);
	FIELD(drawbridge_driver);
	FIELD(drawbridge_serial_auto);
	FIELD(drawbridge_serial_port);
	FIELD(drawbridge_smartspeed);
	FIELD(drawbridge_autocache);
	FIELD(drawbridge_drive_cable);
#undef FIELD
	return wrote;
}

const domain floppy = {
	definitions, sizeof definitions / sizeof definitions[0], commit_floppy, refresh_floppy_choices,
};

} // namespace

const domain& floppy_domain()
{
	return floppy;
}

} // namespace amiberry_settings
