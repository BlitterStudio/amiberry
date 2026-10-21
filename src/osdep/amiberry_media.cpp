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

#ifdef _WIN32
#include <windows.h>
#endif

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

// Whether text is well-formed UTF-8 (no overlong forms, surrogates or code
// points past U+10FFFF). POSIX file names are arbitrary bytes; the API
// promises UTF-8, so names that are not are left out of the lists.
bool valid_utf8(const std::string& text)
{
	const auto* p = reinterpret_cast<const unsigned char*>(text.data());
	const auto* end = p + text.size();
	while (p < end) {
		const unsigned char c = *p++;
		if (c < 0x80)
			continue;
		int extra;
		unsigned int cp;
		if (c >= 0xc2 && c <= 0xdf) { extra = 1; cp = c & 0x1f; }
		else if (c >= 0xe0 && c <= 0xef) { extra = 2; cp = c & 0x0f; }
		else if (c >= 0xf0 && c <= 0xf4) { extra = 3; cp = c & 0x07; }
		else return false;
		if (end - p < extra)
			return false;
		for (int i = 0; i < extra; i++) {
			if ((p[i] & 0xc0) != 0x80)
				return false;
			cp = (cp << 6) | (p[i] & 0x3f);
		}
		p += extra;
		if ((extra == 2 && (cp < 0x800 || (cp >= 0xd800 && cp <= 0xdfff)))
			|| (extra == 3 && (cp < 0x10000 || cp > 0x10ffff)))
			return false;
	}
	return true;
}

// Whether the core can use path: UTF-8, as the API promises, and on Windows
// one the core's narrow file calls can open. Those take the bytes in the
// active code page, so a path beyond ASCII only works where that code page
// is UTF-8.
bool usable_path(const std::string& path)
{
	if (!valid_utf8(path))
		return false;
#ifdef _WIN32
	if (GetACP() != CP_UTF8) {
		for (const char c : path) {
			if (static_cast<unsigned char>(c) >= 0x80)
				return false;
		}
	}
#endif
	return true;
}

