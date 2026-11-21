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
#include "rommgr.h"
#include "filesys.h"
#include "amiberry_filesys_charset.h"
#include "host_fs.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif
#ifdef __linux__
#include <sys/syscall.h>
#endif
#ifdef __APPLE__
#include <stdio.h>
#endif
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// The core's configuration association (src/osdep/amiberry.cpp): the names
// the title and the save name suggestion come from. The path itself is
// get_current_config_path and set_current_config_path (target.h).
extern char last_loaded_config[MAX_DPATH];
extern char last_active_config[MAX_DPATH];
extern void set_last_loaded_config(const char* filename, bool automatic_default);

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

// Paths cross the API as UTF-8. The core holds host paths in either of two
// forms on Windows: UTF-8 (set through this API or a native front end) or the
// ANSI code page bytes older configurations and caches saved. host_path() and
// host_path_utf8() (host_fs.h) read either and give std::filesystem its wide
// path, so every file call here goes through them and nothing depends on the
// process code page.
//
// The API's UTF-8 form of a host path or name the core holds; false when it
// has none: the bytes are not UTF-8 and, off Windows, they are arbitrary
// POSIX file name bytes the API promises not to hand out; on Windows, bytes
// the ANSI code page cannot decode.
bool api_path(const std::string& core, std::string& utf8)
{
	if (amiberry_valid_utf8(core)) {
		utf8 = core;
		return true;
	}
#ifdef _WIN32
	utf8 = host_path_utf8(host_path(core));
	// Undecodable bytes come out as U+FFFD, which no path may contain here.
	return amiberry_valid_utf8(utf8) && utf8.find("\xEF\xBF\xBD") == std::string::npos;
#else
	return false;
#endif
}

// Whether the core's name is the UTF-8 text: both spellings are compared as
// the host reads them.
bool same_core_name(const std::string& utf8, const char* core)
{
	std::string other;
	return api_path(core, other) && other == utf8;
}

