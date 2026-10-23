// Expansion boards: the board types a configuration can list (expansionroms
// in src/expansion.cpp, without the accelerator board), what a unit of each
// has, and reading and editing units in a configuration. The C API
// (amiberry_settings_board_*, settings_session.cpp) runs these on a
// session's copy; the ImGui expansions panel on changed_prefs.
//
// A unit is a board entry in expansionboard[] (any slot), keyed by type and
// unit number. An entry is what makes the emulation build the board, so a
// unit is enabled while it has one. A ROM board's entry keeps a ROM file:
// enabled without one, it gets the scanned ROM of its type (what the board
// would load anyway) or ":NOROM", since the configuration writer leaves an
// entry without a file out. The built-in controllers follow the chipset
// fields that have them, as the core rebuilds their entries from those
// (cfgfile_compatibility_romtype): the IDE and A3000/A4000T SCSI controllers
// like the old panel, on any machine (fixup_prefs keeps the user's choice
// over the machine's), PCMCIA and CDTV through their chipset options, which
// a custom chipset has, the CD32 FMV cartridge with its own flag.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "autoconf.h"
#include "rommgr.h"
#include "ethernet.h"
#include "expansion_display_name.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace amiberry_settings {

// The configuration name list of a selector: its own name, then one per
// position, each NUL-terminated, an empty one last. Only selectors have such
// lists; a switch's names are plain strings (names_of).
std::vector<const char*> board_setting_names(const char* names)
{
	std::vector<const char*> out;
	for (const char* n = names; n && *n; n += std::strlen(n) + 1)
		out.push_back(n);
	return out;
}

