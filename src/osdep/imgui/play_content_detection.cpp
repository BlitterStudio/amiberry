#include "imgui/play_content_detection.h"

#include <algorithm>
#include <cctype>

namespace
{
std::string get_display_name(const std::string& path)
{
	const auto separator = path.find_last_of("/\\");
	if (separator == std::string::npos) {
		return path;
	}

	if (separator + 1 >= path.size()) {
		return path;
	}

	return path.substr(separator + 1);
}

std::string get_lowercase_extension(const std::string& path)
{
	const auto display_name = get_display_name(path);
	const auto dot = display_name.find_last_of('.');
	if (dot == std::string::npos || dot + 1 >= display_name.size()) {
		return {};
	}

	auto extension = display_name.substr(dot);
	std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return extension;
}

std::string get_lowercase_name(const std::string& path)
{
	auto name = get_display_name(path);
	std::transform(name.begin(), name.end(), name.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return name;
}

bool is_one_of(const std::string& extension, const std::vector<const char*>& extensions)
{
	return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

bool is_name_separator(const char ch)
{
	return !std::isalnum(static_cast<unsigned char>(ch));
}

bool has_aga_token(const std::string& path)
{
	const auto name = get_lowercase_name(path);
	size_t offset = 0;
	for (;;) {
		const auto pos = name.find("aga", offset);
		if (pos == std::string::npos)
			return false;

		const bool before_ok = pos == 0 || is_name_separator(name[pos - 1]);
		const bool after_ok = pos + 3 >= name.size() || is_name_separator(name[pos + 3]);
		if (before_ok && after_ok)
			return true;

		offset = pos + 3;
	}
}
}

PlayContentDetection play_detect_content(const std::string& path, bool is_directory)
{
	PlayContentDetection detection;
	detection.original_path = path;
	detection.display_name = get_display_name(path);

	if (is_directory) {
		detection.type = PlayContentType::WhdLoad;
		detection.suggested_model = PlaySuggestedModel::A1200;
		return detection;
	}

	const auto extension = get_lowercase_extension(path);

	if (extension == ".uae") {
		detection.type = PlayContentType::Configuration;
		return detection;
	}

	if (is_one_of(extension, { ".adf", ".adz", ".ipf", ".dms", ".fdi", ".scp", ".wrp", ".dsq" })) {
		detection.type = PlayContentType::Floppy;
		detection.suggested_model = has_aga_token(path) ? PlaySuggestedModel::A1200 : PlaySuggestedModel::A500;
		return detection;
	}

	if (is_one_of(extension, { ".cue", ".bin", ".iso", ".ccd", ".mds", ".nrg" })) {
		detection.type = PlayContentType::Cd;
		detection.suggested_model = PlaySuggestedModel::Cd32;
		detection.follow_up = PlayFollowUp::ChooseCdMachine;
		return detection;
	}

	if (extension == ".chd") {
		detection.type = PlayContentType::Ambiguous;
		detection.follow_up = PlayFollowUp::ChooseCdOrHardfile;
		detection.choices = { PlayContentType::Cd, PlayContentType::Hardfile };
		return detection;
	}

	if (is_one_of(extension, { ".hdf", ".hdz", ".hda", ".vhd" })) {
		detection.type = PlayContentType::Hardfile;
		detection.suggested_model = PlaySuggestedModel::A1200Expanded;
		detection.follow_up = PlayFollowUp::AttachHardfileInExpert;
		return detection;
	}

	if (extension == ".img") {
		detection.type = PlayContentType::Ambiguous;
		detection.follow_up = PlayFollowUp::ChooseDiskOrHardfile;
		detection.choices = { PlayContentType::Floppy, PlayContentType::Hardfile };
		return detection;
	}

	if (is_one_of(extension, { ".lha", ".lzh", ".lzx" })) {
		detection.type = PlayContentType::WhdLoad;
		detection.suggested_model = PlaySuggestedModel::A1200;
		return detection;
	}

	if (is_one_of(extension, { ".zip", ".7z" })) {
		detection.type = PlayContentType::Ambiguous;
		detection.suggested_model = PlaySuggestedModel::A1200;
		detection.follow_up = PlayFollowUp::ChooseArchiveContent;
		detection.choices = { PlayContentType::WhdLoad, PlayContentType::Floppy };
		return detection;
	}

	return detection;
}

PlaySuggestedModel play_suggested_model_for_action(const PlayContentDetection& detection,
	const PlayContentType action_type)
{
	if (detection.type == action_type && detection.suggested_model != PlaySuggestedModel::KeepExisting)
		return detection.suggested_model;

	switch (action_type) {
		case PlayContentType::Floppy:
			return PlaySuggestedModel::A500;
		case PlayContentType::WhdLoad:
			return PlaySuggestedModel::A1200;
		case PlayContentType::Cd:
			return PlaySuggestedModel::Cd32;
		case PlayContentType::Hardfile:
			return PlaySuggestedModel::A1200Expanded;
		default:
			break;
	}

	return PlaySuggestedModel::KeepExisting;
}

const char* play_content_type_name(const PlayContentType type)
{
	switch (type) {
		case PlayContentType::Configuration:
			return "Configuration";
		case PlayContentType::WhdLoad:
			return "WHDLoad";
		case PlayContentType::Floppy:
			return "Floppy";
		case PlayContentType::Cd:
			return "CD";
		case PlayContentType::Hardfile:
			return "Hardfile";
		case PlayContentType::Unknown:
			return "Unknown";
		case PlayContentType::Ambiguous:
			return "Ambiguous";
	}

	return "Unknown";
}
