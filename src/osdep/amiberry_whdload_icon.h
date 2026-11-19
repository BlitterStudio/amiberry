#ifndef AMIBERRY_WHDLOAD_ICON_H
#define AMIBERRY_WHDLOAD_ICON_H

// WHDLoad options from a game's Workbench icon. WHDLoad packs keep some of
// the options a slave needs only in the icon's tool types: Battle Isle's
// slave picks the program to run from Custom1 and fails with "data/disk-files
// are damaged" without it, but declares only Custom2 in its configuration.
// WHDBooter writes its own WHDLoad command line, so it reads them here.

#include <array>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

struct WhdloadIconArgs {
	std::string slave; // SLAVE tool type, empty when the icon has none
	std::array<std::optional<int>, 5> custom; // CUSTOM1..CUSTOM5
	std::optional<std::string> custom_text; // CUSTOM

	bool operator==(const WhdloadIconArgs& other) const
	{
		return custom == other.custom && custom_text == other.custom_text;
	}
};

struct WhdloadIcon {
	std::string name; // file name without ".info"
	WhdloadIconArgs args;
};

namespace whdload_icon_detail {

inline uint32_t be16(const uint8_t* p)
{
	return static_cast<uint32_t>(p[0]) << 8 | p[1];
}

inline uint32_t be32(const uint8_t* p)
{
	return static_cast<uint32_t>(p[0]) << 24 | static_cast<uint32_t>(p[1]) << 16
		| static_cast<uint32_t>(p[2]) << 8 | p[3];
}

inline bool iequals(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++) {
		if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
			return false;
	}
	return true;
}

// The part after the last ':' or '/' of an Amiga path.
inline std::string amiga_file_part(const std::string& path)
{
	const auto pos = path.find_last_of(":/");
	return pos == std::string::npos ? path : path.substr(pos + 1);
}

inline std::optional<int> parse_number(const std::string& text)
{
	if (text.empty())
		return std::nullopt;
	char* end = nullptr;
	errno = 0;
	const long long value = std::strtoll(text.c_str(), &end, 10);
	if (errno != 0 || *end != '\0')
		return std::nullopt;
	return static_cast<int>(value);
}

// The slave an icon starts: its SLAVE tool type, else WHDLoad's default of
// the icon name with ".slave" appended.
inline std::string icon_slave(const WhdloadIcon& icon)
{
	return icon.args.slave.empty() ? icon.name + ".slave" : amiga_file_part(icon.args.slave);
}

} // namespace whdload_icon_detail

// Reads the tool types of a classic Workbench icon (DiskObject). NewIcons and
// OS 3.5 icons keep this part, so their tool types are found too. Returns
// false when the data is not an icon or is cut short.
inline bool whdload_icon_tooltypes(const uint8_t* data, const size_t size, std::vector<std::string>& tooltypes)
{
	using whdload_icon_detail::be16;
	using whdload_icon_detail::be32;

	tooltypes.clear();
	constexpr size_t disk_object_size = 78;
	constexpr size_t drawer_data_size = 56;
	constexpr size_t image_size = 20;
	if (size < disk_object_size || be16(data) != 0xe310)
		return false;

	const bool has_render = be32(data + 22) != 0;
	const bool has_select_render = be32(data + 26) != 0;
	const bool has_default_tool = be32(data + 50) != 0;
	const bool has_tooltypes = be32(data + 54) != 0;
	const bool has_drawer_data = be32(data + 66) != 0;

	size_t pos = disk_object_size;
	if (has_drawer_data)
		pos += drawer_data_size;

	// An Image structure, followed by its planes.
	const auto skip_image = [&]() {
		if (pos > size || size - pos < image_size)
			return false;
		const size_t width = be16(data + pos + 4);
		const size_t height = be16(data + pos + 6);
		const size_t depth = be16(data + pos + 8);
		pos += image_size + (width + 15) / 16 * 2 * height * depth;
		return pos <= size;
	};
	// A string: its length including the terminating NUL, then the bytes.
	const auto read_string = [&](std::string* out) {
		if (pos > size || size - pos < 4)
			return false;
		const size_t length = be32(data + pos);
		pos += 4;
		if (length > size - pos)
			return false;
		if (out) {
			const auto* text = reinterpret_cast<const char*>(data + pos);
			const void* nul = std::memchr(text, 0, length);
			out->assign(text, nul ? static_cast<const char*>(nul) - text : length);
		}
		pos += length;
		return true;
	};

	if (has_render && !skip_image())
		return false;
	if (has_select_render && !skip_image())
		return false;
	if (has_default_tool && !read_string(nullptr))
		return false;
	if (!has_tooltypes)
		return true;

	// The size of the pointer array, including its NULL terminator.
	if (pos > size || size - pos < 4)
		return false;
	const size_t count = be32(data + pos) / 4;
	pos += 4;
	if (count == 0)
		return false;
	for (size_t i = 0; i + 1 < count; i++) {
		std::string tooltype;
		if (!read_string(&tooltype))
			return false;
		tooltypes.push_back(tooltype);
	}
	return true;
}