// The layout cfgfile_read_rom_settings uses.
board_setting_bits board_setting_bits_of(const expansionboardsettings* table, int index)
{
	int bitcnt = 0;
	for (int i = 0; table[i].name; i++) {
		const expansionboardsettings& e = table[i];
		bitcnt += e.bitshift;
		int bits = 0;
		if (e.type == EXPANSIONBOARD_MULTI) {
			const int items = static_cast<int>(board_setting_names(e.configname).size()) - 1;
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

bool has_text_settings(const expansionboardsettings* table)
{
	for (int i = 0; table && table[i].name; i++) {
		if (table[i].type == EXPANSIONBOARD_STRING)
			return true;
	}
	return false;
}

namespace {

enum field_kind {
	FIELD_ENABLED,
	FIELD_SUBTYPE,
	FIELD_ROM_FILE,
	FIELD_SCSI_ID,
	FIELD_AUTOBOOT,
	FIELD_INSERTED,
	FIELD_DMA24,
	FIELD_SETTING,
};

struct board_field {
	field_kind kind;
	int setting = -1; // FIELD_SETTING: index into the type's settings table
	std::string id, label;
	std::vector<std::string> values, labels;
	std::vector<amiberry_setting_choice> choices;
};

// The built-in controllers whose entries the core derives from chipset
// fields. A chipset option (PCMCIA, CDTV) switches them through its rules;
// the others are written here.
struct internal_board {
	uae_u32 romtype;
	const char* option; // nullptr: written directly
};
const internal_board internal_boards[] = {
	{ROMTYPE_MB_IDE, nullptr},
	{ROMTYPE_SCSI_A3000, nullptr},
	{ROMTYPE_SCSI_A4000T, nullptr},
	{ROMTYPE_CD32CART, nullptr},
	{ROMTYPE_MB_PCMCIA, "pcmcia"},
	{ROMTYPE_CDTVDMAC, "cdtvcd"},
	{ROMTYPE_CDTVCR, "cdtv-cr"},
};

const internal_board* internal_board_of(uae_u32 romtype)
{
	for (const internal_board& b : internal_boards) {
		if ((b.romtype & ROMTYPE_MASK) == (romtype & ROMTYPE_MASK))
			return &b;
	}
	return nullptr;
}

struct board_type {
	amiberry_settings_board_type info{};
	const expansionromtype* ert = nullptr;
	std::string label;
	// A built-in controller the core derives from chipset fields; nullptr
	// for the others.
	const internal_board* internal = nullptr;
	std::vector<board_field> fields;
	std::vector<amiberry_setting_descriptor> descriptors;
};

// The categories the GUI sorts boards into (WinUAE's), as AMIBERRY_BOARD_*
// bits: a board is in every category its device flags name, except that
// SASI and custom boards are not built-in ones and bridgeboard cards are
// only bridgeboard cards.
struct category {
	unsigned int bit;
	int flags;
};
const category categories[] = {
	{AMIBERRY_BOARD_INTERNAL, EXPANSIONTYPE_INTERNAL},
	{AMIBERRY_BOARD_SCSI, EXPANSIONTYPE_SCSI},
	{AMIBERRY_BOARD_IDE, EXPANSIONTYPE_IDE},
	{AMIBERRY_BOARD_SASI, EXPANSIONTYPE_SASI},
	{AMIBERRY_BOARD_CUSTOM, EXPANSIONTYPE_CUSTOM},
	{AMIBERRY_BOARD_PCI_BRIDGE, EXPANSIONTYPE_PCI_BRIDGE},
	{AMIBERRY_BOARD_X86_BRIDGE, EXPANSIONTYPE_X86_BRIDGE},
	{AMIBERRY_BOARD_RTG, EXPANSIONTYPE_RTG},
	{AMIBERRY_BOARD_SOUND, EXPANSIONTYPE_SOUND},
	{AMIBERRY_BOARD_NETWORK, EXPANSIONTYPE_NET},
	{AMIBERRY_BOARD_FLOPPY, EXPANSIONTYPE_FLOPPY},
	{AMIBERRY_BOARD_X86_EXPANSION, EXPANSIONTYPE_X86_EXPANSION},
};

unsigned int categories_of(int flags)
{
	unsigned int bits = 0;
	for (const category& c : categories) {
		if (!(flags & c.flags))
			continue;
		if (c.bit == AMIBERRY_BOARD_INTERNAL && (flags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM)))
			continue;
		if ((flags & EXPANSIONTYPE_X86_EXPANSION) && c.bit != AMIBERRY_BOARD_X86_EXPANSION)
			continue;
		bits |= c.bit;
	}
	return bits;
}

// The board's flags with every variant's.
int all_flags(const expansionromtype& ert)
{
	int flags = ert.deviceflags;
	for (int i = 0; ert.subtypes && ert.subtypes[i].name; i++)
		flags |= ert.subtypes[i].deviceflags;
	return flags;
}

bool is_type(const board_type& t, uae_u32 romtype)
{
	return (t.ert->romtype & ROMTYPE_MASK) == (romtype & ROMTYPE_MASK);
}

constexpr double no_range = 0;

amiberry_setting_descriptor describe(const board_field& f, amiberry_setting_type type, const char* help,
	const char* def, const char* constraint)
{
	return {f.id.c_str(), "board", f.label.c_str(), help, type, AMIBERRY_APPLY_RESTART, no_range, no_range,
		no_range, def, f.choices.empty() ? nullptr : f.choices.data(), f.choices.size(), constraint};
}

void add_choices(board_field& f)
{
	for (size_t i = 0; i < f.values.size(); i++)
		f.choices.push_back({f.values[i].c_str(), f.labels[i].c_str()});
}

void build_fields(board_type& t)
{
	const expansionromtype& ert = *t.ert;
	const int flags = all_flags(ert);
	std::vector<board_field>& fields = t.fields;
	fields.push_back({FIELD_ENABLED, -1, "enabled", "Enabled"});
	if (ert.subtypes) {
		board_field f{FIELD_SUBTYPE, -1, "subtype", "Variant"};
		for (int i = 0; ert.subtypes[i].name; i++) {
			f.values.emplace_back(ert.subtypes[i].configname);
			f.labels.emplace_back(ert.subtypes[i].name);
		}
		fields.push_back(std::move(f));
	}
	if (!(ert.romtype & ROMTYPE_NOT))
		fields.push_back({FIELD_ROM_FILE, -1, "rom_file", "ROM image"});
	if (ert.id_jumper) {
		board_field f{FIELD_SCSI_ID, -1, "scsi_id", "SCSI ID"};
		for (int i = 0; i < 8; i++) {
			f.values.push_back(std::to_string(i));
			f.labels.push_back(std::to_string(i));
		}
		fields.push_back(std::move(f));
	}
	if (ert.autoboot_jumper)
		fields.push_back({FIELD_AUTOBOOT, -1, "autoboot_disabled", "Disable autoboot"});
	if (flags & EXPANSIONTYPE_PCMCIA)
		fields.push_back({FIELD_INSERTED, -1, "inserted", "PCMCIA card inserted"});
	if (flags & EXPANSIONTYPE_DMA24)
		fields.push_back({FIELD_DMA24, -1, "dma24bit", "24-bit DMA"});
	for (int i = 0; ert.settings && ert.settings[i].name; i++) {
		const expansionboardsettings& e = ert.settings[i];
		const bool selector = e.type == EXPANSIONBOARD_MULTI;
		board_field f{FIELD_SETTING, i};
		if (selector) {
			const std::vector<const char*> configs = board_setting_names(e.configname);
			const std::vector<const char*> names = board_setting_names(e.name);
			if (configs.size() < 2 || names.empty())
				continue;
			f.id = std::string("setting:") + configs[0];
			f.label = names[0];
			for (size_t k = 1; k < configs.size(); k++) {
				f.values.emplace_back(configs[k]);
				f.labels.emplace_back(k < names.size() ? names[k] : configs[k]);
			}
		} else {
			f.id = std::string("setting:") + e.configname;
			f.label = e.name;
		}
		fields.push_back(std::move(f));
	}
	// Descriptors last: they point into the fields, which stay put now.
	for (board_field& f : fields) {
		add_choices(f);
		switch (f.kind) {
		case FIELD_ENABLED:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_BOOL,
				t.internal ? "A controller built into some machines." : "Whether the configuration has this board.",
				"false", !t.internal ? nullptr
					: t.internal->option ? "Follows the chipset option of the built-in controller (a custom chipset)."
					: is_type(t, ROMTYPE_CD32CART) ? nullptr
					: "Not on the Generic chipset, which sets the controllers from the CPU."));
			break;
		case FIELD_SUBTYPE:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_CHOICE, "The variant or revision of the board.",
				f.values[0].c_str(), nullptr));
			break;
		case FIELD_ROM_FILE:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_PATH,
				"The board's ROM image; none for a board that runs without one.", "", nullptr));
			break;
		case FIELD_SCSI_ID:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_CHOICE,
				"The controller's own SCSI ID (avoid the IDs of its drives).", "7", nullptr));
			break;
		case FIELD_AUTOBOOT:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_BOOL,
				"Keep the board from booting the machine (the autoboot jumper).", "false", nullptr));
			break;
		case FIELD_INSERTED:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_BOOL,
				"Whether the card is in the PCMCIA slot.", "false", "Only on the PCMCIA variants."));
			break;
		case FIELD_DMA24:
			t.descriptors.push_back(describe(f, AMIBERRY_SETTING_BOOL,
				"Limit the board's DMA to the first 16 MB.", "false", "Only on the variants with 24-bit DMA."));
			break;
		case FIELD_SETTING: {
			const expansionboardsettings& e = t.ert->settings[f.setting];
			const amiberry_setting_type type = e.type == EXPANSIONBOARD_MULTI ? AMIBERRY_SETTING_CHOICE
				: e.type == EXPANSIONBOARD_STRING ? AMIBERRY_SETTING_STRING : AMIBERRY_SETTING_BOOL;
			const char* def = type == AMIBERRY_SETTING_CHOICE ? f.values[0].c_str()
				: type == AMIBERRY_SETTING_STRING ? "" : "false";
			t.descriptors.push_back(describe(f, type, "A setting of the board.", def, nullptr));
			break;
		}
		}
	}
	t.info.field_count = t.descriptors.size();
}

