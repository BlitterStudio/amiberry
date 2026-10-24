/*
 * Amiberry runtime actions and status: the platform-neutral C API for
 * everything a front end does while the emulator runs, as opposed to the
 * configuration editing covered by amiberry_settings.h.
 *
 * Actions are the operations the built-in GUI and the IPC transport
 * expose today: inserting and ejecting media, disk swapper control, resets,
 * pause and resume, save states, screenshots and quitting. They can be
 * posted from ANY thread: native front ends have their own UI threads
 * (SwiftUI, WinUI, GTK), and the emulator must never be driven from
 * anywhere but its main thread. Posted actions wait in a queue that the
 * emulator's main thread drains in order, between frames, while the GUI is
 * open, and while a GDB client or the native debugger has the CPU stopped
 * (the debugger window, or its terminal console while IPC is enabled; a
 * terminal console without IPC blocks on its input).
 *
 * Status is a snapshot of the running emulator (run state, drive LEDs and
 * activity, inserted media) that the main thread refreshes at every drain
 * point under a lock; it can be copied from any thread at any time.
 *
 * Strings are NUL-terminated UTF-8. bool is the C99 one-byte type. The
 * header is strict C99 so every language with a C foreign-function
 * interface can bind it.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "amiberry_settings.h"

/*
 * Same export convention as amiberry_settings.h: a build that ships the
 * actions in a shared library defines AMIBERRY_ACTIONS_EXPORT while
 * compiling it, and the front end defines AMIBERRY_ACTIONS_IMPORT on
 * Windows.
 */
#if defined(_WIN32)
#if defined(AMIBERRY_ACTIONS_EXPORT)
#define AMIBERRY_ACTIONS_API __declspec(dllexport)
#elif defined(AMIBERRY_ACTIONS_IMPORT)
#define AMIBERRY_ACTIONS_API __declspec(dllimport)
#else
#define AMIBERRY_ACTIONS_API
#endif
#elif defined(__GNUC__)
#define AMIBERRY_ACTIONS_API __attribute__((visibility("default")))
#else
#define AMIBERRY_ACTIONS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AMIBERRY_ACTIONS_API_VERSION 7

/* Largest MAX_DPATH (PATH_MAX) any supported platform builds with, so every
   media path the emulator can store fits, NUL included. */
#define AMIBERRY_ACTIONS_PATH_MAX 4096
#define AMIBERRY_ACTIONS_FLOPPY_DRIVES 4
#define AMIBERRY_ACTIONS_SWAPPER_ENTRIES 20

typedef enum amiberry_run_state {
	AMIBERRY_RUN_STATE_STOPPED = 0, /* no emulation is running */
	AMIBERRY_RUN_STATE_RUNNING = 1,
	AMIBERRY_RUN_STATE_PAUSED = 2   /* host pause, not a guest program */
} amiberry_run_state;

/* Returned by amiberry_actions_poll(). */
typedef enum amiberry_action_result {
	AMIBERRY_ACTION_PENDING = 0, /* queued or executing */
	AMIBERRY_ACTION_DONE = 1,    /* finished successfully */
	AMIBERRY_ACTION_FAILED = 2,  /* finished with an error */
	AMIBERRY_ACTION_UNKNOWN = 3  /* unknown id, or result already retired */
} amiberry_action_result;

typedef struct amiberry_drive_status {
	bool motor_on;          /* drive LED: the motor is spinning */
	bool writing;           /* motor_on and writing data */
	bool disabled;          /* the drive is disabled */
	bool media_requested;   /* the pending configuration inserts media */
	char media_path[AMIBERRY_ACTIONS_PATH_MAX]; /* pending media path, "" when none */
} amiberry_drive_status;

