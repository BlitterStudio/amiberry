#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

// The whole bookmark store (bookmark data by path, and the paths with
// access started), so a caller whose own commit fails after storing
// bookmarks can put the store back with macos_bookmarks_restore().
struct macos_bookmarks_state
{
	std::map<std::string, std::vector<unsigned char>> data;
	std::set<std::string> active;
};

enum class macos_bookmarks_migration_result
{
	no_change,
	// Every discovered legacy store was reconciled with the active store and
	// persisted when necessary, so callers may safely retire the legacy files.
	migrated,
	failed,
};

#ifdef MACOS_APP_STORE

// Initialize the bookmark system. Call once at startup after
// settings_dir is resolved but before accessing user-selected paths.
// Loads stored bookmarks from disk, optionally migrating from a legacy
// bookmark store, and starts security-scoped access.
macos_bookmarks_migration_result macos_bookmarks_init(
	const std::string& settings_dir, const std::vector<std::string>& legacy_bookmarks_dirs);

// Shut down: releases all security-scoped resource access.
// Call at application exit.
void macos_bookmarks_shutdown();

// Create and store a security-scoped bookmark for the given path.
// If the path is a file, bookmarks its parent directory.
// Returns true on success. Persists the bookmark data immediately.
bool macos_bookmark_store(const std::string& path);

// A copy of the bookmark store, for macos_bookmarks_restore().
macos_bookmarks_state macos_bookmarks_save_state();

// Puts the store back as it was when state was taken: ends access started
// since, restores the bookmark data and saves it. False when the restored
// store could not be written (the file keeps what it had).
bool macos_bookmarks_restore(const macos_bookmarks_state& state);

// Check whether the given path (or a file within a bookmarked directory)
// is currently accessible via an active security-scoped bookmark.
bool macos_bookmark_is_accessible(const std::string& path);

// Return a list of all bookmarked paths.
std::vector<std::string> macos_bookmarks_list();

// Remove a stored bookmark for the given path.
void macos_bookmark_remove(const std::string& path);

#else

// No-op stubs for non-App-Store builds.
// Callers don't need #ifdef guards.
inline macos_bookmarks_migration_result macos_bookmarks_init(
	const std::string&, const std::vector<std::string>&)
{
	return macos_bookmarks_migration_result::no_change;
}
inline void macos_bookmarks_shutdown() {}
inline bool macos_bookmark_store(const std::string&) { return true; }
inline macos_bookmarks_state macos_bookmarks_save_state() { return {}; }
inline bool macos_bookmarks_restore(const macos_bookmarks_state&) { return true; }
inline bool macos_bookmark_is_accessible(const std::string&) { return true; }
inline std::vector<std::string> macos_bookmarks_list() { return {}; }
inline void macos_bookmark_remove(const std::string&) {}

#endif
