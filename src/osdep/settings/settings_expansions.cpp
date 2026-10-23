// Expansion options: the GUI's accelerator board (its type, jumper settings,
// memory and boot ROM) and the miscellaneous expansions (bsdsocket.library,
// uaescsi.device, uaenet.device).
//
// cpuboard_type names a board and its variant in one value, the variant's
// configuration name ("Blizzard1230IV"), as the configuration writes it.
// cpuboard_settings packs the variant's jumpers into one entry; each jumper
// of every board is also an option, available while the selected variant
// has it, so a front end can show them as separate controls: a switch is a
// BOOL named after it ("cpuboard_settings:maprom"), a selector a CHOICE of
// its positions named after the selector ("cpuboard_settings:hwrev", with
// "_choice" added when a switch on another board has the same name).

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "autoconf.h"
#include "cpuboard.h"
#include "rommgr.h"

#include "settings_definitions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

constexpr uae_u32 megabyte = 1024 * 1024;

// A jumper list as cpuboard_settings spells it is short; the core's parser
// copies it into a MAX_DPATH buffer with a comma added.
constexpr size_t max_settings_text = 256;

// ------------------------------------------------------------ board types

int board_count()
{
	static const int count = [] {
		int n = 0;
		while (cpuboards[n].name)
			n++;
		return n;
	}();
	return count;
}

// The selected variant; nullptr without a board (or with indices the table
// does not have).
const cpuboardsubtype* variant(const uae_prefs& p)
{
	if (p.cpuboard_type <= 0 || p.cpuboard_type >= board_count() || p.cpuboard_subtype < 0)
		return nullptr;
	const cpuboardsubtype* subtypes = cpuboards[p.cpuboard_type].subtypes;
	for (int i = 0; subtypes && i <= p.cpuboard_subtype; i++) {
		if (!subtypes[i].name)
			return nullptr;
	}
	return subtypes ? &subtypes[p.cpuboard_subtype] : nullptr;
}

uae_u32 board_memory_limit(const uae_prefs& p)
{
	const cpuboardsubtype* v = variant(p);
	return v ? static_cast<uae_u32>(v->maxmemory) : 0;
}

bool has_board(const uae_prefs& p)
{
	return variant(p) != nullptr;
}

bool variant_available(int type, int subtype)
{
#ifdef WITH_PPC
	(void)type;
	(void)subtype;
	return true;
#else
	return !cpuboard_is_ppc_accelerator_subtype(type, subtype);
#endif
}

// "Variant (maker)", or the maker's board name alone when it has one variant
// named like it.
std::vector<std::string> type_values;
std::vector<std::string> type_labels;
std::vector<amiberry_setting_choice> type_items;

void cpuboard_type_choices(const uae_prefs&, choice_list& out)
{
	if (type_items.empty()) {
		type_values.emplace_back("none");
		type_labels.emplace_back("None");
		for (int type = 1; cpuboards[type].name; type++) {
			const cpuboardsubtype* subtypes = cpuboards[type].subtypes;
			for (int sub = 0; subtypes && subtypes[sub].name; sub++) {
				type_values.emplace_back(subtypes[sub].configname);
				type_labels.push_back(std::string(subtypes[sub].name) + " (" + cpuboards[type].name + ")");
			}
		}
		for (size_t i = 0; i < type_values.size(); i++)
			type_items.push_back({type_values[i].c_str(), type_labels[i].c_str()});
	}
	out.items = type_items.data();
	out.count = type_items.size();
}

// The board and variant a configuration name selects, as the parser finds
// it (case-insensitive). false for an unknown name.
bool find_variant(std::string_view name, int& type, int& subtype)
{
	for (int t = 1; cpuboards[t].name; t++) {
		const cpuboardsubtype* subtypes = cpuboards[t].subtypes;
		for (int s = 0; subtypes && subtypes[s].name; s++) {
			const char* config = subtypes[s].configname;
			if (name.size() == std::strlen(config) && !strncasecmp(config, name.data(), name.size())) {
				type = t;
				subtype = s;
				return true;
			}
		}
	}
	return false;
}

void get_cpuboard_type(const uae_prefs& p, char* out, size_t size)
{
	const cpuboardsubtype* v = variant(p);
	std::snprintf(out, size, "%s", v ? v->configname : "none");
}

