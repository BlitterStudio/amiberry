// --settings-schema: the shared settings layer as JSON, for front ends that
// cannot call it (the Android launcher runs in another process than the
// emulator and bundles this file instead). Everything in it is generated
// from the layer: option descriptors, the machine presets and what they
// configure, how the layer resolves the CPU, chipset, memory and sound
// combinations a front end lets the user pick, which options and choices
// it offers in each state, and what setting an option changes with it.
//
// The output is deterministic (no host paths, devices or ROM scans), so a
// committed copy can be checked for freshness against the binary.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "cpuboard.h"
#include "memory.h"
#include "rommgr.h"

#include "settings/amiberry_settings.h"
#include "settings/settings_definitions.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {

void json_string(std::string& out, const char* text)
{
	out += '"';
	for (const char* c = text ? text : ""; *c; c++) {
		const unsigned char ch = static_cast<unsigned char>(*c);
		switch (ch) {
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (ch < 0x20) {
				char escaped[8];
				std::snprintf(escaped, sizeof escaped, "\\u%04x", ch);
				out += escaped;
			} else {
				out += static_cast<char>(ch);
			}
		}
	}
	out += '"';
}

// The shortest spelling that reads back as v, whole numbers without an
// exponent.
void json_number(std::string& out, double v)
{
	char text[64];
	if (v == static_cast<double>(static_cast<long long>(v)) && v > -1e15 && v < 1e15) {
		std::snprintf(text, sizeof text, "%lld", static_cast<long long>(v));
	} else {
		for (int precision = 1; precision <= 17; ++precision) {
			std::snprintf(text, sizeof text, "%.*g", precision, v);
			if (std::strtod(text, nullptr) == v)
				break;
		}
	}
	out += text;
}

const char* type_name(amiberry_setting_type type)
{
	switch (type) {
	case AMIBERRY_SETTING_BOOL: return "bool";
	case AMIBERRY_SETTING_INT: return "int";
	case AMIBERRY_SETTING_REAL: return "real";
	case AMIBERRY_SETTING_CHOICE: return "choice";
	case AMIBERRY_SETTING_PATH: return "path";
	case AMIBERRY_SETTING_STRING: return "string";
	case AMIBERRY_SETTING_HOTKEY: return "hotkey";
	}
	return "string";
}

const char* apply_name(amiberry_apply_class apply)
{
	switch (apply) {
	case AMIBERRY_APPLY_LIVE: return "live";
	case AMIBERRY_APPLY_RESET: return "reset";
	case AMIBERRY_APPLY_RESTART: return "restart";
	}
	return "restart";
}

// The value a session reads for id, "" when it cannot.
std::string value_of(const amiberry_settings_session* session, const char* id)
{
	std::vector<char> buffer(AMIBERRY_SETTINGS_VALUE_MAX);
	for (;;) {
		const amiberry_settings_status status = amiberry_settings_get(session, id, buffer.data(), buffer.size());
		if (status == AMIBERRY_SETTINGS_OK)
			return buffer.data();
		if (status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL || buffer.size() >= 65536)
			return "";
		buffer.resize(buffer.size() * 4);
	}
}

// The IDs listed in a file, one per line ('#' starts a comment); false
// when it cannot be read or names an option the layer does not have.
bool read_option_list(const char* path, std::set<std::string>& ids)
{
	std::ifstream in(path);
	if (!in) {
		std::fprintf(stderr, "--settings-schema: cannot read '%s'\n", path);
		return false;
	}
	std::string line;
	while (std::getline(in, line)) {
		const size_t hash = line.find('#');
		if (hash != std::string::npos)
			line.erase(hash);
		const size_t begin = line.find_first_not_of(" \t\r");
		if (begin == std::string::npos)
			continue;
		const size_t end = line.find_last_not_of(" \t\r");
		const std::string id = line.substr(begin, end - begin + 1);
		if (!amiberry_settings_find(id.c_str())) {
			std::fprintf(stderr, "--settings-schema: '%s' lists an unknown option '%s'\n", path, id.c_str());
			return false;
		}
		ids.insert(id);
	}
	return true;
}

// The layer's options, or those named in only. One that only names but
// listed (the option list) does not is marked preset_only: the export adds
// it because a preset level sets it, so a front end writes it without
// showing it.
void options(std::string& out, const std::set<std::string>* only, const std::set<std::string>* listed)
{
	out += "\"options\":[";
	bool first = true;
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		if (only && !only->count(d->id))
			continue;
		if (!first)
			out += ',';
		first = false;
		out += "\n{\"id\":";
		json_string(out, d->id);
		out += ",\"group\":";
		json_string(out, d->group);
		out += ",\"label\":";
		json_string(out, d->label);
		out += ",\"help\":";
		json_string(out, d->help);
		out += ",\"type\":";
		json_string(out, type_name(d->type));
		out += ",\"apply\":";
		json_string(out, apply_name(d->apply));
		out += ",\"default\":";
		json_string(out, d->default_value);
		// The Android build's default where it differs (this file is made on
		// a desktop build).
		const amiberry_settings::definition* definition = amiberry_settings::find(d->id);
		if (definition && definition->android_default
			&& std::strcmp(definition->android_default, d->default_value ? d->default_value : "") != 0) {
			out += ",\"android_default\":";
			json_string(out, definition->android_default);
		}
		if (d->type == AMIBERRY_SETTING_INT || d->type == AMIBERRY_SETTING_REAL) {
			out += ",\"min\":";
			json_number(out, d->min);
			out += ",\"max\":";
			json_number(out, d->max);
			out += ",\"step\":";
			json_number(out, d->step);
			// Shown as min + max - value (a volume stored as attenuation).
			if (d->reversed)
				out += ",\"reversed\":true";
			if (d->unit) {
				out += ",\"unit\":";
				json_string(out, d->unit);
			}
			// Shown as value / scale (a gamma stored in tenths).
			if (d->scale != 0 && d->scale != 1) {
				out += ",\"scale\":";
				json_number(out, d->scale);
			}
			// A level a front end may show as a slider.
			if (d->level)
				out += ",\"level\":true";
		}
		if (listed && !listed->count(d->id))
			out += ",\"preset_only\":true";
		if (d->constraint) {
			out += ",\"constraint\":";
			json_string(out, d->constraint);
		}
		// The descriptor's own list; host device lists are left out (they
		// depend on the machine the schema is made on).
		if (d->type == AMIBERRY_SETTING_CHOICE) {
			out += ",\"choices\":[";
			for (size_t c = 0; c < d->choice_count; c++) {
				if (c)
					out += ',';
				out += "{\"value\":";
				json_string(out, d->choices[c].value);
				out += ",\"label\":";
				json_string(out, d->choices[c].label);
				out += '}';
			}
			out += ']';
		}
		out += '}';
	}
	out += "\n]";
}

