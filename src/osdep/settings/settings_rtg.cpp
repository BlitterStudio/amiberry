// RTG board options: the GUI's RTG board panel. The first RTG board (the one
// the panel edits): its type and memory, how it shares the display with the
// native chipset, the UAE board's colour modes and helpers, and how the RTG
// screen is scaled and timed.
//
// gfxcard_options and rtg_modes pack several settings into one
// configuration entry. Each is an option spelled as the configuration
// writes it, and its parts are options too ("gfxcard_options:monitor",
// "rtg_modes:16bit"), so a front end can show them as separate controls.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "gfxboard.h"
#include "rtgmodes.h"
#include "ethernet.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

struct netdriverdata **target_ethernet_enumerate(void);

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

constexpr int megabyte = 1024 * 1024;

const rtgboardconfig& board(const uae_prefs& p)
{
	return p.rtgboards[0];
}

rtgboardconfig& board(uae_prefs& p)
{
	return p.rtgboards[0];
}

bool has_memory(const uae_prefs& p)
{
	return board(p).rtgmem_size > 0;
}

bool is_uae_board(const uae_prefs& p)
{
	return board(p).rtgmem_type < GFXBOARD_HARDWARE;
}

bool is_zz9000(const uae_prefs& p)
{
	return has_memory(p)
		&& (board(p).rtgmem_type == GFXBOARD_ID_ZZ9000_Z2 || board(p).rtgmem_type == GFXBOARD_ID_ZZ9000_Z3);
}

// The memory a board type takes, in MB: the board's own range, 8 MB at most
// on the UAE Zorro II board and 256 MB on the Zorro III one.
void vram_range_mb(int type, int& min_mb, int& max_mb)
{
	int vmin = 0, vmax = 0;
	if (gfxboard_get_vram_range(type, &vmin, &vmax)) {
		min_mb = vmin > 0 ? vmin / megabyte : 1;
		max_mb = vmax > 0 ? vmax / megabyte : 256;
		return;
	}
	min_mb = 1;
	max_mb = type == GFXBOARD_UAE_Z2 ? 8 : 256;
}

// ---------------------------------------------------------------- board

int type_from_config_name(std::string_view name);

// fixup_prefs(): a Zorro III board needs 32-bit addressing, and a Zorro II
// board leaves the Zorro II space no room for more than 2 MB chip RAM.
bool type_fits(const uae_prefs& p, int type)
{
	const int bus = gfxboard_get_configtype_of(type);
	if (bus == 3 && p.address_space_24)
		return false;
	if (bus == 2 && p.chipmem.size > 0x200000)
		return false;
	return true;
}


// gfxcard_size: the board's memory in MB; 0 means no RTG board. The
// configuration has neither gfxcard_size nor gfxcard_type then.
const amiberry_setting_choice size_choices[] = {
	{"0", "None"},
	{"1", "1 MB"},
	{"2", "2 MB"},
	{"4", "4 MB"},
	{"8", "8 MB"},
	{"16", "16 MB"},
	{"32", "32 MB"},
	{"64", "64 MB"},
	{"128", "128 MB"},
	{"256", "256 MB"},
};

void get_gfxcard_size(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, board(p).rtgmem_size / megabyte);
}

void set_gfxcard_size(uae_prefs& p, const value& v)
{
	rtgboardconfig& rbc = board(p);
	const bool was_on = rbc.rtgmem_size > 0;
	const int mb = static_cast<int>(v.number);
	rbc.rtgmem_size = mb * megabyte;
	if (mb == 0) {
		// As read back from a configuration without the board.
		rbc.rtgmem_type = GFXBOARD_UAE_Z3;
		rbc.monitor_id = 0;
		rbc.autoswitch = true;
		rbc.initial_active = false;
		return;
	}
	// A board turned on where its type does not fit (the default UAE
	// Zorro III board under 24-bit addressing) becomes the UAE Zorro II one.
	if (!was_on && !type_fits(p, rbc.rtgmem_type) && type_fits(p, GFXBOARD_UAE_Z2)) {
		rbc.rtgmem_type = GFXBOARD_UAE_Z2;
		if (rbc.rtgmem_size > 8 * megabyte)
			rbc.rtgmem_size = 8 * megabyte;
	}
	// A UAE board starts with zero copy on, as the GUI's board list does.
	if (!was_on && rbc.rtgmem_type < GFXBOARD_HARDWARE)
		p.rtg_zerocopy = true;
}

