#include "amiberry_update_asset_policy.h"

#include <algorithm>
#include <cctype>

namespace {

std::string lowercase(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character)
	{
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

bool ends_with(const std::string& value, const std::string& suffix)
{
	return value.size() >= suffix.size()
		&& value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool matches_architecture(const std::string& lower_name, const linux_release_arch architecture)
{
	switch (architecture)
	{
	case linux_release_arch::aarch64:
		return lower_name.find("aarch64") != std::string::npos
			|| lower_name.find("arm64") != std::string::npos;
	case linux_release_arch::armhf:
		return lower_name.find("armhf") != std::string::npos;
	case linux_release_arch::x86_64:
	default:
		return lower_name.find("x86_64") != std::string::npos
			|| lower_name.find("amd64") != std::string::npos;
	}
}

bool is_sidecar_or_checksum(const std::string& lower_name)
{
	return ends_with(lower_name, ".zsync")
		|| lower_name == "sha256sums"
		|| ends_with(lower_name, ".sha256")
		|| ends_with(lower_name, ".sha256sum");
}

bool is_hex_digest(const std::string& line, const size_t offset)
{
	if (line.size() < offset + 64)
		return false;
	for (size_t index = offset; index < offset + 64; ++index)
	{
		if (!std::isxdigit(static_cast<unsigned char>(line[index])))
			return false;
	}
	return true;
}

} // namespace

std::string select_linux_release_asset(const std::vector<std::string>& asset_names,
	const linux_release_arch architecture, const linux_installation_kind installation_kind)
{
	for (const auto& asset_name : asset_names)
	{
		const auto lower_name = lowercase(asset_name);
		if (lower_name.find("macos") != std::string::npos
			|| lower_name.find("windows") != std::string::npos
			|| is_sidecar_or_checksum(lower_name)
			|| !matches_architecture(lower_name, architecture))
		{
			continue;
		}

		const bool is_appimage = ends_with(lower_name, ".appimage");
		if ((installation_kind == linux_installation_kind::appimage) == is_appimage)
			return asset_name;
	}
	return {};
}

std::string find_release_asset_sha256(const std::string& sums, const std::string& asset_name)
{
	size_t line_start = 0;
	while (line_start < sums.size())
	{
		const auto line_end = sums.find('\n', line_start);
		const auto count = line_end == std::string::npos ? std::string::npos : line_end - line_start;
		const auto line = sums.substr(line_start, count);

		size_t digest_start = 0;
		while (digest_start < line.size() && std::isspace(static_cast<unsigned char>(line[digest_start])))
			++digest_start;
		if (is_hex_digest(line, digest_start))
		{
			size_t filename_start = digest_start + 64;
			if (filename_start < line.size()
				&& std::isspace(static_cast<unsigned char>(line[filename_start])))
			{
				while (filename_start < line.size()
					&& std::isspace(static_cast<unsigned char>(line[filename_start])))
				{
					++filename_start;
				}
				if (filename_start < line.size() && line[filename_start] == '*')
					++filename_start;
				size_t filename_end = line.size();
				while (filename_end > filename_start
					&& std::isspace(static_cast<unsigned char>(line[filename_end - 1])))
				{
					--filename_end;
				}
				if (line.compare(filename_start, filename_end - filename_start, asset_name) == 0
					&& filename_end - filename_start == asset_name.size())
				{
					return lowercase(line.substr(digest_start, 64));
				}
			}
		}

		if (line_end == std::string::npos)
			break;
		line_start = line_end + 1;
	}
	return {};
}
