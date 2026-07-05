#pragma once

#include <string>
#include <vector>

enum class PlayContentType
{
	Configuration,
	WhdLoad,
	Floppy,
	Cd,
	Hardfile,
	Unknown,
	Ambiguous
};

enum class PlaySuggestedModel
{
	KeepExisting,
	A500,
	A1200,
	A1200Expanded,
	Cd32
};

enum class PlayFollowUp
{
	None,
	ChooseArchiveContent,
	ChooseDiskOrHardfile,
	ChooseCdMachine,
	AttachHardfileInExpert
};

struct PlayContentDetection
{
	PlayContentType type = PlayContentType::Unknown;
	std::string original_path;
	std::string display_name;
	PlaySuggestedModel suggested_model = PlaySuggestedModel::KeepExisting;
	int suggested_compatibility = 1;
	PlayFollowUp follow_up = PlayFollowUp::None;
	std::vector<PlayContentType> choices;
};

PlayContentDetection play_detect_content(const std::string& path, bool is_directory);
const char* play_content_type_name(PlayContentType type);