// Sizes the board type takes; None always.
bool gfxcard_size_choice_available(const uae_prefs& p, std::string_view choice)
{
	const int mb = std::atoi(std::string(choice).c_str());
	if (mb == 0)
		return true;
	// With the board off, the type turning it on uses (see set_gfxcard_size).
	int type = board(p).rtgmem_type;
	if (!has_memory(p) && !type_fits(p, type))
		type = GFXBOARD_UAE_Z2;
	if (!type_fits(p, type))
		return false;
	int min_mb, max_mb;
	vram_range_mb(type, min_mb, max_mb);
	return mb >= min_mb && mb <= max_mb;
}

bool gfxcard_type_choice_available(const uae_prefs& p, std::string_view choice)
{
	const int type = type_from_config_name(choice);
	return type >= 0 && type_fits(p, type);
}

// gfxcard_type: the board, by the name the configuration uses. The list is
// the core's board table.
std::vector<std::string> type_values;
std::vector<std::string> type_labels;
std::vector<amiberry_setting_choice> type_items;

void gfxcard_type_choices(const uae_prefs&, choice_list& out)
{
	if (type_items.empty()) {
		for (int index = 0;; index++) {
			const int id = gfxboard_get_id_from_index(index);
			if (id < 0)
				break;
			const TCHAR* config = gfxboard_get_configname(id);
			const TCHAR* name = gfxboard_get_name(id);
			if (!config || !name)
				continue;
			std::string label = name;
			if (const TCHAR* maker = gfxboard_get_manufacturername(id))
				label += std::string(" (") + maker + ")";
			type_values.emplace_back(config);
			type_labels.push_back(std::move(label));
		}
		for (size_t i = 0; i < type_values.size(); i++)
			type_items.push_back({type_values[i].c_str(), type_labels[i].c_str()});
	}
	out.items = type_items.data();
	out.count = type_items.size();
}

int type_from_config_name(std::string_view name)
{
	for (int index = 0;; index++) {
		const int id = gfxboard_get_id_from_index(index);
		if (id < 0)
			return -1;
		const TCHAR* config = gfxboard_get_configname(id);
		if (config && name.size() == std::strlen(config) && !strncasecmp(config, name.data(), name.size()))
			return id;
	}
}

void get_gfxcard_type(const uae_prefs& p, char* out, size_t size)
{
	const TCHAR* config = gfxboard_get_configname(board(p).rtgmem_type);
	std::snprintf(out, size, "%s", config ? config : "");
}

// A new type keeps the memory where the board takes it: its own range for a
// hardware board, at most 8 MB on the UAE Zorro II board.
void set_gfxcard_type(uae_prefs& p, const value& v)
{
	rtgboardconfig& rbc = board(p);
	const int type = type_from_config_name(v.text);
	if (type < 0)
		return;
	const bool was_on = rbc.rtgmem_size > 0;
	rbc.rtgmem_type = type;
	int min_mb, max_mb;
	vram_range_mb(type, min_mb, max_mb);
	// Picked with no board on, a board comes with the memory the GUI gave
	// it: 4 MB on the UAE Zorro II board, 16 MB on the Zorro III one, a
	// hardware board's least (then fitted to its range below).
	if (!was_on) {
		rbc.rtgmem_size = (type == GFXBOARD_UAE_Z2 ? 4 : type == GFXBOARD_UAE_Z3 ? 16 : min_mb) * megabyte;
		if (type < GFXBOARD_HARDWARE)
			p.rtg_zerocopy = true;
	}
	int mb = rbc.rtgmem_size / megabyte;
	if (mb < min_mb)
		mb = min_mb;
	if (mb > max_mb)
		mb = max_mb;
	rbc.rtgmem_size = mb * megabyte;
	// target_fixup_options(): a hardware board has no UAE helpers.
	if (type >= GFXBOARD_HARDWARE) {
		p.rtg_hardwareinterrupt = false;
		p.rtg_hardwaresprite = false;
	}
}

bool board_available(const uae_prefs& p, const context&)
{
	return has_memory(p);
}

constexpr const char* board_constraint = "Needs an RTG board (its memory set).";

