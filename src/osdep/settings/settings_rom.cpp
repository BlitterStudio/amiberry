// ROM, Kickstart, cartridge, flash/RTC and UAE expansion board options
// (the ROM panel). Paths are stored verbatim; maprom uses the same
// hexadecimal token the configuration writes. read_kickstart_version() is
// NOT part of the setter: it writes the core's global kickstart_version
// and cloanto_rom state, so the panel refreshes it on interaction with the
// pending configuration, and fixup_prefs does it on apply.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "rommgr.h"

#include "settings_definitions.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace amiberry_settings {

namespace {

int choice_index(const amiberry_setting_choice* choices, size_t count, std::string_view text)
{
	for (size_t i = 0; i < count; i++) {
		if (text == choices[i].value)
			return static_cast<int>(i);
	}
	return -1;
}

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

void write_bool(char* out, size_t size, bool v)
{
	std::snprintf(out, size, "%s", v ? "true" : "false");
}

void write_path(char* out, size_t size, const TCHAR* path)
{
	std::snprintf(out, size, "%s", path ? path : "");
}

void set_path(TCHAR* field, size_t size, const value& v)
{
	std::snprintf(field, size, "%.*s", static_cast<int>(v.text.size()), v.text.data());
}

// ------------------------------------------------------------------- ROMs

void get_kickstart_rom_file(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.romfile);
}

void set_kickstart_rom_file(uae_prefs& p, const value& v)
{
	set_path(p.romfile, sizeof p.romfile, v);
}

void get_kickstart_ext_rom_file(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.romextfile);
}

void set_kickstart_ext_rom_file(uae_prefs& p, const value& v)
{
	set_path(p.romextfile, sizeof p.romextfile, v);
}

void get_cart_file(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.cartfile);
}

// cart= (cartident) is written after cart_file= and wins on load
// (decode_rom_ident), so it follows the file: a scanned cartridge dump
// carries its romdata configname, anything else clears it so a stale ident
// cannot bring the previous cartridge back or mis-type an unknown ROM. Only
// a real change does this: writing back the current path keeps an ident
// typed for a custom dump.
void set_cart_file(uae_prefs& p, const value& v)
{
	if (v.text == std::string_view(p.cartfile))
		return;
	set_path(p.cartfile, sizeof p.cartfile, v);
	p.cartident[0] = 0;
	const int count = romlist_count();
	const romlist* const list = romlist_getit();
	for (int i = 0; list && i < count; i++) {
		const romlist& entry = list[i];
		if (!entry.rd || !(entry.rd->type & (ROMTYPE_FREEZER | ROMTYPE_ARCADIAGAME | ROMTYPE_CD32CART)))
			continue;
		if (!entry.path || std::strcmp(entry.path, p.cartfile) != 0)
			continue;
		if (entry.rd->configname)
			std::snprintf(p.cartident, sizeof p.cartident, ":%s", entry.rd->configname);
		break;
	}
}

void get_flash_file(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.flashfile);
}

void set_flash_file(uae_prefs& p, const value& v)
{
	set_path(p.flashfile, sizeof p.flashfile, v);
}

void get_rtc_file(const uae_prefs& p, char* out, size_t size)
{
	write_path(out, size, p.rtcfile);
}

void set_rtc_file(uae_prefs& p, const value& v)
{
	set_path(p.rtcfile, sizeof p.rtcfile, v);
}

// ------------------------------------------------------------------ maprom

// cfgfile writes maprom as 0x%x; cfgfile_intval reads it back with hex
// detection. The layer keeps the exact token.
void get_maprom(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "0x%x", p.maprom);
}