// A fresh pending configuration, as a configuration load starts from.
void reset_pending()
{
	default_prefs(&changed_prefs, true, 0);
}

// Whether a preset's value of the option describes the machine: paths are
// left out (the ROM files depend on the ROMs the machine making the schema
// has, the launcher supplies its own), and so are the sub-options
// ("cpuboard_settings:pckbd"), which are part of their parent's value and
// no configuration key of their own.
bool machine_option(const amiberry_setting_descriptor* d)
{
	return d->type != AMIBERRY_SETTING_PATH && !std::strchr(d->id, ':');
}

// Every option's value in a fresh configuration, by descriptor index.
std::vector<std::string> default_values()
{
	std::vector<std::string> values(amiberry_settings_count());
	reset_pending();
	amiberry_settings_session* session = amiberry_settings_session_open();
	if (!session)
		return {};
	for (size_t i = 0; i < values.size(); i++)
		values[i] = value_of(session, amiberry_settings_at(i)->id);
	amiberry_settings_session_close(session);
	return values;
}

// The options preset levels set, which the export describes whatever the
// option list names: a front end writes them, so it needs their order
// and labels.
std::set<std::string> preset_option_ids;

// One compatibility level of a preset configuration: every machine option
// whose value differs from a fresh configuration's, in layer order, so that
// a configuration file of these lines loads as the machine the preset
// builds (whatever the front end shows). Not limited to the option list.
// Sets ppc when the configuration has a PowerPC board.
bool preset_level(std::string& out, const char* id, int config, int level,
	const std::vector<std::string>& defaults, bool& ppc)
{
	reset_pending();
	amiberry_settings_session* session = amiberry_settings_session_open();
	if (!session)
		return false;
	const amiberry_settings_status status = amiberry_settings_preset_apply(session, id, config, level);
	if (status == AMIBERRY_SETTINGS_OK) {
		bool committed = false;
		amiberry_settings_commit(session, &committed);
	}
	amiberry_settings_session_close(session);
	out += '{';
	if (status != AMIBERRY_SETTINGS_OK) {
		out += '}';
		return true;
	}
	session = amiberry_settings_session_open();
	if (!session)
		return false;
	bool first = true;
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		if (!machine_option(d))
			continue;
		const std::string value = value_of(session, d->id);
		if (value == defaults[i])
			continue;
		if (!first)
			out += ',';
		first = false;
		json_string(out, d->id);
		preset_option_ids.insert(d->id);
		out += ':';
		json_string(out, value.c_str());
	}
	out += '}';
	ppc = ppc || cpuboard_is_ppc_accelerator(&changed_prefs);
	amiberry_settings_session_close(session);
	return true;
}

// The ROM IDs presets ask for, for the ROM table after the presets.
std::set<int> requested_roms;

const char* rom_role(int id)
{
	const romdata* rd = getromdatabyid(id);
	if (!rd)
		return "other";
	if (rd->type & (ROMTYPE_KICK | ROMTYPE_KICKCD32))
		return "kick";
	if (rd->type & (ROMTYPE_EXTCD32 | ROMTYPE_EXTCDTV | ROMTYPE_ARCADIABIOS | ROMTYPE_ALG))
		return "ext";
	return "other";
}

// The ROM sets a preset configuration can be built with: its builder is
// run with each pattern of found and missing ROMs (configure_rom_probe),
// as fallbacks are only asked for when an earlier ROM is missing and add-on
// ROMs only when the main one was found. Each set lists the ROM lists that
// were found on a path where the builder succeeded: one ROM from each list
// of a set is what the configuration needs.
//
// The arcade boards' configuration is the first installed game (see
// amiberry_settings::arcade_game_config), which depends on the machine:
// their builder is probed without a game, and every set gets one request
// listing every game.
std::vector<std::vector<std::vector<int>>> preset_rom_sets(const char* id, int config)
{
	std::vector<std::vector<std::vector<int>>> sets;
	const int model = amiberry_settings::preset_model_index(id);
	uae_prefs* scratch = static_cast<uae_prefs*>(std::calloc(1, sizeof(uae_prefs)));
	if (model < 0 || !scratch) {
		std::free(scratch);
		return sets;
	}
	std::vector<int> games;
	const bool arcade = amiberry_settings::is_arcade_model(model);
	if (arcade) {
		romlist** list = getarcadiaroms(model == 10 ? 1 : 0);
		for (int i = 0; list && list[i]; i++) {
			if (list[i]->rd->type == (model == 10 ? ROMTYPE_ALG : ROMTYPE_ARCADIAGAME))
				games.push_back(list[i]->rd->id);
		}
		xfree(list);
		config = -1;
	}
	std::vector<std::vector<bool>> pending = {{}};
	while (!pending.empty()) {
		configure_rom_probe probe;
		probe.answers = pending.back();
		pending.pop_back();
		default_prefs(scratch, true, 0);
		configure_rom_probing = &probe;
		const bool built = built_in_prefs_preview(scratch, model, config, 0) != 0;
		configure_rom_probing = nullptr;
		if (built) {
			std::vector<std::vector<int>> set;
			for (size_t call = 0; call < probe.lists.size(); call++) {
				if (call < probe.answers.size() && probe.answers[call])
					set.push_back(probe.lists[call]);
			}
			if (!games.empty())
				set.push_back(games);
			if (std::find(sets.begin(), sets.end(), set) == sets.end())
				sets.push_back(set);
		}
		// Each call past the given answers was answered "missing"; branch
		// on it being found instead.
		for (size_t call = probe.answers.size(); call < probe.calls; call++) {
			std::vector<bool> answers = probe.answers;
			answers.resize(call, false);
			answers.push_back(true);
			pending.push_back(answers);
		}
	}
	discard_prefs(scratch, 0);
	std::free(scratch);
	// Fewest ROMs first: the builder's first choice needs the fewest (a
	// CD32 Kickstart with its extended ROM in one file before two files).
	std::stable_sort(sets.begin(), sets.end(),
		[](const auto& a, const auto& b) { return a.size() < b.size(); });
	return sets;
}

