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
 * whose names are not valid UTF-8 are left out of the lists. On Windows the
 * emulator opens paths through the wide file APIs, so any Unicode path works
 * whatever the active code page; a path the core holds in the ANSI code page
 * (older configurations) is returned as the UTF-8 of the same path, and one
 * those bytes cannot decode to is left out of the lists (and a folder
 * holding one reads as UNAVAILABLE).
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

#define AMIBERRY_MEDIA_API_VERSION 3

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

/* The state the iOS app saves in the save state folder as it goes to the
   background, where the system may end it; its settings UI offers to
   resume it. */
#define AMIBERRY_AUTOSAVE_STATE_NAME "Autosave.uss"

/*
 * The folder's path. UNKNOWN_ID for a folder outside the enum; UNAVAILABLE
 * when the stored path is not valid UTF-8 (set outside this API);
 * BUFFER_TOO_SMALL for a missing or too small buffer.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_folder_get(amiberry_folder folder,
	char *buffer, size_t size);

/*
 * Points the folder at path and saves the emulator's settings file, as the
 * Paths panel does; on a Mac App Store build the folder is also granted
 * (amiberry_media_grant_access) first. INVALID_VALUE for an empty path, an
 * existing file that is not a directory, or a path the settings file
 * cannot store as written: longer than the host's path limit (MAX_PATH on
 * Windows) less one, for the separator the folder gets, or 2495 bytes,
 * with a control character, or with a space at either end (outside
 * Windows also a backslash at the end). UNAVAILABLE, with the folder
 * unchanged and its new access grant withdrawn, when access to the folder
 * cannot be kept or the settings file cannot be written (the grant stays
 * only if the bookmark file cannot be written either).
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
	                                  effect: the folder of a configuration's
	                                  statefile_path (a state file) overrides
	                                  AMIBERRY_FOLDER_SAVESTATES */
	AMIBERRY_MEDIA_KICKSTART_ROMS, /* the Kickstart ROMs the last ROM scan
	                                  recognised (amiberry_media_rescan_roms),
	                                  named as the ROM list names them
	                                  ("KS ROM v3.1 (A1200)"), by name; the
	                                  path is what kickstart_rom_file takes */
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
 * and saves the emulator's settings. INVALID_VALUE for another list, a path
 * the settings file cannot store as written, a folder, FIFO or socket, or a
 * file that cannot be opened (the settings file keeps only those);
 * UNAVAILABLE, with the list unchanged, when the settings file cannot be
 * written.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_add_recent(amiberry_media_list list,
	const char *path);

/*
 * Takes path off a recent list (floppies, CD images, WHDLoad archives), as
 * a front end's Remove from Recent does, and saves the emulator's
 * settings. path is matched the way the list's entries read: made
 * absolute, so the path amiberry_media_entry gave removes its entry; the
 * other entries keep their order. A path the list does not hold changes
 * nothing. INVALID_VALUE for another list or an empty path; UNAVAILABLE,
 * with the list unchanged, when the settings file cannot be written.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_remove_recent(amiberry_media_list list,
	const char *path);

/*
 * Empties a recent list (floppies, CD images, WHDLoad archives), leaving
 * the other lists as they are, and saves the emulator's settings.
 * INVALID_VALUE for another list; UNAVAILABLE, with the list unchanged,
 * when the settings file cannot be written.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_clear_recent(amiberry_media_list list);

/*
 * Keeps access to a file or folder the user picked: on a Mac App Store
 * build it stores a security-scoped bookmark (for a file, of its folder
 * when the app can reach that, else of the file) so the path stays usable
 * after a restart. Elsewhere there is nothing to keep
 * and it returns true. False when the bookmark could not be stored.
 */
AMIBERRY_SETTINGS_API bool amiberry_media_grant_access(const char *path);

/*
 * The description and category a configuration file declares
 * (config_description, config_category), read from its header without
 * loading it, as the configurations list shows them. Either buffer may be
 * NULL with size 0. UNKNOWN_ID for a file that cannot be read,
 * BUFFER_TOO_SMALL when a value does not fit (up to 256 bytes each).
 */
