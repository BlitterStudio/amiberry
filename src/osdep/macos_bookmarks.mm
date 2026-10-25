#ifdef MACOS_APP_STORE

#import <Foundation/Foundation.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include "macos_bookmarks.h"

extern void write_log(const char* format, ...);

// Module-local state
static NSMutableDictionary<NSString*, NSData*>* s_bookmark_data = nil;
static NSMutableDictionary<NSString*, NSURL*>*  s_active_urls = nil;
static NSString* s_bookmarks_path = nil;
static std::mutex s_mutex;

static void release_bookmark_data()
{
	if (s_bookmark_data)
	{
		[s_bookmark_data release];
		s_bookmark_data = nil;
	}
}

static void release_active_urls()
{
	if (!s_active_urls)
		return;

	for (NSString* key in s_active_urls)
	{
		[s_active_urls[key] stopAccessingSecurityScopedResource];
	}

	[s_active_urls release];
	s_active_urls = nil;
}

static NSString* get_bookmarks_plist_path(const std::string& directory_path)
{
	if (directory_path.empty())
		return nil;

	return [NSString stringWithUTF8String:(directory_path + "/bookmarks.plist").c_str()];
}

// Save bookmark data to disk
static bool save_bookmarks_plist()
{
	if (!s_bookmarks_path || !s_bookmark_data)
		return false;

	NSError* error = nil;
	NSData* plistData = [NSPropertyListSerialization dataWithPropertyList:s_bookmark_data
		format:NSPropertyListBinaryFormat_v1_0
		options:0
		error:&error];

	if (error)
	{
		write_log("Security bookmarks: failed to serialize plist: %s\n",
			[[error localizedDescription] UTF8String]);
		return false;
	}

	if (![plistData writeToFile:s_bookmarks_path options:NSDataWritingAtomic error:&error])
	{
		write_log("Security bookmarks: failed to write %s: %s\n",
			[s_bookmarks_path UTF8String],
			[[error localizedDescription] UTF8String]);
		return false;
	}
	return true;
}

// Read bookmark data from disk. The caller owns the returned dictionary.
static NSMutableDictionary<NSString*, NSData*>* read_bookmarks_plist_from_path(NSString* bookmarks_path)
{
	if (!bookmarks_path)
		return nil;
	if (![[NSFileManager defaultManager] fileExistsAtPath:bookmarks_path])
		return nil;

	NSData* plistData = [NSData dataWithContentsOfFile:bookmarks_path];
	if (!plistData)
	{
		write_log("Security bookmarks: failed to read %s\n", [bookmarks_path UTF8String]);
		return nil;
	}

	NSError* error = nil;
	id plist = [NSPropertyListSerialization propertyListWithData:plistData
		options:NSPropertyListMutableContainersAndLeaves
		format:nil
		error:&error];

	if (error || ![plist isKindOfClass:[NSMutableDictionary class]])
	{
		write_log("Security bookmarks: failed to parse %s: %s\n",
			[bookmarks_path UTF8String],
			error ? [[error localizedDescription] UTF8String] : "not a dictionary");
		return nil;
	}

	return [(NSMutableDictionary<NSString*, NSData*>*)plist retain];
}

static void ensure_bookmark_data_initialized()
{
	if (!s_bookmark_data)
		s_bookmark_data = [[NSMutableDictionary alloc] init];
}

// Resolve a single bookmark and start accessing it
static bool resolve_bookmark(NSString* key, NSData* data)
{
	BOOL isStale = NO;
	NSError* error = nil;

	NSURL* url = [NSURL URLByResolvingBookmarkData:data
		options:NSURLBookmarkResolutionWithSecurityScope
		relativeToURL:nil
		bookmarkDataIsStale:&isStale
		error:&error];

	if (error || !url)
	{
		write_log("Security bookmarks: failed to resolve bookmark for '%s': %s\n",
			[key UTF8String],
			error ? [[error localizedDescription] UTF8String] : "nil URL");
		return false;
	}

	if (![url startAccessingSecurityScopedResource])
	{
		write_log("Security bookmarks: startAccessingSecurityScopedResource failed for '%s'\n",
			[key UTF8String]);
		return false;
	}

	s_active_urls[key] = url;

	// Refresh stale bookmarks
	if (isStale)
	{
		write_log("Security bookmarks: refreshing stale bookmark for '%s'\n", [key UTF8String]);
		NSError* createError = nil;
		NSData* newData = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
			includingResourceValuesForKeys:nil
			relativeToURL:nil
			error:&createError];
		if (newData && !createError)
		{
			s_bookmark_data[key] = newData;
			save_bookmarks_plist();
		}
	}

	return true;
}