bool presets(std::string& out)
{
	const std::vector<std::string> defaults = default_values();
	if (defaults.size() != amiberry_settings_count())
		return false;
	out += ",\n\"compatibility_levels\":[";
	for (int level = 0;; level++) {
		const char* label = amiberry_settings_preset_compatibility_label(level);
		if (!label)
			break;
		if (level)
			out += ',';
		json_string(out, label);
	}
	out += "],\n\"presets\":[";
	for (size_t i = 0; i < amiberry_settings_preset_count(); i++) {
		const amiberry_settings_preset* preset = amiberry_settings_preset_at(i);
		if (i)
			out += ',';
		out += "\n{\"id\":";
		json_string(out, preset->id);
		out += ",\"label\":";
		json_string(out, preset->label);
		out += ",\"compatibility_max\":";
		json_number(out, preset->compatibility_max);
		// The arcade boards: their configuration is the first installed game.
		if (amiberry_settings::is_arcade_model(amiberry_settings::preset_model_index(preset->id)))
			out += ",\"arcade\":true";
		out += ",\"configs\":[";
		for (int config = 0; config < preset->config_count; config++) {
			const auto rom_sets = preset_rom_sets(preset->id, config);
			if (config)
				out += ',';
			out += "\n {\"label\":";
			json_string(out, amiberry_settings_preset_config_label(preset, config));
			// Each set: the ROM lists one ROM each must be installed from.
			out += ",\"rom_sets\":[";
			for (size_t set = 0; set < rom_sets.size(); set++) {
				if (set)
					out += ',';
				out += '[';
				for (size_t r = 0; r < rom_sets[set].size(); r++) {
					const auto& list = rom_sets[set][r];
					if (r)
						out += ',';
					out += "{\"role\":";
					json_string(out, rom_role(list[0]));
					out += ",\"ids\":[";
					for (size_t i = 0; i < list.size(); i++) {
						if (i)
							out += ',';
						json_number(out, list[i]);
						requested_roms.insert(list[i]);
					}
					out += "]}";
				}
				out += ']';
			}
			out += ']';
			// The machine at each compatibility level, best compatibility first.
			out += ",\"levels\":[";
			bool ppc = false;
			for (int level = 0; level <= preset->compatibility_max; level++) {
				if (level)
					out += ',';
				if (!preset_level(out, preset->id, config, level, defaults, ppc))
					return false;
			}
			out += ']';
			// Features of the emulator the configuration needs that a build
			// may leave out: the PowerPC boards need WITH_PPC, which the
			// Android build does not have.
			if (ppc)
				out += ",\"requires\":[\"ppc\"]";
			out += '}';
		}
		out += "]}";
	}
	out += "\n],\n\"roms\":[";
	// The ROMs the presets ask for, so a front end that finds ROM files by
	// checksum can tell which preset they serve.
	bool first_rom = true;
	for (const int id : requested_roms) {
		const romdata* rd = getromdatabyid(id);
		if (!rd)
			continue;
		if (!first_rom)
			out += ',';
		first_rom = false;
		char crc[16];
		std::snprintf(crc, sizeof crc, "%08x", rd->crc32);
		out += "\n{\"id\":";
		json_number(out, id);
		out += ",\"crc32\":";
		json_string(out, crc);
		out += ",\"role\":";
		json_string(out, rom_role(id));
		out += ",\"name\":";
		json_string(out, rd->name);
		out += '}';
	}
	out += "\n]";
	return true;
}

// What the emulator runs with when a configuration names a combination of
// values: for each combination of a table's inputs, the values the core's
// checks (fixup_prefs) leave after a configuration load, as the Android
// launcher's configuration files are loaded. The "free" options are not
// part of the combination; for each of them the row gives what each of its
// values resolves to (the other free options at their defaults), so a front
// end keeps its value unless the checks force one. The context is loaded
// before every combination and is not part of the table.
struct field {
	const char* id;
	std::vector<const char*> values;
};

using pairs = std::vector<std::pair<const char*, const char*>>;

struct resolution_table {
	const char* name;
	std::vector<field> inputs;
	std::vector<field> free;
	pairs context;
};

// Every value of a choice descriptor, so new choices are covered.
std::vector<const char*> choice_values(const char* id)
{
	std::vector<const char*> values;
	const amiberry_setting_descriptor* d = amiberry_settings_find(id);
	for (size_t c = 0; d && c < d->choice_count; c++)
		values.push_back(d->choices[c].value);
	return values;
}

// The values of an integer option from its minimum to its maximum, by its
// step, that the layer takes in a fresh configuration (the KB fast RAM
// sizes: powers of two from 64 KB).
std::vector<const char*> accepted_values(const char* id)
{
	// The texts outlive the call: tables keep pointers to them.
	static std::set<std::string> texts;
	std::vector<const char*> values;
	const amiberry_setting_descriptor* d = amiberry_settings_find(id);
	if (!d || d->step <= 0)
		return values;
	reset_pending();
	amiberry_settings_session* session = amiberry_settings_session_open();
	if (!session)
		return values;
	for (double v = d->min; v <= d->max; v += d->step) {
		const std::string text = std::to_string(static_cast<long long>(v));
		if (amiberry_settings_set(session, id, text.c_str()) == AMIBERRY_SETTINGS_OK)
			values.push_back(texts.insert(text).first->c_str());
	}
	amiberry_settings_session_close(session);
	return values;
}