std::deque<board_type>& catalog()
{
	static std::deque<board_type> types = [] {
		// The network boards' mode selector is filled in from the host's
		// network drivers, as the configuration reader does first.
		ethernet_updateselection();
		std::deque<board_type> list;
		for (int i = 0; expansionroms[i].name; i++) {
			const expansionromtype& ert = expansionroms[i];
			if (ert.romtype & ROMTYPE_CPUBOARD)
				continue;
			const unsigned int cats = categories_of(ert.deviceflags);
			if (!cats)
				continue; // the GUI never offered it
			list.emplace_back();
			board_type& t = list.back();
			t.ert = &ert;
			t.label = expansion_display_name(ert.friendlyname, ert.friendlymanufacturer);
			t.internal = internal_board_of(ert.romtype);
			const bool several = (ert.zorro >= 2 && !ert.singleonly) || (all_flags(ert) & EXPANSIONTYPE_CLOCKPORT);
			t.info.units = several ? MAX_AVAILABLE_DUPLICATE_EXPANSION_BOARDS : 1;
			t.info.categories = cats;
			build_fields(t);
		}
		// Pointers into the list last: a deque keeps its elements in place.
		for (board_type& t : list) {
			t.info.id = t.ert->name;
			t.info.label = t.label.c_str();
			t.info.manufacturer = t.ert->friendlymanufacturer;
		}
		return list;
	}();
	return types;
}

