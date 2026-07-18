#include "rp9_manifest.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <filesystem>
#include <limits>
#include <string_view>

#include "tinyxml2.h"

namespace rp9
{
namespace
{
constexpr Version supported_player_version { 10, 0, 3, 0 };

std::string trim(std::string value)
{
	const auto not_space = [](const unsigned char ch) { return !std::isspace(ch); };
	value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
	value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
	return value;
}

std::string lowercase(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

std::string_view local_name(const char* name)
{
	if (!name)
		return {};
	const std::string_view view(name);
	const auto colon = view.find_last_of(':');
	return colon == std::string_view::npos ? view : view.substr(colon + 1);
}

bool has_name(const tinyxml2::XMLElement* element, const std::string_view expected)
{
	return element && local_name(element->Name()) == expected;
}

const tinyxml2::XMLElement* child(const tinyxml2::XMLElement* parent, const std::string_view name)
{
	if (!parent)
		return nullptr;
	for (auto element = parent->FirstChildElement(); element; element = element->NextSiblingElement()) {
		if (has_name(element, name))
			return element;
	}
	return nullptr;
}

std::string text(const tinyxml2::XMLElement* element)
{
	return element && element->GetText() ? trim(element->GetText()) : std::string {};
}

std::string attribute(const tinyxml2::XMLElement* element, const char* name)
{
	if (!element)
		return {};
	const auto value = element->Attribute(name);
	return value ? trim(value) : std::string {};
}

template<typename T>
bool parse_unsigned(const std::string& value, T& result)
{
	static_assert(std::is_unsigned_v<T>);
	if (value.empty())
		return false;
	T parsed = 0;
	const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
	if (error != std::errc {} || end != value.data() + value.size())
		return false;
	result = parsed;
	return true;
}

bool parse_bool(const std::string& value, bool& result)
{
	const auto normalized = lowercase(trim(value));
	if (normalized == "true" || normalized == "1" || normalized == "yes") {
		result = true;
		return true;
	}
	if (normalized == "false" || normalized == "0" || normalized == "no") {
		result = false;
		return true;
	}
	return false;
}

bool parse_optional_bool(const tinyxml2::XMLElement* element, const char* name, bool& value,
	std::string& error)
{
	const auto raw = attribute(element, name);
	if (raw.empty())
		return true;
	if (!parse_bool(raw, value)) {
		error = std::string("Invalid boolean value for ") + name + ": " + raw;
		return false;
	}
	return true;
}

bool parse_required_clip_value(const tinyxml2::XMLElement* element, const char* name,
	unsigned int& result, std::string& error)
{
	const auto raw = attribute(element, name);
	if (!parse_unsigned(raw, result)) {
		error = std::string("RP9 clip has an invalid or missing '") + name + "' value";
		return false;
	}
	return true;
}

bool version_greater(const Version& left, const Version& right)
{
	const std::array<unsigned int, 4> lhs { left.major, left.minor, left.patch, left.build };
	const std::array<unsigned int, 4> rhs { right.major, right.minor, right.patch, right.build };
	return lhs > rhs;
}

bool is_portable_package_component(const std::string_view component)
{
	if (component.empty())
		return true;
	if (component == ".")
		return true;
	if (component == ".." || component.size() > 255
		|| component.back() == '.' || component.back() == ' ')
		return false;
	for (const unsigned char ch : component) {
		if (ch < 0x20 || ch == ':')
			return false;
	}

	auto device_name = lowercase(std::string(component.substr(0, component.find('.'))));
	if (device_name == "con" || device_name == "prn" || device_name == "aux" || device_name == "nul"
		|| device_name == "conin$" || device_name == "conout$")
		return false;
	return !(device_name.size() == 4
		&& (device_name.rfind("com", 0) == 0 || device_name.rfind("lpt", 0) == 0)
		&& device_name[3] >= '1' && device_name[3] <= '9');
}

bool parse_configuration(const tinyxml2::XMLElement* configuration, Manifest& manifest, std::string& error)
{
	for (auto element = configuration->FirstChildElement(); element; element = element->NextSiblingElement()) {
		const auto name = local_name(element->Name());
		if (name == "system") {
			manifest.system = lowercase(text(element));
		} else if (name == "video") {
			manifest.video = lowercase(text(element));
		} else if (name == "rom") {
			if (lowercase(attribute(element, "type")) == "system")
				manifest.system_rom = text(element);
		} else if (name == "ram") {
			Ram ram;
			ram.type = lowercase(attribute(element, "type"));
			const auto raw_size = text(element);
			if (!parse_unsigned(raw_size, ram.size)) {
				error = "Invalid RP9 RAM size: " + raw_size;
				return false;
			}
			manifest.ram.emplace_back(std::move(ram));
		} else if (name == "boot") {
			manifest.has_boot = true;
			manifest.boot.type = lowercase(attribute(element, "type"));
			manifest.boot.value = text(element);
			if (!parse_optional_bool(element, "readonly", manifest.boot.readonly, error))
				return false;
		} else if (name == "peripheral") {
			Peripheral peripheral;
			peripheral.name = lowercase(text(element));
			peripheral.type = lowercase(attribute(element, "type"));
			peripheral.speed = lowercase(attribute(element, "speed"));
			peripheral.layout = lowercase(attribute(element, "layout"));
			peripheral.path = attribute(element, "path");
			peripheral.device = attribute(element, "device");
			peripheral.device_name = attribute(element, "devicename");
			peripheral.volume_name = attribute(element, "volumename");

			const auto unit = attribute(element, "unit");
			if (!unit.empty()) {
				unsigned int parsed = 0;
				if (!parse_unsigned(unit, parsed) || parsed > static_cast<unsigned int>(std::numeric_limits<int>::max())) {
					error = "Invalid RP9 peripheral unit: " + unit;
					return false;
				}
				peripheral.unit = static_cast<int>(parsed);
			}

			const auto memory = attribute(element, "memory");
			if (!memory.empty()) {
				if (!parse_unsigned(memory, peripheral.memory)) {
					error = "Invalid RP9 peripheral memory size: " + memory;
					return false;
				}
				peripheral.has_memory = true;
			}

			const auto fpu = attribute(element, "fpu");
			if (!fpu.empty()) {
				if (!parse_bool(fpu, peripheral.fpu)) {
					error = "Invalid RP9 peripheral fpu value: " + fpu;
					return false;
				}
				peripheral.has_fpu = true;
			}
			manifest.peripherals.emplace_back(std::move(peripheral));
		} else if (name == "compatibility") {
			const auto value = lowercase(text(element));
			if (!value.empty())
				manifest.compatibility.emplace_back(value);
		} else if (name == "clip") {
			manifest.has_clip = true;
			if (!parse_required_clip_value(element, "left", manifest.clip.left, error)
				|| !parse_required_clip_value(element, "top", manifest.clip.top, error)
				|| !parse_required_clip_value(element, "width", manifest.clip.width, error)
				|| !parse_required_clip_value(element, "height", manifest.clip.height, error))
				return false;
			if (manifest.clip.width == 0 || manifest.clip.height == 0) {
				error = "RP9 clip width and height must be greater than zero";
				return false;
			}
		} else {
			manifest.warnings.emplace_back("Unsupported RP9 configuration element: " + std::string(name));
		}
	}

	if (manifest.system.empty()) {
		error = "RP9 manifest is missing application/configuration/system";
		return false;
	}
	return true;
}

bool parse_media(const tinyxml2::XMLElement* media_element, Manifest& manifest, std::string& error)
{
	if (!media_element)
		return true;

	for (auto element = media_element->FirstChildElement(); element; element = element->NextSiblingElement()) {
		const auto name = local_name(element->Name());
		Media media;
		if (name == "floppy")
			media.type = MediaType::Floppy;
		else if (name == "harddrive")
			media.type = MediaType::HardDrive;
		else if (name == "cd")
			media.type = MediaType::Cd;
		else if (name == "tape")
			media.type = MediaType::Tape;
		else if (name == "snapshot")
			media.type = MediaType::Snapshot;
		else {
			manifest.warnings.emplace_back("Unsupported RP9 media element: " + std::string(name));
			continue;
		}

		media.root = lowercase(attribute(element, "root"));
		media.path = trim(text(element));
		if (media.path.empty() && media.root != "shared") {
			error = "RP9 media entry has an empty path";
			return false;
		}
		media.format = lowercase(attribute(element, "type"));
		media.volume_name = attribute(element, "volumename");
		const auto priority = attribute(element, "priority");
		if (!priority.empty()) {
			if (!parse_unsigned(priority, media.priority)) {
				error = "Invalid RP9 media priority: " + priority;
				return false;
			}
			media.has_priority = true;
		}
		if (!parse_optional_bool(element, "readonly", media.readonly, error))
			return false;
		const auto undo = attribute(element, "undo");
		if (!undo.empty()) {
			if (!parse_bool(undo, media.undo)) {
				error = "Invalid RP9 media undo value: " + undo;
				return false;
			}
			media.has_undo = true;
		}
		if (!parse_optional_bool(element, "deploy", media.deploy, error))
			return false;
		manifest.media.emplace_back(std::move(media));
	}
	return true;
}
}

bool parse_version(const std::string& value, Version& version)
{
	std::array<unsigned int, 4> parts {};
	std::size_t start = 0;
	for (std::size_t index = 0; index < parts.size(); ++index) {
		const auto end = value.find('.', start);
		const auto part = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
		if (!parse_unsigned(part, parts[index]))
			return false;
		if (index + 1 < parts.size()) {
			if (end == std::string::npos)
				return false;
			start = end + 1;
		} else if (end != std::string::npos) {
			return false;
		}
	}
	version = { parts[0], parts[1], parts[2], parts[3] };
	return true;
}

bool is_supported_version(const Version& version)
{
	return !version_greater(version, supported_player_version);
}

bool normalize_package_path(const std::string& original, std::string& normalized)
{
	normalized.clear();
	if (original.empty() || original.size() > 4096)
		return false;
	std::string portable = original;
	std::replace(portable.begin(), portable.end(), '\\', '/');
	if (portable.front() == '/')
		return false;
	if (portable.size() >= 2 && std::isalpha(static_cast<unsigned char>(portable[0])) && portable[1] == ':')
		return false;
	std::size_t component_start = 0;
	for (;;) {
		const auto separator = portable.find('/', component_start);
		const auto component = std::string_view(portable).substr(component_start,
			separator == std::string::npos ? std::string::npos : separator - component_start);
		if (!is_portable_package_component(component))
			return false;
		if (separator == std::string::npos)
			break;
		component_start = separator + 1;
	}

	std::filesystem::path path(portable);
	path = path.lexically_normal();
	if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory())
		return false;
	for (const auto& part : path) {
		if (part == "..")
			return false;
	}
	normalized = path.generic_string();
	return normalized != "." && !normalized.empty();
}

bool parse_manifest(const char* xml, const std::size_t size, Manifest& manifest, std::string& error)
{
	manifest = {};
	error.clear();
	if (!xml || size == 0) {
		error = "RP9 manifest is empty";
		return false;
	}
	if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
		error = "RP9 manifest is too large";
		return false;
	}