// The path's one spelling, so a bookmark is found however a caller names
// the path: a panel can say /tmp where the emulator says /private/tmp.
// Unchanged when the path does not exist.
static std::string canonical_path(const std::string& path)
{
	char resolved[PATH_MAX];
	return realpath(path.c_str(), resolved) ? std::string(resolved) : path;
}

// Normalize a path to a directory (if it's a file, use parent)
static std::string normalize_to_directory(const std::string& path)
{
	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	BOOL isDir = NO;
	if ([[NSFileManager defaultManager] fileExistsAtPath:nsPath isDirectory:&isDir])
	{
		if (!isDir)
		{
			// It's a file — use the parent directory
			return [[nsPath stringByDeletingLastPathComponent] UTF8String];
		}
	}
	// Remove trailing slash for consistency (unless it's root "/")
	std::string result = path;
	while (result.size() > 1 && result.back() == '/')
		result.pop_back();
	return result;
}

macos_bookmarks_migration_result macos_bookmarks_init(
	const std::string& settings_dir, const std::vector<std::string>& legacy_bookmarks_dirs)
{
	@autoreleasepool
	{
		auto migration_result = macos_bookmarks_migration_result::no_change;
		std::lock_guard<std::mutex> lock(s_mutex);

		[s_bookmarks_path release];
		s_bookmarks_path = [get_bookmarks_plist_path(settings_dir) retain];

		release_active_urls();
		s_active_urls = [[NSMutableDictionary alloc] init];

		release_bookmark_data();
		s_bookmark_data = read_bookmarks_plist_from_path(s_bookmarks_path);
		const bool loaded_current_store = s_bookmark_data != nil;
		ensure_bookmark_data_initialized();

		bool legacy_store_found = false;
		bool legacy_store_failed = false;
		NSUInteger imported_bookmarks = 0;
		for (const auto& legacy_bookmarks_dir : legacy_bookmarks_dirs)
		{
			const auto legacy_bookmarks_path = get_bookmarks_plist_path(legacy_bookmarks_dir);
			if (legacy_bookmarks_path == nil
				|| [legacy_bookmarks_path isEqualToString:s_bookmarks_path])
			{
				continue;
			}
			if (![[NSFileManager defaultManager] fileExistsAtPath:legacy_bookmarks_path])
				continue;

			auto* legacy_bookmark_data = read_bookmarks_plist_from_path(legacy_bookmarks_path);
			if (!legacy_bookmark_data)
			{
				legacy_store_failed = true;
				continue;
			}

			legacy_store_found = true;
			NSUInteger imported_from_store = 0;
			for (NSString* key in legacy_bookmark_data)
			{
				if (s_bookmark_data[key] != nil)
					continue;
				s_bookmark_data[key] = legacy_bookmark_data[key];
				++imported_from_store;
			}
			imported_bookmarks += imported_from_store;
			write_log("Security bookmarks: reconciled legacy store %s (%lu imported)\n",
				[legacy_bookmarks_path UTF8String], static_cast<unsigned long>(imported_from_store));
			[legacy_bookmark_data release];
		}

		bool store_saved = true;
		if (legacy_store_found && (!loaded_current_store || imported_bookmarks > 0))
			store_saved = save_bookmarks_plist();

		if (legacy_store_failed || !store_saved)
			migration_result = macos_bookmarks_migration_result::failed;
		else if (legacy_store_found)
			migration_result = macos_bookmarks_migration_result::migrated;

		int resolved = 0;
		int failed = 0;
		NSArray<NSString*>* keys = [s_bookmark_data allKeys];
		for (NSString* key in keys)
		{
			if (resolve_bookmark(key, s_bookmark_data[key]))
				resolved++;
			else
				failed++;
		}

		write_log("Security bookmarks: initialized (%d resolved, %d failed, %d total)\n",
			resolved, failed, (int)[keys count]);
		return migration_result;
	}
}

