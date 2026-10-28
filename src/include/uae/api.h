/*
 * UAE export/import linkage macros and plugin function-pointer helpers.
 *
 * In a UAE build (UAE defined) this side imports plugin functions through
 * function-pointer variables that uae_dlsym() fills in; in a plugin build
 * (UAE undefined, e.g. the external QEMU-UAE plugin using its own copy of
 * this header) the roles are reversed and the plugin imports from UAE.
 *
 * The UAE_DECLARE_ and UAE_DEFINE_ macros are invoked at namespace scope
 * without a trailing semicolon, so every expansion must be a complete
 * declaration ending in one. UAE_IMPORT_FUNCTION/UAE_EXPORT_FUNCTION are
 * brace-enclosed statements and are followed by a semicolon at the call
 * site. uae_dlsym() and write_log() are resolved at the expansion site.
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UAE_API_H
#define UAE_API_H

#include "uae/attributes.h"

#ifdef __cplusplus
#define UAE_EXTERN_C extern "C"
#else
#define UAE_EXTERN_C
#endif

#ifdef _WIN32
#define UAE_EXPORT __declspec(dllexport)
#define UAE_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#define UAE_EXPORT __attribute__((visibility("default")))
#define UAE_IMPORT
#else
#define UAE_EXPORT
#define UAE_IMPORT
#endif

/* UAE builds export these symbols for plugins to import; plugin builds
 * import them from UAE instead. */
#ifdef UAE
#define UAEAPI UAE_EXTERN_C UAE_EXPORT
#else
#define UAEAPI UAE_EXTERN_C UAE_IMPORT
#endif

#ifdef _WIN32
#define UAECALL __cdecl
#else
#define UAECALL
#endif

#ifdef UAE

/* UAE side: N is an extern function-pointer variable resolved with
 * uae_dlsym() at plugin load time. */
#define UAE_DECLARE_IMPORT_FUNCTION(rettype, name, ...) \
	typedef rettype (UAECALL *name##_function)(__VA_ARGS__); \
	extern name##_function name;

/* UAE side: N is our own function whose address is handed to the plugin. */
#define UAE_DECLARE_EXPORT_FUNCTION(rettype, name, ...) \
	typedef rettype (UAECALL *name##_function)(__VA_ARGS__); \
	rettype UAECALL name(__VA_ARGS__);

#define UAE_DEFINE_IMPORT_FUNCTION(name) \
	name##_function name = NULL;

#define UAE_IMPORT_FUNCTION(handle, name) \
	{ \
		name = (name##_function)uae_dlsym(handle, #name); \
		if (name != NULL) { \
			write_log("Imported %s\n", #name); \
		} else { \
			write_log("WARNING: Could not import %s\n", #name); \
		} \
	}

#define UAE_EXPORT_FUNCTION(handle, name) \
	{ \
		name##_function *name##_variable = (name##_function *)uae_dlsym(handle, #name); \
		if (name##_variable != NULL) { \
			*name##_variable = name; \
			write_log("Exported %s\n", #name); \
		} else { \
			write_log("WARNING: Could not export %s\n", #name); \
		} \
	}

#else /* plugin build */

/* Plugin side: N is a function exported by UAE. */
#define UAE_DECLARE_IMPORT_FUNCTION(rettype, name, ...) \
	rettype UAECALL name(__VA_ARGS__);

/* Plugin side: N is an extern function-pointer variable that UAE patches
 * with the address of its implementation after loading the plugin. */
#define UAE_DECLARE_EXPORT_FUNCTION(rettype, name, ...) \
	typedef rettype (UAECALL *name##_function)(__VA_ARGS__); \
	extern name##_function name;

#define UAE_DEFINE_IMPORT_FUNCTION(name) \
	name##_function name = NULL;

#endif /* UAE */

#endif /* UAE_API_H */