// ---------------------------------------------------------------- gfxcard_options

// As the configuration writer spells it: order=N (only with a custom board
// order), monitor=N, noautoswitch, initial; empty without a board.
void get_gfxcard_options(const uae_prefs& p, char* out, size_t size)
{
	const rtgboardconfig& rbc = board(p);
	std::string text;
	const auto append = [&text](const std::string& part) {
		if (!text.empty())
			text += ",";
		text += part;
	};
	if (rbc.rtgmem_size) {
		if (rbc.device_order > 0 && p.autoconfig_custom_sort)
			append("order=" + std::to_string(rbc.device_order));
		if (rbc.monitor_id)
			append("monitor=" + std::to_string(rbc.monitor_id));
		if (!rbc.autoswitch)
			append("noautoswitch");
		if (rbc.initial_active)
			append("initial");
	}
	std::snprintf(out, size, "%s", text.c_str());
}

struct parsed_options {
	bool ok = true;
	int order = -1;
	int monitor = 0;
	bool autoswitch = true;
	bool initial = false;
};

// As the parser reads it; unknown parts are refused rather than lost.
parsed_options parse_gfxcard_options(std::string_view text)
{
	parsed_options parsed;
	size_t start = 0;
	while (start <= text.size()) {
		size_t end = text.find(',', start);
		if (end == std::string_view::npos)
			end = text.size();
		const std::string part(text.substr(start, end - start));
		if (!part.empty()) {
			const size_t equals = part.find('=');
			const std::string key = part.substr(0, equals);
			const std::string number = equals == std::string::npos ? "" : part.substr(equals + 1);
			char* stop = nullptr;
			const long n = number.empty() ? 0 : std::strtol(number.c_str(), &stop, 10);
			const bool numeric = !number.empty() && stop && !*stop;
			if (key == "order" && numeric && n >= 0)
				parsed.order = static_cast<int>(n);
			else if (key == "monitor" && numeric && n >= 0 && n < MAX_AMIGAMONITORS)
				parsed.monitor = static_cast<int>(n);
			else if (part == "noautoswitch")
				parsed.autoswitch = false;
			else if (part == "autoswitch")
				parsed.autoswitch = true;
			else if (part == "initial")
				parsed.initial = true;
			else
				parsed.ok = false;
		}
		start = end + 1;
	}
	return parsed;
}

bool gfxcard_options_accepts(const uae_prefs&, const value& v)
{
	return parse_gfxcard_options(v.text).ok;
}

void set_initial_active(uae_prefs& p, bool initial)
{
	// One board starts on the RTG screen at most.
	if (initial) {
		for (int i = 0; i < MAX_RTG_BOARDS; i++)
			p.rtgboards[i].initial_active = false;
	}
	board(p).initial_active = initial;
}

void set_gfxcard_options(uae_prefs& p, const value& v)
{
	const parsed_options parsed = parse_gfxcard_options(v.text);
	rtgboardconfig& rbc = board(p);
	// The entry carries the order only under custom autoconfig sorting (as
	// the writer spells it), where an entry without one means none.
	if (parsed.order >= 0)
		rbc.device_order = parsed.order;
	else if (p.autoconfig_custom_sort)
		rbc.device_order = 0;
	rbc.monitor_id = parsed.monitor;
	rbc.autoswitch = parsed.autoswitch;
	set_initial_active(p, parsed.initial);
}

const amiberry_setting_choice monitor_choices[] = {
	{"0", "Monitor 1"},
	{"1", "Monitor 2"},
	{"2", "Monitor 3"},
	{"3", "Monitor 4"},
};
static_assert(MAX_AMIGAMONITORS == 4, "monitor_choices lists every monitor");

void get_monitor(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, board(p).monitor_id);
}

void set_monitor(uae_prefs& p, const value& v)
{
	board(p).monitor_id = static_cast<int>(v.number);
}

void get_autoswitch(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, board(p).autoswitch);
}

void set_autoswitch(uae_prefs& p, const value& v)
{
	board(p).autoswitch = v.number != 0;
}

// The UAE board always switches between the native and the RTG screen, and
// a board with its own switcher does it itself.
bool autoswitch_available(const uae_prefs& p, const context&)
{
	return has_memory(p) && !is_uae_board(p)
		&& !gfxboard_get_switcher(const_cast<rtgboardconfig*>(&board(p)));
}