	tinyxml2::XMLDocument document;
	const auto result = document.Parse(xml, size);
	if (result != tinyxml2::XML_SUCCESS) {
		error = "Invalid RP9 manifest XML";
		if (document.ErrorStr())
			error += std::string(": ") + document.ErrorStr();
		return false;
	}

	const auto root = document.RootElement();
	if (!has_name(root, "rp9")) {
		error = "RP9 manifest root element must be 'rp9'";
		return false;
	}

	const auto requirements = child(root, "requirements");
	manifest.host = text(child(requirements, "host"));
	const auto version_text = text(child(requirements, "playerversion"));
	if (manifest.host.empty() || version_text.empty()) {
		error = "RP9 manifest is missing requirements/host or requirements/playerversion";
		return false;
	}
	if (!parse_version(version_text, manifest.player_version)) {
		error = "Invalid RP9 player version: " + version_text;
		return false;
	}
	if (!is_supported_version(manifest.player_version)) {
		error = "RP9 requires a newer player version than Amiberry supports: " + version_text;
		return false;
	}

	const auto application = child(root, "application");
	if (!application) {
		error = "RP9 manifest is missing application";
		return false;
	}
	const auto description = child(application, "description");
	manifest.title = text(child(description, "title"));

	const auto configuration = child(application, "configuration");
	if (!configuration) {
		error = "RP9 manifest is missing application/configuration";
		return false;
	}
	if (!parse_configuration(configuration, manifest, error))
		return false;
	return parse_media(child(application, "media"), manifest, error);
}
}