bool cpuboard_type_choice_available(const uae_prefs&, std::string_view choice)
{
	int type, subtype;
	if (choice == "none")
		return true;
	return find_variant(choice, type, subtype) && variant_available(type, subtype);
}

// Fits the board RAM to the board and puts it in the memory slot the board
// uses, as fixup_prefs() would.
void fit_board_memory(uae_prefs& p)
{
	if (has_board(p) && p.cpuboardmem1.size > board_memory_limit(p))
		p.cpuboardmem1.size = board_memory_limit(p);
	mirror_cpuboard_mem(p);
}

// Another board (or variant) comes with its own jumpers, sets the CPU it
// needs and leaves the automatic PPC configuration, which would put its own
// board back: a PPC board is the manual PPC configuration, any other none.
void set_cpuboard_type(uae_prefs& p, const value& v)
{
	int type = 0, subtype = 0;
	if (v.text != "none" && !find_variant(v.text, type, subtype))
		return;
	if (type == p.cpuboard_type && subtype == p.cpuboard_subtype)
		return;
	// The memory slot the old board's RAM was copied to goes with it when
	// the new board puts its RAM elsewhere (WinUAE clears the 25-bit slot
	// whenever no board is left; the slot is not an option of its own).
	const int old_memory = cpuboard_memorytype(&p);
	p.cpuboard_type = type;
	p.cpuboard_subtype = subtype;
	p.cpuboard_settings = 0;
	if (old_memory != cpuboard_memorytype(&p)) {
		if (old_memory == BOARD_MEMORY_HIGHMEM && p.mbresmem_high.size == p.cpuboardmem1.size)
			p.mbresmem_high.size = 0;
		else if (old_memory == BOARD_MEMORY_Z2 && p.fastmem[0].size == p.cpuboardmem1.size)
			p.fastmem[0].size = 0;
		else if (old_memory == BOARD_MEMORY_25BITMEM)
			p.mem25bit.size = 0;
	}
	if (!type)
		p.mem25bit.size = 0;
	if (cpuboard_is_ppc_accelerator(&p))
		p.ppc_mode = 2;
	else
		p.ppc_mode = 0;
	cpuboard_set_cpu(&p);
	fit_board_memory(p);
}

// ---------------------------------------------------------------- jumpers

const expansionboardsettings* jumpers(const uae_prefs& p)
{
	const cpuboardsubtype* v = variant(p);
	return v ? v->settings : nullptr;
}

bool settings_available(const uae_prefs& p, const context&)
{
	return jumpers(p) != nullptr;
}

// The configuration name list of a selector: its own name, then one per
// position, each NUL-terminated, an empty one last.
std::vector<const char*> split_names(const char* names)
{
	std::vector<const char*> out;
	for (const char* n = names; n && *n; n += std::strlen(n) + 1)
		out.push_back(n);
	return out;
}

// Where an entry's bits are in the settings word, as the core's codec lays
// them out (cfgfile_read_rom_settings): a switch takes one bit, a selector
// the bits its positions need, a text none.
struct bit_field {
	int shift;
	int bits;
};

bit_field field_of(const expansionboardsettings* table, int index)
{
	int bitcnt = 0;
	for (int i = 0; table[i].name; i++) {
		const expansionboardsettings& e = table[i];
		bitcnt += e.bitshift;
		int bits = 0;
		if (e.type == EXPANSIONBOARD_MULTI) {
			const int items = static_cast<int>(split_names(e.configname).size()) - 1;
			bits = 1;
			for (int j = 0; j < 8; j++) {
				if ((1 << j) >= items) {
					bits = j;
					break;
				}
			}
		} else if (e.type == EXPANSIONBOARD_CHECKBOX) {
			bits = 1;
		}
		if (i == index)
			return {bitcnt, bits};
		bitcnt += bits;
	}
	return {0, 0};
}

bool has_text_entries(const expansionboardsettings* table)
{
	for (int i = 0; table && table[i].name; i++) {
		if (table[i].type == EXPANSIONBOARD_STRING)
			return true;
	}
	return false;
}

