#include "settings_definitions.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace amiberry_settings {

namespace {

// Function-local so the list is built on first use, never during another
// translation unit's static initialisation.
struct domain_list {
	const domain* items[5];
};

const domain_list& domains()
{
	static const domain_list list = {{
		&cpu_domain(),
		&chipset_domain(),
		&memory_domain(),
		&rom_domain(),
		&machine_domain(),
	}};
	return list;
}

struct definition_index {
	std::vector<const definition*> ordered;
	std::unordered_map<std::string_view, const definition*> by_id;
	std::unordered_map<const definition*, size_t> position;
};

const definition_index& definitions_index()
{
	static const definition_index built = [] {
		definition_index result;
		for (const domain* dom : domains().items) {
			for (size_t i = 0; i < dom->count; i++) {
				const definition& d = dom->definitions[i];
				result.position.emplace(&d, result.ordered.size());
				result.ordered.push_back(&d);
				if (!result.by_id.emplace(d.descriptor.id, &d).second) {
					std::fprintf(stderr, "amiberry settings: duplicate option ID %s\n", d.descriptor.id);
					std::abort();
				}
			}
		}
		return result;
	}();
	return built;
}

// Parses the whole of text as a number; rejects empty input, trailing
// characters and NaN/infinity.
bool parse_number(std::string_view text, bool integer, double& out)
{
	if (text.empty() || text.size() >= AMIBERRY_SETTINGS_VALUE_MAX)
		return false;
	char buffer[AMIBERRY_SETTINGS_VALUE_MAX];
	std::memcpy(buffer, text.data(), text.size());
	buffer[text.size()] = 0;
	char* end = nullptr;
	errno = 0;
	if (integer) {
		const long long v = std::strtoll(buffer, &end, 10);
		if (errno || *end)
			return false;
		out = static_cast<double>(v);
	} else {
		const double v = std::strtod(buffer, &end);
		if (errno || *end || !std::isfinite(v))
			return false;
		out = v;
	}
	return true;
}

bool is_choice(const definition& d, std::string_view text)
{
	for (size_t i = 0; i < d.descriptor.choice_count; i++) {
		if (text == d.descriptor.choices[i].value)
			return true;
	}
	return false;
}

// Checks text against the option's type and current range and fills v.
amiberry_settings_status check_value(const definition& d, const uae_prefs& p,
	const context& ctx, std::string_view text, value& v)
{
	v.text = text;
	v.number = 0;
	switch (d.descriptor.type) {
	case AMIBERRY_SETTING_BOOL:
		if (text == "true")
			v.number = 1;
		else if (text != "false")
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		return AMIBERRY_SETTINGS_OK;
	case AMIBERRY_SETTING_INT:
	case AMIBERRY_SETTING_REAL: {
		const bool integer = d.descriptor.type == AMIBERRY_SETTING_INT;
		if (!parse_number(text, integer, v.number))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		double min, max;
		get_range(d, p, min, max);
		if (v.number < min || v.number > max)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (integer && d.descriptor.step > 1
			&& std::fmod(v.number - d.descriptor.min, d.descriptor.step) != 0)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		return AMIBERRY_SETTINGS_OK;
	}
	case AMIBERRY_SETTING_PATH:
	case AMIBERRY_SETTING_STRING:
		// Free-form token: the setter validates. Length is capped so a
		// path always fits the field it is stored in.
		if (text.size() >= max_value_length)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		v.text = text;
		return AMIBERRY_SETTINGS_OK;
	case AMIBERRY_SETTING_CHOICE:
		if (!is_choice(d, text))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (!is_choice_available(d, p, ctx, text))
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		if (!parse_number(text, true, v.number))
			v.number = 0;
		return AMIBERRY_SETTINGS_OK;
	}
	return AMIBERRY_SETTINGS_INVALID_VALUE;
}

// Whether two configurations hold the same value for d, compared at full
// length so two paths that share a prefix still differ.
bool equal_values(const definition& d, const uae_prefs& a, const uae_prefs& b)
{
	char left[max_value_length];
	char right[max_value_length];
	d.get(a, left, sizeof left);
	d.get(b, right, sizeof right);
	return std::strcmp(left, right) == 0;
}

} // namespace

size_t domain_count()
{
	return sizeof domains().items / sizeof domains().items[0];
}

const domain& domain_at(size_t index)
{
	return *domains().items[index];
}

size_t definition_count()
{
	return definitions_index().ordered.size();
}

const definition& definition_at(size_t index)
{
	return *definitions_index().ordered[index];
}