void get_initial(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, board(p).initial_active);
}

void set_initial(uae_prefs& p, const value& v)
{
	set_initial_active(p, v.number != 0);
}

// ---------------------------------------------------------------- rtg_modes

// The UAE board's pixel formats, as the configuration writes them (0x...).
void get_rtg_modes(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "0x%x", static_cast<unsigned>(p.picasso96_modeflags));
}

bool parse_modes(std::string_view text, int& modes)
{
	const std::string s(text);
	if (s.empty())
		return false;
	// As the parser reads it: hexadecimal after 0x, decimal otherwise (a
	// leading 0 is not octal).
	const bool hex = s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X');
	char* stop = nullptr;
	const unsigned long n = std::strtoul(s.c_str() + (hex ? 2 : 0), &stop, hex ? 16 : 10);
	if (!stop || *stop || n > 0x7fffffffUL)
		return false;
	modes = static_cast<int>(n);
	return true;
}

bool rtg_modes_accepts(const uae_prefs&, const value& v)
{
	int modes;
	return parse_modes(v.text, modes);
}

void set_rtg_modes(uae_prefs& p, const value& v)
{
	int modes;
	if (parse_modes(v.text, modes))
		p.picasso96_modeflags = modes;
}

// One depth of the modes: none, all of its formats, or one format (as the
// GUI picks them; a configuration with a few reads as the first).
struct depth_formats {
	const char* id;
	const uint32_t* formats;
	size_t count;
	const amiberry_setting_choice* choices;
	size_t choice_count;
};

const uint32_t formats_8[] = {RGBFF_CLUT};
const uint32_t formats_16[] = {RGBFF_R5G6B5PC, RGBFF_R5G5B5PC, RGBFF_R5G6B5, RGBFF_R5G5B5, RGBFF_B5G6R5PC,
	RGBFF_B5G5R5PC};
const uint32_t formats_24[] = {RGBFF_R8G8B8, RGBFF_B8G8R8};
const uint32_t formats_32[] = {RGBFF_A8R8G8B8, RGBFF_A8B8G8R8, RGBFF_R8G8B8A8, RGBFF_B8G8R8A8};

const amiberry_setting_choice choices_8[] = {
	{"none", "None"},
	{"CLUT", "CLUT"},
};
const amiberry_setting_choice choices_16[] = {
	{"none", "None"},
	{"all", "All"},
	{"R5G6B5PC", "R5G6B5PC"},
	{"R5G5B5PC", "R5G5B5PC"},
	{"R5G6B5", "R5G6B5"},
	{"R5G5B5", "R5G5B5"},
	{"B5G6R5PC", "B5G6R5PC"},
	{"B5G5R5PC", "B5G5R5PC"},
};
const amiberry_setting_choice choices_24[] = {
	{"none", "None"},
	{"all", "All"},
	{"R8G8B8", "R8G8B8"},
	{"B8G8R8", "B8G8R8"},
};
const amiberry_setting_choice choices_32[] = {
	{"none", "None"},
	{"all", "All"},
	{"A8R8G8B8", "A8R8G8B8"},
	{"A8B8G8R8", "A8B8G8R8"},
	{"R8G8B8A8", "R8G8B8A8"},
	{"B8G8R8A8", "B8G8R8A8"},
};