// The core's reader writes a text entry's value to configtext whether or
// not the text names it, so a table with one needs somewhere to put it.
int read_settings(const expansionboardsettings* table, const char* text)
{
	TCHAR configtext[MAX_DPATH];
	return cfgfile_read_rom_settings(table, text, has_text_entries(table) ? configtext : nullptr);
}

void get_cpuboard_settings(const uae_prefs& p, char* out, size_t size)
{
	// As the writer: no entry while no jumper is set.
	TCHAR text[MAX_DPATH] = "";
	const expansionboardsettings* table = jumpers(p);
	if (table && p.cpuboard_settings && !has_text_entries(table))
		cfgfile_write_rom_settings(table, text, p.cpuboard_settings, nullptr);
	std::snprintf(out, size, "%s", text);
}

bool names_entry(const expansionboardsettings& e, std::string_view token)
{
	auto same = [token](const char* name) {
		return token.size() == std::strlen(name) && !strncasecmp(name, token.data(), token.size());
	};
	if (e.type == EXPANSIONBOARD_CHECKBOX)
		return same(e.configname);
	if (e.type == EXPANSIONBOARD_MULTI) {
		const std::vector<const char*> names = split_names(e.configname);
		for (size_t i = 1; i < names.size(); i++) {
			if (same(names[i]))
				return true;
		}
	}
	return false;
}

// Every comma-separated name is a switch or a selector position of the
// variant (the parser would drop anything else silently).
bool cpuboard_settings_accepts(const uae_prefs& p, const value& v)
{
	const expansionboardsettings* table = jumpers(p);
	if (!table || has_text_entries(table) || v.text.size() >= max_settings_text)
		return false;
	size_t start = 0;
	while (start <= v.text.size()) {
		size_t end = v.text.find(',', start);
		if (end == std::string_view::npos)
			end = v.text.size();
		const std::string_view token = v.text.substr(start, end - start);
		if (!token.empty() || end < v.text.size()) {
			bool known = false;
			for (int i = 0; table[i].name && !known; i++)
				known = names_entry(table[i], token);
			if (!known)
				return false;
		}
		start = end + 1;
	}
	return true;
}

// No text is no entry, as the writer leaves it for 0 (the reader would turn
// inverted switches round).
void set_cpuboard_settings(uae_prefs& p, const value& v)
{
	const expansionboardsettings* table = jumpers(p);
	if (!table)
		return;
	const std::string text(v.text);
	p.cpuboard_settings = text.empty() ? 0 : read_settings(table, text.c_str());
}

// One option per distinct switch or selector of every board, built from the
// board tables when the domain is first used. Each option's functions know
// their part by its slot.
struct jumper_part {
	std::string id;
	std::string label;
	std::string config; // the switch's or selector's configuration name
	int type;           // EXPANSIONBOARD_CHECKBOX or EXPANSIONBOARD_MULTI
	std::vector<std::string> values;
	std::vector<std::string> labels;
	std::vector<amiberry_setting_choice> choices;
};

constexpr size_t max_jumper_parts = 48;
std::vector<jumper_part>& jumper_parts()
{
	static std::vector<jumper_part> parts;
	return parts;
}

// The variant's entry for a part, with its index; nullptr without one.
const expansionboardsettings* entry_of(const uae_prefs& p, const jumper_part& part, int& index)
{
	const expansionboardsettings* table = jumpers(p);
	for (int i = 0; table && table[i].name; i++) {
		const expansionboardsettings& e = table[i];
		if (e.type == part.type && !_tcsicmp(split_names(e.configname)[0], part.config.c_str())) {
			index = i;
			return table;
		}
	}
	return nullptr;
}

template <size_t Slot>
bool jumper_available(const uae_prefs& p, const context&)
{
	int index;
	return entry_of(p, jumper_parts()[Slot], index) != nullptr;
}