std::vector<resolution_table> resolution_tables()
{
	// The sound modes with the older spellings cfgfile still reads.
	std::vector<const char*> sound_outputs = choice_values("sound_output");
	sound_outputs.push_back("good");
	sound_outputs.push_back("best");
	return {
		// The CPU and chipset combination.
		{"resolutions",
			{
				{"chipset", {"ocs", "ecs", "aga"}},
				{"cpu_model", {"68000", "68010", "68020", "68030", "68040", "68060"}},
				{"fpu_model", {"0", "68881", "68882", "68040", "68060"}},
				{"cpu_24bit_addressing", {"true", "false"}},
				{"cycle_exact", {"false", "true"}},
				{"cachesize", {"0", "8192"}},
				{"z3mem_size", {"0", "16"}},
			},
			{
				{"cpu_speed", {"real", "max"}},
				{"cpu_compatible", {"true", "false"}},
				{"compfpu", {"false", "true"}},
				{"immediate_blits", {"false", "true"}},
			},
			{}},
		// Memory sizes against the chipset, the addressing and the JIT, on a
		// 68020 the JIT can run on (the CPU table above says when it can).
		// The board is an A500's, without Gary, Ramsey or Gayle: their 1.8 MB
		// slow RAM conflict depends on the preset's hardware, which a front
		// end leaves to the emulator.
		{"memory_resolutions",
			{
				{"chipset", {"ocs", "ecs", "aga"}},
				{"cpu_24bit_addressing", {"true", "false"}},
				{"cachesize", {"0", "8192"}},
				{"chipmem_size", choice_values("chipmem_size")},
				{"bogomem_size", choice_values("bogomem_size")},
				{"fastmem_size", choice_values("fastmem_size")},
			},
			{},
			{{"chipset_compatible", "A500"}, {"cpu_model", "68020"}, {"cycle_exact", "false"}}},
		// The sound mode against the cycle-exact modes, which need Paula
		// emulation.
		{"sound_resolutions",
			{
				{"cycle_exact", choice_values("cycle_exact")},
				{"sound_output", sound_outputs},
			},
			{},
			{}},
	};
}

// Loads id=value pairs into a fresh configuration, runs the core's checks
// and reads ids back; empty strings for what cannot be read.
bool resolve(const pairs& values, const std::vector<const char*>& ids, std::vector<std::string>& out)
{
	uae_prefs* p = static_cast<uae_prefs*>(std::calloc(1, sizeof(uae_prefs)));
	if (!p)
		return false;
	default_prefs(p, true, 0);
	for (const auto& [id, value] : values) {
		TCHAR text[64];
		std::snprintf(text, sizeof text, "%s", value);
		cfgfile_parse_option(p, id, text, 0);
	}
	fixup_prefs_preview(p);
	out.clear();
	for (const char* id : ids) {
		const amiberry_settings::definition* d = amiberry_settings::find(id);
		char text[amiberry_settings::max_value_length] = {};
		if (d)
			amiberry_settings::get(*d, *p, text, sizeof text);
		out.emplace_back(text);
	}
	discard_prefs(p, 0);
	std::free(p);
	return true;
}

bool resolution(std::string& out, const resolution_table& table)
{
	const auto& inputs = table.inputs;
	const auto& free = table.free;
	std::vector<const char*> input_ids;
	for (const field& f : inputs) {
		if (f.values.empty())
			return false;
		input_ids.push_back(f.id);
	}

	out += ",\n\"";
	out += table.name;
	out += "\":{\"inputs\":[";
	for (size_t i = 0; i < inputs.size(); i++) {
		if (i)
			out += ',';
		json_string(out, inputs[i].id);
	}
	out += "],\"free\":[";
	for (size_t i = 0; i < free.size(); i++) {
		if (i)
			out += ',';
		out += "{\"id\":";
		json_string(out, free[i].id);
		out += ",\"values\":[";
		for (size_t v = 0; v < free[i].values.size(); v++) {
			if (v)
				out += ',';
			json_string(out, free[i].values[v]);
		}
		out += "]}";
	}
	out += "],\"rows\":[";
	std::vector<size_t> pick(inputs.size(), 0);
	std::vector<std::string> resolved;
	bool first_row = true;
	for (;;) {
		pairs request = table.context;
		for (size_t i = 0; i < inputs.size(); i++)
			request.emplace_back(inputs[i].id, inputs[i].values[pick[i]]);
		if (!first_row)
			out += ',';
		first_row = false;
		out += "\n[[";
		for (size_t i = 0; i < inputs.size(); i++) {
			if (i)
				out += ',';
			json_string(out, inputs[i].values[pick[i]]);
		}
		out += "],[";
		if (!resolve(request, input_ids, resolved))
			return false;
		for (size_t i = 0; i < resolved.size(); i++) {
			if (i)
				out += ',';
			json_string(out, resolved[i].c_str());
		}
		out += "],[";
		for (size_t f = 0; f < free.size(); f++) {
			if (f)
				out += ',';
			out += '[';
			for (size_t v = 0; v < free[f].values.size(); v++) {
				auto with_free = request;
				with_free.emplace_back(free[f].id, free[f].values[v]);
				if (!resolve(with_free, {free[f].id}, resolved))
					return false;
				if (v)
					out += ',';
				json_string(out, resolved[0].c_str());
			}
			out += ']';
		}
		out += "]]";
		// Next combination, the last input changing fastest.
		bool done = true;
		for (size_t i = inputs.size(); i-- > 0;) {
			if (++pick[i] < inputs[i].values.size()) {
				done = false;
				break;
			}
			pick[i] = 0;
		}
		if (done)
			break;
	}
	out += "\n]}";
	return true;
}

bool resolutions(std::string& out)
{
	for (const resolution_table& table : resolution_tables()) {
		if (!resolution(out, table))
			return false;
	}
	return true;
}

// What the layer answers while a front end edits: which options and
// choices are available, and what setting an option changes with it (the
// setters' dependent changes). Each table's states are the combinations of
// its inputs; a state is loaded into a fresh pending configuration the way
// a configuration file loads it (cfgfile, so no setter's dependent changes
// apply), after the table's context, and an edit session on it is asked.
// Rows list the states in combination order, the last input changing
// fastest.
//
// The inputs are declared here by hand. So that a rule that starts reading
// another option cannot go unnoticed, every state is also asked with other
// options a front end edits set differently (the probes): an answer that
// changes with them fails the export.
struct state_space {
	std::vector<field> inputs;
	pairs context;
};

// The CPU options a front end edits, cpu_model first (loading it resets
// the FPU).
state_space cpu_states()
{
	return {
		{
			{"cpu_model", choice_values("cpu_model")},
			{"cpu_24bit_addressing", {"true", "false"}},
			{"cycle_exact", choice_values("cycle_exact")},
			{"cachesize", {"0", "8192"}},
			{"cpu_compatible", {"true", "false"}},
			{"fpu_model", choice_values("fpu_model")},
		},
		{}};
}