// The file name the host's path rules give (a backslash is a separator on
// Windows only).
std::string file_name(const std::string& path)
{
	return to_utf8(from_utf8(path).filename());
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

// Saves the emulator's settings; a save that throws (out of memory while
// building the file) counts as failed, so callers undo their change.
bool save_settings()
{
	try {
		return save_amiberry_settings_with_result();
	} catch (...) {
		return false;
	}
}

// The folder the state files are in: the configuration's statefile_path
// override names a state file, whose folder is the one in effect.
std::string state_folder()
{
	const std::string path = get_savestate_path();
	if (path == get_savestate_folder())
		return path;
	return to_utf8(from_utf8(path).parent_path());
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

// The settings file stores one value per line and trims trailing white
// space when it reads it back, so a path with a line break, another control
// character or white space at either end would not come back as written.
bool storable_path(const char* path)
{
	if (!path || !path[0])
		return false;
	const size_t length = std::strlen(path);
	if (length >= max_stored_path)
		return false;
	for (size_t i = 0; i < length; i++) {
		if (static_cast<unsigned char>(path[i]) < 0x20 || path[i] == 0x7f)
			return false;
	}
	// Only what the API can return: its strings are UTF-8.
	return path[0] != ' ' && path[length - 1] != ' ' && usable_path(path);
}

// The absolute spelling of a path, so a list entry is a full path whatever
// the working directory of the caller that reads it; empty when it cannot
// be made absolute (the working directory is gone).
std::string absolute_path(const std::string& path)
{
	std::error_code ec;
	const std::filesystem::path absolute = std::filesystem::absolute(from_utf8(path), ec);
	return ec ? std::string() : to_utf8(absolute);
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
	// An unset folder lists nothing (resolving it would list the working
	// directory).
	if (folder.empty())
		return files;
	std::error_code ec;
	// Absolute, so entries are full paths whatever the working directory
	// of the caller that reads them.
	const std::filesystem::path base = std::filesystem::absolute(from_utf8(folder), ec);
	if (ec)
		return files;
	for (std::filesystem::directory_iterator it(base, ec), end; !ec && it != end; it.increment(ec)) {
		// Its own error code: an entry that cannot be examined (a dangling
		// link) is skipped without ending the scan.
		std::error_code entry_ec;
		if (!it->is_regular_file(entry_ec) || !has_extension(it->path(), extension))
			continue;
		const std::string path = to_utf8(it->path());
		if (usable_path(path))
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
			// Recorded absolute; one that cannot be made absolute now is
			// left out rather than listed relative.
			const std::string absolute = path.empty() ? std::string() : absolute_path(path);
			if (!absolute.empty() && usable_path(absolute))
				entries.push_back({absolute, file_name(path)});
		}
		return entries;
	}
	switch (list) {
	case AMIBERRY_MEDIA_CD_DRIVES:
		for (const std::string& drive : get_cd_drives()) {
			if (usable_path(drive))
				entries.push_back({drive, drive});
		}
		break;
	case AMIBERRY_MEDIA_CONFIGURATIONS:
		entries = folder_files(get_configuration_path(), ".uae");
		break;
	case AMIBERRY_MEDIA_SAVESTATES:
		// The folder in effect: a configuration's statefile_path overrides
		// the save states folder.
		entries = folder_files(state_folder(), ".uss");
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
		// A folder set outside the API (an old settings file, the GUI) can
		// hold a path this API would not accept, which it never returns.
		const std::string path = folder_path(folder);
		if (!usable_path(path)) {
			if (buffer && size)
				buffer[0] = 0;
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		return copy_out(path, buffer, size);
	});
}

amiberry_settings_status amiberry_folder_set(amiberry_folder folder, const char* path)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_INVALID_VALUE, [=] {
		if (folder < 0 || folder >= AMIBERRY_FOLDER_COUNT)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		// The core's folder getters append a separator in MAX_DPATH buffers,
		// so a folder leaves room for it.
		if (!storable_path(path) || std::strlen(path) + 1 >= max_stored_path)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// Everything that can throw before the grant is done first; after
		// it, any failure (a throw included) undoes the change.
		const std::string previous = folder_path(folder);
		const macos_bookmarks_state bookmarks = macos_bookmarks_save_state();
		// The folder is granted before it is stored, so a sandboxed build
		// never keeps a folder it cannot open after a restart.
		if (!macos_bookmark_store(path))
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		// A folder that cannot be saved would revert at the next start:
		// the change is undone and reported instead. The bookmark store is
		// put back last, as the folder setters also grant the folder they
		// set, the previous one included.
		bool saved = false;
		try {
			set_folder_path(folder, path);
			saved = save_settings();
		} catch (...) {
		}
		if (!saved) {
			try {
				set_folder_path(folder, previous);
			} catch (...) {
			}
			macos_bookmarks_restore(bookmarks);
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
		// What is stored is the absolute spelling, which must fit too.
		const std::string absolute = absolute_path(path);
		if (!storable_path(absolute.c_str()))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// A folder is not media (fopen opens one on POSIX).
		std::error_code ec;
		if (std::filesystem::is_directory(from_utf8(absolute), ec))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// The settings loader keeps only recent entries it can open, so an
		// entry that cannot be opened now would vanish at the next start.
		FILE* const file = uae_fopen(absolute.c_str(), "rbe");
		if (!file)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		fclose(file);
		// Saved at once, like a folder; a list that cannot be saved is put
		// back and the failure reported.
		std::vector<std::string> previous = *recent;
		bool saved = false;
		try {
			add_file_to_mru_list(*recent, absolute);
			saved = save_settings();
		} catch (...) {
		}
		if (!saved) {
			recent->swap(previous); // cannot throw
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