// A part's position in the settings word: a switch is on (or a selector at
// position n) as the old panel showed it, an inverted entry turned round.
template <size_t Slot>
void get_jumper(const uae_prefs& p, char* out, size_t size)
{
	const jumper_part& part = jumper_parts()[Slot];
	int index = 0;
	const expansionboardsettings* table = entry_of(p, part, index);
	if (!table) {
		std::snprintf(out, size, "%s", part.type == EXPANSIONBOARD_CHECKBOX ? "false" : part.values[0].c_str());
		return;
	}
	const bit_field f = field_of(table, index);
	const int mask = (1 << f.bits) - 1;
	int position = (p.cpuboard_settings >> f.shift) & mask;
	if (table[index].invert)
		position ^= mask;
	if (part.type == EXPANSIONBOARD_CHECKBOX) {
		std::snprintf(out, size, "%s", position ? "true" : "false");
		return;
	}
	// The board's own positions (boards sharing a selector name may number
	// theirs differently); out of range reads as the first, as the panel did.
	const std::vector<const char*> names = split_names(table[index].configname);
	const size_t n = static_cast<size_t>(position) + 1;
	std::snprintf(out, size, "%s", n < names.size() ? names[n] : names.size() > 1 ? names[1] : "");
}

template <size_t Slot>
void set_jumper(uae_prefs& p, const value& v)
{
	const jumper_part& part = jumper_parts()[Slot];
	int index = 0;
	const expansionboardsettings* table = entry_of(p, part, index);
	if (!table)
		return;
	const bit_field f = field_of(table, index);
	const int mask = (1 << f.bits) - 1;
	int position = 0;
	if (part.type == EXPANSIONBOARD_CHECKBOX) {
		position = v.number != 0;
	} else {
		const std::vector<const char*> names = split_names(table[index].configname);
		for (size_t k = 1; k < names.size(); k++) {
			if (v.text == names[k])
				position = static_cast<int>(k - 1);
		}
	}
	if (table[index].invert)
		position ^= mask;
	p.cpuboard_settings = (p.cpuboard_settings & ~(mask << f.shift)) | ((position & mask) << f.shift);
}

// A selector's positions are those of the variant that has it (boards that
// share a selector name may have fewer).
template <size_t Slot>
bool jumper_choice_available(const uae_prefs& p, std::string_view choice)
{
	const jumper_part& part = jumper_parts()[Slot];
	int index = 0;
	const expansionboardsettings* table = entry_of(p, part, index);
	if (!table)
		return false;
	const std::vector<const char*> names = split_names(table[index].configname);
	for (size_t i = 1; i < names.size(); i++) {
		if (choice == names[i])
			return true;
	}
	return false;
}

struct jumper_functions {
	void (*get)(const uae_prefs&, char*, size_t);
	void (*set)(uae_prefs&, const value&);
	bool (*available)(const uae_prefs&, const context&);
	bool (*choice_available)(const uae_prefs&, std::string_view);
};

template <size_t... Slots>
constexpr std::array<jumper_functions, sizeof...(Slots)> make_jumper_functions(std::index_sequence<Slots...>)
{
	return {{{get_jumper<Slots>, set_jumper<Slots>, jumper_available<Slots>, jumper_choice_available<Slots>}...}};
}

constexpr std::array<jumper_functions, max_jumper_parts> jumper_function_table =
	make_jumper_functions(std::make_index_sequence<max_jumper_parts>{});

// Collects the parts from every variant's table, first label first.
void collect_jumper_parts()
{
	std::vector<jumper_part>& parts = jumper_parts();
	auto find_part = [&parts](const char* config, int type) -> jumper_part* {
		for (jumper_part& part : parts) {
			if (part.type == type && !_tcsicmp(part.config.c_str(), config))
				return &part;
		}
		return nullptr;
	};
	for (int t = 1; cpuboards[t].name; t++) {
		const cpuboardsubtype* subtypes = cpuboards[t].subtypes;
		for (int s = 0; subtypes && subtypes[s].name; s++) {
			const expansionboardsettings* table = subtypes[s].settings;
			for (int i = 0; table && table[i].name; i++) {
				const expansionboardsettings& e = table[i];
				if (e.type != EXPANSIONBOARD_CHECKBOX && e.type != EXPANSIONBOARD_MULTI)
					continue;
				const std::vector<const char*> configs = split_names(e.configname);
				const std::vector<const char*> names = split_names(e.name);
				if (configs.empty() || names.empty())
					continue;
				jumper_part* part = find_part(configs[0], e.type);
				if (!part) {
					parts.push_back({});
					part = &parts.back();
					part->config = configs[0];
					part->type = e.type;
					part->label = names[0];
				} else if (part->label != names[0]) {
					// Boards label (even mean) it differently ("Autoboot"
					// on one, "Autoboot disable" on another): the option
					// names the jumper itself.
					part->label = std::string(configs[0]) + " jumper";
					part->label[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(part->label[0])));
				}
				// A selector's positions: every one any board offers.
				for (size_t k = 1; e.type == EXPANSIONBOARD_MULTI && k < configs.size(); k++) {
					if (std::find(part->values.begin(), part->values.end(), configs[k]) == part->values.end()) {
						part->values.emplace_back(configs[k]);
						part->labels.emplace_back(k < names.size() ? names[k] : configs[k]);
					}
				}
			}
		}
	}
	for (jumper_part& part : parts) {
		part.id = std::string("cpuboard_settings:") + part.config;
		if (part.type == EXPANSIONBOARD_MULTI && find_part(part.config.c_str(), EXPANSIONBOARD_CHECKBOX))
			part.id += "_choice";
		for (size_t i = 0; i < part.values.size(); i++)
			part.choices.push_back({part.values[i].c_str(), part.labels[i].c_str()});
	}
}