#define DEPTH(n) {"rtg_modes:" #n "bit", formats_##n, sizeof formats_##n / sizeof formats_##n[0], \
	CHOICES(choices_##n)}
const depth_formats depths[] = {DEPTH(8), DEPTH(16), DEPTH(24), DEPTH(32)};
#undef DEPTH

uint32_t depth_mask(const depth_formats& d)
{
	uint32_t mask = 0;
	for (size_t i = 0; i < d.count; i++)
		mask |= d.formats[i];
	return mask;
}

void get_depth(const depth_formats& d, const uae_prefs& p, char* out, size_t size)
{
	const uint32_t modes = static_cast<uint32_t>(p.picasso96_modeflags);
	const uint32_t mask = depth_mask(d);
	const char* text = "none";
	if ((modes & mask) == mask && d.count > 1) {
		text = "all";
	} else {
		for (size_t i = 0; i < d.count; i++) {
			if (modes & d.formats[i]) {
				text = d.choices[i + (d.count > 1 ? 2 : 1)].value;
				break;
			}
		}
	}
	std::snprintf(out, size, "%s", text);
}

void set_depth(const depth_formats& d, uae_prefs& p, const value& v)
{
	uint32_t modes = static_cast<uint32_t>(p.picasso96_modeflags) & ~depth_mask(d);
	if (v.text == "all") {
		modes |= depth_mask(d);
	} else {
		for (size_t i = 0; i < d.count; i++) {
			if (v.text == d.choices[i + (d.count > 1 ? 2 : 1)].value)
				modes |= d.formats[i];
		}
	}
	p.picasso96_modeflags = static_cast<int>(modes);
}

#define DEPTH_ACCESSORS(n, index) \
	void get_depth_##n(const uae_prefs& p, char* out, size_t size) { get_depth(depths[index], p, out, size); } \
	void set_depth_##n(uae_prefs& p, const value& v) { set_depth(depths[index], p, v); }
DEPTH_ACCESSORS(8, 0)
DEPTH_ACCESSORS(16, 1)
DEPTH_ACCESSORS(24, 2)
DEPTH_ACCESSORS(32, 3)
#undef DEPTH_ACCESSORS

// The UAE board's own options.
bool uae_board_available(const uae_prefs& p, const context&)
{
	return has_memory(p) && is_uae_board(p);
}

constexpr const char* uae_board_constraint = "Only the UAE RTG board has it.";

// ---------------------------------------------------------------- helpers

#define BOOL_FIELD(field) \
	void get_##field(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_bool(out, size, p.field); \
	} \
	void set_##field(uae_prefs& p, const value& v) \
	{ \
		p.field = v.number != 0; \
	}

BOOL_FIELD(rtg_zerocopy)
BOOL_FIELD(rtg_multithread)
BOOL_FIELD(rtg_hardwaresprite)
BOOL_FIELD(rtg_hardwareinterrupt)
BOOL_FIELD(rtgallowscaling)
BOOL_FIELD(zz9000_int2)

#undef BOOL_FIELD

// ZZ9000 network: "none" or a host network driver.
std::vector<std::string> net_values;
std::vector<std::string> net_labels;
std::vector<amiberry_setting_choice> net_items;

void zz9000_net_choices(const uae_prefs&, choice_list& out)
{
	net_values.clear();
	net_labels.clear();
	net_items.clear();
	net_values.emplace_back("none");
	net_labels.emplace_back("Disconnected");
	if (netdriverdata** drivers = target_ethernet_enumerate()) {
		for (int i = 0; drivers[i]; i++) {
			// Unnamed entries (a backend the build lacks) cannot be chosen.
			if (!drivers[i]->name || !drivers[i]->name[0])
				continue;
			net_values.emplace_back(drivers[i]->name);
			net_labels.emplace_back(drivers[i]->desc && drivers[i]->desc[0] ? drivers[i]->desc : drivers[i]->name);
		}
	}
	for (size_t i = 0; i < net_values.size(); i++)
		net_items.push_back({net_values[i].c_str(), net_labels[i].c_str()});
	out.items = net_items.data();
	out.count = net_items.size();
}

void get_zz9000_net(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.zz9000net_name);
}

void set_zz9000_net(uae_prefs& p, const value& v)
{
	std::snprintf(p.zz9000net_name, sizeof p.zz9000net_name, "%.*s", static_cast<int>(v.text.size()),
		v.text.data());
}

bool zz9000_available(const uae_prefs& p, const context&)
{
	return is_zz9000(p);
}

// ---------------------------------------------------------------- screen

const amiberry_setting_choice autoscale_choices[] = {
	{"resize", "Resize"},
	{"scale", "Scale if smaller than the display"},
	{"center", "Always center"},
	{"integer", "Integer scaling"},
};
static_assert(RTG_MODE_SCALE == 1 && RTG_MODE_CENTER == 2 && RTG_MODE_INTEGER_SCALE == 3,
	"autoscale_choices follow the RTG_MODE_* numbers");

void get_autoscale_rtg(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, autoscale_choices, sizeof autoscale_choices / sizeof autoscale_choices[0],
		p.gf[GF_RTG].gfx_filter_autoscale);
}