// The whole value must be one hexadecimal number (with or without 0x)
// that fits the 32-bit address.
bool parse_maprom(std::string_view text, uae_u32& address)
{
	char buffer[max_value_length];
	if (text.empty() || text.size() >= sizeof buffer)
		return false;
	std::memcpy(buffer, text.data(), text.size());
	buffer[text.size()] = 0;
	if (buffer[0] == '-' || buffer[0] == '+' || std::isspace(static_cast<unsigned char>(buffer[0])))
		return false;
	char* end = nullptr;
	const unsigned long long parsed = std::strtoull(buffer, &end, 16);
	if (end == buffer || *end != 0 || parsed > 0xffffffffULL)
		return false;
	address = static_cast<uae_u32>(parsed);
	return true;
}

bool maprom_accepts(const uae_prefs&, const value& v)
{
	uae_u32 address;
	return parse_maprom(v.text, address);
}

void set_maprom(uae_prefs& p, const value& v)
{
	uae_u32 address;
	if (parse_maprom(v.text, address))
		p.maprom = address;
}

// An accelerator board owns the maprom window; fixup_prefs clears maprom
// when one is configured, so the option is not available there.
bool maprom_available(const uae_prefs& p, const context&)
{
	return p.cpuboard_type == 0;
}

void get_kickshifter(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.kickshifter);
}

void set_kickshifter(uae_prefs& p, const value& v)
{
	p.kickshifter = v.number != 0;
}

// The UAE expansion board and its boot ROM are two configuration keys. The
// ROM panel offers the five meaningful combinations as one combo: entry 0
// disables both, entry 1 ("Original UAE") keeps the automatic boot ROM
// without a board, and the rest pick the board with the automatic ROM,
// exactly like the old panel mapped them. The *_off spellings are the same
// boards without the diagnostics ROM (uaeboard_nodiag); cfgfile writes them
// whenever that flag is set, so the getter reports them too.
const amiberry_setting_choice uaeboard_choices[] = {
	{"disabled", "Disabled"},
	{"min", "New UAE (64k + F0 ROM)"},
	{"full", "New UAE (128k, ROM, Direct)"},
	{"full+indirect", "New UAE (128k, ROM, Indirect)"},
	{"disabled_off", "Disabled, no diagnostics ROM"},
	{"min_off", "New UAE (64k + F0 ROM), no diagnostics ROM"},
	{"full_off", "New UAE (128k, ROM, Direct), no diagnostics ROM"},
	{"full+indirect_off", "New UAE (128k, ROM, Indirect), no diagnostics ROM"},
};

constexpr int uaeboard_types = 4;

void get_uaeboard(const uae_prefs& p, char* out, size_t size)
{
	int index = p.uaeboard;
	if (index < 0 || index >= uaeboard_types)
		index = 0;
	if (p.uaeboard_nodiag)
		index += uaeboard_types;
	std::snprintf(out, size, "%s", uaeboard_choices[index].value);
}

void set_uaeboard(uae_prefs& p, const value& v)
{
	const int index = choice_index(CHOICES(uaeboard_choices), v.text);
	p.uaeboard = index % uaeboard_types;
	p.uaeboard_nodiag = index >= uaeboard_types;
}

const amiberry_setting_choice boot_rom_choices[] = {
	{"automatic", "Automatic"},
	{"disabled", "Disabled"},
	{"min", "Minimal"},
	{"full", "Full"},
};

void get_boot_rom(const uae_prefs& p, char* out, size_t size)
{
	int index = p.boot_rom;
	if (index < 0 || static_cast<size_t>(index) >= sizeof boot_rom_choices / sizeof boot_rom_choices[0])
		index = 0;
	std::snprintf(out, size, "%s", boot_rom_choices[index].value);
}

void set_boot_rom(uae_prefs& p, const value& v)
{
	p.boot_rom = choice_index(CHOICES(boot_rom_choices), v.text);
}

// The board rewires autoconfig boards, so it can only change while the
// emulator is stopped (the old panel disabled the control while emulating).
bool not_emulating(const uae_prefs&, const context& ctx)
{
	return !ctx.emulating;
}

// ---------------------------------------------------------------- table

constexpr double no_range = 0;

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, CHOICES(choices), constraint}
#define PATH_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_PATH, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}

