/*
 * Helpers used to export UAE functions for use with other modules
 * Copyright (C) 2014 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#ifndef UAE_API_H
#define UAE_API_H

/* This file is intended to be included by external libraries as well,
 * so don't pull in too much UAE-specific stuff. */

#include "uae/attributes.h"

/* Handy define so we can disable C++ name mangling without considering
 * whether the source language is C or C++. */

#ifdef __cplusplus
#define UAE_EXTERN_C extern "C"
#else
#define UAE_EXTERN_C
#endif

/* UAE_EXPORT / UAE_IMPORT are mainly intended as helpers for UAEAPI
 * defined below. */

#ifdef _WIN32
#define UAE_EXPORT __declspec(dllexport)
#define UAE_IMPORT __declspec(dllimport)
#else
#define UAE_EXPORT __attribute__((visibility("default")))
#define UAE_IMPORT
#endif

/* UAEAPI marks a function for export across library boundaries. You'll
 * likely want to use this together with UAECALL. */

#ifdef UAE
#define UAEAPI UAE_EXTERN_C UAE_EXPORT
#else
#define UAEAPI UAE_EXTERN_C UAE_IMPORT
#endif

/* WinUAE (or external libs) might be compiled with fastcall by default,
 * so we force all external functions to use cdecl calling convention. */

#ifdef _WIN32
#define UAECALL __cdecl
#else
#define UAECALL
#endif

/* Helpers to make it easy to import functions from plugins. */

#ifdef UAE

#define UAE_DECLARE_IMPORT_FUNCTION(return_type, function_name, ...) \
	typedef return_type (UAECALL * function_name ## _function)(__VA_ARGS__); \
	extern function_name ## _function function_name;

#define UAE_IMPORT_FUNCTION(handle, function_name) \
{ \
	function_name = (function_name ## _function) uae_dlsym( \
		handle, #function_name); \
	if (function_name) { \
		write_log("Imported " #function_name "\n"); \
	} else { \
		write_log("WARNING: Could not import " #function_name "\n"); \
	} \
}

#define UAE_DECLARE_EXPORT_FUNCTION(return_type, function_name, ...) \
	typedef return_type (UAECALL * function_name ## _function)(__VA_ARGS__); \
	return_type UAECALL function_name(__VA_ARGS__);

#define UAE_EXPORT_FUNCTION(handle, function_name) \
{ \
	void *ptr = uae_dlsym(handle, #function_name); \
	if (ptr) { \
		*((function_name ## _function *) ptr) = function_name; \
		write_log("Exported " #function_name "\n"); \
	} else { \
		write_log("WARNING: Could not export " #function_name "\n"); \
	} \
}

#else

#define UAE_DECLARE_IMPORT_FUNCTION(return_type, function_name, ...) \
	return_type UAECALL function_name(__VA_ARGS__);

#define UAE_DECLARE_EXPORT_FUNCTION(return_type, function_name, ...) \
	typedef return_type (UAECALL * function_name ## _function)(__VA_ARGS__); \
	extern function_name ## _function function_name;

#endif

/**
 * Used in both UAE and plugin code to define storage for the function
 * imported from the other module.
 */
#define UAE_DEFINE_IMPORT_FUNCTION(function_name) \
    function_name ## _function function_name = NULL;

#endif /* UAE_API_H */