void set_autoscale_rtg(uae_prefs& p, const value& v)
{
	for (size_t i = 0; i < sizeof autoscale_choices / sizeof autoscale_choices[0]; i++) {
		if (v.text == autoscale_choices[i].value)
			p.gf[GF_RTG].gfx_filter_autoscale = static_cast<int>(i);
	}
}

const amiberry_setting_choice scale_limit_choices[] = {
	{"1/1", "1/1"},
	{"1/2", "1/2"},
	{"1/4", "1/4"},
	{"1/8", "1/8"},
};

void get_scale_limit_rtg(const uae_prefs& p, char* out, size_t size)
{
	write_choice(out, size, scale_limit_choices, sizeof scale_limit_choices / sizeof scale_limit_choices[0],
		p.gf[GF_RTG].gfx_filter_integerscalelimit);
}

void set_scale_limit_rtg(uae_prefs& p, const value& v)
{
	for (size_t i = 0; i < sizeof scale_limit_choices / sizeof scale_limit_choices[0]; i++) {
		if (v.text == scale_limit_choices[i].value)
			p.gf[GF_RTG].gfx_filter_integerscalelimit = static_cast<int>(i);
	}
}

bool screen_available(const uae_prefs& p, const context&);

bool scale_limit_available(const uae_prefs& p, const context& ctx)
{
	return screen_available(p, ctx) && p.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE;
}

// The RTG screen's refresh: the chipset's, the host display's ("real"),
// none, or a rate in Hz.
const amiberry_setting_choice vblank_choices[] = {
	{"chipset", "Chipset"},
	{"real", "Default"},
	{"disabled", "Disabled"},
	{"50", "50 Hz"},
	{"60", "60 Hz"},
	{"70", "70 Hz"},
	{"75", "75 Hz"},
};

void get_rtg_vblank(const uae_prefs& p, char* out, size_t size)
{
	if (p.rtgvblankrate == -1)
		std::snprintf(out, size, "real");
	else if (p.rtgvblankrate == -2)
		std::snprintf(out, size, "disabled");
	else if (p.rtgvblankrate <= 0)
		std::snprintf(out, size, "chipset");
	else
		write_int(out, size, p.rtgvblankrate);
}

void set_rtg_vblank(uae_prefs& p, const value& v)
{
	if (v.text == "real")
		p.rtgvblankrate = -1;
	else if (v.text == "disabled")
		p.rtgvblankrate = -2;
	else if (v.text == "chipset")
		p.rtgvblankrate = 0;
	else
		p.rtgvblankrate = std::atoi(std::string(v.text).c_str());
}

const amiberry_setting_choice buffer_choices[] = {
	{"1", "Double buffering"},
	{"2", "Triple buffering"},
};

void get_backbuffers_rtg(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, p.gfx_apmode[APMODE_RTG].gfx_backbuffers);
}

void set_backbuffers_rtg(uae_prefs& p, const value& v)
{
	p.gfx_apmode[APMODE_RTG].gfx_backbuffers = static_cast<int>(v.number);
}

// The aspect ratio the RTG screen is scaled to keep, as the configuration
// writes it: -1:-1 automatic, 0:0 disabled (the GUI offers these two).
const amiberry_setting_choice aspect_choices[] = {
	{"0:0", "Disabled"},
	{"-1:-1", "Automatic"},
};

void get_rtg_aspect(const uae_prefs& p, char* out, size_t size)
{
	const int ratio = p.rtgscaleaspectratio;
	if (ratio < 0)
		std::snprintf(out, size, "-1:-1");
	else
		std::snprintf(out, size, "%d:%d", ratio / ASPECTMULT, ratio & (ASPECTMULT - 1));
}

void set_rtg_aspect(uae_prefs& p, const value& v)
{
	p.rtgscaleaspectratio = v.text == "0:0" ? 0 : -1;
}