const definition definitions[] = {
	{PATH_OPTION("kickstart_rom_file", "rom", "Main ROM file",
			"Kickstart ROM image. The default configuration picks the first suitable installed ROM.",
			AMIBERRY_APPLY_RESET, "", nullptr),
		get_kickstart_rom_file, set_kickstart_rom_file, nullptr, nullptr, nullptr},
	{PATH_OPTION("kickstart_ext_rom_file", "rom", "Extended ROM file",
			"Extended ROM image (CD32/CDTV extension or Arcadia bios).",
			AMIBERRY_APPLY_RESET, "", nullptr),
		get_kickstart_ext_rom_file, set_kickstart_ext_rom_file, nullptr, nullptr, nullptr},
	{PATH_OPTION("cart_file", "rom", "Cartridge ROM file",
			"Cartridge (Freezer, Action Replay, Arcadia game) ROM image.",
			AMIBERRY_APPLY_RESET, "", nullptr),
		get_cart_file, set_cart_file, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("boot_rom_uae", "rom", "UAE boot ROM",
			"UAE boot ROM type. Automatic uses the FS + F0 ROM when software needs it.",
			AMIBERRY_APPLY_RESTART, "automatic", boot_rom_choices, nullptr),
		get_boot_rom, set_boot_rom, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("uaeboard", "rom", "UAE expansion board",
			"UAE expansion board (needs the UAE boot ROM), for software that requires it.",
			AMIBERRY_APPLY_RESTART, "disabled", uaeboard_choices,
			"The board rewires expansion hardware, so it can only change while the emulator is stopped."),
		get_uaeboard, set_uaeboard, not_emulating, nullptr, nullptr},
	{PATH_OPTION("flash_file", "rom", "Flash RAM file",
			"Flash RAM or A2286/A2386SX BIOS CMOS RAM save file.",
			AMIBERRY_APPLY_RESET, "", nullptr),
		get_flash_file, set_flash_file, nullptr, nullptr, nullptr},
	{PATH_OPTION("rtc_file", "rom", "Real time clock file",
			"Real-time clock battery-backed RAM save file.",
			AMIBERRY_APPLY_RESET, "", nullptr),
		get_rtc_file, set_rtc_file, nullptr, nullptr, nullptr},
	{STRING_OPTION("maprom", "rom", "MapROM emulation",
			"Map the ROM into fast RAM (hexadecimal address, 0x0f000000 switches it on). The core relocates the window for the address space.",
			AMIBERRY_APPLY_RESET, "0x0",
			"UAE MapROM and accelerator board emulation are not compatible."),
		get_maprom, set_maprom, maprom_available, nullptr, nullptr, maprom_accepts},
	{BOOL_OPTION("kickshifter", "rom", "ShapeShifter support",
			"Enable support for the ShapeShifter Mac emulator.",
			AMIBERRY_APPLY_RESTART, "false", nullptr),
		get_kickshifter, set_kickshifter, nullptr, nullptr, nullptr},
};

bool commit_rom(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
	const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(romfile);
	FIELD(romextfile);
	FIELD(romextfile2);
	FIELD(romextfile2addr);
	FIELD(flashfile);
	FIELD(rtcfile);
	FIELD(cartfile);
	FIELD(cartident);
	FIELD(maprom);
	FIELD(kickshifter);
	FIELD(uaeboard);
	FIELD(uaeboard_nodiag);
	FIELD(uaeboard_order);
	FIELD(boot_rom);
	FIELD(romboards);
#undef FIELD
	return wrote;
}

#undef BOOL_OPTION
#undef CHOICE_OPTION
#undef PATH_OPTION
#undef STRING_OPTION
#undef CHOICES

const domain rom = {
	definitions, sizeof definitions / sizeof definitions[0], commit_rom,
};

} // namespace

const domain& rom_domain()
{
	return rom;
}

} // namespace amiberry_settings
