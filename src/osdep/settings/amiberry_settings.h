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

#define AMIBERRY_SETTINGS_API_VERSION 6

/* Large enough for every value a descriptor can produce, including the
   terminating NUL. */
#define AMIBERRY_SETTINGS_VALUE_MAX 64

typedef enum amiberry_setting_type {
	AMIBERRY_SETTING_BOOL,   /* "true" or "false" */
	AMIBERRY_SETTING_INT,    /* decimal integer within [min, max] */
	AMIBERRY_SETTING_REAL,   /* decimal number within [min, max] */
	AMIBERRY_SETTING_CHOICE, /* one of the descriptor's choice values */
	/* Free-form .uae token. PATH is a file system path a front end can
	   attach a file picker to; STRING is any other literal value, for
	   example a hexadecimal address. These values may be longer than
	   AMIBERRY_SETTINGS_VALUE_MAX: read them with a larger buffer (the
	   get call reports AMIBERRY_SETTINGS_BUFFER_TOO_SMALL until it fits)
	   and never through amiberry_settings_adjustment, which truncates.
	   A PATH longer than the host's path limit (PATH_MAX, 1024 bytes on
	   macOS) is refused with AMIBERRY_SETTINGS_INVALID_VALUE. */
	AMIBERRY_SETTING_PATH,
	AMIBERRY_SETTING_STRING
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
 * Some CHOICE options enumerate devices connected to the host (audio
 * devices, joysticks and mice) instead of a fixed list. For those,
 * amiberry_settings_choices() returns the choices that can be selected
 * now; a configuration may still hold a value outside the list (a device
 * that is no longer connected), which amiberry_settings_get() reports
 * as-is. Both calls, like every session call, run on the emulator's main
 * thread; the returned pointer is valid until the next main-thread call
 * of the layer. amiberry_settings_refresh_choices() re-reads the host
 * device lists, for example when a device was connected.
 */
AMIBERRY_SETTINGS_API size_t amiberry_settings_choices(
	const amiberry_settings_session *session, const char *id,
	const amiberry_setting_choice **choices);
AMIBERRY_SETTINGS_API void amiberry_settings_refresh_choices(void);

/*
 * The current value in .uae spelling. A CHOICE option can hold a value
 * outside its choice list when a configuration file set one.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_get(const amiberry_settings_session *session,
	const char *id, char *buffer, size_t size);
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_set(amiberry_settings_session *session,
	const char *id, const char *value);
/*
 * The value the running emulation uses now (the active configuration),
 * which reaches the pending one's after amiberry_actions_apply_settings
 * with the option's apply class. Same spelling and statuses as get.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_get_active(const char *id,
	char *buffer, size_t size);

AMIBERRY_SETTINGS_API bool amiberry_settings_is_available(const amiberry_settings_session *session, const char *id);
/* Whether a CHOICE or BOOL option accepts this value now ("true"/"false" for
   BOOL): a front end greys out the values that return false. */
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
 * Merges the session's changes into the current pending configuration
 * (changes made elsewhere since the session last read it included), runs
 * the core's configuration fixups on a copy of that and reports every
 * option they would change. Writes at most capacity entries, zero-filled
 * past each string, and returns the total number found; the session itself
 * is not modified. Settings that do not have an option of their own yet
 * are adjusted by commit but not reported.
 */
AMIBERRY_SETTINGS_API size_t amiberry_settings_validate(const amiberry_settings_session *session,
	amiberry_settings_adjustment *adjustments, size_t capacity);

/*
 * Stores the session in the pending configuration exactly as validation
 * resolved it: the session's changes, changes made elsewhere since, and
 * every adjustment the fixups make for them. The session continues from
 * the stored configuration. It does not signal a running emulation:
 * amiberry_actions_apply_settings (amiberry_actions.h) with the class
 * returned brings the emulation up to it.
 * *changed tells whether the pending configuration changed; the result is
 * the strongest apply class among the options that did (or, when only
 * settings without a value of their own changed, among the options the
 * caller set), AMIBERRY_APPLY_LIVE when nothing changed.
 */
AMIBERRY_SETTINGS_API amiberry_apply_class amiberry_settings_commit(amiberry_settings_session *session, bool *changed);


/*
 * Machine presets (the Quickstart models). A preset replaces most of the
 * configuration exactly like the Quickstart panel does, so it must be
 * followed by amiberry_settings_commit().
 */
typedef struct amiberry_settings_preset {
	const char *id;          /* stable ID, for example "a500" */
	const char *label;       /* display name */
	int config_count;        /* number of configurations */
	int compatibility_max;   /* highest compatibility level */
} amiberry_settings_preset;

AMIBERRY_SETTINGS_API size_t amiberry_settings_preset_count(void);
/* NULL when index >= amiberry_settings_preset_count(). */
AMIBERRY_SETTINGS_API const amiberry_settings_preset *amiberry_settings_preset_at(size_t index);
/* NULL for an unknown ID. */
AMIBERRY_SETTINGS_API const amiberry_settings_preset *amiberry_settings_preset_find(const char *id);
/* NULL when config is out of range. */
AMIBERRY_SETTINGS_API const char *amiberry_settings_preset_config_label(
	const amiberry_settings_preset *preset, int config);
/* NULL when level is out of range. */
AMIBERRY_SETTINGS_API const char *amiberry_settings_preset_compatibility_label(int level);
/* Whether every ROM the preset's configuration needs is present in the
   scanned ROM list; never shows a dialog or modifies anything. */
AMIBERRY_SETTINGS_API bool amiberry_settings_preset_rom_installed(const char *id, int config);
/* Applies the preset to the session the way Quickstart does: built-in model
   defaults (including the defaults of a fresh configuration outside the
   options) plus the Quickstart constraints. Validate and commit replay it on
   the pending configuration, after the session's earlier changes and before
   its later ones. UNKNOWN_ID for an unknown preset or a NULL session,
   INVALID_VALUE for a configuration out of range, UNAVAILABLE for a
   compatibility level out of range or when memory is exhausted. Commit
   afterwards to store the result; a preset commits at least
   AMIBERRY_APPLY_RESET. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_preset_apply(
	amiberry_settings_session *session, const char *id, int config, int compatibility);

/*
 * Mounted drives: directories, hardfiles, CD and tape units, in the order a
 * configuration file lists them (at most AMIBERRY_SETTINGS_MOUNT_MAX). An
 * entry is spelled as the value of its uaehfN line, for example
 * "dir,rw,DH0:Work:/home/amiga/work,0" or
 * "hdf,rw,DH1:/disks/system.hdf,32,1,2,512,0,,uae0"; get returns that
 * spelling and add and replace take it. The edits stay in the session like
 * every other change; commit reports a changed list as
 * AMIBERRY_APPLY_RESET, as drives attach when the emulation resets.
 */
#define AMIBERRY_SETTINGS_MOUNT_MAX 50

AMIBERRY_SETTINGS_API size_t amiberry_settings_mount_count(const amiberry_settings_session *session);
/* UNKNOWN_ID for an index past the list. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_mount_get(
	const amiberry_settings_session *session, size_t index, char *buffer, size_t size);
/*
 * Appends an entry, checked like the hard drive panel's: INVALID_VALUE for
 * a value that is not an entry, a device name another entry uses, a
 * controller without a free unit, a second CD with the same unit, or an
 * entry too long for a configuration file line; UNAVAILABLE when the list
 * is full.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_mount_add(
	amiberry_settings_session *session, const char *value);
/*
 * Replaces entry index, checked as add checks a new entry against the
 * others. What a uaehfN value does not carry (a directory's injected
 * icons) stays with the entry. UNKNOWN_ID for an index past the list.
 */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_mount_replace(
	amiberry_settings_session *session, size_t index, const char *value);
/* Removes entry index; the later entries move up. UNKNOWN_ID for an index
   past the list. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_mount_remove(
	amiberry_settings_session *session, size_t index);

/*
 * Expansion boards: SCSI, IDE and network controllers, bridgeboards, sound
 * and graphics cards and the machines' built-in controllers, which a
 * configuration lists by board type and unit ("a2091", "a2091-2"). The
 * accelerator board is not one of them (the cpuboard_* options).
 *
 * A board type lists the fields a unit of it has, as setting descriptors:
 * "enabled" (BOOL) first, then as the type has them "subtype" (CHOICE of
 * the board's variants), "rom_file" (PATH), "scsi_id" (CHOICE "0"-"7"),
 * "autoboot_disabled", "inserted" (a PCMCIA card), "dma24bit" (BOOL), and
 * the board's own settings, "setting:<name>" (BOOL switch, CHOICE
 * selector, STRING text). Units are numbered from 0, below the type's
 * unit count.
 *
 * A session reads and edits units like options: the edits stay in the
 * session, and commit reports changed boards as AMIBERRY_APPLY_RESTART
 * (boards are built when the emulation starts). A ROM board enabled
 * without a ROM file gets the scanned ROM of its type, or none (":NOROM",
 * which the configuration keeps; the ROM is then picked through
 * "rom_file"). Enabling a built-in controller switches the chipset field
 * the machine has it as: the IDE controller ("ide"), A3000 or A4000T SCSI
 * (one of the two) and the CD32 FMV cartridge on any machine, PCMCIA and
 * the CDTV controllers through their options ("pcmcia", "cdtvcd",
 * "cdtv-cr"), which a custom chipset has (the CDTV DMAC or the CDTV-CR).
 */
#define AMIBERRY_BOARD_INTERNAL      0x0001u /* built into a machine */
#define AMIBERRY_BOARD_SCSI          0x0002u
#define AMIBERRY_BOARD_IDE           0x0004u
#define AMIBERRY_BOARD_SASI          0x0008u
#define AMIBERRY_BOARD_CUSTOM        0x0010u /* other and UAE boards */
#define AMIBERRY_BOARD_PCI_BRIDGE    0x0020u
#define AMIBERRY_BOARD_X86_BRIDGE    0x0040u
#define AMIBERRY_BOARD_RTG           0x0080u
#define AMIBERRY_BOARD_SOUND         0x0100u
#define AMIBERRY_BOARD_NETWORK       0x0200u
#define AMIBERRY_BOARD_FLOPPY        0x0400u
#define AMIBERRY_BOARD_X86_EXPANSION 0x0800u /* cards for a bridgeboard */

typedef struct amiberry_settings_board_type {
	const char *id;           /* configuration name, for example "a2091" */
	const char *label;        /* display name */
	const char *manufacturer; /* NULL when unknown */
	unsigned int categories;  /* AMIBERRY_BOARD_* bits */
	int units;                /* how many of it a configuration can have */
	size_t field_count;
} amiberry_settings_board_type;

AMIBERRY_SETTINGS_API size_t amiberry_settings_board_type_count(void);
/* NULL when index >= amiberry_settings_board_type_count(). */
AMIBERRY_SETTINGS_API const amiberry_settings_board_type *amiberry_settings_board_type_at(size_t index);
/* NULL for an unknown ID. */
AMIBERRY_SETTINGS_API const amiberry_settings_board_type *amiberry_settings_board_type_find(const char *id);
/* NULL when index >= type->field_count. */
AMIBERRY_SETTINGS_API const amiberry_setting_descriptor *amiberry_settings_board_field_at(
	const amiberry_settings_board_type *type, size_t index);

/* UNKNOWN_ID for an unknown type or field or a unit out of range,
   BUFFER_TOO_SMALL when the value does not fit. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_board_get(
	const amiberry_settings_session *session, const char *type, int unit, const char *field,
	char *buffer, size_t size);
/* UNKNOWN_ID as get, INVALID_VALUE for a value the field does not take,
   UNAVAILABLE for a field the unit does not have now (a field of a board
   that is not enabled, a PCMCIA field of another variant) or a board the
   configuration has no room for (20 boards). */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_board_set(
	amiberry_settings_session *session, const char *type, int unit, const char *field,
	const char *value);
AMIBERRY_SETTINGS_API bool amiberry_settings_board_field_available(
	const amiberry_settings_session *session, const char *type, int unit, const char *field);

/*
 * Custom controls: the Amiga input event each button and axis of the
 * joystick on a port (0-3) triggers, alone or with the device's hotkey
 * button held (hotkey). A configuration keeps them per port, for the
 * device the port has.
 *
 * A control's value is an input event's configuration name from
 * amiberry_settings_input_event_at(), "none" (the control does nothing),
 * or "" (unset: the control does what the port's mode gives it, such as
 * the joystick's fire button or the CD32 pad's colour buttons). A
 * session keeps its edits until commit, which applies them live (and
 * drops the edits for a port that has another device by then). A control
 * is not available while the port has no joystick, the device lacks it,
 * or a hotkey function holds it (custom_control_use names the function:
 * "hotkey", "quit", "menu", "reset" or "vkbd").
 */
typedef struct amiberry_settings_custom_control {
	const char *id;    /* configuration name: "a", "dpup", "leftx" */
	const char *label; /* display name */
	bool axis;
} amiberry_settings_custom_control;

AMIBERRY_SETTINGS_API size_t amiberry_settings_custom_control_count(void);
/* NULL when index >= amiberry_settings_custom_control_count(). */
AMIBERRY_SETTINGS_API const amiberry_settings_custom_control *amiberry_settings_custom_control_at(size_t index);
AMIBERRY_SETTINGS_API size_t amiberry_settings_input_event_count(void);
/* NULL when index >= amiberry_settings_input_event_count(). */
AMIBERRY_SETTINGS_API const amiberry_setting_choice *amiberry_settings_input_event_at(size_t index);

/* UNKNOWN_ID for an unknown control or a port out of range, UNAVAILABLE
   while the port has no joystick, BUFFER_TOO_SMALL when the value does
   not fit. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_custom_control_get(
	const amiberry_settings_session *session, int port, bool hotkey, const char *control,
	char *buffer, size_t size);
/* UNKNOWN_ID as get, INVALID_VALUE for an unknown event, UNAVAILABLE for a
   control that is not available. */
AMIBERRY_SETTINGS_API amiberry_settings_status amiberry_settings_custom_control_set(
	amiberry_settings_session *session, int port, bool hotkey, const char *control, const char *event);
AMIBERRY_SETTINGS_API bool amiberry_settings_custom_control_available(
	const amiberry_settings_session *session, int port, bool hotkey, const char *control);
/* NULL when no hotkey function holds the control. */
AMIBERRY_SETTINGS_API const char *amiberry_settings_custom_control_use(
	const amiberry_settings_session *session, int port, bool hotkey, const char *control);

#ifdef __cplusplus
}
#endif