// The WHDLoad options among an icon's tool types. Disabled tool types, in
// parentheses, are skipped.
inline WhdloadIconArgs whdload_icon_args(const std::vector<std::string>& tooltypes)
{
	using whdload_icon_detail::iequals;

	WhdloadIconArgs args;
	for (const auto& tooltype : tooltypes) {
		if (tooltype.empty() || tooltype[0] == '(')
			continue;
		const auto equals = tooltype.find('=');
		if (equals == std::string::npos)
			continue;
		const std::string key = tooltype.substr(0, equals);
		std::string value = tooltype.substr(equals + 1);

		if (iequals(key, "SLAVE")) {
			args.slave = value;
		} else if (iequals(key, "CUSTOM")) {
			if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
				value = value.substr(1, value.size() - 2);
			args.custom_text = value;
		} else if (key.size() == 7 && iequals(key.substr(0, 6), "CUSTOM") && key[6] >= '1' && key[6] <= '5') {
			args.custom[key[6] - '1'] = whdload_icon_detail::parse_number(value);
		}
	}
	return args;
}

// The icon that starts `slave` from the game drawer `drawer` (its last path
// component), or nullptr when no icon or more than one plausible icon does.
// Several icons often start the same slave with different options (Intro,
// the game, each data disk); the game's own icon is the one named after the
// drawer, or after its leading part as in "BattleIsle&DataDisks".
inline const WhdloadIcon* whdload_pick_icon(const std::vector<WhdloadIcon>& icons,
	const std::string& drawer, const std::string& slave)
{
	using whdload_icon_detail::iequals;

	std::vector<const WhdloadIcon*> candidates;
	for (const auto& icon : icons) {
		if (iequals(whdload_icon_detail::icon_slave(icon), slave))
			candidates.push_back(&icon);
	}
	if (candidates.empty())
		return nullptr;

	for (const auto* icon : candidates) {
		if (iequals(icon->name, drawer))
			return icon;
	}

	const WhdloadIcon* prefix = nullptr;
	for (const auto* icon : candidates) {
		const auto& name = icon->name;
		if (name.empty() || name.size() >= drawer.size() || !iequals(name, drawer.substr(0, name.size())))
			continue;
		if (std::isalnum(static_cast<unsigned char>(drawer[name.size()])))
			continue;
		if (!prefix || name.size() > prefix->name.size())
			prefix = icon;
	}
	if (prefix)
		return prefix;

	const auto dot = slave.find_last_of('.');
	const std::string slave_base = dot == std::string::npos ? slave : slave.substr(0, dot);
	for (const auto* icon : candidates) {
		if (iequals(icon->name, slave_base))
			return icon;
	}

	for (const auto* icon : candidates) {
		if (!(icon->args == candidates.front()->args))
			return nullptr;
	}
	return candidates.front();
}

#endif
