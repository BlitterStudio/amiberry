/*
 * Platform-independent loadable module functions for UAE.
 * Copyright (C) 2014 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#ifndef UAE_DLOPEN_H
#define UAE_DLOPEN_H

#include "uae/types.h"

#ifdef _WIN32
#define UAE_DLHANDLE HINSTANCE
#else
#define UAE_DLHANDLE void *
#endif

/* General loadable module support */

UAE_DLHANDLE uae_dlopen(const TCHAR *path);
void *uae_dlsym(UAE_DLHANDLE handle, const char *symbol);
void uae_dlclose(UAE_DLHANDLE handle);

/* UAE plugin support functions */

UAE_DLHANDLE uae_dlopen_plugin(const TCHAR *name);
void uae_dlopen_patch_common(UAE_DLHANDLE handle);

#endif /* UAE_DLOPEN_H */
