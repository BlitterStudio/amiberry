/*
 * Amiberry WHDBoot tools API.
 *
 * WHDLoad (by Bert Jahn, whdload.de), the kickstart RTB files (Aminet,
 * skick346) and JST (Aminet) are not bundled with Amiberry: their licences
 * do not permit commercial redistribution. This API tells a front end
 * which of them are installed and downloads or updates them from their
 * authors' distribution points with progress and cancellation, as the
 * Paths panel's "Update WHDBooter files" button does.
 *
 * Plain C99, like amiberry_media.h. Reading the status is cheap and may
 * happen from any thread. The download blocks its calling thread (front
 * ends start it off their main thread) and reports progress from one of
 * the implementation's own worker threads - never assume the calling
 * thread. Strings are UTF-8; progress.label is only valid during the
 * callback. The API belongs to the standalone emulator; the libretro core
 * does not build it.
 */

#ifndef AMIBERRY_WHDBOOT_H
#define AMIBERRY_WHDBOOT_H

#include "amiberry_settings.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMIBERRY_WHDBOOT_API_VERSION 1

typedef struct amiberry_whdboot_status {
	bool whdload_installed;   /* WHDLoad, from whdload.de */
	bool jst_installed;       /* JST, from Aminet */
	bool rtb_files_installed; /* every kickstart RTB file the booter links */
	bool amiquit_installed;
	bool boot_data_installed;
	bool database_installed;
	bool downloads_available; /* false where nothing can download (libretro) */
	bool emulation_running;   /* a machine runs unpaused: downloads wait */
} amiberry_whdboot_status;

/* Which of the on-demand WHDBoot tools are installed in the WHDBoot folder. */
AMIBERRY_SETTINGS_API void amiberry_whdboot_status_get(amiberry_whdboot_status *status);

typedef struct amiberry_whdboot_progress {
	int step;
	int total_steps;
	const char *label; /* only valid during the callback */
	int64_t bytes_downloaded;
	int64_t bytes_total;
} amiberry_whdboot_progress;

/* Non-zero cancels the download. */
typedef int (*amiberry_whdboot_progress_fn)(const amiberry_whdboot_progress *progress, void *user);

typedef enum amiberry_whdboot_download_result {
	AMIBERRY_WHDBOOT_DOWNLOAD_OK,
	AMIBERRY_WHDBOOT_DOWNLOAD_PARTIAL,
	AMIBERRY_WHDBOOT_DOWNLOAD_FAILED,
	AMIBERRY_WHDBOOT_DOWNLOAD_CANCELLED,
	AMIBERRY_WHDBOOT_DOWNLOAD_UNAVAILABLE,
	/* Another download is running, or the machine is running. */
	AMIBERRY_WHDBOOT_DOWNLOAD_BUSY
} amiberry_whdboot_download_result;

/*
 * Downloads or updates the WHDBoot tools: WHDLoad is always re-fetched
 * from whdload.de; missing RTB files and a selected JST come from Aminet;
 * AmiQuit, boot-data.zip and the WHDLoad game database come from the
 * Amiberry project's GitHub repository. Existing RTB files and JST are
 * never overwritten.
 */
AMIBERRY_SETTINGS_API amiberry_whdboot_download_result amiberry_whdboot_download(
	amiberry_whdboot_progress_fn progress, void *user);

#ifdef __cplusplus
}
#endif

#endif /* AMIBERRY_WHDBOOT_H */
