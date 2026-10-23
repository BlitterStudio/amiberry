#pragma once

// Internal side of the shared settings layer: option definitions and the
// operations on a uae_prefs instance that both edit sessions and the ImGui
// panels (which edit changed_prefs directly) use. Front ends outside the
// emulator source use amiberry_settings.h instead.

#include "amiberry_settings.h"

#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string_view>
#include <vector>

struct uae_prefs;

namespace amiberry_settings {

struct context {
	bool emulating;
};

// The choices of one option at one moment: pointers to the descriptor's
// static list, or to a list a dynamic_choices hook filled.
struct choice_list {
	const amiberry_setting_choice* items;
	size_t count;
};

// A value that passed the type, range and choice checks. text is the .uae
// spelling; number holds its numeric value for INT, REAL, numeric CHOICE
// values and BOOL (0 or 1).
struct value {
	std::string_view text;
	double number;
};

struct definition {
	amiberry_setting_descriptor descriptor;
	// Writes the current value in .uae spelling.
	void (*get)(const uae_prefs& p, char* out, size_t size);
	// Stores a checked value and makes the dependent changes the option
	// implies. It is also called with the current value (a re-clicked radio
	// button), so a dependent change that must only follow a real change
	// checks the old value itself.
	void (*set)(uae_prefs& p, const value& v);
	// nullptr: always available.
	bool (*available)(const uae_prefs& p, const context& ctx);
	// nullptr: every choice is available while the option is.
	bool (*choice_available)(const uae_prefs& p, std::string_view choice);
	// nullptr: the descriptor's static range.
	void (*range)(const uae_prefs& p, double& min, double& max);
	// nullptr: every value within the range and step. Otherwise rejects
	// values the core would not keep (a JIT cache below 128 KB).
	bool (*accepts)(const uae_prefs& p, const value& v);
	// nullptr: the choice list is the descriptor's static one. Otherwise
	// enumerates the choices the host offers now (connected audio
	// devices, joysticks and mice). Main thread only; the storage the
	// returned items point into lives until the next call of the same
	// hook.
	void (*dynamic_choices)(const uae_prefs& p, choice_list& out);
};

// What a commit knows about the session besides its configurations: the
// options its caller set (by definition position), so a domain can tell a
// field the caller chose from one a dependent change wrote.
struct commit_info {
	const std::vector<bool>& touched;
	bool was_set(const definition& d) const;
};

struct domain {
	const definition* definitions;
	size_t count;
	// Copies to `to` every field this domain's options and their dependent
	// changes write whose value differs between `from` and `base`, so a
	// commit keeps dependent changes and leaves fields the session did not
	// change as other code may have set them since. `base` and `to` may be
	// the same object. Returns whether it wrote anything.
	bool (*commit)(const uae_prefs& base, const uae_prefs& from, uae_prefs& to,
		const commit_info& info);
	// nullptr: no host-dependent choices. Re-reads the host device lists
	// this domain's dynamic choices enumerate. Main thread only.
	void (*refresh_choices)();
};

// For domain commit functions: copies one field (scalar or array) when the
// session changed it and reports whether it did. Compares bytes, so floats
// compare exactly.
template <typename T>
bool commit_field(const T& base, const T& from, T& to)
{
	if (std::memcmp(&base, &from, sizeof(T)) == 0)
		return false;
	std::memcpy(&to, &from, sizeof(T));
	return true;
}

// Runs the body of a C entry point. No C++ exception (std::bad_alloc while
// building the option index or a session) may cross into C, Swift, C# or
// Kotlin callers, so a failure returns the entry point's error value.
template <typename T, typename F>
T no_throw(T fallback, F&& body) noexcept
{
	try {
		return body();
	} catch (...) {
		return fallback;
	}
}

// Large enough for any value a descriptor can produce, including paths
// (uae_prefs stores paths as MAX_DPATH, 4096 bytes). Public adjustment
// reports stay limited to AMIBERRY_SETTINGS_VALUE_MAX and truncate.
constexpr size_t max_value_length = 4096;

// The choices an option accepts now: the descriptor's static list, or the
// host-dependent list when it has one. Main thread only.
choice_list choices_of(const definition& d, const uae_prefs& p);
// Re-reads every host device list the layer enumerates. Main thread only.
void refresh_dynamic_choices();
// Calls visit for every configuration an open session holds (working
// copy, base and preset snapshots), for host device refreshes that must
// find the devices those configurations select again. Main thread only.
void for_each_session_config(void (*visit)(uae_prefs& p, void* context), void* context);

const domain& cpu_domain();
const domain& chipset_domain();
const domain& memory_domain();
const domain& rom_domain();
const domain& display_domain();
const domain& sound_domain();
const domain& input_domain();
const domain& onscreen_domain();
const domain& floppy_domain();
const domain& storage_domain();
const domain& misc_domain();
const domain& rtg_domain();

// target_fixup_options() resets an SDL sound device to the first one while
// a screen uses the lagless vsync mode (fixup_prefs_dimensions() turns RTG
// vsync into it). Whether that happens to card, and the dependent change
// the vsync and screen-mode setters make.
bool soundcard_reset_by_vsync(const uae_prefs& p, int card);
void clear_soundcard_for_vsync(uae_prefs& p);

// The Quickstart constraints that complete built_in_prefs(); applies model,
// config and compatibility level exactly like the Quickstart panel (which
// historically ran these steps in src/osdep/imgui/quickstart.cpp). The
// panel and --apply-preset apply to the live pending configuration and
// reset the global defaults a configuration load resets; preview applies
// to a copy (a session) and writes nothing but p.
void apply_preset(uae_prefs& p, int model, int config, int compatibility, bool preview);

// The Quickstart model table: index <-> stable preset ID. Negative for an
// unknown ID; nullptr for an out-of-range model.
int preset_model_index(std::string_view id);
const char* preset_id(int model);

// WinUAE copycpuboardmem(tomem = true): keeps the accelerator board RAM in
// sync with the memory subsystem the user edited, so the core fixup (which
// copies the other way) does not undo the edit.
void sync_cpuboard_mem(uae_prefs& p);

// The other direction, the copy fixup_prefs() makes: the accelerator
// board RAM lands in the memory slot the board uses. Options that change
// the board (the automatic PPC configuration) call it so the stored
// configuration matches what the fixup resolves to.
void mirror_cpuboard_mem(uae_prefs& p);

// The JIT cache size the checks after fixup_cpu() see (0 when the JIT is
// not compiled in, inhibited, below a 68020 or in memory cycle-exact
// mode). The memory domain uses it for the sizes the JIT cannot map.
int jit_cache_after_fixup_cpu(const uae_prefs& p);

// fixup_prefs()'s RAM clamps that depend on other options (the JIT, the
// Gayle/Gary address conflict). Setters of those options run it, so the RAM
// options never show a size the fixup will change.
void clamp_ram_for_fixups(uae_prefs& p);

// Machine presets (settings_presets.cpp). The public C wrappers live next
// to the implementations; amiberry_settings_preset_apply lives in
// settings_session.cpp because it owns the session.
size_t machine_preset_count();
const amiberry_settings_preset* machine_preset_at(size_t index);
const amiberry_settings_preset* machine_preset_find(const char* id);
const char* machine_preset_config_label(const amiberry_settings_preset* preset, int config);
const char* machine_preset_compatibility_label(int level);
bool machine_preset_rom_installed(const char* id, int config);
// bip_alg (model 10) and bip_arcadia (11) load the config-th entry of
// getarcadiaroms(): every known ALG game, or every Arcadia BIOS and game,
// installed or not. Their one preset configuration is the first installed
// game; this is its builder index, or -1 (none installed, other models).
int arcade_game_config(int model);
// Checks a public preset request and returns its built_in_prefs() model:
// UNKNOWN_ID / INVALID_VALUE / UNAVAILABLE as amiberry_settings_preset_apply
// reports them.
amiberry_settings_status check_preset(const char* id, int config, int compatibility, int& model);

// Whether a and b hold the same value for d (full length, for paths).
bool values_equal(const definition& d, const uae_prefs& a, const uae_prefs& b);

// Whether a and b mount the same drives: the same entries, in the same
// order (the storage domain commits the list as a whole).
bool same_mount_list(const uae_prefs& a, const uae_prefs& b);

size_t domain_count();
const domain& domain_at(size_t index);

size_t definition_count();
const definition& definition_at(size_t index);
// The index of d in definition order.
size_t position_of(const definition& d);
const definition* find(std::string_view id);
// For IDs compiled into a caller: aborts when the ID is unknown.
const definition& require(std::string_view id);

bool is_available(const definition& d, const uae_prefs& p, const context& ctx);
bool is_choice_available(const definition& d, const uae_prefs& p, const context& ctx,
	std::string_view choice);
void get_range(const definition& d, const uae_prefs& p, double& min, double& max);

amiberry_settings_status get(const definition& d, const uae_prefs& p, char* out, size_t size);
// Leaves p untouched unless it returns AMIBERRY_SETTINGS_OK.
amiberry_settings_status set(const definition& d, uae_prefs& p, const context& ctx,
	std::string_view text);

// Typed forms for BOOL, INT, REAL and numeric CHOICE options.
bool get_bool(const definition& d, const uae_prefs& p);
double get_number(const definition& d, const uae_prefs& p);
amiberry_settings_status set_bool(const definition& d, uae_prefs& p, const context& ctx, bool v);
amiberry_settings_status set_number(const definition& d, uae_prefs& p, const context& ctx,
	double v);

// Writers the domains' get functions share.
inline void write_bool(char* out, size_t size, bool v)
{
	std::snprintf(out, size, "%s", v ? "true" : "false");
}

inline void write_int(char* out, size_t size, int v)
{
	std::snprintf(out, size, "%d", v);
}

// cfgfile writes the throttle and refresh rate options with one decimal.
inline void write_real(char* out, size_t size, float v)
{
	std::snprintf(out, size, "%.1f", v);
}

// Index of text in choices, or -1.
inline int choice_index(const amiberry_setting_choice* choices, size_t count,
	std::string_view text)
{
	for (size_t i = 0; i < count; i++) {
		if (text == choices[i].value)
			return static_cast<int>(i);
	}
	return -1;
}

inline int choice_index(const choice_list& choices, std::string_view text)
{
	return choice_index(choices.items, choices.count, text);
}

inline void write_choice(char* out, size_t size, const amiberry_setting_choice* choices,
	size_t count, int index)
{
	std::snprintf(out, size, "%s",
		index >= 0 && static_cast<size_t>(index) < count ? choices[index].value : "");
}

inline void write_choice(char* out, size_t size, const choice_list& choices, int index)
{
	write_choice(out, size, choices.items, choices.count, index);
}

} // namespace amiberry_settings