void macos_bookmarks_shutdown()
{
	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		release_active_urls();

		release_bookmark_data();

		[s_bookmarks_path release];
		s_bookmarks_path = nil;

		write_log("Security bookmarks: shutdown complete\n");
	}
}

// Whether an active bookmark covers the path: it is the bookmarked file or
// folder, or inside the folder. Called with s_mutex held.
static bool is_accessible_locked(NSString* nsPath)
{
	if (!s_active_urls)
		return false;
	for (NSString* key in s_active_urls)
	{
		// A path prefix, not just a string prefix.
		if ([nsPath hasPrefix:key]
			&& ([nsPath length] == [key length] || [nsPath characterAtIndex:[key length]] == '/'))
			return true;
	}
	return false;
}

// Creates, starts and saves a security-scoped bookmark of the file or
// folder at key. False, with the store (and its file) as it was, when the
// process cannot reach it or the bookmark cannot be saved. Called with
// s_mutex held.
static bool store_bookmark_locked(NSString* key, BOOL is_directory)
{
	// Already bookmarked and active: nothing to do.
	if (s_active_urls[key] != nil)
		return true;

	NSURL* url = [NSURL fileURLWithPath:key isDirectory:is_directory];
	if (!url)
	{
		write_log("Security bookmarks: invalid path '%s'\n", [key UTF8String]);
		return false;
	}

	NSError* error = nil;
	NSData* data = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
		includingResourceValuesForKeys:nil
		relativeToURL:nil
		error:&error];

	if (error || !data)
	{
		write_log("Security bookmarks: failed to create bookmark for '%s': %s\n",
			[key UTF8String],
			error ? [[error localizedDescription] UTF8String] : "nil data");
		return false;
	}

	// Start access through the bookmark itself, as a restart will: the
	// security scope belongs to the URL the bookmark resolves to. A path
	// this process cannot use is not kept. Done before anything is stored,
	// so a failure leaves the store (and its file) as it was.
	BOOL stale = NO;
	NSError* resolve_error = nil;
	NSURL* scoped = [NSURL URLByResolvingBookmarkData:data
		options:NSURLBookmarkResolutionWithSecurityScope
		relativeToURL:nil
		bookmarkDataIsStale:&stale
		error:&resolve_error];
	if (!scoped || ![scoped startAccessingSecurityScopedResource])
	{
		write_log("Security bookmarks: startAccess failed for new bookmark '%s'\n", [key UTF8String]);
		return false;
	}

	// A bookmark this call replaces (stored, but not active: it did not
	// resolve at startup) is put back when it cannot be saved. Copied,
	// not referenced: replacing it releases the dictionary's reference.
	NSData* stored = s_bookmark_data[key];
	NSData* previous = stored ? [NSData dataWithData:stored] : nil;
	s_bookmark_data[key] = data;
	// A bookmark that is not on disk is gone after a restart: callers
	// must not keep a path they could not reopen then. The file is
	// written atomically, so a failed write leaves it as it was.
	if (!save_bookmarks_plist())
	{
		write_log("Security bookmarks: could not save the bookmark for '%s'\n", [key UTF8String]);
		[scoped stopAccessingSecurityScopedResource];
		if (previous)
			s_bookmark_data[key] = previous;
		else
			[s_bookmark_data removeObjectForKey:key];
		return false;
	}
	s_active_urls[key] = scoped;

	write_log("Security bookmarks: stored bookmark for '%s'\n", [key UTF8String]);
	return true;
}