/*
 * The configuration file the pending configuration was loaded from or last
 * saved to (by the load and save actions, the GUI or the command line), as
 * a full path; empty for a default configuration or an RP9 package.
 * BUFFER_TOO_SMALL when it does not fit.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_current_configuration(char *path, size_t size);

/*
 * Forgets that file: the pending configuration becomes a new machine (one
 * started from a model), which is no saved file's until it is saved. The
 * names taken from it go too (the window title, the name saving suggests).
 */
AMIBERRY_SETTINGS_API void amiberry_media_forget_current_configuration(void);

/*
 * Rescans the ROMs folder, as the GUI's Rescan ROMs does, after ROM files
 * were added or removed: the presets' availability
 * (amiberry_settings_preset_rom_installed) follows. Setting the ROMs folder
 * rescans it too. Returns the number of ROMs the scan recognised. Main
 * thread.
 */
AMIBERRY_SETTINGS_API int amiberry_media_rescan_roms(void);

/*
 * Adds a ROM key file (Amiga Forever's rom.key, or another set's key kept
 * beside it under another name) to the keys the next rescan decodes
 * encrypted ROMs with; a missing file or a known key changes nothing. Main
 * thread.
 */
AMIBERRY_SETTINGS_API void amiberry_media_add_rom_key(const char *path);

AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_configuration_info(const char *path,
	char *description, size_t description_size, char *category, size_t category_size);

/*
 * Renames the configuration file path to new_path, as a front end's Rename
 * does, without reading or changing the pending configuration: the file's
 * contents are untouched. Both must name a .uae file in the same folder (a
 * rename, not a move) and be paths the API accepts (see
 * amiberry_folder_set: not empty, shorter than the host's path limit, no
 * control characters or space at either end, UTF-8, and on Windows
 * within MAX_PATH UTF-16 units); relative ones are made absolute.
 * An existing file of the new name is never replaced, and the file is
 * renamed in one step without copying: MoveFileExW without replacing on
 * Windows; renameat2 with RENAME_NOREPLACE, renamex_np with RENAME_EXCL or,
 * where those are unsupported, link then unlink elsewhere. A file system
 * with none of them (exFAT, FAT outside Windows) gets UNAVAILABLE rather
 * than an unsafe rename. On Windows (the system's ordinal case folding, so
 * École to école works) and macOS (ASCII case) a new_path that differs from
 * path only by case is renamed to that spelling; the same path is a no-op
 * returning OK. If the pending configuration is path
 * (amiberry_media_current_configuration), the saved association follows to
 * new_path, with the names the title and the save name suggestion take
 * from it, so the next save writes new_path and not path again. Main
 * thread.
 * UNKNOWN_ID when path does not exist; INVALID_VALUE for a path the API
 * does not accept, another extension, different folders (or a missing
 * folder), a path that is not a regular file, or any existing file or link
 * of the new name other than that case-only spelling; UNAVAILABLE when the
 * file system refuses or cannot do a no-replace rename, or cannot be
 * examined. On any failure nothing changed.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_rename_configuration(const char *path,
	const char *new_path);

/*
 * Creates an empty hardfile of size_bytes at path, as the hard drive
 * panel's Create Hardfile does: a raw image, or with dynamic_vhd a dynamic
 * VHD that grows as it is written (a path ending in .hdf gets .vhd
 * instead; the path written is copied into created). An existing file is
 * never overwritten. INVALID_VALUE for a size below 1 MiB or an existing
 * path, UNAVAILABLE when the file cannot be written (any partial file is
 * removed), BUFFER_TOO_SMALL for a missing or too small created buffer.
 * A raw image touches no emulator state and may be created on any thread
 * (writing gigabytes takes a while); a dynamic VHD, which is small and
 * written through the core's file layer, only on the main thread.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_media_create_hardfile(const char *path,
	unsigned long long size_bytes, bool dynamic_vhd, char *created, size_t created_size);

#ifdef __cplusplus
}
#endif

#endif /* AMIBERRY_MEDIA_H */
