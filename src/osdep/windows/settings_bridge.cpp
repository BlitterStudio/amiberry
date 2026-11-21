/* Native Windows settings protocol. The transport calls this on the core thread. */
#include "settings_bridge.h"
#include "settings/amiberry_settings.h"
#include "settings/amiberry_actions.h"
#include "settings/amiberry_media.h"
#include "settings/amiberry_whdboot.h"
#include "native_ui_host.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "amiberry_filesys_charset.h"

namespace {
using json = nlohmann::json;

// Replies leave the process as UTF-8 text: no ANSI path may reach the JSON
// writer, which would replace what it cannot decode.
void reply_text(json& value)
{
	if (value.is_string()) {
		auto& text = value.get_ref<std::string&>();
		if (!amiberry_valid_utf8(text))
			text = windows_native_core_to_utf8(text);
	} else if (value.is_structured()) {
		for (auto& item : value)
			reply_text(item);
	}
}

const char* text(const char* value) { return value ? value : ""; }

void check(amiberry_settings_status status, const std::string& operation)
{
	if (status == AMIBERRY_SETTINGS_OK)
		return;
	const char* reason = "Settings layer failure";
	switch (status) {
	case AMIBERRY_SETTINGS_UNKNOWN_ID: reason = "Unknown setting, field or entry"; break;
	case AMIBERRY_SETTINGS_INVALID_VALUE: reason = "Invalid value"; break;
	case AMIBERRY_SETTINGS_UNAVAILABLE: reason = "Unavailable in the current configuration, or could not be stored"; break;
	case AMIBERRY_SETTINGS_BUFFER_TOO_SMALL: reason = "Value exceeds the supported buffer size"; break;
	default: break;
	}
	throw std::runtime_error(operation + ": " + reason);
}

struct LayerText {
	std::string value;
	amiberry_settings_status status;
};

// Most values fit the core's maximum path length without a heap buffer.
// BUFFER_TOO_SMALL is retried for configuration lines and future long values.
template<class Read>
LayerText read_text(Read&& read)
{
	std::array<char, AMIBERRY_ACTIONS_PATH_MAX> buffer;
	auto status = read(buffer.data(), buffer.size());
	if (status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL)
		return {status == AMIBERRY_SETTINGS_OK ? buffer.data() : "", status};
	for (size_t size = buffer.size() * 2; size <= 4 * 1024 * 1024; size *= 2) {
		std::vector<char> larger(size);
		status = read(larger.data(), larger.size());
		if (status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL)
			return {status == AMIBERRY_SETTINGS_OK ? larger.data() : "", status};
	}
	return {"", status};
}

template<class Read>
std::string require_text(const std::string& operation, Read&& read)
{
	auto result = read_text(std::forward<Read>(read));
	check(result.status, operation);
	return std::move(result.value);
}

std::string argument(const json& request, const char* key)
{
	auto value = request.at(key).get<std::string>();
	if (value.find('\0') != std::string::npos)
		throw std::runtime_error(std::string(key) + ": embedded NUL is not supported");
	return value;
}

std::string optional_argument(const json& request, const char* key)
{
	return request.contains(key) ? argument(request, key) : std::string();
}

int integer(const json& request, const char* key, int fallback = 0)
{
	if (!request.contains(key))
		return fallback;
	const auto& value = request.at(key);
	if (!value.is_number_integer())
		throw std::runtime_error(std::string(key) + ": expected an integer");
	if (value.is_number_unsigned()) {
		const auto number = value.get<std::uint64_t>();
		if (number > static_cast<std::uint64_t>(INT32_MAX))
			throw std::runtime_error(std::string(key) + ": integer out of range");
		return static_cast<int>(number);
	}
	const auto number = value.get<std::int64_t>();
	if (number < INT32_MIN || number > INT32_MAX)
		throw std::runtime_error(std::string(key) + ": integer out of range");
	return static_cast<int>(number);
}

std::uint64_t unsigned_integer(const json& request, const char* key)
{
	const auto& value = request.at(key);
	if (!value.is_number_integer() || (!value.is_number_unsigned() && value.get<std::int64_t>() < 0))
		throw std::runtime_error(std::string(key) + ": expected an unsigned integer");
	return value.get<std::uint64_t>();
}

unsigned int unsigned_argument(const json& request, const char* key)
{
	const auto value = unsigned_integer(request, key);
	if (value > UINT32_MAX)
		throw std::runtime_error(std::string(key) + ": integer out of range");
	return static_cast<unsigned int>(value);
}

size_t mount_index(const json& request)
{
	return static_cast<size_t>(unsigned_integer(request, "index"));
}

bool visible(const amiberry_setting_descriptor& descriptor)
{
	return !(descriptor.hidden_on & AMIBERRY_SETTINGS_HIDE_ON_WINDOWS);
}

json descriptor_json(const amiberry_setting_descriptor& descriptor)
{
	const char* kind = "string";
	switch (descriptor.type) {
	case AMIBERRY_SETTING_BOOL: kind = "bool"; break;
	case AMIBERRY_SETTING_INT: kind = "int"; break;
	case AMIBERRY_SETTING_REAL: kind = "real"; break;
	case AMIBERRY_SETTING_CHOICE: kind = "choice"; break;
	case AMIBERRY_SETTING_PATH: kind = "path"; break;
	case AMIBERRY_SETTING_FILE: kind = "file"; break;
	case AMIBERRY_SETTING_STRING: kind = "string"; break;
	case AMIBERRY_SETTING_HOTKEY: kind = "hotkey"; break;
	}
	json choices = json::array();
	for (size_t i = 0; i < descriptor.choice_count; ++i)
		choices.push_back({{"value", text(descriptor.choices[i].value)}, {"label", text(descriptor.choices[i].label)}});
	json result = {{"id", text(descriptor.id)}, {"group", text(descriptor.group)},
		{"label", text(descriptor.label)}, {"help", text(descriptor.help)},
		{"type", kind}, {"apply", descriptor.apply}, {"min", descriptor.min},
		{"max", descriptor.max}, {"step", descriptor.step},
		{"defaultValue", text(descriptor.default_value)}, {"reversed", descriptor.reversed},
		{"unit", text(descriptor.unit)}, {"scale", descriptor.scale}, {"level", descriptor.level},
		{"choices", std::move(choices)}, {"constraint", text(descriptor.constraint)}};
	if (descriptor.type == AMIBERRY_SETTING_HOTKEY) {
		result["takesModifiers"] = amiberry_settings_hotkey_takes_modifiers(descriptor.id);
		json buttons = json::array();
		if (amiberry_settings_hotkey_takes_buttons(descriptor.id)) {
			const amiberry_setting_choice* entries = nullptr;
			const auto count = amiberry_settings_hotkey_buttons(&entries);
			for (size_t i = 0; entries && i < count; ++i)
				buttons.push_back({{"value", text(entries[i].value)}, {"label", text(entries[i].label)}});
		}
		result["buttons"] = std::move(buttons);
	}
	return result;
}

struct BaselineValue {
	std::string value;
	bool readable;
};
using Baseline = std::map<std::string, BaselineValue>;

amiberry_settings_session* session = nullptr;
Baseline baseline_values;
Baseline baseline_boards;
Baseline baseline_controls;
std::vector<std::string> baseline_mounts;
bool preset_applied = false;
bool boards_edited = false;
bool controls_edited = false;

amiberry_settings_session* active_session()
{
	if (!session)
		throw std::runtime_error("No machine edit session is open");
	return session;
}

std::string current_configuration()
{
	return require_text("Reading current configuration", [](char* buffer, size_t size) {
		return amiberry_media_current_configuration(buffer, size);
	});
}

std::vector<std::string> mounts()
{
	std::vector<std::string> entries;
	const auto count = amiberry_settings_mount_count(active_session());
	entries.reserve(count);
	for (size_t i = 0; i < count; ++i)
		entries.push_back(require_text("Reading mount", [i](char* buffer, size_t size) {
			return amiberry_settings_mount_get(session, i, buffer, size);
		}));
	return entries;
}

json mount_json(const std::string& entry)
{
	json fields = json::object();
	static const char* const names[] = {"kind", "path", "device", "volume", "read_only",
		"boot_priority", "sectors", "surfaces", "reserved", "block_size", "controller", "unit"};
	for (const auto* field : names) {
		auto value = read_text([&](char* buffer, size_t size) {
			return amiberry_settings_mount_field_get(active_session(), entry.c_str(), field, buffer, size);
		});
		// A field that does not belong to this kind is absent, not an error.
		if (value.status == AMIBERRY_SETTINGS_UNKNOWN_ID)
			continue;
		check(value.status, std::string("Reading mount ") + field);
		fields[field] = std::move(value.value);
	}
	return {{"entry", entry}, {"fields", std::move(fields)}};
}

Baseline option_baseline()
{
	Baseline result;
	for (size_t i = 0; i < amiberry_settings_count(); ++i) {
		const auto* descriptor = amiberry_settings_at(i);
		if (!descriptor || !visible(*descriptor))
			continue;
		auto value = read_text([&](char* buffer, size_t size) {
			return amiberry_settings_get(active_session(), descriptor->id, buffer, size);
		});
		if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
			check(value.status, descriptor->id);
		result.emplace(descriptor->id, BaselineValue{std::move(value.value), value.status == AMIBERRY_SETTINGS_OK});
	}
	return result;
}

// Keep unavailable values in the baseline so enabling an option does not by
// itself invent an edit. Compare only fields the layer currently lets us edit.
Baseline board_baseline(bool available_only)
{
	Baseline result;
	for (size_t i = 0; i < amiberry_settings_board_type_count(); ++i) {
		const auto* type = amiberry_settings_board_type_at(i);
		if (!type)
			continue;
		for (int unit = 0; unit < type->units; ++unit) {
			for (size_t field = 0; field < type->field_count; ++field) {
				const auto* descriptor = amiberry_settings_board_field_at(type, field);
				if (!descriptor || (available_only && !amiberry_settings_board_field_available(session, type->id, unit, descriptor->id)))
					continue;
				auto value = read_text([&](char* buffer, size_t size) {
					return amiberry_settings_board_get(session, type->id, unit, descriptor->id, buffer, size);
				});
				if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
					check(value.status, type->id);
				result.emplace(std::string(type->id) + "/" + std::to_string(unit) + "/" + descriptor->id,
					BaselineValue{std::move(value.value), value.status == AMIBERRY_SETTINGS_OK});
			}
		}
	}
	return result;
}

Baseline control_baseline(bool available_only)
{
	Baseline result;
	for (int port = 0; port < 4; ++port) {
		for (bool hotkey : {false, true}) {
			for (size_t i = 0; i < amiberry_settings_custom_control_count(); ++i) {
				const auto* control = amiberry_settings_custom_control_at(i);
				if (!control || (available_only && !amiberry_settings_custom_control_available(session, port, hotkey, control->id)))
					continue;
				auto value = read_text([&](char* buffer, size_t size) {
					return amiberry_settings_custom_control_get(session, port, hotkey, control->id, buffer, size);
				});
				if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
					check(value.status, control->id);
				result.emplace(std::to_string(port) + (hotkey ? "/hotkey/" : "/normal/") + control->id,
					BaselineValue{std::move(value.value), value.status == AMIBERRY_SETTINGS_OK});
			}
		}
	}
	return result;
}

bool differs(const Baseline& current, const Baseline& baseline)
{
	for (const auto& [id, value] : current) {
		const auto found = baseline.find(id);
		if (value.readable && found != baseline.end() && found->second.readable && value.value != found->second.value)
			return true;
	}
	return false;
}

void mark_clean()
{
	baseline_values = option_baseline();
	baseline_mounts = mounts();
	baseline_boards = board_baseline(false);
	baseline_controls = control_baseline(false);
	preset_applied = boards_edited = controls_edited = false;
}

void open_session()
{
	windows_native_close_session();
	session = amiberry_settings_session_open();
	if (!session)
		throw std::runtime_error("Could not allocate a machine edit session");
	try {
		mark_clean();
	} catch (...) {
		windows_native_close_session();
		throw;
	}
}

json choices_json(const amiberry_setting_descriptor& descriptor, bool host)
{
	const amiberry_setting_choice* choices = nullptr;
	const size_t count = host ? amiberry_host_settings_choices(descriptor.id, &choices)
		: amiberry_settings_choices(active_session(), descriptor.id, &choices);
	json result = json::array();
	// Copy transient strings before making another main-thread layer call.
	for (size_t i = 0; choices && i < count; ++i)
		result.push_back({{"value", text(choices[i].value)}, {"label", text(choices[i].label)}, {"available", true}});
	if (!host) {
		for (auto& choice : result) {
			const auto& value = choice.at("value").get_ref<const std::string&>();
			choice["available"] = amiberry_settings_choice_is_available(session, descriptor.id, value.c_str());
		}
		if (descriptor.type == AMIBERRY_SETTING_BOOL) {
			for (const auto* value : {"false", "true"})
				result.push_back({{"value", value}, {"label", value},
					{"available", amiberry_settings_choice_is_available(session, descriptor.id, value)}});
		}
	}
	return result;
}

json snapshot()
{
	active_session();
	json values = json::array();
	bool changed = preset_applied;
	// The apply class comes from the differences remaining now, not from the strongest intermediate edit, so
	// restoring a baseline value returns a running machine's editor to a live Resume.
	int strongest = preset_applied ? AMIBERRY_APPLY_RESET : AMIBERRY_APPLY_LIVE;
	for (size_t i = 0; i < amiberry_settings_count(); ++i) {
		const auto* descriptor = amiberry_settings_at(i);
		if (!descriptor || !visible(*descriptor))
			continue;
		auto value = read_text([&](char* buffer, size_t size) {
			return amiberry_settings_get(session, descriptor->id, buffer, size);
		});
		if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
			check(value.status, descriptor->id);
		const bool available = value.status == AMIBERRY_SETTINGS_OK && amiberry_settings_is_available(session, descriptor->id);
		const auto baseline = baseline_values.find(descriptor->id);
		if (available && baseline != baseline_values.end() && baseline->second.readable && value.value != baseline->second.value) {
			changed = true;
			strongest = std::max(strongest, static_cast<int>(descriptor->apply));
		}
		double low = descriptor->min, high = descriptor->max;
		amiberry_settings_get_range(session, descriptor->id, &low, &high);
		values.push_back({{"id", descriptor->id}, {"value", std::move(value.value)}, {"available", available},
			{"min", low}, {"max", high}, {"choices", choices_json(*descriptor, false)}});
	}
	json mounted = json::array();
	const auto entries = mounts();
	for (const auto& entry : entries)
		mounted.push_back(mount_json(entry));
	if (entries != baseline_mounts) {
		changed = true;
		strongest = std::max(strongest, static_cast<int>(AMIBERRY_APPLY_RESET));
	}
	if ((boards_edited || preset_applied) && differs(board_baseline(true), baseline_boards)) {
		changed = true;
		strongest = AMIBERRY_APPLY_RESTART;
	}
	if (controls_edited && differs(control_baseline(true), baseline_controls))
		changed = true;
	std::vector<amiberry_settings_adjustment> adjustments(32);
	for (;;) {
		const auto count = amiberry_settings_validate(session, adjustments.data(), adjustments.size());
		if (count <= adjustments.size()) {
			adjustments.resize(count);
			break;
		}
		adjustments.resize(count);
	}
	json validation = json::array();
	for (const auto& adjustment : adjustments)
		validation.push_back({{"id", text(adjustment.id)}, {"requested", adjustment.requested},
			{"adjusted", adjustment.adjusted}, {"reason", text(adjustment.reason)}});
	return {{"values", std::move(values)}, {"mounts", std::move(mounted)},
		{"apply", strongest}, {"changed", changed}, {"adjustments", std::move(validation)},
		{"currentConfiguration", current_configuration()}};
}

json catalog()
{
	json options = json::array(), host = json::array(), presets = json::array();
	for (size_t i = 0; i < amiberry_settings_count(); ++i) {
		const auto* descriptor = amiberry_settings_at(i);
		if (descriptor && visible(*descriptor))
			options.push_back(descriptor_json(*descriptor));
	}
	for (size_t i = 0; i < amiberry_host_settings_count(); ++i) {
		const auto* descriptor = amiberry_host_settings_at(i);
		if (descriptor && visible(*descriptor))
			host.push_back(descriptor_json(*descriptor));
	}
	for (size_t i = 0; i < amiberry_settings_preset_count(); ++i) {
		const auto* preset = amiberry_settings_preset_at(i);
		if (!preset)
			continue;
		json configs = json::array(), labels = json::array();
		for (int config = 0; config < preset->config_count; ++config)
			configs.push_back({{"label", text(amiberry_settings_preset_config_label(preset, config))},
				{"romInstalled", amiberry_settings_preset_rom_installed(preset->id, config)}});
		for (int level = 0; level <= preset->compatibility_max; ++level)
			labels.push_back(text(amiberry_settings_preset_compatibility_label(level)));
		presets.push_back({{"id", preset->id}, {"label", text(preset->label)}, {"configs", std::move(configs)},
			{"compatibilityMax", preset->compatibility_max}, {"compatibilityLabels", std::move(labels)}});
	}
	return {{"options", std::move(options)}, {"hostOptions", std::move(host)}, {"presets", std::move(presets)}};
}

json media_list(amiberry_media_list list)
{
	json entries = json::array();
	const auto count = amiberry_media_refresh(list);
	for (size_t i = 0; i < count; ++i) {
		std::array<char, AMIBERRY_ACTIONS_PATH_MAX> path, name;
		check(amiberry_media_entry(list, i, path.data(), path.size(), name.data(), name.size()), "Reading media");
		json entry = {{"path", path.data()}, {"name", name.data()}};
		if (list == AMIBERRY_MEDIA_CONFIGURATIONS) {
			// One unreadable, locked or oversized configuration must not hide every other machine: it is listed
			// without metadata, and opening it reports its own error.
			std::array<char, 256> description{}, category{};
			if (amiberry_media_configuration_info(path.data(), description.data(), description.size(),
				category.data(), category.size()) != AMIBERRY_SETTINGS_OK) {
				description[0] = category[0] = '\0';
			}
			entry["description"] = description.data();
			entry["category"] = category.data();
		}
		entries.push_back(std::move(entry));
	}
	return entries;
}

json library()
{
	json folders = json::array();
	static const char* const labels[] = {"Kickstart ROMs", "Floppy disks", "CD images", "Hard drives",
		"Configurations", "Save states", "Screenshots", "WHDLoad archives"};
	for (int i = 0; i < AMIBERRY_FOLDER_COUNT; ++i) {
		const auto path = require_text("Reading folder", [i](char* buffer, size_t size) {
			return amiberry_folder_get(static_cast<amiberry_folder>(i), buffer, size);
		});
		folders.push_back({{"id", i}, {"label", labels[i]}, {"path", path}});
	}
	return {{"machines", media_list(AMIBERRY_MEDIA_CONFIGURATIONS)},
		{"recentFloppies", media_list(AMIBERRY_MEDIA_RECENT_FLOPPIES)},
		{"recentCds", media_list(AMIBERRY_MEDIA_RECENT_CDS)},
		{"recentWhdload", media_list(AMIBERRY_MEDIA_RECENT_WHDLOAD)},
		{"roms", media_list(AMIBERRY_MEDIA_KICKSTART_ROMS)},
		{"states", media_list(AMIBERRY_MEDIA_SAVESTATES)}, {"folders", std::move(folders)},
		{"currentConfiguration", current_configuration()}};
}

json host_values()
{
	amiberry_host_settings_refresh_choices();
	json values = json::array();
	for (size_t i = 0; i < amiberry_host_settings_count(); ++i) {
		const auto* descriptor = amiberry_host_settings_at(i);
		if (!descriptor || !visible(*descriptor))
			continue;
		auto value = read_text([&](char* buffer, size_t size) {
			return amiberry_host_settings_get(descriptor->id, buffer, size);
		});
		if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
			check(value.status, descriptor->id);
		values.push_back({{"id", descriptor->id}, {"value", std::move(value.value)},
			{"available", value.status == AMIBERRY_SETTINGS_OK}, {"min", descriptor->min}, {"max", descriptor->max},
			{"choices", choices_json(*descriptor, true)}});
	}
	return {{"values", std::move(values)}};
}

json boards()
{
	active_session();
	json result = json::array();
	for (size_t i = 0; i < amiberry_settings_board_type_count(); ++i) {
		const auto* type = amiberry_settings_board_type_at(i);
		if (!type)
			continue;
		json units = json::array();
		for (int unit = 0; unit < type->units; ++unit) {
			json fields = json::array();
			for (size_t field = 0; field < type->field_count; ++field) {
				const auto* descriptor = amiberry_settings_board_field_at(type, field);
				if (!descriptor)
					continue;
				auto value = read_text([&](char* buffer, size_t size) {
					return amiberry_settings_board_get(session, type->id, unit, descriptor->id, buffer, size);
				});
				if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
					check(value.status, descriptor->id);
				fields.push_back({{"descriptor", descriptor_json(*descriptor)}, {"value", std::move(value.value)},
					{"available", value.status == AMIBERRY_SETTINGS_OK && amiberry_settings_board_field_available(session, type->id, unit, descriptor->id)}});
			}
			units.push_back({{"unit", unit}, {"fields", std::move(fields)}});
		}
		result.push_back({{"id", type->id}, {"label", text(type->label)}, {"manufacturer", text(type->manufacturer)},
			{"categories", type->categories}, {"units", std::move(units)}});
	}
	return result;
}

json controls()
{
	active_session();
	json events = json::array(), ports = json::array();
	events.push_back({{"value", ""}, {"label", "Port default"}});
	events.push_back({{"value", "none"}, {"label", "None"}});
	for (size_t i = 0; i < amiberry_settings_input_event_count(); ++i) {
		const auto* event = amiberry_settings_input_event_at(i);
		if (event)
			events.push_back({{"value", text(event->value)}, {"label", text(event->label)}});
	}
	for (int port = 0; port < 4; ++port) {
		for (bool hotkey : {false, true}) {
			json entries = json::array();
			for (size_t i = 0; i < amiberry_settings_custom_control_count(); ++i) {
				const auto* control = amiberry_settings_custom_control_at(i);
				if (!control)
					continue;
				auto value = read_text([&](char* buffer, size_t size) {
					return amiberry_settings_custom_control_get(session, port, hotkey, control->id, buffer, size);
				});
				if (value.status != AMIBERRY_SETTINGS_OK && value.status != AMIBERRY_SETTINGS_UNAVAILABLE)
					check(value.status, control->id);
				const bool available = value.status == AMIBERRY_SETTINGS_OK && amiberry_settings_custom_control_available(session, port, hotkey, control->id);
				entries.push_back({{"id", control->id}, {"label", text(control->label)}, {"axis", control->axis},
					{"value", std::move(value.value)}, {"available", available},
					{"use", text(amiberry_settings_custom_control_use(session, port, hotkey, control->id))}});
			}
			ports.push_back({{"port", port}, {"hotkey", hotkey}, {"controls", std::move(entries)}});
		}
	}
	return {{"events", std::move(events)}, {"ports", std::move(ports)}};
}

json mount_controllers(const std::string& kind)
{
	active_session();
	json result = json::array();
	const auto count = amiberry_settings_mount_controller_count(session, kind.c_str());
	for (size_t index = 0; index < count; ++index) {
		std::array<char, AMIBERRY_ACTIONS_PATH_MAX> id{}, label{};
		check(amiberry_settings_mount_controller_get(session, kind.c_str(), index,
			id.data(), id.size(), label.data(), label.size()), "Reading mount controller");
		json units = json::array();
		const auto unit_count = amiberry_settings_mount_controller_unit_count(session, kind.c_str(), index);
		for (size_t unit = 0; unit < unit_count; ++unit) {
			int value = 0;
			const auto name = require_text("Reading controller unit", [&](char* buffer, size_t size) {
				return amiberry_settings_mount_controller_unit_get(session, kind.c_str(), index, unit, &value, buffer, size);
			});
			units.push_back({{"value", value}, {"label", name}});
		}
		result.push_back({{"id", id.data()}, {"label", label.data()}, {"units", std::move(units)}});
	}
	return result;
}

amiberry_action_id accepted(amiberry_action_id id)
{
	if (!id)
		throw std::runtime_error("Action rejected: invalid arguments or the action queue is full");
	return id;
}

void perform(amiberry_action_id id)
{
	accepted(id);
	amiberry_actions_run_pending();
	std::array<char, AMIBERRY_ACTIONS_PATH_MAX> error{};
	const auto state = amiberry_actions_poll(id, error.data(), error.size());
	if (state != AMIBERRY_ACTION_DONE)
		throw std::runtime_error(error[0] ? error.data() : "Action did not complete successfully");
}

void apply_preset(const json& request, const char* key)
{
	const auto id = argument(request, key);
	const auto* preset = amiberry_settings_preset_find(id.c_str());
	if (!preset)
		throw std::runtime_error("Unknown machine preset: " + id);
	check(amiberry_settings_preset_apply(active_session(), id.c_str(), integer(request, "config"),
		integer(request, "compatibility", std::min(1, preset->compatibility_max))), "Applying preset " + id);
	preset_applied = true;
}

json commit_session()
{
	active_session();
	bool changed = false;
	const auto apply = amiberry_settings_commit(session, &changed);
	// The layer resolves and merges the actual pending configuration; its
	// result, not an estimate from UI fields, determines runtime application.
	mark_clean();
	return {{"apply", apply}, {"changed", changed}};
}

void require_download_idle();

amiberry_action_id queue_action(const json& request)
{
	const auto name = argument(request, "name");
	// Downloads install boot media and use the core's archive decoders.
	// Do not load another machine or let one restart until installation ends.
	if (name == "quick_play" || name == "load_config" || name == "apply" ||
		name == "resume" || name == "reset" || name == "hard_reset" ||
		name == "load_state" || name == "load_slot")
		require_download_idle();
	if (name == "pause") return accepted(amiberry_actions_pause());
	if (name == "resume") return accepted(amiberry_actions_resume());
	if (name == "reset") return accepted(amiberry_actions_reset(false, true));
	if (name == "hard_reset") return accepted(amiberry_actions_reset(true, false));
	if (name == "stop") return accepted(amiberry_actions_stop());
	if (name == "quit") return accepted(amiberry_actions_quit());
	if (name == "fullscreen") return accepted(amiberry_actions_toggle_fullscreen());
	if (name == "eject_floppy") return accepted(amiberry_actions_eject_floppy(integer(request, "drive")));
	if (name == "eject_cd") return accepted(amiberry_actions_eject_cd());
	if (name == "save_slot") return accepted(amiberry_actions_save_state_slot(integer(request, "index")));
	if (name == "load_slot") return accepted(amiberry_actions_load_state_slot(integer(request, "index")));
	if (name == "settings") return accepted(amiberry_actions_open_settings());
	if (name == "key_down" || name == "key_up") return accepted(amiberry_actions_key(integer(request, "index"), name == "key_down"));
	if (name == "apply") {
		const int apply = integer(request, "apply");
		if (apply < AMIBERRY_APPLY_LIVE || apply > AMIBERRY_APPLY_RESTART)
			throw std::runtime_error("Invalid settings apply class");
		return accepted(amiberry_actions_apply_settings(static_cast<amiberry_apply_class>(apply)));
	}
	const auto path = optional_argument(request, "path");
	if (name == "insert_floppy") return accepted(amiberry_actions_insert_floppy(integer(request, "drive"), path.c_str()));
	if (name == "insert_cd") return accepted(amiberry_actions_insert_cd(path.c_str()));
	if (name == "save_state") return accepted(amiberry_actions_save_state(path.c_str()));
	if (name == "load_state") return accepted(amiberry_actions_load_state(path.c_str()));
	if (name == "screenshot") return accepted(amiberry_actions_screenshot(path.c_str()));
	if (name == "load_config") return accepted(amiberry_actions_load_config(path.c_str()));
	if (name == "quick_play") return accepted(amiberry_actions_quick_play(path.c_str()));
	throw std::runtime_error("Unknown action: " + name);
}

struct DownloadState {
	std::mutex mutex;
	std::atomic<bool> cancel{false};
	std::thread worker;
	bool running = false;
	int step = 0;
	int total_steps = 0;
	std::string label;
	std::int64_t downloaded = 0;
	std::int64_t total_bytes = 0;
	amiberry_whdboot_download_result result = AMIBERRY_WHDBOOT_DOWNLOAD_OK;
};

DownloadState download;

void require_download_idle()
{
	std::lock_guard<std::mutex> lock(download.mutex);
	if (download.running)
		throw std::runtime_error("Wait for the WHDBoot download to finish, or cancel it first");
}

int download_progress(const amiberry_whdboot_progress* progress, void*)
{
	try {
		if (progress) {
			std::lock_guard<std::mutex> lock(download.mutex);
			download.step = progress->step;
			download.total_steps = progress->total_steps;
			download.label = text(progress->label);
			download.downloaded = progress->bytes_downloaded;
			download.total_bytes = progress->bytes_total;
		}
	} catch (...) {
		// No exception may escape the C callback into the downloader.
		download.cancel.store(true);
	}
	return download.cancel.load() ? 1 : 0;
}

json whdboot_status()
{
	amiberry_whdboot_status status{};
	amiberry_whdboot_status_get(&status);
	std::lock_guard<std::mutex> lock(download.mutex);
	return {{"whdload", status.whdload_installed}, {"jst", status.jst_installed},
		{"rtb", status.rtb_files_installed}, {"hostQuit", status.host_quit_installed},
		{"bootData", status.boot_data_installed}, {"database", status.database_installed},
		{"libraries", status.libraries_installed}, {"downloadsAvailable", status.downloads_available},
		{"emulationRunning", status.emulation_running}, {"downloading", download.running},
		{"step", download.step}, {"totalSteps", download.total_steps}, {"label", download.label},
		{"downloaded", std::max<std::int64_t>(0, download.downloaded)},
		{"totalBytes", std::max<std::int64_t>(0, download.total_bytes)}, {"result", download.result}};
}

json start_whdboot_download()
{
	{
		std::lock_guard<std::mutex> lock(download.mutex);
		if (download.running)
			throw std::runtime_error("A WHDBoot download is already running");
	}
	if (download.worker.joinable())
		download.worker.join();
	amiberry_whdboot_status status{};
	amiberry_whdboot_status_get(&status);
	if (!status.downloads_available)
		throw std::runtime_error("WHDBoot downloads are unavailable");
	if (status.emulation_running)
		throw std::runtime_error("Pause or stop emulation before downloading WHDBoot tools");
	download.cancel.store(false);
	{
		std::lock_guard<std::mutex> lock(download.mutex);
		download.running = true;
		download.step = download.total_steps = 0;
		download.downloaded = download.total_bytes = 0;
		download.label.clear();
		download.result = AMIBERRY_WHDBOOT_DOWNLOAD_OK;
	}
	try {
		download.worker = std::thread([] {
			auto result = AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
			try {
				result = amiberry_whdboot_download(download_progress, nullptr);
			} catch (...) {
				result = AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
			}
			std::lock_guard<std::mutex> lock(download.mutex);
			download.result = result;
			download.running = false;
		});
	} catch (...) {
		std::lock_guard<std::mutex> lock(download.mutex);
		download.running = false;
		download.result = AMIBERRY_WHDBOOT_DOWNLOAD_FAILED;
		throw std::runtime_error("Could not start the WHDBoot download worker");
	}
	return whdboot_status();
}

json dispatch(const json& request)
{
	const auto op = argument(request, "op");
	if (op == "catalog") return catalog();
	if (op == "library") return library();
	if (op == "whdboot_status") return whdboot_status();
	if (op == "whdboot_download") return start_whdboot_download();
	if (op == "whdboot_cancel") { download.cancel.store(true); return whdboot_status(); }
	if (op == "open") {
		if (request.contains("path"))
			require_download_idle();
		windows_native_close_session();
		if (request.contains("path"))
			perform(amiberry_actions_load_config(argument(request, "path").c_str()));
		open_session();
		if (request.contains("preset")) {
			apply_preset(request, "preset");
			amiberry_media_forget_current_configuration();
		}
		amiberry_settings_refresh_choices();
		return snapshot();
	}
	if (op == "snapshot") {
		amiberry_settings_refresh_choices();
		return snapshot();
	}
	if (op == "set") {
		const auto id = argument(request, "id"), value = argument(request, "value");
		const auto* descriptor = amiberry_settings_find(id.c_str());
		if (!descriptor || !visible(*descriptor))
			throw std::runtime_error("Unknown or hidden setting: " + id);
		check(amiberry_settings_set(active_session(), id.c_str(), value.c_str()), "Setting " + id);
		return snapshot();
	}
	if (op == "preset") { apply_preset(request, "id"); return snapshot(); }
	if (op == "revert") { active_session(); open_session(); return snapshot(); }
	if (op == "commit") return commit_session();
	if (op == "save") {
		const auto path = argument(request, "path");
		auto result = commit_session();
		perform(amiberry_actions_save_config(path.c_str()));
		return result;
	}
	if (op == "close") { windows_native_close_session(); return nullptr; }
	if (op == "host") return host_values();
	if (op == "host_set") {
		const auto id = argument(request, "id"), value = argument(request, "value");
		const auto* descriptor = amiberry_host_settings_find(id.c_str());
		if (!descriptor || !visible(*descriptor))
			throw std::runtime_error("Unknown or hidden host setting: " + id);
		// The worker reads these globals throughout installation.
		if (id == "whdboot_path" || id == "use_jst_instead_of_whd" || id == "base_content_path")
			require_download_idle();
		check(amiberry_host_settings_set(id.c_str(), value.c_str()), "Setting " + id);
		if (!amiberry_host_settings_save())
			throw std::runtime_error("Could not save host settings");
		return host_values();
	}
	if (op == "configuration_rename") {
		const auto path = argument(request, "path"), new_path = argument(request, "newPath");
		check(amiberry_media_rename_configuration(path.c_str(), new_path.c_str()), "Renaming configuration");
		return library();
	}
	if (op == "folder_set") {
		const int id = integer(request, "id", -1);
		check(amiberry_folder_set(static_cast<amiberry_folder>(id), argument(request, "path").c_str()), "Setting folder");
		return library();
	}
	if (op == "rescan_roms") { amiberry_media_rescan_roms(); return library(); }
	if (op == "rom_key") {
		amiberry_media_add_rom_key(argument(request, "path").c_str());
		amiberry_media_rescan_roms();
		return library();
	}
	if (op == "recent_add") {
		const int list = integer(request, "list", -1);
		if (list < 0 || list > 2) throw std::runtime_error("Invalid recent media list");
		check(amiberry_media_add_recent(static_cast<amiberry_media_list>(list), argument(request, "path").c_str()), "Recording recent media");
		return library();
	}
	if (op == "recent_remove") {
		const int list = integer(request, "list", -1);
		if (list < 0 || list > 2) throw std::runtime_error("Invalid recent media list");
		check(amiberry_media_remove_recent(static_cast<amiberry_media_list>(list), argument(request, "path").c_str()), "Removing recent media");
		return library();
	}
	if (op == "hardfile_create") {
		const auto path = argument(request, "path");
		const auto size_bytes = unsigned_integer(request, "sizeBytes");
		const bool dynamic = request.at("dynamic").get<bool>();
		// Do not retry a side-effecting creation when reading its output.
		std::array<char, AMIBERRY_ACTIONS_PATH_MAX> created{};
		check(amiberry_media_create_hardfile(path.c_str(), size_bytes, dynamic, created.data(), created.size()), "Creating hardfile");
		return {{"path", created.data()}};
	}
	if (op == "mount_default") {
		const auto kind = argument(request, "kind"), path = optional_argument(request, "path");
		const auto entry = require_text("Constructing mount", [&](char* buffer, size_t size) {
			return amiberry_settings_mount_default(active_session(), kind.c_str(), path.c_str(), buffer, size);
		});
		return mount_json(entry);
	}
	if (op == "mount_field") {
		const auto entry = argument(request, "entry"), field = argument(request, "field"), value = argument(request, "value");
		const auto changed = require_text("Setting mount field", [&](char* buffer, size_t size) {
			return amiberry_settings_mount_field_set(active_session(), entry.c_str(), field.c_str(), value.c_str(), buffer, size);
		});
		return mount_json(changed);
	}
	if (op == "mount_controllers") return mount_controllers(argument(request, "kind"));
	if (op == "mount_add") {
		check(amiberry_settings_mount_add(active_session(), argument(request, "entry").c_str()), "Adding mount");
		return snapshot();
	}
	if (op == "mount_replace") {
		check(amiberry_settings_mount_replace(active_session(), mount_index(request), argument(request, "entry").c_str()), "Replacing mount");
		return snapshot();
	}
	if (op == "mount_remove") {
		check(amiberry_settings_mount_remove(active_session(), mount_index(request)), "Removing mount");
		return snapshot();
	}
	if (op == "boards") return boards();
	if (op == "board_set") {
		const auto type = argument(request, "type"), field = argument(request, "field"), value = argument(request, "value");
		check(amiberry_settings_board_set(active_session(), type.c_str(), integer(request, "unit"), field.c_str(), value.c_str()), "Setting board field");
		boards_edited = true;
		return boards();
	}
	if (op == "controls") return controls();
	if (op == "control_set") {
		const auto control = argument(request, "control"), event = argument(request, "event");
		check(amiberry_settings_custom_control_set(active_session(), integer(request, "port"), request.at("hotkey").get<bool>(), control.c_str(), event.c_str()), "Setting custom control");
		controls_edited = true;
		return controls();
	}
	if (op == "hotkey_format") {
		std::array<char, AMIBERRY_SETTINGS_VALUE_MAX> value{};
		if (!amiberry_settings_hotkey_format(unsigned_argument(request, "usage"), unsigned_argument(request, "modifiers"), value.data(), value.size()))
			throw std::runtime_error("This key combination cannot be used as a hotkey");
		return {{"value", value.data()}};
	}
	if (op == "action") return {{"id", queue_action(request)}};
	if (op == "poll") {
		std::array<char, AMIBERRY_ACTIONS_PATH_MAX> error{};
		const auto result = amiberry_actions_poll(unsigned_integer(request, "id"), error.data(), error.size());
		const char* state = "unknown";
		switch (result) {
		case AMIBERRY_ACTION_PENDING: state = "pending"; break;
		case AMIBERRY_ACTION_DONE: state = "done"; break;
		case AMIBERRY_ACTION_FAILED: state = "failed"; break;
		case AMIBERRY_ACTION_UNKNOWN: break;
		}
		return {{"state", state}, {"error", error.data()}};
	}
	if (op == "status") {
		amiberry_actions_status status{};
		amiberry_actions_get_status(&status);
		const char* state = status.run_state == AMIBERRY_RUN_STATE_RUNNING ? "running"
			: status.run_state == AMIBERRY_RUN_STATE_PAUSED ? "paused" : "stopped";
		json floppy = json::array();
		for (int drive = 0; drive < AMIBERRY_ACTIONS_FLOPPY_DRIVES; ++drive) {
			const auto& disk = status.floppy[drive];
			floppy.push_back({{"drive", drive}, {"path", disk.media_path}, {"disabled", disk.disabled},
				{"motor", disk.motor_on}, {"writing", disk.writing}});
		}
		return {{"state", state}, {"paused", status.paused}, {"power", status.power_led}, {"capsLock", status.caps_lock},
			{"fps", status.fps_tenths / 10.0}, {"description", status.config_description},
			{"floppy", std::move(floppy)}, {"cdPath", status.cd_path}, {"settingsOpen", native_host_settings_open()}};
	}
	throw std::runtime_error("Unknown native UI operation: " + op);
}
} // namespace

nlohmann::json windows_native_dispatch(const nlohmann::json& request)
{
	try {
		auto result = dispatch(request);
		reply_text(result);
		return result;
	} catch (const json::exception& error) {
		throw std::runtime_error(std::string("Invalid native UI request: ") + error.what());
	}
}

std::string windows_native_core_to_utf8(const std::string& text)
{
	if (GetACP() == CP_UTF8 || amiberry_valid_utf8(text))
		return text;
	const int count = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
	if (count <= 0)
		return text;
	std::wstring wide(static_cast<size_t>(count), L'\0');
	MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count);
	const int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, nullptr, 0, nullptr, nullptr);
	if (length <= 0)
		return text;
	std::string utf8(static_cast<size_t>(length), '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, utf8.data(), length, nullptr, nullptr);
	return utf8;
}

void windows_native_close_session()
{
	if (session) {
		amiberry_settings_session_close(session);
		session = nullptr;
	}
	baseline_values.clear();
	baseline_mounts.clear();
	baseline_boards.clear();
	baseline_controls.clear();
	preset_applied = boards_edited = controls_edited = false;
}

void windows_native_shutdown_bridge()
{
	download.cancel.store(true);
	if (download.worker.joinable())
		download.worker.join();
	windows_native_close_session();
}