bool macos_bookmark_store(const std::string& requested)
{
	if (requested.empty())
		return false;
	const std::string path = canonical_path(requested);

	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (!s_bookmark_data || !s_bookmarks_path)
		{
			// Not initialized yet — silently ignore
			return false;
		}

		std::string dir_path = normalize_to_directory(path);
		if (dir_path.empty())
			return false;
		NSString* dir_key = [NSString stringWithUTF8String:dir_path.c_str()];
		NSString* ns_path = [NSString stringWithUTF8String:path.c_str()];
		BOOL is_directory = NO;
		const bool is_file = [[NSFileManager defaultManager] fileExistsAtPath:ns_path isDirectory:&is_directory]
			&& !is_directory;
		if (!is_file)
			return store_bookmark_locked(dir_key, YES);

		// A file. Picking one grants the sandbox that file only, so its
		// folder can be bookmarked only when the process already reaches
		// it (a folder picked before, or the app's own); the folder is
		// preferred, so files beside it (a CD's tracks) open too. Otherwise
		// the file itself is bookmarked.
		if (is_accessible_locked(ns_path))
			return true;
		if (store_bookmark_locked(dir_key, YES))
			return true;
		return store_bookmark_locked(ns_path, NO);
	}
}

bool macos_bookmark_is_accessible(const std::string& path)
{
	if (path.empty())
		return false;

	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		return is_accessible_locked([NSString stringWithUTF8String:canonical_path(path).c_str()]);
	}
}

std::vector<std::string> macos_bookmarks_list()
{
	std::vector<std::string> result;

	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (!s_bookmark_data)
			return result;

		for (NSString* key in s_bookmark_data)
		{
			result.push_back([key UTF8String]);
		}
	}

	return result;
}

macos_bookmarks_state macos_bookmarks_save_state()
{
	macos_bookmarks_state state;
	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		if (!s_bookmark_data || !s_active_urls)
			return state;
		for (NSString* key in s_bookmark_data)
		{
			NSData* data = s_bookmark_data[key];
			const auto* bytes = static_cast<const unsigned char*>(data.bytes);
			state.data[[key UTF8String]].assign(bytes, bytes + data.length);
		}
		for (NSString* key in s_active_urls)
			state.active.insert([key UTF8String]);
	}
	return state;
}

bool macos_bookmarks_restore(const macos_bookmarks_state& state)
{
	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		if (!s_bookmark_data || !s_active_urls || !s_bookmarks_path)
			return true;

		// Compared as C strings, so nothing here allocates on the C++ side
		// and the restore cannot stop halfway.
		for (NSString* key in [s_active_urls allKeys])
		{
			const char* name = [key UTF8String];
			const bool was_active = std::any_of(state.active.begin(), state.active.end(),
				[name](const std::string& active) { return std::strcmp(active.c_str(), name) == 0; });
			if (!was_active)
			{
				[s_active_urls[key] stopAccessingSecurityScopedResource];
				[s_active_urls removeObjectForKey:key];
			}
		}
		[s_bookmark_data removeAllObjects];
		for (const auto& [path, bytes] : state.data)
		{
			NSString* key = [NSString stringWithUTF8String:path.c_str()];
			s_bookmark_data[key] = [NSData dataWithBytes:bytes.data() length:bytes.size()];
		}
		if (!save_bookmarks_plist())
		{
			write_log("Security bookmarks: could not save the restored bookmarks\n");
			return false;
		}
		return true;
	}
}

void macos_bookmark_remove(const std::string& requested)
{
	if (requested.empty())
		return;
	const std::string path = canonical_path(requested);

	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (!s_bookmark_data || !s_bookmarks_path)
			return;

		// The key macos_bookmark_store() used for the same path: the file's
		// own when it was bookmarked alone, else its folder's.
		NSString* key = [NSString stringWithUTF8String:path.c_str()];
		if (!s_bookmark_data[key])
		{
			const std::string dir_path = normalize_to_directory(path);
			key = [NSString stringWithUTF8String:dir_path.c_str()];
		}

		// Stop accessing if active
		if (s_active_urls[key])
		{
			[s_active_urls[key] stopAccessingSecurityScopedResource];
			[s_active_urls removeObjectForKey:key];
		}

		[s_bookmark_data removeObjectForKey:key];
		save_bookmarks_plist();

		write_log("Security bookmarks: removed bookmark for '%s'\n", path.c_str());
	}
}

#endif // MACOS_APP_STORE
