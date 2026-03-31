#pragma once

#include <string>
#include <vector>

#ifdef MACOS_APP_STORE

// Initialize the bookmark system. Call once at startup after
// settings_dir is resolved but before accessing user-selected paths.
// Loads stored bookmarks from disk, optionally migrating from a legacy
// bookmark store, and starts security-scoped access.
void macos_bookmarks_init(const std::string& settings_dir, const std::vector<std::string>& legacy_bookmarks_dirs);

// Shut down: releases all security-scoped resource access.
// Call at application exit.
void macos_bookmarks_shutdown();

// Create and store a security-scoped bookmark for the given path.
// If the path is a file, bookmarks its parent directory.
// Returns true on success. Persists the bookmark data immediately.
bool macos_bookmark_store(const std::string& path);

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
inline void macos_bookmarks_init(const std::string&, const std::vector<std::string>&) {}
inline void macos_bookmarks_shutdown() {}
inline bool macos_bookmark_store(const std::string&) { return true; }
inline bool macos_bookmark_is_accessible(const std::string&) { return true; }
inline std::vector<std::string> macos_bookmarks_list() { return {}; }
inline void macos_bookmark_remove(const std::string&) {}

#endif