// The file name the host's path rules give (a backslash is a separator on
// Windows only).
std::string file_name(const std::string& path)
{
	return host_path_utf8(host_path(path).filename());
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

// The folder the state files are in: the configuration's statefile_path
// override names a state file, whose folder is the one in effect.
std::string state_folder()
{
	// Either spelling of the core's path is read by host_path() below.
	const std::string path = get_savestate_path();
	if (path == get_savestate_folder())
		return path;
	// Relative to the working directory, where savestate_quick() opens a
	// relative one (a bare "slot.uss" has no parent of its own).
	std::error_code ec;
	const std::filesystem::path file = std::filesystem::absolute(host_path(path), ec);
	return ec ? std::string() : host_path_utf8(file.parent_path());
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
// settings file, which is read back in CONFIG_BLEN pieces. On Windows the
// emulator is not long-path aware (no longPathAware manifest entry), so a
// path must also stay within MAX_PATH there, counted in UTF-16 units as the
// system counts it, not in UTF-8 bytes.
constexpr size_t max_line_path = MAX_DPATH < CONFIG_BLEN - 64 ? MAX_DPATH : CONFIG_BLEN - 64;

// Whether path, with extra more characters (the separator a folder gets),
// is shorter than those limits.
bool path_fits(const std::string_view path, const size_t extra)
{
	if (path.size() + extra >= max_line_path)
		return false;
#ifdef _WIN32
	const int units = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
	if (units < 0 || (units == 0 && !path.empty()) || static_cast<size_t>(units) + extra >= MAX_PATH)
		return false;
#endif
	return true;
}

// The settings file stores one value per line and trims trailing white
// space when it reads it back, so a path with a line break, another control
// character or white space at either end would not come back as written.
bool storable_path(const char* path)
{
	if (!path || !path[0])
		return false;
	const size_t length = std::strlen(path);
	if (!path_fits(std::string_view(path, length), 0))
		return false;
	for (size_t i = 0; i < length; i++) {
		if (static_cast<unsigned char>(path[i]) < 0x20 || path[i] == 0x7f)
			return false;
	}
	// Only what the API can return: its strings are UTF-8.
	return path[0] != ' ' && path[length - 1] != ' ' && amiberry_valid_utf8(std::string_view(path, length));
}

// The absolute spelling of a path, so a list entry is a full path whatever
// the working directory of the caller that reads it; empty when it cannot
// be made absolute (the working directory is gone).
std::string absolute_path(const std::string& path)
{
	std::error_code ec;
	const std::filesystem::path absolute = std::filesystem::absolute(host_path(path), ec);
	return ec ? std::string() : host_path_utf8(absolute);
}

bool has_extension(const std::filesystem::path& file, const char* extension)
{
	std::string ext = host_path_utf8(file.extension());
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
	const std::filesystem::path base = std::filesystem::absolute(host_path(folder), ec);
	if (ec)
		return files;
	for (std::filesystem::directory_iterator it(base, ec), end; !ec && it != end; it.increment(ec)) {
		// Its own error code: an entry that cannot be examined (a dangling
		// link) is skipped without ending the scan.
		std::error_code entry_ec;
		if (!it->is_regular_file(entry_ec) || !has_extension(it->path(), extension))
			continue;
		const std::string path = host_path_utf8(it->path());
		if (amiberry_valid_utf8(path))
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
			if (!absolute.empty() && amiberry_valid_utf8(absolute))
				entries.push_back({absolute, file_name(path)});
		}
		return entries;
	}
	switch (list) {
	case AMIBERRY_MEDIA_CD_DRIVES:
		for (const std::string& drive : get_cd_drives()) {
			std::string path;
			if (api_path(drive, path))
				entries.push_back({path, path});
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
	case AMIBERRY_MEDIA_KICKSTART_ROMS: {
		// The main ROMs of the scanned list, named as the GUI's ROM combo
		// names them; a ROM found twice is listed once.
		const romlist* roms = romlist_getit();
		const int count = romlist_count();
		for (int i = 0; roms && i < count; i++) {
			const romdata* rd = roms[i].rd;
			if (!rd || !(rd->type & (ROMTYPE_KICK | ROMTYPE_KICKCD32)) || !roms[i].path || !roms[i].path[0])
				continue;
			std::string path;
			if (!api_path(roms[i].path, path)
				|| std::any_of(entries.begin(), entries.end(), [&](const media_entry& e) { return e.path == path; }))
				continue;
			entries.push_back({path, rd->name ? rd->name : file_name(path)});
		}
		std::stable_sort(entries.begin(), entries.end(),
			[](const media_entry& a, const media_entry& b) { return a.name < b.name; });
		break;
	}
	default:
		break;
	}
	return entries;
}

// Puts next in place of a recent list and saves the emulator's settings at
// once, as a folder is saved; a list that cannot be saved is put back and
// the failure reported.
amiberry_settings_status store_recent(std::vector<std::string>& recent, std::vector<std::string> next)
{
	recent.swap(next); // cannot throw; next now holds the list as it was
	bool saved = false;
	try {
		saved = save_amiberry_settings_with_result();
	} catch (...) {
	}
	if (!saved) {
		recent.swap(next);
		return AMIBERRY_SETTINGS_UNAVAILABLE;
	}
	return AMIBERRY_SETTINGS_OK;
}

// Creates path, which must not exist, holding size zero bytes; created
// tells whether the file was made (so a failure can remove it). With size 0
// it only claims the name, which no other writer can then take.
bool create_raw_hardfile(const std::string& path, unsigned long long size, bool& created)
{
	created = false;
	static const uae_u8 zeroes[1024 * 1024]{};
#ifdef _WIN32
	const std::filesystem::path wide = host_path(path);
	HANDLE file = CreateFileW(wide.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return false;
	created = true;
	bool complete = true;
	unsigned long long remaining = size;
	while (remaining) {
		const DWORD bytes = static_cast<DWORD>(std::min<unsigned long long>(remaining, sizeof zeroes));
		DWORD written = 0;
		if (!WriteFile(file, zeroes, bytes, &written, nullptr) || written != bytes) {
			complete = false;
			break;
		}
		remaining -= bytes;
	}
	CloseHandle(file);
	return complete;
#else
	const int file = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (file < 0)
		return false;
	created = true;
	bool complete = true;
	unsigned long long remaining = size;
	while (remaining) {
		const size_t bytes = static_cast<size_t>(std::min<unsigned long long>(remaining, sizeof zeroes));
		if (write(file, zeroes, bytes) != static_cast<ssize_t>(bytes)) {
			complete = false;
			break;
		}
		remaining -= bytes;
	}
	close(file);
	return complete;
#endif
}

enum class rename_result { done, exists, failed };

// Renames from to to, which must not exist, in one step that never replaces
// a file and copies no data. Both are in one folder (one volume).
rename_result rename_noreplace(const std::filesystem::path& from, const std::filesystem::path& to)
{
#ifdef _WIN32
	// No MOVEFILE_REPLACE_EXISTING: an existing destination fails.
	if (MoveFileExW(from.c_str(), to.c_str(), 0))
		return rename_result::done;
	const DWORD error = GetLastError();
	return error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS ? rename_result::exists : rename_result::failed;
#else
	const std::string source = from.string();
	const std::string target = to.string();
#if defined(__linux__) && defined(SYS_renameat2)
	// RENAME_NOREPLACE (1): atomic, fails with EEXIST. Not every kernel or
	// file system has it; those fall through to the steps below.
	if (syscall(SYS_renameat2, AT_FDCWD, source.c_str(), AT_FDCWD, target.c_str(), 1u) == 0)
		return rename_result::done;
	if (errno == EEXIST)
		return rename_result::exists;
	if (errno != ENOSYS && errno != EINVAL && errno != ENOTSUP)
		return rename_result::failed;
#elif defined(__APPLE__) && defined(RENAME_EXCL)
	if (renamex_np(source.c_str(), target.c_str(), RENAME_EXCL) == 0)
		return rename_result::done;
	if (errno == EEXIST)
		return rename_result::exists;
	if (errno != ENOTSUP && errno != EINVAL)
		return rename_result::failed;
#endif
	// link() never replaces a file. Where neither atomic no-replace rename
	// nor link is possible (exFAT, FAT) nothing is attempted.
	if (link(source.c_str(), target.c_str()) != 0)
		return errno == EEXIST ? rename_result::exists : rename_result::failed;
	if (unlink(source.c_str()) == 0)
		return rename_result::done;
	const int saved = errno;
	unlink(target.c_str());
	errno = saved;
	return rename_result::failed;
#endif
}

// Whether to is another spelling of from's own file name that a
// case-insensitive file system (Windows, macOS) resolves to the file: the
// names differ only in case (Windows: the system's ordinal case folding of
// the UTF-16 names; macOS: ASCII case), and the destination is the file
// itself.
bool is_case_only_rename([[maybe_unused]] const std::filesystem::path& from, [[maybe_unused]] const std::filesystem::path& to)
{
#if defined(_WIN32)
	const std::wstring a = from.filename().native();
	const std::wstring b = to.filename().native();
	if (a == b || a.empty() || b.empty()
		|| CompareStringOrdinal(a.c_str(), static_cast<int>(a.size()), b.c_str(), static_cast<int>(b.size()), TRUE) != CSTR_EQUAL)
		return false;
	std::error_code ec;
	return std::filesystem::equivalent(from, to, ec) && !ec;
#elif defined(__APPLE__)
	std::string a = host_path_utf8(from.filename());
	std::string b = host_path_utf8(to.filename());
	if (a == b || a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++) {
		if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
			return false;
	}
	std::error_code ec;
	return std::filesystem::equivalent(from, to, ec) && !ec;
#else
	return false;
#endif
}

// Renames a file to such a spelling of its own name, by way of a name that
// is free for the moment; if the final name is refused the file is put back.
rename_result rename_to_equivalent(const std::filesystem::path& from, const std::filesystem::path& to)
{
	std::filesystem::path temporary;
	rename_result moved = rename_result::exists;
	for (int i = 0; i < 100 && moved == rename_result::exists; i++) {
		temporary = from.parent_path() / (".rename-" + std::to_string(i) + ".tmp");
		moved = rename_noreplace(from, temporary);
	}
	if (moved != rename_result::done)
		return rename_result::failed;
	const rename_result result = rename_noreplace(temporary, to);
	if (result != rename_result::done && rename_noreplace(temporary, from) != rename_result::done)
		write_log("amiberry_media_rename_configuration: '%s' could not be put back\n", host_path_utf8(temporary).c_str());
	return result;
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
		std::string path;
		if (!api_path(folder_path(folder), path)) {
			if (buffer && size)
				buffer[0] = 0;
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		// The path is the only output, so it must have somewhere to go.
		if (!buffer)
			return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
		return copy_out(path, buffer, size);
	});
}

void amiberry_media_add_rom_key(const char* path)
{
	if (!path || !path[0])
		return;
	amiberry_settings::no_throw(0, [=] {
		addkeyfile(path);
		return 0;
	});
}

int amiberry_media_rescan_roms(void)
{
	return amiberry_settings::no_throw(0, [] { return amiberry_settings::rescan_roms(); });
}

amiberry_settings_status amiberry_folder_set(amiberry_folder folder, const char* path)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_INVALID_VALUE, [=] {
		if (folder < 0 || folder >= AMIBERRY_FOLDER_COUNT)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		// The core's folder getters append a separator in MAX_DPATH buffers,
		// so a folder leaves room for it.
		if (!storable_path(path) || !path_fits(path, 1))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
#ifndef _WIN32
		// The folder getters treat a trailing backslash as the separator
		// (fix_trailing), which on POSIX is part of the name instead.
		if (path[std::strlen(path) - 1] == '\\')
			return AMIBERRY_SETTINGS_INVALID_VALUE;
#endif
		// The emulator lists and opens every folder as a directory; one
		// that does not exist yet is allowed.
		std::error_code ec;
		const auto status = std::filesystem::status(host_path(path), ec);
		if (std::filesystem::exists(status) && !std::filesystem::is_directory(status))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// Everything that can throw before the grant is done first; after
		// it, any failure (a throw included) undoes the change.
		const std::string previous = folder_path(folder);
		std::string previous_utf8;
		const bool path_changed = !api_path(previous, previous_utf8) || previous_utf8 != path;
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
			saved = save_amiberry_settings_with_result();
		} catch (...) {
		}
		if (!saved) {
			try {
				set_folder_path(folder, previous);
			} catch (...) {
			}
			// If even the bookmark file cannot be written, the grant for
			// the folder the caller picked stays on disk (logged).
			if (!macos_bookmarks_restore(bookmarks))
				write_log("amiberry_folder_set: the access grant for '%s' could not be withdrawn\n", path);
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		// Another ROMs folder holds other ROMs, as the GUI's Paths panel
		// rescans after its ROMs folder changes.
		if (folder == AMIBERRY_FOLDER_ROMS && path_changed)
			amiberry_settings::rescan_roms();
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

amiberry_settings_status amiberry_media_current_configuration(char* path, size_t size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		return copy_out(get_current_config_path(), path, size);
	});
}

void amiberry_media_forget_current_configuration(void)
{
	amiberry_settings::no_throw(0, [] {
		forget_loaded_config();
		return 0;
	});
}

amiberry_settings_status amiberry_media_create_hardfile(const char* path,
	unsigned long long size_bytes, bool dynamic_vhd, char* created, size_t created_size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNAVAILABLE, [=] {
		if (!path || !path[0] || size_bytes < 1024ull * 1024ull)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		std::string target(path);
#ifdef _WIN32
		// Bytes that are not UTF-8 are the ANSI code page spelling; the path
		// is made and handed back as UTF-8.
		std::string normalized;
		if (!api_path(target, normalized))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		target = std::move(normalized);
#endif
		if (dynamic_vhd && has_extension(host_path(target), ".hdf"))
			target.replace(target.size() - 4, 4, ".vhd");
		if (!created || target.size() >= created_size)
			return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
		std::error_code ec;
		if (std::filesystem::exists(host_path(target), ec))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (ec)
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		bool complete = false;
		bool raw_created = false;
		if (dynamic_vhd) {
			// Claim the name first, so an existing file is never written over
			// (vhd_create opens with "wb"), then write the image into it. No
			// rename or hard link: exFAT and FAT volumes have no hard links.
			complete = create_raw_hardfile(target, 0, raw_created)
				&& vhd_create(target.c_str(), size_bytes, 0) != 0;
		} else {
			complete = create_raw_hardfile(target, size_bytes, raw_created);
		}
		if (!complete) {
			if (raw_created)
				std::filesystem::remove(host_path(target), ec);
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		std::memcpy(created, target.c_str(), target.size() + 1);
		return AMIBERRY_SETTINGS_OK;
	});
}

amiberry_settings_status amiberry_media_configuration_info(const char* path,
	char* description, size_t description_size, char* category, size_t category_size)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNKNOWN_ID, [=] {
		if (!path || !path[0])
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		uae_prefs* p = cfgfile_open(path, nullptr);
		if (!p)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		const std::string text = p->description;
		const std::string group = p->category;
		cfgfile_close(p);
		const amiberry_settings_status description_status = copy_out(text, description, description_size);
		const amiberry_settings_status category_status = copy_out(group, category, category_size);
		return description_status != AMIBERRY_SETTINGS_OK ? description_status : category_status;
	});
}

