// The media and folders API (settings/amiberry_media.h): the emulator's
// folders, the media lists front ends offer for picking, and sandbox
// access. It wraps what the Paths panel and the media panels use, so every
// front end sees the same folders and histories.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "target.h"

#include "settings/amiberry_media.h"
#include "settings/settings_definitions.h"
#include "macos_bookmarks.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct media_entry {
	std::string path;
	std::string name;
};

std::vector<media_entry> snapshots[AMIBERRY_MEDIA_LIST_COUNT];

amiberry_settings_status copy_out(const std::string& text, char* buffer, size_t size)
{
	if (!buffer)
		return AMIBERRY_SETTINGS_OK;
	if (text.size() >= size) {
		if (size)
			buffer[0] = 0;
		return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
	}
	std::memcpy(buffer, text.c_str(), text.size() + 1);
	return AMIBERRY_SETTINGS_OK;
}

// Paths cross the API as UTF-8; on Windows the narrow std::filesystem
// conversions use the ANSI code page instead.
std::filesystem::path from_utf8(const std::string& text)
{
#ifdef __cpp_char8_t
	return std::filesystem::path(std::u8string(text.begin(), text.end()));
#else
	return std::filesystem::u8path(text);
#endif
}

std::string to_utf8(const std::filesystem::path& path)
{
	const auto text = path.u8string();
	return std::string(text.begin(), text.end());
}

std::string file_name(const std::string& path)
{
	const size_t slash = path.find_last_of("/\\");
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string folder_path(amiberry_folder folder)
{
	switch (folder) {
	case AMIBERRY_FOLDER_ROMS: return get_rom_path();
	case AMIBERRY_FOLDER_FLOPPIES: return get_floppy_path();
	case AMIBERRY_FOLDER_CDS: return get_cdrom_path();
	case AMIBERRY_FOLDER_HARD_DRIVES: return get_harddrive_path();
	case AMIBERRY_FOLDER_CONFIGURATIONS: return get_configuration_path();
	case AMIBERRY_FOLDER_SAVESTATES: return get_savestate_folder();
	case AMIBERRY_FOLDER_SCREENSHOTS: return get_screenshot_path();
	case AMIBERRY_FOLDER_WHDLOAD_ARCHIVES: return get_whdload_arch_path();
	default: return {};
	}
}

bool set_folder_path(amiberry_folder folder, const std::string& path)
{
	switch (folder) {
	case AMIBERRY_FOLDER_ROMS: set_rom_path(path); return true;
	case AMIBERRY_FOLDER_FLOPPIES: set_floppy_path(path); return true;
	case AMIBERRY_FOLDER_CDS: set_cdrom_path(path); return true;
	case AMIBERRY_FOLDER_HARD_DRIVES: set_harddrive_path(path); return true;
	case AMIBERRY_FOLDER_CONFIGURATIONS: set_configuration_path(path); return true;
	case AMIBERRY_FOLDER_SAVESTATES: set_savestate_path(path); return true;
	case AMIBERRY_FOLDER_SCREENSHOTS: set_screenshot_path(path); return true;
	case AMIBERRY_FOLDER_WHDLOAD_ARCHIVES: set_whdload_arch_path(path); return true;
	default: return false;
	}
}

std::vector<std::string>* recent_list(amiberry_media_list list)
{
	switch (list) {
	case AMIBERRY_MEDIA_RECENT_FLOPPIES: return &lstMRUDiskList;
	case AMIBERRY_MEDIA_RECENT_CDS: return &lstMRUCDList;
	case AMIBERRY_MEDIA_RECENT_WHDLOAD: return &lstMRUWhdloadList;
	default: return nullptr;
	}
}

// The longest path a folder or recent entry may have: it must fit the
// path fields (MAX_DPATH) and, with its setting name, one line of the
// settings file, which is read back in CONFIG_BLEN pieces.
constexpr size_t max_stored_path = MAX_DPATH < CONFIG_BLEN - 64 ? MAX_DPATH : CONFIG_BLEN - 64;

bool storable_path(const char* path)
{
	return path && path[0] && std::strlen(path) < max_stored_path;
}

bool has_extension(const std::filesystem::path& file, const char* extension)
{
	std::string ext = to_utf8(file.extension());
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext == extension;
}

// The regular files with the extension in a folder, sorted by name. A
// folder that is missing or unreadable gives an empty list.
std::vector<media_entry> folder_files(const std::string& folder, const char* extension)
{
	std::vector<media_entry> files;
	std::error_code ec;
	for (std::filesystem::directory_iterator it(from_utf8(folder), ec), end; !ec && it != end; it.increment(ec)) {
		if (!it->is_regular_file(ec) || !has_extension(it->path(), extension))
			continue;
		const std::string path = to_utf8(it->path());
		files.push_back({path, file_name(path)});
	}
	std::sort(files.begin(), files.end(),
		[](const media_entry& a, const media_entry& b) { return a.name < b.name; });
	return files;
}

std::vector<media_entry> build_list(amiberry_media_list list)
{
	std::vector<media_entry> entries;
	if (const std::vector<std::string>* recent = recent_list(list)) {
		for (const std::string& path : *recent) {
			if (!path.empty())
				entries.push_back({path, file_name(path)});
		}
		return entries;
	}
	switch (list) {
	case AMIBERRY_MEDIA_CD_DRIVES:
		for (const std::string& drive : get_cd_drives())
			entries.push_back({drive, drive});
		break;
	case AMIBERRY_MEDIA_CONFIGURATIONS:
		entries = folder_files(get_configuration_path(), ".uae");
		break;
	case AMIBERRY_MEDIA_SAVESTATES:
		// The folder in effect: a configuration's statefile_path overrides
		// the save states folder.
		entries = folder_files(get_savestate_path(), ".uss");
		break;
	default:
		break;
	}
	return entries;
}

} // namespace