size_t position_of(const definition& d)
{
	return definitions_index().position.at(&d);
}

const definition* find(std::string_view id)
{
	const auto& by_id = definitions_index().by_id;
	const auto it = by_id.find(id);
	return it == by_id.end() ? nullptr : it->second;
}

const definition& require(std::string_view id)
{
	const definition* d = find(id);
	if (!d) {
		std::fprintf(stderr, "amiberry settings: unknown option ID %.*s\n",
			static_cast<int>(id.size()), id.data());
		std::abort();
	}
	return *d;
}

bool is_available(const definition& d, const uae_prefs& p, const context& ctx)
{
	return !d.available || d.available(p, ctx);
}

bool is_choice_available(const definition& d, const uae_prefs& p, const context& ctx,
	std::string_view choice)
{
	if (!is_available(d, p, ctx))
		return false;
	value v{choice, 0};
	if (d.descriptor.type == AMIBERRY_SETTING_BOOL) {
		if (choice != "true" && choice != "false")
			return false;
		v.number = choice == "true" ? 1 : 0;
	} else {
		if (!is_choice(d, choice))
			return false;
		if (d.choice_available && !d.choice_available(p, choice))
			return false;
		if (!parse_number(choice, true, v.number))
			v.number = 0;
	}
	return !d.accepts || d.accepts(p, v);
}

void get_range(const definition& d, const uae_prefs& p, double& min, double& max)
{
	min = d.descriptor.min;
	max = d.descriptor.max;
	if (d.range)
		d.range(p, min, max);
}

amiberry_settings_status get(const definition& d, const uae_prefs& p, char* out, size_t size)
{
	char text[max_value_length];
	d.get(p, text, sizeof text);
	const size_t length = std::strlen(text);
	if (!out || length >= size)
		return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
	std::memcpy(out, text, length + 1);
	return AMIBERRY_SETTINGS_OK;
}

amiberry_settings_status set(const definition& d, uae_prefs& p, const context& ctx,
	std::string_view text)
{
	if (!is_available(d, p, ctx))
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	value v{};
	const amiberry_settings_status status = check_value(d, p, ctx, text, v);
	if (status != AMIBERRY_SETTINGS_OK)
		return status;
	if (d.accepts && !d.accepts(p, v))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	d.set(p, v);
	return AMIBERRY_SETTINGS_OK;
}

bool get_bool(const definition& d, const uae_prefs& p)
{
	char text[AMIBERRY_SETTINGS_VALUE_MAX];
	d.get(p, text, sizeof text);
	return std::strcmp(text, "true") == 0;
}

double get_number(const definition& d, const uae_prefs& p)
{
	char text[AMIBERRY_SETTINGS_VALUE_MAX];
	d.get(p, text, sizeof text);
	double number;
	return parse_number(text, false, number) ? number : 0;
}

amiberry_settings_status set_bool(const definition& d, uae_prefs& p, const context& ctx, bool v)
{
	return set(d, p, ctx, v ? "true" : "false");
}

amiberry_settings_status set_number(const definition& d, uae_prefs& p, const context& ctx,
	double v)
{
	char text[AMIBERRY_SETTINGS_VALUE_MAX];
	if (d.descriptor.type == AMIBERRY_SETTING_REAL)
		std::snprintf(text, sizeof text, "%.17g", v);
	else
		std::snprintf(text, sizeof text, "%lld", static_cast<long long>(std::llround(v)));
	return set(d, p, ctx, text);
}

bool values_equal(const definition& d, const uae_prefs& a, const uae_prefs& b)
{
	return equal_values(d, a, b);
}

} // namespace amiberry_settings

extern "C" {

size_t amiberry_settings_count(void)
{
	return amiberry_settings::no_throw<size_t>(0, [] { return amiberry_settings::definition_count(); });
}

const amiberry_setting_descriptor* amiberry_settings_at(size_t index)
{
	return amiberry_settings::no_throw<const amiberry_setting_descriptor*>(nullptr,
		[index]() -> const amiberry_setting_descriptor* {
			if (index >= amiberry_settings::definition_count())
				return nullptr;
			return &amiberry_settings::definition_at(index).descriptor;
		});
}

const amiberry_setting_descriptor* amiberry_settings_find(const char* id)
{
	return amiberry_settings::no_throw<const amiberry_setting_descriptor*>(nullptr,
		[id]() -> const amiberry_setting_descriptor* {
			const amiberry_settings::definition* d = id ? amiberry_settings::find(id) : nullptr;
			return d ? &d->descriptor : nullptr;
		});
}

}
