#pragma once

// Internal side of the shared settings layer: option definitions and the
// operations on a uae_prefs instance that both edit sessions and the ImGui
// panels (which edit changed_prefs directly) use. Front ends outside the
// emulator source use amiberry_settings.h instead.

#include "amiberry_settings.h"

#include <cstddef>
#include <cstring>
#include <string_view>

struct uae_prefs;

namespace amiberry_settings {

struct context {
	bool emulating;
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
};

struct domain {
	const definition* definitions;
	size_t count;
	// Copies to `to` every field this domain's options and their dependent
	// changes write whose value differs between `from` and `base`, so a
	// commit keeps dependent changes and leaves fields the session did not
	// change as other code may have set them since. `base` and `to` may be
	// the same object. Returns whether it wrote anything.
	bool (*commit)(const uae_prefs& base, const uae_prefs& from, uae_prefs& to);
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

const domain& cpu_domain();
const domain& chipset_domain();
const domain& memory_domain();
const domain& rom_domain();
const domain& machine_domain();

// The Quickstart constraints that complete built_in_prefs(); applies model,
// config and compatibility level exactly like the Quickstart panel (which
// historically ran these steps in src/osdep/imgui/quickstart.cpp).
void apply_preset(uae_prefs& p, int model, int config, int compatibility);

// The Quickstart model table: index <-> stable preset ID. Negative for an
// unknown ID; nullptr for an out-of-range model.
int preset_model_index(std::string_view id);
const char* preset_id(int model);

// WinUAE copycpuboardmem(tomem = true): keeps the accelerator board RAM in
// sync with the memory subsystem the user edited, so the core fixup (which
// copies the other way) does not undo the edit.
void sync_cpuboard_mem(uae_prefs& p);

// Machine presets (settings_presets.cpp). The public C wrappers live next
// to the implementations; amiberry_settings_preset_apply lives in
// settings_session.cpp because it owns the session.
size_t machine_preset_count();
const amiberry_settings_preset* machine_preset_at(size_t index);
const amiberry_settings_preset* machine_preset_find(const char* id);
const char* machine_preset_config_label(const amiberry_settings_preset* preset, int config);
const char* machine_preset_compatibility_label(int level);
bool machine_preset_rom_installed(const char* id, int config);
amiberry_settings_status apply_preset_checked(uae_prefs& p, const char* id, int config,
	int compatibility);

// Whether a and b hold the same value for d (full length, for paths).
bool values_equal(const definition& d, const uae_prefs& a, const uae_prefs& b);

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

} // namespace amiberry_settings
