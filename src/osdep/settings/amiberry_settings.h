/*
 * Amiberry shared settings layer: the platform-neutral C API every settings
 * UI uses: ImGui, native front ends on macOS, iOS, Windows, Linux and
 * Android, and IPC.
 *
 * An option is identified by its .uae configuration key, and its values use
 * the .uae spelling, so "cpu_model" = "68030" means exactly what the line
 * cpu_model=68030 means in a configuration file.
 *
 * The API is plain C so any language with a C foreign-function interface can
 * bind it (Swift, C++/WinRT, C# P/Invoke, Kotlin/JNI, GTK or Qt). Strings
 * are NUL-terminated UTF-8; front ends that use UTF-16 convert at the
 * boundary. Pointers returned by the API stay valid for the lifetime of the
 * process unless a function says otherwise. bool is the C99 one-byte type
 * (C# needs [MarshalAs(UnmanagedType.I1)]).
 *
 * Descriptors are static and may be read from any thread. Every other call
 * must be made on the emulator's main thread: sessions read and write the
 * pending configuration and run the core's configuration fixups. A front end
 * with its own UI thread hands those calls to the emulator thread.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

/*
 * The core links the layer statically by default. A build that ships it in
 * a shared library for a separate front end defines AMIBERRY_SETTINGS_EXPORT
 * while compiling the library, and the front end defines
 * AMIBERRY_SETTINGS_IMPORT on Windows.
 */
#if defined(_WIN32)
#if defined(AMIBERRY_SETTINGS_EXPORT)
#define AMIBERRY_SETTINGS_API __declspec(dllexport)
#elif defined(AMIBERRY_SETTINGS_IMPORT)
#define AMIBERRY_SETTINGS_API __declspec(dllimport)
#else
#define AMIBERRY_SETTINGS_API
#endif
#elif defined(__GNUC__)
#define AMIBERRY_SETTINGS_API __attribute__((visibility("default")))
#else
#define AMIBERRY_SETTINGS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AMIBERRY_SETTINGS_API_VERSION 1

/* Large enough for every value a descriptor can produce, including the
   terminating NUL. */
#define AMIBERRY_SETTINGS_VALUE_MAX 64

typedef enum amiberry_setting_type {
	AMIBERRY_SETTING_BOOL,   /* "true" or "false" */
	AMIBERRY_SETTING_INT,    /* decimal integer within [min, max] */
	AMIBERRY_SETTING_REAL,   /* decimal number within [min, max] */
	AMIBERRY_SETTING_CHOICE  /* one of the descriptor's choice values */
} amiberry_setting_type;

typedef enum amiberry_apply_class {
	AMIBERRY_APPLY_LIVE,     /* picked up by the running emulation */
	AMIBERRY_APPLY_RESET,    /* takes effect at the next hard reset */
	AMIBERRY_APPLY_RESTART   /* takes effect the next time emulation starts */
} amiberry_apply_class;

typedef struct amiberry_setting_choice {
	const char *value;       /* .uae spelling */
	const char *label;       /* display text */
} amiberry_setting_choice;

typedef struct amiberry_setting_descriptor {
	const char *id;          /* .uae key, including any "amiberry." prefix */
	const char *group;       /* stable group ID, for example "cpu" or "jit" */
	const char *label;
	const char *help;
	amiberry_setting_type type;
	amiberry_apply_class apply;
	double min, max, step;   /* INT and REAL only; the current range can be narrower */
	const char *default_value;
	const amiberry_setting_choice *choices; /* CHOICE only */
	size_t choice_count;
	/* Why the emulator may change a value the user picked. Reported as the
	   reason of an adjustment found by amiberry_settings_validate(). */
	const char *constraint;
} amiberry_setting_descriptor;

AMIBERRY_SETTINGS_API size_t amiberry_settings_count(void);
/* NULL when index >= amiberry_settings_count(). */
AMIBERRY_SETTINGS_API const amiberry_setting_descriptor *amiberry_settings_at(size_t index);
/* NULL for an unknown ID. */
AMIBERRY_SETTINGS_API const amiberry_setting_descriptor *amiberry_settings_find(const char *id);

typedef enum amiberry_settings_status {
	AMIBERRY_SETTINGS_OK,
	AMIBERRY_SETTINGS_UNKNOWN_ID,
	AMIBERRY_SETTINGS_INVALID_VALUE,  /* not parseable, out of range or not a choice */
	AMIBERRY_SETTINGS_UNAVAILABLE,    /* the option or this choice is unavailable now */
	AMIBERRY_SETTINGS_BUFFER_TOO_SMALL
} amiberry_settings_status;

/*
 * An edit session works on a private copy of the pending configuration
 * (the one the emulator applies next). Setting a value applies the same
 * dependent changes the ImGui GUI makes, for example switching to a 68040
 * selects its internal FPU. Nothing reaches the emulator until commit.
 */
typedef struct amiberry_settings_session amiberry_settings_session;

/* NULL when memory is exhausted. */
AMIBERRY_SETTINGS_API amiberry_settings_session *amiberry_settings_session_open(void);
AMIBERRY_SETTINGS_API void amiberry_settings_session_close(amiberry_settings_session *session);

/*
 * The current value in .uae spelling. A CHOICE option can hold a value
 * outside its choice list when a configuration file set one.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_get(const amiberry_settings_session *session,
	const char *id, char *buffer, size_t size);
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_set(amiberry_settings_session *session,
	const char *id, const char *value);

AMIBERRY_SETTINGS_API bool amiberry_settings_is_available(const amiberry_settings_session *session, const char *id);
AMIBERRY_SETTINGS_API bool amiberry_settings_choice_is_available(const amiberry_settings_session *session,
	const char *id, const char *value);
/* The range an INT or REAL option accepts now; false for other types. */
AMIBERRY_SETTINGS_API bool amiberry_settings_get_range(const amiberry_settings_session *session, const char *id,
	double *min, double *max);

typedef struct amiberry_settings_adjustment {
	const char *id;
	char requested[AMIBERRY_SETTINGS_VALUE_MAX];
	char adjusted[AMIBERRY_SETTINGS_VALUE_MAX];
	const char *reason;
} amiberry_settings_adjustment;

/*
 * Runs the core's configuration fixups on a copy of the session and reports
 * every option they would change. Writes at most capacity entries and
 * returns the total number found; the session itself is not modified.
 */
AMIBERRY_SETTINGS_API size_t amiberry_settings_validate(const amiberry_settings_session *session,
	amiberry_settings_adjustment *adjustments, size_t capacity);

/*
 * Applies the adjustments amiberry_settings_validate() reports to the
 * session and stores it in the pending configuration. It does not signal a
 * running emulation. *changed tells whether any option differed from the
 * pending configuration; the result is the strongest apply class among
 * those that did (AMIBERRY_APPLY_LIVE when none did).
 */
AMIBERRY_SETTINGS_API amiberry_apply_class amiberry_settings_commit(amiberry_settings_session *session, bool *changed);

#ifdef __cplusplus
}
#endif