const board_type* find_type(std::string_view id)
{
	for (const board_type& t : catalog()) {
		if (id == t.ert->name)
			return &t;
	}
	return nullptr;
}

const board_field* find_field(const board_type& t, std::string_view id)
{
	for (const board_field& f : t.fields) {
		if (id == f.id)
			return &f;
	}
	return nullptr;
}

const amiberry_setting_descriptor& descriptor_of(const board_type& t, const board_field& f)
{
	return t.descriptors[static_cast<size_t>(&f - t.fields.data())];
}

// The unit's ROM configuration (romconfig of its entry); nullptr while it
// has no entry.
romconfig* rom_of(const uae_prefs& p, const board_type& t, int unit)
{
	int index = 0;
	boardromconfig* brc = get_device_rom(const_cast<uae_prefs*>(&p), t.ert->romtype, unit, &index);
	return brc ? &brc->roms[index] : nullptr;
}

bool is_special_rom(const char* file)
{
	return !_tcsicmp(file, ":NOROM") || !_tcsicmp(file, ":ENABLED");
}

// What gets built: the unit's entry, which the built-in controllers' fields
// keep (or drop) in step.
bool enabled(const uae_prefs& p, const board_type& t, int unit)
{
	return rom_of(p, t, unit) != nullptr;
}

int flags_now(const board_type& t, const romconfig* rom)
{
	int flags = t.ert->deviceflags;
	if (t.ert->subtypes && rom)
		flags |= t.ert->subtypes[rom->subtype].deviceflags;
	return flags;
}

bool available(const uae_prefs& p, const context& ctx, const board_type& t, int unit, const board_field& f)
{
	if (f.kind == FIELD_ENABLED) {
		if (!t.internal)
			return true;
		if (t.internal->option)
			return is_available(require(t.internal->option), p, ctx);
		// The Generic chipset sets the IDE and SCSI controllers from the
		// CPU (fixup_prefs).
		return p.cs_compatible != CP_GENERIC || is_type(t, ROMTYPE_CD32CART);
	}
	if (!enabled(p, t, unit))
		return false;
	const romconfig* rom = rom_of(p, t, unit);
	if (!rom)
		return false;
	if (f.kind == FIELD_INSERTED)
		return (flags_now(t, rom) & EXPANSIONTYPE_PCMCIA) != 0;
	if (f.kind == FIELD_DMA24)
		return (flags_now(t, rom) & EXPANSIONTYPE_DMA24) != 0;
	return true;
}

// The text setting's slot in configtext: one NUL-terminated string per text
// setting, in table order.
int text_slot(const expansionboardsettings* table, int index)
{
	int slot = 0;
	for (int i = 0; i < index; i++) {
		if (table[i].type == EXPANSIONBOARD_STRING)
			slot++;
	}
	return slot;
}

std::vector<std::string> texts_of(const expansionboardsettings* table, const romconfig& rom)
{
	std::vector<std::string> texts;
	const char* text = rom.configtext;
	const char* end = rom.configtext + sizeof rom.configtext;
	for (int i = 0; table && table[i].name; i++) {
		if (table[i].type != EXPANSIONBOARD_STRING)
			continue;
		const size_t length = text < end ? strnlen(text, static_cast<size_t>(end - text)) : 0;
		texts.emplace_back(text < end ? text : "", length);
		text += length + 1;
	}
	return texts;
}