// ----------------------------------------------------------------- memory

const amiberry_setting_choice memory_choices[] = {
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

bool memory_available(const uae_prefs& p, const context&)
{
	return board_memory_limit(p) > 0;
}

void get_cpuboardmem1_size(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%u", p.cpuboardmem1.size / megabyte);
}

bool memory_choice_available(const uae_prefs& p, std::string_view choice)
{
	const double mb = std::strtod(std::string(choice).c_str(), nullptr);
	return mb * megabyte <= board_memory_limit(p);
}

void set_cpuboardmem1_size(uae_prefs& p, const value& v)
{
	p.cpuboardmem1.size = static_cast<uae_u32>(v.number) * megabyte;
	fit_board_memory(p);
}

// --------------------------------------------------------------- boot ROM

bool rom_available(const uae_prefs& p, const context&)
{
	return has_board(p);
}

void get_cpuboard_rom_file(const uae_prefs& p, char* out, size_t size)
{
	int index = 0;
	const boardromconfig* brc = get_device_rom(const_cast<uae_prefs*>(&p), ROMTYPE_CPUBOARD, 0, &index);
	std::snprintf(out, size, "%s", brc ? brc->roms[index].romfile : "");
}

// A new file drops the ROM ident (cpuboard_rom=) of the old one, which would
// otherwise pick the old ROM again on load. Writing back the current path
// keeps it.
void set_cpuboard_rom_file(uae_prefs& p, const value& v)
{
	int index = 0;
	boardromconfig* brc = get_device_rom(&p, ROMTYPE_CPUBOARD, 0, &index);
	if (brc && v.text == std::string_view(brc->roms[index].romfile))
		return;
	if (!brc) {
		if (v.text.empty())
			return;
		brc = get_device_rom_new(&p, ROMTYPE_CPUBOARD, 0, &index);
		if (!brc)
			return;
	}
	romconfig& rom = brc->roms[index];
	std::snprintf(rom.romfile, sizeof rom.romfile, "%.*s", static_cast<int>(v.text.size()), v.text.data());
	rom.romident[0] = 0;
}

// ------------------------------------------------------------- expansions

// Builds without an expansion zero its option in fixup_prefs().
bool bsdsocket_compiled(const uae_prefs&, const context&)
{
#ifdef BSDSOCKET
	return true;
#else
	return false;
#endif
}

bool scsi_compiled(const uae_prefs&, const context&)
{
#ifdef SCSIEMU
	return true;
#else
	return false;
#endif
}

bool sana2_compiled(const uae_prefs&, const context&)
{
#ifdef SANA2
	return true;
#else
	return false;
#endif
}

// Mapped host CD drives need uaescsi.device: target_fixup_options() turns it
// on while they are.
bool scsi_choice_available(const uae_prefs& p, std::string_view choice)
{
	return choice != "false" || !p.automount_cddrives;
}

void get_bsdsocket_emu(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.socket_emu ? "true" : "false");
}

void set_bsdsocket_emu(uae_prefs& p, const value& v)
{
	p.socket_emu = v.number != 0;
}

const amiberry_setting_choice scsi_choices[] = {
	{"false", "Off"},
	{"true", "uaescsi.device"},
	{"scsi", "Host CD drives only (no uaescsi.device)"},
};