bool screen_available(const uae_prefs& p, const context&)
{
	return has_memory(p) || !is_uae_board(p);
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

const definition definitions[] = {
	{CHOICE_OPTION("gfxcard_size", "rtg", "RTG board memory",
			"Video memory of the RTG graphics board; None removes the board.",
			AMIBERRY_APPLY_RESET, "0", size_choices,
			"Sizes outside the board's range are not available, nor any while no board fits the machine."),
		get_gfxcard_size, set_gfxcard_size, nullptr, gfxcard_size_choice_available, nullptr, nullptr, nullptr},
	{DYNAMIC_CHOICE_OPTION("gfxcard_type", "rtg", "RTG board",
			"The RTG graphics board: the UAE board AmigaOS drives through Picasso96, or an emulated card.",
			AMIBERRY_APPLY_RESET, "ZorroIII",
			"Zorro III boards need 32-bit addressing; Zorro II boards, at most 2 MB chip RAM."),
		get_gfxcard_type, set_gfxcard_type, nullptr, gfxcard_type_choice_available, nullptr, nullptr,
		gfxcard_type_choices},
	{STRING_OPTION("gfxcard_options", "rtg", "RTG board options",
			"The board's monitor, screen switching and start screen, as the configuration writes them.",
			AMIBERRY_APPLY_RESET, "", board_constraint),
		get_gfxcard_options, set_gfxcard_options, board_available, nullptr, nullptr, gfxcard_options_accepts,
		nullptr},
	{CHOICE_OPTION("gfxcard_options:monitor", "rtg", "RTG monitor",
			"Which emulated monitor shows the RTG screen.",
			AMIBERRY_APPLY_RESET, "0", monitor_choices, board_constraint),
		get_monitor, set_monitor, board_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfxcard_options:autoswitch", "rtg", "Native/RTG autoswitch",
			"Switch between the native and the RTG screen as the Amiga does.",
			AMIBERRY_APPLY_RESET, "true",
			"The UAE board always switches, and a board with its own switcher switches itself."),
		get_autoswitch, set_autoswitch, autoswitch_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfxcard_options:initial", "rtg", "Start on the RTG screen",
			"Show the RTG screen instead of the native chipset display at start.",
			AMIBERRY_APPLY_RESET, "false", board_constraint),
		get_initial, set_initial, board_available, nullptr, nullptr, nullptr, nullptr},
	{DYNAMIC_CHOICE_OPTION("zz9000_net", "rtg", "ZZ9000 network",
			"The host network the ZZ9000's network device uses.",
			AMIBERRY_APPLY_RESTART, "slirp", "Needs a ZZ9000 board."),
		get_zz9000_net, set_zz9000_net, zz9000_available, nullptr, nullptr, nullptr, zz9000_net_choices},
	{BOOL_OPTION("zz9000_int2", "rtg", "ZZ9000 INT2",
			"Interrupt on INT2 instead of INT6, matching the real card's ZZ9K_INT2 setting.",
			AMIBERRY_APPLY_RESTART, "false", "Needs a ZZ9000 board."),
		get_zz9000_int2, set_zz9000_int2, zz9000_available, nullptr, nullptr, nullptr, nullptr},
	{STRING_OPTION("rtg_modes", "rtg", "RTG colour modes",
			"The pixel formats the UAE board offers, as the configuration writes them.",
			AMIBERRY_APPLY_RESTART, "0x112", nullptr),
		get_rtg_modes, set_rtg_modes, nullptr, nullptr, nullptr, rtg_modes_accepts, nullptr},
	{CHOICE_OPTION("rtg_modes:8bit", "rtg", "8-bit modes", "The UAE board's 8-bit pixel format.",
			AMIBERRY_APPLY_RESTART, "CLUT", choices_8, uae_board_constraint),
		get_depth_8, set_depth_8, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("rtg_modes:16bit", "rtg", "15/16-bit modes", "The UAE board's 15- and 16-bit pixel formats.",
			AMIBERRY_APPLY_RESTART, "R5G6B5PC", choices_16, uae_board_constraint),
		get_depth_16, set_depth_16, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("rtg_modes:24bit", "rtg", "24-bit modes", "The UAE board's 24-bit pixel formats.",
			AMIBERRY_APPLY_RESTART, "none", choices_24, uae_board_constraint),
		get_depth_24, set_depth_24, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("rtg_modes:32bit", "rtg", "32-bit modes", "The UAE board's 32-bit pixel formats.",
			AMIBERRY_APPLY_RESTART, "R8G8B8A8", choices_32, uae_board_constraint),
		get_depth_32, set_depth_32, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfxcard_zerocopy", "rtg", "Zero copy",
			"Share the UAE board's memory with the display instead of copying it.",
			AMIBERRY_APPLY_LIVE, "true", uae_board_constraint),
		get_rtg_zerocopy, set_rtg_zerocopy, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfxcard_multithread", "rtg", "Multithreaded",
			"Draw the UAE board's screen on its own thread.",
			AMIBERRY_APPLY_LIVE, "false", uae_board_constraint),
		get_rtg_multithread, set_rtg_multithread, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfxcard_hardware_sprite", "rtg", "Hardware sprite emulation",
			"Show the mouse pointer as the UAE board's hardware sprite.",
			AMIBERRY_APPLY_LIVE, "true", uae_board_constraint),
		get_rtg_hardwaresprite, set_rtg_hardwaresprite, uae_board_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("gfxcard_hardware_vblank", "rtg", "Hardware vertical blank interrupt",
			"Give the UAE board a vertical blank interrupt of its own.",
			AMIBERRY_APPLY_LIVE, "false", uae_board_constraint),
		get_rtg_hardwareinterrupt, set_rtg_hardwareinterrupt, uae_board_available, nullptr, nullptr, nullptr,
		nullptr},
	{CHOICE_OPTION("gfx_filter_autoscale_rtg", "rtg", "RTG scaling",
			"How the RTG screen fits the window or display.",
			AMIBERRY_APPLY_LIVE, "scale", autoscale_choices, board_constraint),
		get_autoscale_rtg, set_autoscale_rtg, screen_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_filter_autoscale_limit_rtg", "rtg", "Integer scale limit",
			"Let integer scaling use 1/x steps when no whole multiple fits.",
			AMIBERRY_APPLY_LIVE, "1/1", scale_limit_choices, "Needs integer scaling."),
		get_scale_limit_rtg, set_scale_limit_rtg, scale_limit_available, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.rtg_scale_allow", "rtg", "Always scale in windowed mode",
			"Scale the RTG screen in a window too.",
			AMIBERRY_APPLY_LIVE, "false", board_constraint),
		get_rtgallowscaling, set_rtgallowscaling, screen_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.rtg_scale_aspect_ratio", "rtg", "RTG aspect ratio",
			"Keep the RTG screen's pixels square when it is scaled.",
			AMIBERRY_APPLY_LIVE, "-1:-1", aspect_choices, board_constraint),
		get_rtg_aspect, set_rtg_aspect, screen_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.rtg_vblank", "rtg", "RTG refresh rate",
			"The RTG screen's refresh: the chipset's, the display's (Default), or a rate.",
			AMIBERRY_APPLY_LIVE, "chipset", vblank_choices, board_constraint),
		get_rtg_vblank, set_rtg_vblank, screen_available, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("gfx_backbuffers_rtg", "rtg", "RTG buffering",
			"Buffers for the RTG screen; triple buffering tears less and uses more memory.",
			AMIBERRY_APPLY_LIVE, "1", buffer_choices, board_constraint),
		get_backbuffers_rtg, set_backbuffers_rtg, screen_available, nullptr, nullptr, nullptr, nullptr},
};