// The CPU states with the MMU, which rules out the JIT and with it the
// options that read the effective JIT. It loads after cpu_model, which
// does not reset it.
state_space cpu_mmu_states()
{
	state_space space = cpu_states();
	space.inputs.insert(space.inputs.begin() + 1, {"mmu_model", choice_values("mmu_model")});
	return space;
}

// The IDs of a group's options, in layer order.
std::vector<const char*> group_options(const char* group)
{
	std::vector<const char*> ids;
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		if (std::strcmp(d->group, group) == 0)
			ids.push_back(d->id);
	}
	return ids;
}

// The option values every state is also asked with; the first is the
// state alone, whose answers the tables give. The transition fields that
// are not inputs take both of their values, so a transition can tell a
// value a setter forces from one it keeps.
const std::vector<pairs>& probes()
{
	static const std::vector<pairs> list = {
		{},
		{{"cpu_speed", "real"}, {"immediate_blits", "false"}, {"compfpu", "false"}},
		{{"cpu_speed", "max"}, {"immediate_blits", "true"}, {"compfpu", "true"}},
		{{"chipset", "aga"}, {"chipmem_size", "4"}, {"fastmem_size", "4"}, {"ntsc", "true"},
			{"sound_channels", "mono"}, {"collision_level", "none"}},
		{{"chipset", "ecs"}, {"bogomem_size", "0"}, {"z3mem_size", "16"}, {"sound_frequency", "22050"},
			{"sound_max_buff", "1024"}},
	};
	return list;
}

size_t state_count(const state_space& space)
{
	size_t count = 1;
	for (const field& f : space.inputs)
		count *= f.values.size();
	return count;
}

// The input values of the index-th state.
pairs state_inputs(const state_space& space, size_t index)
{
	pairs values(space.inputs.size());
	for (size_t i = space.inputs.size(); i-- > 0;) {
		const field& f = space.inputs[i];
		values[i] = {f.id, f.values[index % f.values.size()]};
		index /= f.values.size();
	}
	return values;
}

std::string describe(const pairs& values)
{
	std::string text;
	for (const auto& [id, value] : values) {
		if (!text.empty())
			text += ' ';
		text += id;
		text += '=';
		text += value;
	}
	return text.empty() ? "the defaults" : text;
}

// A state loaded into a fresh pending configuration as a configuration file
// loads it (cfgfile, so no setter's dependent changes apply): the table's
// context, then the state. Each probe starts from a copy of that
// configuration and is loaded last, so that loading an input cannot
// override it (cfgfile drops "max" CPU speed in cycle-exact modes of old
// configurations); a probe sets no input (see without_inputs). Copying the
// loaded configuration back is several times cheaper than a fresh
// configuration and a parse per input for every probe, which was most of
// the export's time. One state is loaded at a time, so one buffer (the
// configuration is about 11 MB) serves every state.
class loaded_state {
public:
	loaded_state(const state_space& space, const pairs& state)
		: saved(buffer())
	{
		reset_pending();
		parse(space.context);
		parse(state);
		if (saved)
			std::memcpy(static_cast<void*>(saved), static_cast<const void*>(&changed_prefs), sizeof(uae_prefs));
	}
	loaded_state(const loaded_state&) = delete;
	loaded_state& operator=(const loaded_state&) = delete;

	bool ok() const { return saved != nullptr; }

	// The pending configuration becomes the state with the probe loaded after it.
	void with(const pairs& probe) const
	{
		std::memcpy(static_cast<void*>(&changed_prefs), static_cast<const void*>(saved), sizeof(uae_prefs));
		parse(probe);
	}

private:
	static uae_prefs* buffer()
	{
		static uae_prefs* const prefs = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
		return prefs;
	}

	static void parse(const pairs& values)
	{
		for (const auto& [id, value] : values) {
			TCHAR text[64];
			std::snprintf(text, sizeof text, "%s", value);
			cfgfile_parse_option(&changed_prefs, id, text, 0);
		}
	}

	uae_prefs* saved;
};

// A probe without the values it has for inputs of the space: an
// availability table may have an option the probes vary as an input (sound
// channels for the stereo options), and its state gives that one.
pairs without_inputs(const pairs& probe, const state_space& space)
{
	pairs values;
	for (const auto& [id, value] : probe) {
		const bool input = std::any_of(space.inputs.begin(), space.inputs.end(),
			[&](const field& f) { return std::strcmp(f.id, id) == 0; });
		if (!input)
			values.emplace_back(id, value);
	}
	return values;
}

void json_fields(std::string& out, const std::vector<field>& fields)
{
	out += '[';
	for (size_t i = 0; i < fields.size(); i++) {
		if (i)
			out += ',';
		out += "{\"id\":";
		json_string(out, fields[i].id);
		out += ",\"values\":[";
		for (size_t v = 0; v < fields[i].values.size(); v++) {
			if (v)
				out += ',';
			json_string(out, fields[i].values[v]);
		}
		out += "]}";
	}
	out += ']';
}

void json_ids(std::string& out, const std::vector<const char*>& ids)
{
	out += '[';
	for (size_t i = 0; i < ids.size(); i++) {
		if (i)
			out += ',';
		json_string(out, ids[i]);
	}
	out += ']';
}

// "inputs" and "context" of a state space.
void json_state_space(std::string& out, const state_space& space)
{
	out += "\"inputs\":";
	json_fields(out, space.inputs);
	out += ",\"context\":{";
	for (size_t i = 0; i < space.context.size(); i++) {
		if (i)
			out += ',';
		json_string(out, space.context[i].first);
		out += ':';
		json_string(out, space.context[i].second);
	}
	out += '}';
}

// An availability table: per state, a bit mask of the options that are
// available (bit i for options[i]), then one mask per choice option of the
// descriptor's choices that are (bit j for its j-th choice).
struct availability_table {
	state_space space;
	std::vector<const char*> options;
	std::vector<const char*> choices;
};