typedef struct amiberry_actions_status {
	amiberry_run_state run_state;
	bool paused;            /* the raw pause flag, as GET_STATUS reports it */
	bool power_led;
	bool caps_lock;
	int hd_activity;        /* hard drive LED level, -1 when there is none */
	int cd_activity;        /* CD LED level, -1 when there is none */
	int fps_tenths;         /* frames per second in tenths, as the status line shows them */
	amiberry_drive_status floppy[AMIBERRY_ACTIONS_FLOPPY_DRIVES];
	bool cd_requested;      /* the pending configuration inserts a CD */
	char cd_path[AMIBERRY_ACTIONS_PATH_MAX]; /* pending CD path, "" when none */
	char config_description[256]; /* pending configuration description */
	unsigned revision;      /* increased by every main-thread refresh */
} amiberry_actions_status;

/*
 * Post a runtime action. Callable from any thread. Actions execute in
 * posting order on the emulator's main thread; each makes exactly the
 * changes the built-in GUI and the IPC transport make today.
 *
 * The return value identifies the action for amiberry_actions_poll() and is
 * non-zero unless the action was rejected on the spot: an argument out of
 * range, a NULL path or one too long for the emulator, or a queue already
 * holding AMIBERRY_ACTIONS_MAX_PENDING actions (the main thread is not
 * draining). Rejected actions never execute. An empty path is accepted:
 * for a floppy or CD it empties the drive, as the IPC commands always did;
 * for a state file or screenshot the action fails when it runs.
 *
 * reset: hard resets like a power cycle, keyboard like the Ctrl-Amiga-Amiga
 * key combination. save_state/load_state take a state file path;
 * save_state_slot/load_state_slot use the quick-save slots 0-9.
 */
#define AMIBERRY_ACTIONS_MAX_PENDING 1024

typedef uint64_t amiberry_action_id;

AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_insert_floppy(int drive, const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_eject_floppy(int drive);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_set_swapper_entry(int index, const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_insert_swapper_entry(int index, int drive);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_insert_cd(const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_eject_cd(void);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_reset(bool hard, bool keyboard);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_pause(void);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_resume(void);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_save_state(const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_load_state(const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_save_state_slot(int slot);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_load_state_slot(int slot);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_screenshot(const char *path);

/*
 * Configuration files, as the GUI's Configurations panel: load_config makes
 * a .uae (or .rp9) file the pending configuration, inserting the floppies it
 * names at once; the rest takes effect when the emulation next starts, or
 * with amiberry_actions_apply_settings(AMIBERRY_APPLY_RESTART). FAILED for
 * a file that is missing or not a configuration, which leaves the pending
 * configuration as it was. save_config writes the pending configuration
 * (what amiberry_settings_commit stored) to path.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_load_config(const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_save_config(const char *path);

/*
 * Quick play: makes the machine for a media file the pending configuration,
 * as dropping it on the emulator does. A .uae or .rp9 loads as with
 * load_config; a WHDLoad .lha gets the WHDLoad booter's machine; a CD image
 * (.cue, .iso, .chd) a CD32 or CDTV; a disk image (.adf, .adz, .dms, .ipf,
 * .zip, .7z) the configuration of the same name in the configurations
 * folder, or else the Amiga 500 model with the disk in DF0:. Except for a
 * configuration, the machine is a new one: nothing is its configuration
 * file until it is saved. It runs when the emulation next starts, or after
 * amiberry_actions_apply_settings(AMIBERRY_APPLY_RESTART). FAILED for a file
 * that is missing or of another type, and in the libretro core.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_quick_play(const char *path);
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_quit(void);

/*
 * Stop the running emulation and open the settings UI with nothing running
 * (the Library in the Apple apps, the built-in GUI elsewhere), as dropping a
 * configuration on the GUI does. The running configuration stays the
 * pending one. FAILED while nothing runs, while the emulator quits, and in
 * the libretro core, whose front end owns the emulation's lifetime.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_stop(void);

/*
 * Brings the running emulation up to the pending configuration (what
 * amiberry_settings_commit stored) the way the apply class commit returned
 * needs: AMIBERRY_APPLY_LIVE has the emulation pick the changes up between
 * frames; AMIBERRY_APPLY_RESET also attaches the committed mount list and
 * hard-resets, as the GUI's Reboot does; AMIBERRY_APPLY_RESTART restarts
 * the emulation with the pending configuration, without the built-in GUI.
 * DONE at once while no emulation runs (it starts with the pending
 * configuration); FAILED for another value or while the emulator is
 * quitting.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_apply_settings(amiberry_apply_class apply);

/*
 * Switches the emulation display between window and full screen, as the
 * full-screen hotkey does. FAILED in a libretro core, whose front end owns
 * the display.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_toggle_fullscreen(void);

/*
 * Opens the settings UI (the built-in GUI, or the native one where a front
 * end provides it) over a running emulation, as the GUI hotkey does; it
 * opens at the next frame. DONE at once while the settings UI is already
 * open or no emulation runs (the settings UI is what shows then). FAILED in
 * a libretro core.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_open_settings(void);

/* What amiberry_actions_choose_media asks the user for. */
typedef enum amiberry_media_request {
	AMIBERRY_REQUEST_FLOPPY0 = 0, /* a disk image for DF0: ... */
	AMIBERRY_REQUEST_FLOPPY1,
	AMIBERRY_REQUEST_FLOPPY2,
	AMIBERRY_REQUEST_FLOPPY3,     /* ... DF3: */
	AMIBERRY_REQUEST_LOAD_STATE,  /* a state file to load */
	AMIBERRY_REQUEST_SAVE_STATE,  /* where to save the state */
	AMIBERRY_REQUEST_CD           /* a CD image */
} amiberry_media_request;

/*
 * Asks the user for a file over a running emulation, as the media and state
 * hotkeys do, and uses it: the emulation pauses with input released while
 * the requester shows, at the next frame. DONE once requested (and at once
 * while no emulation runs, when it does nothing); 0 for a request outside
 * the enum. FAILED while the settings UI is open (it has the media then)
 * and in a libretro core.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_choose_media(amiberry_media_request request);

/* Largest Amiga key code (raw keyboard code, 0x00-0x7F). */
#define AMIBERRY_ACTIONS_MAX_AMIGA_KEY 0x7F

/*
 * Presses or releases a key of the emulated keyboard, by Amiga raw key code
 * (0x00-0x7F, as the keyboard sends it: 0x45 Esc, 0x60 left Shift ...), as an
 * on-screen keyboard does. A press reaches the emulation as the next keyboard
 * event; every press needs its release. 0 for a code outside the range.
 * A press FAILS while no emulation runs or the settings UI is open; a
 * release always succeeds, so no key stays down. FAILED in a libretro core,
 * whose front end owns the keyboard.
 */
AMIBERRY_ACTIONS_API amiberry_action_id amiberry_actions_key(int amiga_key, bool pressed);

/*
 * Ask what became of a posted action. Callable from any thread; polling
 * CONSUMES the result, so exactly one poll sees DONE or FAILED and later
 * polls see AMIBERRY_ACTION_UNKNOWN. Results are kept for the most recent
 * AMIBERRY_ACTIONS_MAX_PENDING completions, then retired. On FAILED the
 * error text (UTF-8, possibly empty) is copied into error_buffer,
 * truncated to fit; error_buffer may be NULL.
 */
AMIBERRY_ACTIONS_API amiberry_action_result amiberry_actions_poll(amiberry_action_id id,
	char *error_buffer, size_t size);

/*
 * Main thread only: execute the queued actions in order and refresh the
 * status snapshot. The emulator calls this between frames and while the
 * GUI is open; transports that already run on the main thread call it to
 * execute what they just posted. Returns the number of actions executed.
 */
AMIBERRY_ACTIONS_API size_t amiberry_actions_run_pending(void);

/* Copy the current status snapshot. Callable from any thread. */
AMIBERRY_ACTIONS_API void amiberry_actions_get_status(amiberry_actions_status *out);

/*
 * Copy the pending disk-swapper entry `index`
 * (0..AMIBERRY_ACTIONS_SWAPPER_ENTRIES-1); "" when the entry is unset.
 * Callable from any thread.
 */
AMIBERRY_ACTIONS_API void amiberry_actions_get_swapper_entry(int index,
	char *buffer, size_t size);

#ifdef __cplusplus
}
#endif