extern "C" {

amiberry_settings_status amiberry_folder_get(amiberry_folder folder, char* buffer, size_t size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		if (folder < 0 || folder >= AMIBERRY_FOLDER_COUNT)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		return copy_out(folder_path(folder), buffer, size);
	});
}

amiberry_settings_status amiberry_folder_set(amiberry_folder folder, const char* path)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_INVALID_VALUE, [=] {
		if (folder < 0 || folder >= AMIBERRY_FOLDER_COUNT)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		if (!storable_path(path))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// The folder is granted before it is stored, so a sandboxed build
		// never keeps a folder it cannot open after a restart.
		if (!macos_bookmark_store(path))
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		// A folder that cannot be saved would revert at the next start:
		// the change is undone and reported instead.
		const std::string previous = folder_path(folder);
		set_folder_path(folder, path);
		if (!save_amiberry_settings_with_result()) {
			set_folder_path(folder, previous);
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		return AMIBERRY_SETTINGS_OK;
	});
}

size_t amiberry_media_refresh(amiberry_media_list list)
{
	return amiberry_settings::no_throw(static_cast<size_t>(0), [=] {
		if (list < 0 || list >= AMIBERRY_MEDIA_LIST_COUNT)
			return static_cast<size_t>(0);
		snapshots[list] = build_list(list);
		return snapshots[list].size();
	});
}

amiberry_settings_status amiberry_media_entry(amiberry_media_list list, size_t index,
	char* path, size_t path_size, char* name, size_t name_size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		if (list < 0 || list >= AMIBERRY_MEDIA_LIST_COUNT || index >= snapshots[list].size())
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		const media_entry& entry = snapshots[list][index];
		const amiberry_settings_status path_status = copy_out(entry.path, path, path_size);
		const amiberry_settings_status name_status = copy_out(entry.name, name, name_size);
		return path_status != AMIBERRY_SETTINGS_OK ? path_status : name_status;
	});
}

amiberry_settings_status amiberry_media_add_recent(amiberry_media_list list, const char* path)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_INVALID_VALUE, [=] {
		std::vector<std::string>* recent = recent_list(list);
		if (!recent || !storable_path(path))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// Saved at once, like a folder; a list that cannot be saved is put
		// back and the failure reported.
		const std::vector<std::string> previous = *recent;
		add_file_to_mru_list(*recent, path);
		if (!save_amiberry_settings_with_result()) {
			*recent = previous;
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		return AMIBERRY_SETTINGS_OK;
	});
}

bool amiberry_media_grant_access(const char* path)
{
	return amiberry_settings::no_throw(false, [=] {
		return path && path[0] && macos_bookmark_store(path);
	});
}

} // extern "C"