std::vector<availability_table> availability_tables()
{
	static const char* const drive_sounds[] = {"floppy0sound", "floppy1sound", "floppy2sound", "floppy3sound"};
	static const char* const disk_volumes[] = {"floppy0soundvolume_disk", "floppy1soundvolume_disk",
		"floppy2soundvolume_disk", "floppy3soundvolume_disk"};
	static const char* const empty_volumes[] = {"floppy0soundvolume_empty", "floppy1soundvolume_empty",
		"floppy2soundvolume_empty", "floppy3soundvolume_empty"};
	std::vector<availability_table> tables = {
		// The CPU, MMU, FPU and JIT options against the CPU, the MMU and
		// the JIT. The data cache also needs the indirect UAE Boot ROM when
		// a boot ROM is needed (hard drives); the core fixup turns it off
		// otherwise, and the answers here are those without one.
		{cpu_mmu_states(),
			{"cpu_24bit_addressing", "cpu_compatible", "cachesize", "compfpu", "z3mem_size",
				"cpu_no_unimplemented", "cpu_data_cache", "mmu_model", "fpu_strict", "fpu_softfloat",
				"fpu_no_unimplemented", "cpu_threaded", "comp_constjump", "comp_flushmode", "comp_nf",
				"comp_catchfault", "comp_trustbyte"},
			{"fpu_model", "cycle_exact", "mmu_model"}},
		// The speed options against the cycle-exact modes, CPU
		// compatibility and the speed.
		{{{{"cycle_exact", choice_values("cycle_exact")}, {"cpu_compatible", {"true", "false"}},
			{"cpu_speed", choice_values("cpu_speed")}}, {}},
			{"cpu_throttle", "amiberry.cpu_idle", "cpu_multiplier"}, {}},
		// Waiting blits against immediate blits and the 68000/010 memory
		// cycle-exact mode.
		{{{{"cpu_model", choice_values("cpu_model")}, {"cycle_exact", choice_values("cycle_exact")},
			{"immediate_blits", {"true", "false"}}}, {}},
			{"waiting_blits"}, {}},
		{{{{"keyboard_type", choice_values("keyboard_type")}}, {}}, {"keyboard_nkro"}, {}},
		// The advanced chipset options while a chipset extra is active (it
		// sets them), and the 1 MB chip jumper against chip RAM.
		{{{{"chipset_compatible", choice_values("chipset_compatible")},
			{"chipmem_size", choice_values("chipmem_size")}}, {}},
			group_options("adv_chipset"), {}},
		// The RTC chip a chipset extra keeps, which depends on the CPU.
		{{{{"cpu_model", choice_values("cpu_model")}, {"cpu_compatible", {"true", "false"}},
			{"chipset_compatible", choice_values("chipset_compatible")}}, {}},
			{}, {"rtc"}},
		{{{{"genlock", {"true", "false"}}}, {}},
			{"genlockmode", "genlock_mix", "genlock_alpha", "genlock_aspect"}, {"genlockmode"}},
		// The Zorro III boards and mappings need 32-bit addressing.
		{{{{"cpu_24bit_addressing", {"true", "false"}}}, {}},
			{"z3mem2_size", "z3mem3_size", "z3mem4_size", "megachipmem_size", "z3mapping"}, {}},
		// The native resolution and line mode while autoswitching follows
		// the Amiga's screen mode, and VGA autoswitching against them.
		{{{{"gfx_autoresolution", choice_values("gfx_autoresolution")}}, {}},
			{"gfx_resolution", "gfx_linemode"}, {}},
		{{{{"gfx_resolution", choice_values("gfx_resolution")}, {"gfx_linemode", choice_values("gfx_linemode")}}, {}},
			{"gfx_autoresolution_vga"}, {}},
		// Centering against the overscan mode and auto crop, and manual crop
		// against auto crop.
		{{{{"gfx_overscanmode", choice_values("gfx_overscanmode")}, {"amiberry.gfx_auto_crop", {"true", "false"}}}, {}},
			{"gfx_center_horizontal", "gfx_center_vertical"}, {}},
		{{{{"amiberry.gfx_auto_crop", {"true", "false"}}, {"amiberry.gfx_manual_crop", {"true", "false"}}}, {}},
			{"amiberry.gfx_manual_crop", "amiberry.gfx_manual_crop_width", "amiberry.gfx_manual_crop_height",
				"amiberry.gfx_horizontal_offset", "amiberry.gfx_vertical_offset"}, {}},
		// What needs sound emulation, and the sound modes the cycle-exact
		// modes allow.
		{{{{"sound_output", choice_values("sound_output")}, {"cycle_exact", choice_values("cycle_exact")}}, {}},
			{"sound_channels", "sound_frequency", "sound_max_buff", "sound_volume_paula", "sound_volume_cd",
				"sound_volume_ahi", "sound_volume_midi", "floppy0sound", "floppy1sound", "floppy2sound",
				"floppy3sound", "sound_auto", "sound_interpol", "sound_stereo_swap_paula", "sound_stereo_swap_ahi",
				"sound_filter", "sound_filter_type"},
			{"sound_output", "sound_filter"}},
		// The stereo options against the channel mode.
		{{{{"sound_output", choice_values("sound_output")}, {"sound_channels", choice_values("sound_channels")}}, {}},
			{"sound_stereo_separation", "sound_stereo_mixing_delay"}, {}},
		// The virtual mouse driver's options, and the on-screen keyboard's.
		{{{{"absolute_mouse", choice_values("absolute_mouse")}}, {}}, {"magic_mousecursor", "tablet_library"}, {}},
		{{{{"amiberry.vkbd_enabled", {"true", "false"}}}, {}},
			{"amiberry.vkbd_transparency", "amiberry.vkbd_language"}, {}},
		// The chip and slow RAM sizes the machine allows: the 1.8 MB slow
		// RAM conflicts with the Gayle, Gary and Ramsey boards the chipset
		// extra brings, the 1.5 MB sizes with an effective JIT. The chipset
		// extra comes after the CPU, as in a configuration file: its boards
		// depend on the CPU model and, for Generic, on CPU compatibility. A
		// Zorro II RTG board also limits chip RAM; a front end without one
		// gets the answers without it.
		{{{{"cpu_model", choice_values("cpu_model")},
			{"cpu_compatible", {"true", "false"}},
			{"cpu_24bit_addressing", {"true", "false"}},
			{"cycle_exact", choice_values("cycle_exact")},
			{"cachesize", {"0", "8192"}},
			{"chipset_compatible", choice_values("chipset_compatible")}}, {}},
			{}, {"chipmem_size", "bogomem_size"}},
	};
	// Each drive's click volumes against its drive sound.
	for (int drive = 0; drive < 4; drive++) {
		tables.push_back({{{{"sound_output", choice_values("sound_output")},
			{drive_sounds[drive], choice_values(drive_sounds[drive])}}, {}},
			{disk_volumes[drive], empty_volumes[drive]}, {}});
	}
	return tables;
}