bool put_texts(romconfig& rom, const std::vector<std::string>& texts)
{
	size_t used = 0;
	for (const std::string& t : texts)
		used += t.size() + 1;
	if (used > sizeof rom.configtext)
		return false;
	char* out = rom.configtext;
	std::memset(rom.configtext, 0, sizeof rom.configtext);
	for (const std::string& t : texts) {
		std::memcpy(out, t.data(), t.size());
		out += t.size() + 1;
	}
	return true;
}

void get_field(const uae_prefs& p, const board_type& t, int unit, const board_field& f, char* out, size_t size)
{
	if (f.kind == FIELD_ENABLED) {
		write_bool(out, size, enabled(p, t, unit));
		return;
	}
	const romconfig* rom = rom_of(p, t, unit);
	if (!rom) {
		std::snprintf(out, size, "%s", descriptor_of(t, f).default_value);
		return;
	}
	switch (f.kind) {
	case FIELD_ENABLED:
		break;
	case FIELD_SUBTYPE: {
		const size_t i = static_cast<size_t>(rom->subtype);
		std::snprintf(out, size, "%s", i < f.values.size() ? f.values[i].c_str() : f.values[0].c_str());
		break;
	}
	case FIELD_ROM_FILE:
		std::snprintf(out, size, "%s", is_special_rom(rom->romfile) ? "" : rom->romfile);
		break;
	case FIELD_SCSI_ID:
		write_int(out, size, rom->device_id);
		break;
	case FIELD_AUTOBOOT:
		write_bool(out, size, rom->autoboot_disabled);
		break;
	case FIELD_INSERTED:
		write_bool(out, size, rom->inserted);
		break;
	case FIELD_DMA24:
		write_bool(out, size, rom->dma24bit);
		break;
	case FIELD_SETTING: {
		const expansionboardsettings* table = t.ert->settings;
		const expansionboardsettings& e = table[f.setting];
		if (e.type == EXPANSIONBOARD_STRING) {
			const std::vector<std::string> texts = texts_of(table, *rom);
			const size_t slot = static_cast<size_t>(text_slot(table, f.setting));
			std::snprintf(out, size, "%s", slot < texts.size() ? texts[slot].c_str() : "");
			break;
		}
		const board_setting_bits b = board_setting_bits_of(table, f.setting);
		const int mask = (1 << b.bits) - 1;
		int position = (rom->device_settings >> b.shift) & mask;
		if (e.invert)
			position ^= mask;
		if (e.type == EXPANSIONBOARD_CHECKBOX)
			write_bool(out, size, position != 0);
		else
			std::snprintf(out, size, "%s",
				static_cast<size_t>(position) < f.values.size() ? f.values[position].c_str() : f.values[0].c_str());
		break;
	}
	}
}

// The ROM a board of this type loads when its entry names none (the first
// scanned ROM of its type, as read_device_from_romconfig falls back to).
bool scanned_rom(const expansionromtype& ert, char* out, size_t size)
{
	const romlist* list = romlist_getit();
	const int count = romlist_count();
	for (int i = 0; list && i < count; i++) {
		const romdata* rd = list[i].rd;
		if (!rd || !list[i].path)
			continue;
		const uae_u32 type = rd->type & ROMTYPE_MASK;
		if (type == (ert.romtype & ROMTYPE_MASK) || (ert.romtype_extra && type == (ert.romtype_extra & ROMTYPE_MASK))) {
			std::snprintf(out, size, "%s", list[i].path);
			return true;
		}
	}
	return false;
}

// Gives the unit an entry, with a ROM file the configuration keeps: the
// marker of a board without a ROM, the scanned ROM of its type, or none.
amiberry_settings_status add_entry(uae_prefs& p, const board_type& t, int unit)
{
	if (rom_of(p, t, unit))
		return AMIBERRY_SETTINGS_OK;
	int index = 0;
	boardromconfig* brc = get_device_rom_new(&p, t.ert->romtype, unit, &index);
	if (!brc || brc->device_type == 0)
		return AMIBERRY_SETTINGS_UNAVAILABLE; // every slot is taken
	romconfig& rom = brc->roms[index];
	if (!rom.romfile[0]) {
		if (t.ert->romtype & ROMTYPE_NOT)
			std::snprintf(rom.romfile, sizeof rom.romfile, "%s", ":ENABLED");
		else if (!scanned_rom(*t.ert, rom.romfile, sizeof rom.romfile))
			std::snprintf(rom.romfile, sizeof rom.romfile, "%s", ":NOROM");
	}
	return AMIBERRY_SETTINGS_OK;
}