amiberry_settings_status amiberry_media_rename_configuration(const char* path, const char* new_path)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_UNAVAILABLE, [=] {
		if (!storable_path(path) || !storable_path(new_path))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		const std::string from_text = absolute_path(path);
		const std::string to_text = absolute_path(new_path);
		if (!storable_path(from_text.c_str()) || !storable_path(to_text.c_str()))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		const std::filesystem::path from = host_path(from_text);
		const std::filesystem::path to = host_path(to_text);
		if (!has_extension(from, ".uae") || !has_extension(to, ".uae"))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		std::error_code ec;
		const auto source = std::filesystem::status(from, ec);
		if (source.type() == std::filesystem::file_type::not_found)
			return AMIBERRY_SETTINGS_UNKNOWN_ID;
		if (ec)
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		if (source.type() != std::filesystem::file_type::regular)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// A rename within a folder, not a move: both names in one folder.
		const bool same_folder = std::filesystem::equivalent(from.parent_path(), to.parent_path(), ec);
		if (ec || !same_folder)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// The same file under the same name changes nothing.
		if (from.lexically_normal() == to.lexically_normal())
			return AMIBERRY_SETTINGS_OK;
		// An absent destination is not an error, whether or not the library
		// also sets ec for it (libc++ does); any other failure is.
		const auto target = std::filesystem::symlink_status(to, ec);
		const bool target_exists = target.type() != std::filesystem::file_type::not_found;
		if (target_exists && ec)
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		if (!target_exists && ec && ec != std::errc::no_such_file_or_directory && ec != std::errc::not_a_directory)
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		bool case_only = false;
		if (target_exists) {
			// Only a case-insensitive file system names the file by another
			// spelling of its own name; a rename to that spelling is allowed
			// on Windows and macOS. Any other existing file or link of the
			// new name is never replaced.
			case_only = is_case_only_rename(from, to);
			if (!case_only)
				return AMIBERRY_SETTINGS_INVALID_VALUE;
		}

		// Whether the pending configuration is this file, found before the
		// rename while both names resolve.
		bool associated = false;
		const std::string current = get_current_config_path();
		if (!current.empty()) {
			const std::filesystem::path current_file = host_path(current);
			std::error_code compare_ec;
			associated = current_file.lexically_normal() == from.lexically_normal()
				|| std::filesystem::equivalent(current_file, from, compare_ec);
		}
		const std::string old_name = remove_file_extension(extract_filename(from_text));

		const rename_result result = case_only ? rename_to_equivalent(from, to) : rename_noreplace(from, to);
		if (result == rename_result::exists)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (result != rename_result::done)
			return AMIBERRY_SETTINGS_UNAVAILABLE;

		// The configuration in memory is untouched; only the file it came
		// from or was saved to has a new name, so the next save goes there.
		if (associated) {
			set_current_config_path(to_text.c_str());
			if (same_core_name(old_name, last_active_config))
				set_last_active_config(to_text.c_str());
			if (same_core_name(old_name, last_loaded_config))
				set_last_loaded_config(to_text.c_str(), false);
		}
		return AMIBERRY_SETTINGS_OK;
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
		// A folder is not media (fopen opens one on POSIX), and opening a
		// FIFO or socket would block or fail; files and devices (a CD
		// drive) are media.
		std::error_code ec;
		const auto type = std::filesystem::status(host_path(absolute), ec).type();
		if (type == std::filesystem::file_type::directory || type == std::filesystem::file_type::fifo
			|| type == std::filesystem::file_type::socket)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// The settings loader keeps only recent entries it can open, so an
		// entry that cannot be opened now would vanish at the next start.
#ifdef _WIN32
		FILE* const file = host_fopen(absolute.c_str(), "rb");
#else
		FILE* const file = host_fopen(absolute.c_str(), "rbe");
#endif
		if (!file)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		fclose(file);
		std::vector<std::string> next = *recent;
		add_file_to_mru_list(next, absolute);
		return store_recent(*recent, std::move(next));
	});
}

