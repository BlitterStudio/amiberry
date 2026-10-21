/*
 * Amiberry media and folders API.
 *
 * Plain C99, like amiberry_settings.h, for native front ends on every
 * platform (Swift, C#, Kotlin/JNI) and the emulator's own UI:
 *
 * - the folders the emulator uses (ROMs, floppies, CDs, hard drives,
 *   configurations, save states, screenshots, WHDLoad archives), with their
 *   paths read and changed the way the Paths panel does it;
 * - media lists a front end offers for picking: recently used floppies, CD
 *   images and WHDLoad archives, the host's CD drives, the configuration
 *   files and the save states in their folders;
 * - sandbox access: on a Mac App Store build a file or folder the user
 *   picked must be granted to be usable after a restart.
 *
 * Every call runs on the emulator's main thread, like the settings session
 * calls. Strings are UTF-8 and copied into caller buffers; a buffer that is
 * too small gets AMIBERRY_SETTINGS_BUFFER_TOO_SMALL and is left empty. Files
 * whose names are not valid UTF-8 are left out of the lists.
 *
 * The API belongs to the standalone emulator: the libretro core does not
 * build it, because a libretro front end owns its folders and media.
 */

#ifndef AMIBERRY_MEDIA_H
#define AMIBERRY_MEDIA_H

#include "amiberry_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AMIBERRY_MEDIA_API_VERSION 1

typedef enum amiberry_folder {
	AMIBERRY_FOLDER_ROMS,
	AMIBERRY_FOLDER_FLOPPIES,
	AMIBERRY_FOLDER_CDS,
	AMIBERRY_FOLDER_HARD_DRIVES,
	AMIBERRY_FOLDER_CONFIGURATIONS,
	AMIBERRY_FOLDER_SAVESTATES,
	AMIBERRY_FOLDER_SCREENSHOTS,
	AMIBERRY_FOLDER_WHDLOAD_ARCHIVES,
	AMIBERRY_FOLDER_COUNT
} amiberry_folder;

/* The folder's path. UNKNOWN_ID for a folder outside the enum. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_folder_get(amiberry_folder folder,
	char *buffer, size_t size);

/*
 * Points the folder at path and saves the emulator's settings file, as the
 * Paths panel does; on a Mac App Store build the folder is also granted
 * (amiberry_media_grant_access) first. INVALID_VALUE for an empty path or
 * one the settings file cannot store as written (longer than the host's
 * path limit or 2495 bytes, with a control character, or with a space at
 * either end); UNAVAILABLE, with nothing changed,
 * when access to the folder cannot be kept or the settings file cannot be
 * written.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_folder_set(amiberry_folder folder,
	const char *path);

typedef enum amiberry_media_list {
	AMIBERRY_MEDIA_RECENT_FLOPPIES,
	AMIBERRY_MEDIA_RECENT_CDS,
	AMIBERRY_MEDIA_RECENT_WHDLOAD,
	AMIBERRY_MEDIA_CD_DRIVES,
	AMIBERRY_MEDIA_CONFIGURATIONS, /* .uae files in the configurations folder */
	AMIBERRY_MEDIA_SAVESTATES,     /* .uss files in the save states folder in
	                                  effect: a configuration's statefile_path
	                                  overrides AMIBERRY_FOLDER_SAVESTATES */
	AMIBERRY_MEDIA_LIST_COUNT
} amiberry_media_list;

/*
 * Takes a snapshot of the list and returns its number of entries; the
 * recent lists are most recent first, the folder lists sorted by name.
 * amiberry_media_entry reads from the last snapshot of that list.
 */
AMIBERRY_SETTINGS_API size_t amiberry_media_refresh(amiberry_media_list list);

/*
 * Entry index of the list's last snapshot: its full path, and a name to
 * show (the file name without its folder, or the drive's name). Either
 * buffer may be NULL with size 0. UNKNOWN_ID for a list or index outside
 * the snapshot.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_entry(amiberry_media_list list,
	size_t index, char *path, size_t path_size, char *name, size_t name_size);

/*
 * Records path (made absolute) as the most recent entry of a recent list
 * (floppies, CD images, WHDLoad archives), as the panels do after a pick,
 * and saves the emulator's settings. INVALID_VALUE for another list or a path the folder
 * setter would refuse; UNAVAILABLE, with the list unchanged, when the
 * settings file cannot be written.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_add_recent(amiberry_media_list list,
	const char *path);

/*
 * Keeps access to a file or folder the user picked: on a Mac App Store
 * build it stores a security-scoped bookmark (for a file, of its folder) so
 * the path stays usable after a restart. Elsewhere there is nothing to keep
 * and it returns true. False when the bookmark could not be stored.
 */
AMIBERRY_SETTINGS_API bool amiberry_media_grant_access(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* AMIBERRY_MEDIA_H */