// Switches a built-in controller through the chipset field the core
// derives its entry from, then lets the core bring the entries in step.
amiberry_settings_status set_internal(uae_prefs& p, const context& ctx, const board_type& t, bool on)
{
	if (t.internal->option) {
		const amiberry_settings_status status = set(require(t.internal->option), p, ctx, on ? "true" : "false");
		if (status != AMIBERRY_SETTINGS_OK)
			return status;
	} else if (is_type(t, ROMTYPE_MB_IDE)) {
		// The IDE controller of the machine it goes with (the old panel's).
		if (!on)
			p.cs_ide = 0;
		else if (p.cs_ide != IDE_A600A1200 && p.cs_ide != IDE_A4000)
			p.cs_ide = p.cs_compatible == CP_A4000 || p.cs_compatible == CP_A4000T ? IDE_A4000 : IDE_A600A1200;
	} else if (is_type(t, ROMTYPE_SCSI_A3000) || is_type(t, ROMTYPE_SCSI_A4000T)) {
		// One of the two at most: the core builds neither for another value.
		const int mine = is_type(t, ROMTYPE_SCSI_A3000) ? 1 : 2;
		if (on)
			p.cs_mbdmac = mine;
		else if (p.cs_mbdmac == mine)
			p.cs_mbdmac = 0;
	} else if (is_type(t, ROMTYPE_CD32CART)) {
		// The cartridge's entry is the board, with its own ROM (the core
		// would fill an empty one with the cartridge file); the flag would
		// bring a removed one back.
		if (on) {
			const amiberry_settings_status status = add_entry(p, t, 0);
			if (status != AMIBERRY_SETTINGS_OK)
				return status;
		} else {
			clear_device_rom(&p, t.ert->romtype, 0, true);
		}
		p.cs_cd32fmv = on;
	}
	cfgfile_compatibility_romtype(&p);
	return AMIBERRY_SETTINGS_OK;
}

amiberry_settings_status set_enabled(uae_prefs& p, const context& ctx, const board_type& t, int unit, bool on)
{
	if (t.internal)
		return set_internal(p, ctx, t, on);
	if (!on) {
		// A board on a parent's entry (its second ROM) leaves the parent.
		clear_device_rom(&p, t.ert->romtype, unit, t.ert->parentromtype == 0);
		return AMIBERRY_SETTINGS_OK;
	}
	return add_entry(p, t, unit);
}

bool parse_bool(std::string_view text, bool& v)
{
	if (text == "true") {
		v = true;
		return true;
	}
	if (text == "false") {
		v = false;
		return true;
	}
	return false;
}

int choice_index(const board_field& f, std::string_view text)
{
	for (size_t i = 0; i < f.values.size(); i++) {
		if (text == f.values[i])
			return static_cast<int>(i);
	}
	return -1;
}

