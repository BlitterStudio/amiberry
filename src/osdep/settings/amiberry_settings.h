/*
 * Amiberry shared settings layer: the platform-neutral C API every settings
 * UI uses (ImGui, native Apple and Android front ends, IPC).
 *
 * An option is identified by its .uae configuration key, and its values use
 * the .uae spelling, so "cpu_model" = "68030" means exactly what the line
 * cpu_model=68030 means in a configuration file.
 *
 * Descriptors are static and may be read from any thread. Every other call
 * must be made on the emulator's main thread: sessions read and write the
 * pending configuration and run the core's configuration fixups.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

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

size_t amiberry_settings_count(void);
/* NULL when index >= amiberry_settings_count(). */
const amiberry_setting_descriptor *amiberry_settings_at(size_t index);
/* NULL for an unknown ID. */
const amiberry_setting_descriptor *amiberry_settings_find(const char *id);

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
amiberry_settings_session *amiberry_settings_session_open(void);
void amiberry_settings_session_close(amiberry_settings_session *session);

/*
 * The current value in .uae spelling. A CHOICE option can hold a value
 * outside its choice list when a configuration file set one.
 */
amiberry_settings_status amiberry_settings_get(const amiberry_settings_session *session,
	const char *id, char *buffer, size_t size);
amiberry_settings_status amiberry_settings_set(amiberry_settings_session *session,
	const char *id, const char *value);

bool amiberry_settings_is_available(const amiberry_settings_session *session, const char *id);
bool amiberry_settings_choice_is_available(const amiberry_settings_session *session,
	const char *id, const char *value);
/* The range an INT or REAL option accepts now; false for other types. */
bool amiberry_settings_get_range(const amiberry_settings_session *session, const char *id,
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
size_t amiberry_settings_validate(const amiberry_settings_session *session,
	amiberry_settings_adjustment *adjustments, size_t capacity);

/*
 * Applies the adjustments amiberry_settings_validate() reports to the
 * session and stores it in the pending configuration. It does not signal a
 * running emulation. *changed tells whether any option differed from the
 * pending configuration; the result is the strongest apply class among
 * those that did (AMIBERRY_APPLY_LIVE when none did).
 */
amiberry_apply_class amiberry_settings_commit(amiberry_settings_session *session, bool *changed);

#ifdef __cplusplus
}
#endif