bool commit_rtg(const uae_prefs& base, const uae_prefs& from, uae_prefs& to, const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		FIELD(rtgboards[i].rtgmem_size);
		FIELD(rtgboards[i].rtgmem_type);
		FIELD(rtgboards[i].device_order);
		FIELD(rtgboards[i].monitor_id);
		FIELD(rtgboards[i].autoswitch);
		FIELD(rtgboards[i].initial_active);
	}
	FIELD(picasso96_modeflags);
	FIELD(rtg_zerocopy);
	FIELD(rtg_multithread);
	FIELD(rtg_hardwaresprite);
	FIELD(rtg_hardwareinterrupt);
	FIELD(rtgallowscaling);
	FIELD(rtgscaleaspectratio);
	FIELD(rtgvblankrate);
	FIELD(zz9000net_name);
	FIELD(zz9000_int2);
	FIELD(gf[GF_RTG].gfx_filter_autoscale);
	FIELD(gf[GF_RTG].gfx_filter_integerscalelimit);
	FIELD(gfx_apmode[APMODE_RTG].gfx_backbuffers);
#undef FIELD
	return wrote;
}

const domain rtg = {
	definitions, sizeof definitions / sizeof definitions[0], commit_rtg, nullptr,
};

} // namespace

const domain& rtg_domain()
{
	return rtg;
}

} // namespace amiberry_settings