void get_scsi(const uae_prefs& p, char* out, size_t size)
{
	const int mode = std::clamp(p.scsi, 0, 2);
	std::snprintf(out, size, "%s", scsi_choices[mode].value);
}

void set_scsi(uae_prefs& p, const value& v)
{
	for (int mode = 0; mode < 3; mode++) {
		if (v.text == scsi_choices[mode].value)
			p.scsi = mode;
	}
}

void get_sana2(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.sana2 ? "true" : "false");
}

void set_sana2(uae_prefs& p, const value& v)
{
	p.sana2 = v.number != 0;
}

// ---------------------------------------------------------------- table

constexpr double no_range = 0;

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, CHOICES(choices), constraint}
#define DYNAMIC_CHOICE_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}
#define PATH_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_PATH, apply, no_range, no_range, no_range, def, nullptr, 0, constraint}

const definition fixed_definitions[] = {
	{DYNAMIC_CHOICE_OPTION("cpuboard_type", "accelerator", "Accelerator board",
			"A CPU accelerator board: a faster CPU, often with its own memory, SCSI controller or boot ROM.",
			AMIBERRY_APPLY_RESTART, "none",
			"PPC boards need an emulator built with PPC support."),
		get_cpuboard_type, set_cpuboard_type, nullptr, cpuboard_type_choice_available, nullptr, nullptr,
		cpuboard_type_choices},
	{STRING_OPTION("cpuboard_settings", "accelerator", "Accelerator jumpers",
			"The board's jumpers that are on, as the configuration writes them.",
			AMIBERRY_APPLY_RESET, "", "Only on boards with jumpers."),
		get_cpuboard_settings, set_cpuboard_settings, settings_available, nullptr, nullptr,
		cpuboard_settings_accepts},
	{CHOICE_OPTION("cpuboardmem1_size", "accelerator", "Accelerator memory",
			"Memory on the accelerator board.",
			AMIBERRY_APPLY_RESET, "0", memory_choices,
			"Only on boards with memory, up to the most the board takes."),
		get_cpuboardmem1_size, set_cpuboardmem1_size, memory_available, memory_choice_available, nullptr},
	{PATH_OPTION("cpuboard_rom_file", "accelerator", "Accelerator boot ROM",
			"The board's boot ROM image, for boards that need one.",
			AMIBERRY_APPLY_RESTART, "", "Only with an accelerator board."),
		get_cpuboard_rom_file, set_cpuboard_rom_file, rom_available, nullptr, nullptr},
	{BOOL_OPTION("bsdsocket_emu", "expansions", "bsdsocket.library",
			"TCP/IP networking for Amiga programs through the host's network.",
			AMIBERRY_APPLY_RESTART, "false", nullptr),
		get_bsdsocket_emu, set_bsdsocket_emu, bsdsocket_compiled, nullptr, nullptr},
	{CHOICE_OPTION("scsi", "expansions", "uaescsi.device",
			"Amiga access to host and emulated CD drives through uaescsi.device.",
			AMIBERRY_APPLY_RESTART, "false", scsi_choices,
			"Stays on while host CD drives are mapped (Map CD drives)."),
		get_scsi, set_scsi, scsi_compiled, scsi_choice_available, nullptr},
	{BOOL_OPTION("sana2", "expansions", "uaenet.device",
			"A SANA-II network device for Amiga programs, on the host's network.",
			AMIBERRY_APPLY_RESTART, "false", nullptr),
		get_sana2, set_sana2, sana2_compiled, nullptr, nullptr},
};

// --------------------------------------------------------------- boards

// The configuration's boards are entries keyed by board type and unit, in
// any slot. Two entries are the same board configuration when everything
// but where their ROMs point back to (their slot) matches.
int board_slot(const uae_prefs& p, uae_u32 type, int unit)
{
	for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
		const boardromconfig& b = p.expansionboard[i];
		if (b.device_type && (b.device_type & ROMTYPE_MASK) == (type & ROMTYPE_MASK) && b.device_num == unit)
			return i;
	}
	return -1;
}