std::vector<unsigned long long> availability_answers(const amiberry_settings_session* session,
	const availability_table& table)
{
	std::vector<unsigned long long> masks(1 + table.choices.size(), 0);
	for (size_t i = 0; i < table.options.size(); i++) {
		if (amiberry_settings_is_available(session, table.options[i]))
			masks[0] |= 1ull << i;
	}
	for (size_t c = 0; c < table.choices.size(); c++) {
		const std::vector<const char*> values = choice_values(table.choices[c]);
		for (size_t j = 0; j < values.size(); j++) {
			if (amiberry_settings_choice_is_available(session, table.choices[c], values[j]))
				masks[1 + c] |= 1ull << j;
		}
	}
	return masks;
}

bool availability_table_json(std::string& out, const availability_table& table)
{
	if (table.options.size() > 64)
		return false;
	for (const char* id : table.choices) {
		if (choice_values(id).empty() || choice_values(id).size() > 64)
			return false;
	}
	out += "\n{";
	json_state_space(out, table.space);
	out += ",\"options\":";
	json_ids(out, table.options);
	out += ",\"choices\":[";
	for (size_t c = 0; c < table.choices.size(); c++) {
		if (c)
			out += ',';
		out += "{\"id\":";
		json_string(out, table.choices[c]);
		out += ",\"values\":";
		json_ids(out, choice_values(table.choices[c]));
		out += '}';
	}
	out += "],\"rows\":[";
	const size_t states = state_count(table.space);
	for (size_t s = 0; s < states; s++) {
		const pairs state = state_inputs(table.space, s);
		std::vector<unsigned long long> answers;
		const loaded_state loaded(table.space, state);
		if (!loaded.ok())
			return false;
		for (size_t p = 0; p < probes().size(); p++) {
			loaded.with(without_inputs(probes()[p], table.space));
			amiberry_settings_session* session = amiberry_settings_session_open();
			if (!session)
				return false;
			const std::vector<unsigned long long> probed = availability_answers(session, table);
			amiberry_settings_session_close(session);
			if (p == 0) {
				answers = probed;
				continue;
			}
			for (size_t m = 0; m < answers.size(); m++) {
				if (probed[m] != answers[m]) {
					std::fprintf(stderr,
						"--settings-schema: the availability of %s in %s changes with %s; "
						"a rule reads an option the table does not have as an input\n",
						m ? table.choices[m - 1] : "the table's options", describe(state).c_str(),
						describe(probes()[p]).c_str());
					return false;
				}
			}
		}
		if (s)
			out += ',';
		out += '[';
		for (size_t m = 0; m < answers.size(); m++) {
			if (m)
				out += ',';
			json_number(out, static_cast<double>(answers[m]));
		}
		out += ']';
	}
	out += "]}";
	return true;
}

bool availability(std::string& out)
{
	out += ",\n\"availability\":[";
	const std::vector<availability_table> tables = availability_tables();
	for (size_t t = 0; t < tables.size(); t++) {
		if (t)
			out += ',';
		if (!availability_table_json(out, tables[t]))
			return false;
	}
	out += "\n]";
	return true;
}

// What setting an option does in each state: the session's set, as a
// front end's edit makes it, and the fields read back. An outcome lists
// the fields the setting writes as [field index, value, ...]: an input
// whose value it changes, or another field it forces to one value whatever
// that was (a field it keeps is not listed). Each row has one outcome
// index per action value (the actions' values in order); -1 is a value the
// layer refuses in that state.
struct transition_table {
	state_space space;
	std::vector<field> actions;
	std::vector<const char*> fields;
};

transition_table cpu_transitions()
{
	return {cpu_states(),
		{
			{"cpu_model", choice_values("cpu_model")},
			{"cycle_exact", choice_values("cycle_exact")},
			{"cachesize", {"0", "8192"}},
			{"cpu_compatible", {"true", "false"}},
		},
		{"cpu_model", "cpu_24bit_addressing", "cycle_exact", "cachesize", "cpu_compatible", "fpu_model",
			"cpu_speed", "immediate_blits", "compfpu"}};
}

// The Zorro II fast RAM boards, as cfgfile names them: a board's size is
// one value written under one of two keys (the MB key, or the KB key for a
// board below 1 MB), so setting either changes what both read. One table
// per board.
std::vector<transition_table> memory_transitions()
{
	static const char* const mb_keys[] = {"fastmem_size", "fastmem2_size", "fastmem3_size", "fastmem4_size"};
	static const char* const kb_keys[] = {"fastmem_size_k", "fastmem2_size_k", "fastmem3_size_k", "fastmem4_size_k"};
	std::vector<transition_table> tables;
	for (int board = 0; board < 4; board++) {
		const std::vector<const char*> kb = accepted_values(kb_keys[board]);
		tables.push_back({{{{mb_keys[board], choice_values(mb_keys[board])}, {kb_keys[board], kb}}, {}},
			{{mb_keys[board], choice_values(mb_keys[board])}, {kb_keys[board], kb}},
			{mb_keys[board], kb_keys[board]}});
	}
	return tables;
}

std::vector<std::string> read_fields(const amiberry_settings_session* session, const std::vector<const char*>& fields)
{
	std::vector<std::string> values;
	for (const char* id : fields)
		values.push_back(value_of(session, id));
	return values;
}