amiberry_settings_status set_field(uae_prefs& p, const context& ctx, const board_type& t, int unit,
	const board_field& f, std::string_view text)
{
	const amiberry_setting_descriptor& d = descriptor_of(t, f);
	bool flag = false;
	int index = -1;
	if (d.type == AMIBERRY_SETTING_BOOL && !parse_bool(text, flag))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (d.type == AMIBERRY_SETTING_CHOICE && (index = choice_index(f, text)) < 0)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (d.type == AMIBERRY_SETTING_PATH && text.size() >= MAX_DPATH)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (d.type == AMIBERRY_SETTING_STRING && text.find('\0') != std::string_view::npos)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	if (!available(p, ctx, t, unit, f))
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	if (f.kind == FIELD_ENABLED)
		return set_enabled(p, ctx, t, unit, flag);

	romconfig* rom = rom_of(p, t, unit);
	if (!rom)
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	switch (f.kind) {
	case FIELD_ENABLED:
		break;
	case FIELD_SUBTYPE:
		rom->subtype = index;
		break;
	case FIELD_ROM_FILE: {
		// No file is a board without its ROM (an entry without a file is
		// left out of the configuration). A new file drops the old file's
		// ROM ident, which would otherwise pick the old ROM on load.
		const std::string file = text.empty() ? std::string(":NOROM") : std::string(text);
		if (file != rom->romfile) {
			std::snprintf(rom->romfile, sizeof rom->romfile, "%s", file.c_str());
			rom->romident[0] = 0;
		}
		break;
	}
	case FIELD_SCSI_ID:
		rom->device_id = index;
		break;
	case FIELD_AUTOBOOT:
		rom->autoboot_disabled = flag;
		break;
	case FIELD_INSERTED:
		rom->inserted = flag;
		break;
	case FIELD_DMA24:
		rom->dma24bit = flag;
		break;
	case FIELD_SETTING: {
		const expansionboardsettings* table = t.ert->settings;
		const expansionboardsettings& e = table[f.setting];
		if (e.type == EXPANSIONBOARD_STRING) {
			std::vector<std::string> texts = texts_of(table, *rom);
			const size_t slot = static_cast<size_t>(text_slot(table, f.setting));
			if (slot >= texts.size())
				return AMIBERRY_SETTINGS_INVALID_VALUE;
			texts[slot] = std::string(text);
			if (!put_texts(*rom, texts))
				return AMIBERRY_SETTINGS_INVALID_VALUE;
			break;
		}
		const board_setting_bits b = board_setting_bits_of(table, f.setting);
		const int mask = (1 << b.bits) - 1;
		int position = e.type == EXPANSIONBOARD_CHECKBOX ? (flag ? 1 : 0) : index;
		if (e.invert)
			position ^= mask;
		rom->device_settings = (rom->device_settings & ~(mask << b.shift)) | ((position & mask) << b.shift);
		break;
	}
	}
	return AMIBERRY_SETTINGS_OK;
}

bool valid_unit(const board_type& t, int unit)
{
	return unit >= 0 && unit < t.info.units;
}

} // namespace

size_t board_type_count()
{
	return catalog().size();
}

const amiberry_settings_board_type* board_type_at(size_t index)
{
	std::deque<board_type>& types = catalog();
	return index < types.size() ? &types[index].info : nullptr;
}

const amiberry_settings_board_type* board_type_find(std::string_view id)
{
	const board_type* t = find_type(id);
	return t ? &t->info : nullptr;
}

const amiberry_setting_descriptor* board_field_at(const amiberry_settings_board_type* type, size_t index)
{
	const board_type* t = type && type->id ? find_type(type->id) : nullptr;
	return t && index < t->descriptors.size() ? &t->descriptors[index] : nullptr;
}

amiberry_settings_status board_get(const uae_prefs& p, std::string_view type, int unit, std::string_view field,
	char* out, size_t size)
{
	const board_type* t = find_type(type);
	const board_field* f = t ? find_field(*t, field) : nullptr;
	if (!f || !valid_unit(*t, unit))
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!out || !size)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	char value[max_value_length];
	get_field(p, *t, unit, *f, value, sizeof value);
	if (std::strlen(value) >= size)
		return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
	std::snprintf(out, size, "%s", value);
	return AMIBERRY_SETTINGS_OK;
}

amiberry_settings_status board_set(uae_prefs& p, const context& ctx, std::string_view type, int unit,
	std::string_view field, std::string_view value)
{
	const board_type* t = find_type(type);
	const board_field* f = t ? find_field(*t, field) : nullptr;
	if (!f || !valid_unit(*t, unit))
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	return set_field(p, ctx, *t, unit, *f, value);
}

bool board_field_available(const uae_prefs& p, const context& ctx, std::string_view type, int unit,
	std::string_view field)
{
	const board_type* t = find_type(type);
	const board_field* f = t ? find_field(*t, field) : nullptr;
	return f && valid_unit(*t, unit) && available(p, ctx, *t, unit, *f);
}

} // namespace amiberry_settings