bool same_board(const boardromconfig& a, const boardromconfig& b)
{
	static boardromconfig x, y; // too large for the stack
	x = a;
	y = b;
	for (int j = 0; j < MAX_BOARD_ROMS; j++)
		x.roms[j].back = y.roms[j].back = nullptr;
	return std::memcmp(&x, &y, sizeof x) == 0;
}

} // namespace

// The boards a session added, changed or removed (from against base), and
// only those, into to: boards edited elsewhere since keep their state.
bool merge_boards(const uae_prefs& base, const uae_prefs& from, uae_prefs& to)
{
	bool wrote = false;
	for (const boardromconfig& b : from.expansionboard) {
		if (!b.device_type)
			continue;
		const int was = board_slot(base, b.device_type, b.device_num);
		if (was >= 0 && same_board(base.expansionboard[was], b))
			continue;
		int slot = board_slot(to, b.device_type, b.device_num);
		for (int i = 0; slot < 0 && i < MAX_EXPANSION_BOARDS; i++) {
			if (!to.expansionboard[i].device_type)
				slot = i;
		}
		if (slot < 0)
			continue; // no room: the core could not have added it either
		to.expansionboard[slot] = b;
		wrote = true;
	}
	for (const boardromconfig& b : base.expansionboard) {
		if (!b.device_type || board_slot(from, b.device_type, b.device_num) >= 0)
			continue;
		const int slot = board_slot(to, b.device_type, b.device_num);
		if (slot >= 0) {
			std::memset(static_cast<void*>(&to.expansionboard[slot]), 0, sizeof to.expansionboard[slot]);
			wrote = true;
		}
	}
	return wrote;
}

namespace {

bool commit_expansions(const uae_prefs& base, const uae_prefs& from, uae_prefs& to, const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(cpuboard_type);
	FIELD(cpuboard_subtype);
	FIELD(cpuboard_settings);
	FIELD(cpuboardmem1.size);
	// What a board change writes besides: the CPU it needs, the PPC mode and
	// the memory slot its RAM goes to.
	FIELD(cpu_model);
	FIELD(fpu_model);
	FIELD(mmu_model);
	FIELD(address_space_24);
	FIELD(ppc_mode);
	FIELD(fastmem[0].size);
	FIELD(mem25bit.size);
	FIELD(mbresmem_high.size);
	wrote |= merge_boards(base, from, to);
	FIELD(socket_emu);
	FIELD(scsi);
	FIELD(sana2);
#undef FIELD
	return wrote;
}

// The fixed options, then one per jumper part (after cpuboard_settings).
const domain& build_domain()
{
	static std::vector<definition> definitions;
	static domain expansions{};
	collect_jumper_parts();
	const std::vector<jumper_part>& parts = jumper_parts();
	if (parts.size() > max_jumper_parts)
		write_log(_T("Settings: %zu accelerator jumpers, options for the first %zu only\n"), parts.size(), max_jumper_parts);
	for (const definition& d : fixed_definitions) {
		definitions.push_back(d);
		if (std::strcmp(d.descriptor.id, "cpuboard_settings") != 0)
			continue;
		for (size_t slot = 0; slot < parts.size() && slot < max_jumper_parts; slot++) {
			const jumper_part& part = parts[slot];
			const jumper_functions& f = jumper_function_table[slot];
			const bool is_switch = part.type == EXPANSIONBOARD_CHECKBOX;
			amiberry_setting_descriptor descriptor{part.id.c_str(), "accelerator", part.label.c_str(),
				"A jumper of the accelerator board.",
				is_switch ? AMIBERRY_SETTING_BOOL : AMIBERRY_SETTING_CHOICE, AMIBERRY_APPLY_RESET,
				no_range, no_range, no_range, is_switch ? "false" : part.values[0].c_str(),
				is_switch ? nullptr : part.choices.data(), is_switch ? 0 : part.choices.size(),
				"Only on the boards that have this jumper."};
			definitions.push_back({descriptor, f.get, f.set, f.available,
				is_switch ? nullptr : f.choice_available, nullptr, nullptr, nullptr});
		}
	}
	expansions = {definitions.data(), definitions.size(), commit_expansions, nullptr};
	return expansions;
}

} // namespace

const domain& expansions_domain()
{
	static const domain& expansions = build_domain();
	return expansions;
}

} // namespace amiberry_settings
