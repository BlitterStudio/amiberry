#ifdef MACOS_APP_STORE

#import <Foundation/Foundation.h>
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

// Save bookmark data to disk
static void save_bookmarks_plist()
{
	if (!s_bookmarks_path || !s_bookmark_data)
		return;

	NSError* error = nil;
	NSData* plistData = [NSPropertyListSerialization dataWithPropertyList:s_bookmark_data
		format:NSPropertyListBinaryFormat_v1_0
		options:0
		error:&error];

	if (error)
	{
		write_log("Security bookmarks: failed to serialize plist: %s\n",
			[[error localizedDescription] UTF8String]);
		return;
	}

	if (![plistData writeToFile:s_bookmarks_path options:NSDataWritingAtomic error:&error])
	{
		write_log("Security bookmarks: failed to write %s: %s\n",
			[s_bookmarks_path UTF8String],
			[[error localizedDescription] UTF8String]);
	}
}

// Load bookmark data from disk
static void load_bookmarks_plist()
{
	if (!s_bookmarks_path)
		return;

	if (![[NSFileManager defaultManager] fileExistsAtPath:s_bookmarks_path])
	{
		s_bookmark_data = [[NSMutableDictionary alloc] init];
		return;
	}

	NSData* plistData = [NSData dataWithContentsOfFile:s_bookmarks_path];
	if (!plistData)
	{
		write_log("Security bookmarks: failed to read %s\n", [s_bookmarks_path UTF8String]);
		s_bookmark_data = [[NSMutableDictionary alloc] init];
		return;
	}

	NSError* error = nil;
	id plist = [NSPropertyListSerialization propertyListWithData:plistData
		options:NSPropertyListMutableContainersAndLeaves
		format:nil
		error:&error];

	if (error || ![plist isKindOfClass:[NSMutableDictionary class]])
	{
		write_log("Security bookmarks: failed to parse plist: %s\n",
			error ? [[error localizedDescription] UTF8String] : "not a dictionary");
		s_bookmark_data = [[NSMutableDictionary alloc] init];
		return;
	}

	s_bookmark_data = (NSMutableDictionary<NSString*, NSData*>*)plist;
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

void macos_bookmarks_init(const std::string& app_support_dir)
{
	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		s_bookmarks_path = [[NSString stringWithUTF8String:
			(app_support_dir + "/bookmarks.plist").c_str()] retain];
		s_active_urls = [[NSMutableDictionary alloc] init];

		load_bookmarks_plist();

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
	}
}

void macos_bookmarks_shutdown()
{
	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (s_active_urls)
		{
			for (NSString* key in s_active_urls)
			{
				[s_active_urls[key] stopAccessingSecurityScopedResource];
			}
			[s_active_urls release];
			s_active_urls = nil;
		}

		[s_bookmark_data release];
		s_bookmark_data = nil;

		[s_bookmarks_path release];
		s_bookmarks_path = nil;

		write_log("Security bookmarks: shutdown complete\n");
	}
}

bool macos_bookmark_store(const std::string& path)
{
	if (path.empty())
		return false;

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

		NSString* key = [NSString stringWithUTF8String:dir_path.c_str()];

		// Already bookmarked and active — skip
		if (s_active_urls[key] != nil)
			return true;

		NSURL* url = [NSURL fileURLWithPath:key isDirectory:YES];
		if (!url)
		{
			write_log("Security bookmarks: invalid path '%s'\n", dir_path.c_str());
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
				dir_path.c_str(),
				error ? [[error localizedDescription] UTF8String] : "nil data");
			return false;
		}

		s_bookmark_data[key] = data;
		save_bookmarks_plist();

		// Start access immediately
		if (![url startAccessingSecurityScopedResource])
		{
			write_log("Security bookmarks: startAccess failed for new bookmark '%s'\n",
				dir_path.c_str());
		}
		else
		{
			s_active_urls[key] = url;
		}

		write_log("Security bookmarks: stored bookmark for '%s'\n", dir_path.c_str());
		return true;
	}
}

bool macos_bookmark_is_accessible(const std::string& path)
{
	if (path.empty())
		return false;

	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (!s_active_urls)
			return false;

		NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];

		for (NSString* key in s_active_urls)
		{
			if ([nsPath hasPrefix:key])
			{
				// Ensure it's a proper path prefix (not just a string prefix)
				if ([nsPath length] == [key length] ||
					[nsPath characterAtIndex:[key length]] == '/')
				{
					return true;
				}
			}
		}
		return false;
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

void macos_bookmark_remove(const std::string& path)
{
	if (path.empty())
		return;

	@autoreleasepool
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (!s_bookmark_data || !s_bookmarks_path)
			return;

		NSString* key = [NSString stringWithUTF8String:path.c_str()];

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
