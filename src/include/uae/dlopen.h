/*
 * UAE shared-library loading interface (declarations only).
 *
 * The implementation lives in src/dlopen.cpp. All functions keep C++
 * linkage; floppybridge_lib.cpp redeclares uae_dlopen_plugin locally.
 *
 * On Windows the caller must have <windows.h> visible already (sysconfig.h
 * includes <winsock2.h> first; including <windows.h> here before it would
 * break the build), which is where HINSTANCE comes from.
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UAE_DLOPEN_H
#define UAE_DLOPEN_H

#include "uae/types.h"

#ifdef _WIN32
#define UAE_DLHANDLE HINSTANCE
#else
#define UAE_DLHANDLE void *
#endif

/* Load a shared library from `path`. Null or empty path fails. */
UAE_DLHANDLE uae_dlopen(const TCHAR *path);

/* Look up `symbol` (a narrow string) in a loaded library. */
void *uae_dlsym(UAE_DLHANDLE handle, const char *symbol);

/* Close a library obtained from uae_dlopen/uae_dlopen_plugin. */
void uae_dlclose(UAE_DLHANDLE handle);

/* Search the plugin directories for `name` (no extension), append the
 * platform suffix (LT_MODULE_EXT) and load it. */
UAE_DLHANDLE uae_dlopen_plugin(const TCHAR *name);

/* Patch a freshly loaded plugin: point its exported `uae_log` variable
 * at write_log. */
void uae_dlopen_patch_common(UAE_DLHANDLE handle);

#endif /* UAE_DLOPEN_H */
