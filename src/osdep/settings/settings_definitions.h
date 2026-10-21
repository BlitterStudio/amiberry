#pragma once

// Internal side of the shared settings layer: option definitions and the
// operations on a uae_prefs instance that both edit sessions and the ImGui
// panels (which edit changed_prefs directly) use. Front ends outside the
// emulator source use amiberry_settings.h instead.

#include "amiberry_settings.h"

#include <cstddef>
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
	// implies.
	void (*set)(uae_prefs& p, const value& v);
	// nullptr: always available.
	bool (*available)(const uae_prefs& p, const context& ctx);
	// nullptr: every choice is available while the option is.
	bool (*choice_available)(const uae_prefs& p, std::string_view choice);
	// nullptr: the descriptor's static range.
	void (*range)(const uae_prefs& p, double& min, double& max);
};

struct domain {
	const definition* definitions;
	size_t count;
	// The core fixup that resolves this domain's options before the
	// emulator uses them. It must change nothing but the prefs it is given.
	void (*fixup)(uae_prefs* p);
	// Copies every field this domain's options and their dependent changes
	// write, so a commit cannot drop a dependent change.
	void (*commit)(const uae_prefs& from, uae_prefs& to);
};

const domain& cpu_domain();

size_t domain_count();
const domain& domain_at(size_t index);

size_t definition_count();
const definition& definition_at(size_t index);
const definition* find(std::string_view id);
// For IDs compiled into a caller: aborts when the ID is unknown.
const definition& require(std::string_view id);

bool is_available(const definition& d, const uae_prefs& p, const context& ctx);
bool is_choice_available(const definition& d, const uae_prefs& p, const context& ctx,
	std::string_view choice);
void get_range(const definition& d, const uae_prefs& p, double& min, double& max);

amiberry_settings_status get(const definition& d, const uae_prefs& p, char* out, size_t size);
// Leaves p untouched unless it returns AMIBERRY_SETTINGS_OK. Setting the
// current value again is a no-op, so dependent changes are not repeated.
amiberry_settings_status set(const definition& d, uae_prefs& p, const context& ctx,
	std::string_view text);

// Typed forms for BOOL, INT, REAL and numeric CHOICE options.
bool get_bool(const definition& d, const uae_prefs& p);
double get_number(const definition& d, const uae_prefs& p);
amiberry_settings_status set_bool(const definition& d, uae_prefs& p, const context& ctx, bool v);
amiberry_settings_status set_number(const definition& d, uae_prefs& p, const context& ctx,
	double v);

} // namespace amiberry_settings