bool transition_table_json(std::string& out, const transition_table& table)
{
	const auto& fields = table.fields;
	std::vector<bool> is_input(fields.size(), false);
	for (size_t f = 0; f < fields.size(); f++) {
		for (const field& input : table.space.inputs)
			is_input[f] = is_input[f] || std::strcmp(input.id, fields[f]) == 0;
	}
	pairs actions;
	for (const field& action : table.actions) {
		for (const char* value : action.values)
			actions.emplace_back(action.id, value);
	}

	using outcome = std::vector<std::pair<size_t, std::string>>;
	std::vector<outcome> outcomes;
	std::string rows;
	const size_t states = state_count(table.space);
	for (size_t s = 0; s < states; s++) {
		const pairs state = state_inputs(table.space, s);
		// Per probe: the fields before, and after each action (none when refused).
		std::vector<std::vector<std::string>> before(probes().size());
		std::vector<std::vector<std::vector<std::string>>> after(probes().size());
		std::vector<std::vector<bool>> refused(probes().size());
		const loaded_state loaded(table.space, state);
		if (!loaded.ok())
			return false;
		for (size_t p = 0; p < probes().size(); p++) {
			loaded.with(without_inputs(probes()[p], table.space));
			for (const auto& [id, value] : actions) {
				amiberry_settings_session* session = amiberry_settings_session_open();
				if (!session)
					return false;
				if (before[p].empty())
					before[p] = read_fields(session, fields);
				const bool set = amiberry_settings_set(session, id, value) == AMIBERRY_SETTINGS_OK;
				refused[p].push_back(!set);
				after[p].push_back(set ? read_fields(session, fields) : std::vector<std::string>());
				amiberry_settings_session_close(session);
			}
		}
		for (size_t f = 0; f < fields.size(); f++) {
			bool varied = false;
			for (size_t p = 1; p < probes().size(); p++)
				varied = varied || before[p][f] != before[0][f];
			if (is_input[f] && varied) {
				std::fprintf(stderr, "--settings-schema: a probe changes the input %s in %s\n",
					fields[f], describe(state).c_str());
				return false;
			}
			if (!is_input[f] && !varied) {
				std::fprintf(stderr, "--settings-schema: no probe changes %s in %s, so a transition cannot "
					"tell a value it forces from one it keeps\n", fields[f], describe(state).c_str());
				return false;
			}
		}

		if (s)
			rows += ',';
		rows += "\n[";
		for (size_t a = 0; a < actions.size(); a++) {
			const std::string action = std::string(actions[a].first) + '=' + actions[a].second;
			for (size_t p = 1; p < probes().size(); p++) {
				if (refused[p][a] != refused[0][a]) {
					std::fprintf(stderr, "--settings-schema: whether %s is refused in %s changes with %s; "
						"a rule reads an option the table does not have as an input\n",
						action.c_str(), describe(state).c_str(), describe(probes()[p]).c_str());
					return false;
				}
			}
			if (a)
				rows += ',';
			if (refused[0][a]) {
				rows += "-1";
				continue;
			}
			outcome changes;
			for (size_t f = 0; f < fields.size(); f++) {
				bool kept = true;
				bool same = true;
				for (size_t p = 0; p < probes().size(); p++) {
					kept = kept && after[p][a][f] == before[p][f];
					same = same && after[p][a][f] == after[0][a][f];
				}
				// An input is part of the state, so its new value must not
				// depend on the probes; another field is either kept or set
				// to one value whatever it was.
				if (!is_input[f] && kept)
					continue;
				if (!same) {
					std::fprintf(stderr, "--settings-schema: %s after %s in %s changes with the probes; "
						"a setter reads an option the table does not have as an input\n",
						fields[f], action.c_str(), describe(state).c_str());
					return false;
				}
				if (!is_input[f] || after[0][a][f] != before[0][f])
					changes.emplace_back(f, after[0][a][f]);
			}
			size_t index = std::find(outcomes.begin(), outcomes.end(), changes) - outcomes.begin();
			if (index == outcomes.size())
				outcomes.push_back(changes);
			rows += std::to_string(index);
		}
		rows += ']';
	}

	out += '{';
	json_state_space(out, table.space);
	out += ",\"fields\":";
	json_ids(out, fields);
	out += ",\"actions\":";
	json_fields(out, table.actions);
	out += ",\"outcomes\":[";
	for (size_t o = 0; o < outcomes.size(); o++) {
		if (o)
			out += ',';
		out += '[';
		for (size_t c = 0; c < outcomes[o].size(); c++) {
			if (c)
				out += ',';
			json_number(out, static_cast<double>(outcomes[o][c].first));
			out += ',';
			json_string(out, outcomes[o][c].second.c_str());
		}
		out += ']';
	}
	out += "],\"rows\":[";
	out += rows;
	out += "\n]}";
	return true;
}

// The CPU table, then the memory boards' tables.
bool transitions(std::string& out)
{
	out += ",\n\"transitions\":";
	if (!transition_table_json(out, cpu_transitions()))
		return false;
	out += ",\n\"memory_transitions\":[";
	const std::vector<transition_table> boards = memory_transitions();
	for (size_t t = 0; t < boards.size(); t++) {
		out += t ? ",\n" : "\n";
		if (!transition_table_json(out, boards[t]))
			return false;
	}
	out += "\n]";
	return true;
}

// The built-in AROS Kickstart replacement (memory.cpp): the ROM file name
// that selects it and the files it needs in a ROM directory.
void aros(std::string& out)
{
	out += ",\n\"aros\":{\"rom_file\":";
	json_string(out, aros_rom_token);
	out += ",\"main_file\":";
	json_string(out, aros_main_rom_file);
	out += ",\"ext_file\":";
	json_string(out, aros_ext_rom_file);
	out += '}';
}

} // namespace

// Writes the schema to stdout; 0 on success.
int settings_schema_and_exit(const char* option_list)
{
	std::set<std::string> only;
	if (option_list && !read_option_list(option_list, only))
		return 1;
	std::string out = "{\"schema_version\":1,\"api_version\":";
	json_number(out, AMIBERRY_SETTINGS_API_VERSION);
	out += ",\n";
	// The presets first: the options their levels set are described too.
	std::string preset_json;
	if (!presets(preset_json)) {
		std::fprintf(stderr, "--settings-schema: out of memory\n");
		return 1;
	}
	const std::set<std::string> listed = only;
	only.insert(preset_option_ids.begin(), preset_option_ids.end());
	options(out, option_list ? &only : nullptr, option_list ? &listed : nullptr);
	out += preset_json;
	if (!resolutions(out)) {
		std::fprintf(stderr, "--settings-schema: out of memory or an empty resolution input\n");
		return 1;
	}
	if (!availability(out) || !transitions(out)) {
		std::fprintf(stderr, "--settings-schema: the availability or transition tables cannot be made\n");
		return 1;
	}
	aros(out);
	out += "\n}\n";
	if (std::fwrite(out.data(), 1, out.size(), stdout) != out.size() || std::fflush(stdout) != 0) {
		std::fprintf(stderr, "--settings-schema: writing to stdout failed\n");
		return 1;
	}
	return 0;
}