amiberry_settings_status amiberry_media_remove_recent(amiberry_media_list list, const char* path)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_INVALID_VALUE, [=] {
		std::vector<std::string>* recent = recent_list(list);
		if (!recent || !path || !path[0])
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// Matched as build_list shows the entries: a list can hold a path
		// recorded relative or in another spelling of the same file, and
		// every entry listed under path goes.
		const std::string absolute = absolute_path(path);
		std::vector<std::string> next;
		for (const std::string& entry : *recent) {
			const bool listed = !entry.empty()
				&& (entry == path || (!absolute.empty() && absolute_path(entry) == absolute));
			if (!listed)
				next.push_back(entry);
		}
		if (next.size() == recent->size())
			return AMIBERRY_SETTINGS_OK;
		return store_recent(*recent, std::move(next));
	});
}

amiberry_settings_status amiberry_media_clear_recent(amiberry_media_list list)
{
	return amiberry_settings::no_throw(AMIBERRY_SETTINGS_INVALID_VALUE, [=] {
		std::vector<std::string>* recent = recent_list(list);
		if (!recent)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (recent->empty())
			return AMIBERRY_SETTINGS_OK;
		return store_recent(*recent, {});
	});
}

bool amiberry_media_grant_access(const char* path)
{
	return amiberry_settings::no_throw(false, [=] {
		return path && path[0] && macos_bookmark_store(path);
	});
}

} // extern "C"
