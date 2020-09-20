#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/dlopen.h"

#ifdef _WIN32
#include "windows.h"
#else
#include <dlfcn.h>
#endif
#ifdef WINUAE
#include "od-win32/win32.h"
#endif

UAE_DLHANDLE uae_dlopen(const TCHAR *path)
{
	UAE_DLHANDLE result;
	if (path == NULL || path[0] == _T('\0')) {
		write_log(_T("DLOPEN: No path given\n"));
		return NULL;
	}
#ifdef _WIN32
	result = LoadLibrary(path);
#else
	result = dlopen(path, RTLD_NOW);
	const char *error = dlerror();
	if (error != NULL)  {
		write_log("DLOPEN: %s\n", error);
	}
#endif
	if (result == NULL) {
		write_log("DLOPEN: Failed to open %s\n", path);
	}
	return result;
}

void *uae_dlsym(UAE_DLHANDLE handle, const char *name)
{
#ifdef _WIN32
	return (void *) GetProcAddress(handle, name);
#else
	return dlsym(handle, name);
#endif
}

void uae_dlclose(UAE_DLHANDLE handle)
{
#ifdef _WIN32
	FreeLibrary (handle);
#else
	dlclose(handle);
#endif
}

#ifdef AMIBERRY
#include "uae/uae.h"
static amiga_plugin_lookup_function plugin_lookup;
/*UAE_EXTERN_C*/ void amiga_set_plugin_lookup_function(
		amiga_plugin_lookup_function function)
{
	plugin_lookup = function;
}
#endif

UAE_DLHANDLE uae_dlopen_plugin(const TCHAR *name)
{
#if defined(AMIBERRY)
	const TCHAR *path = NULL;
	if (plugin_lookup) {
		path = plugin_lookup(name);
	}
/*	if (path == NULL or path[0] == _T('\0')) {
		write_log(_T("DLOPEN: Could not find plugin \"%s\"\n"), name);
		return NULL;
	}
*/
	UAE_DLHANDLE handle = uae_dlopen("./capsimg.so");
#elif defined(WINUAE)
	TCHAR path[MAX_DPATH];
	_tcscpy(path, name);
	_tcscat(path, LT_MODULE_EXT);
	UAE_DLHANDLE handle = WIN32_LoadLibrary(path);
#else
	TCHAR path[MAX_DPATH];
	_tcscpy(path, name);
#ifdef _WIN64
	_tcscat(path, _T("_x64"));
#endif
	_tcscat(path, LT_MODULE_EXT);
	UAE_DLHANDLE handle = uae_dlopen(path);
#endif
	if (handle) {
		write_log(_T("DLOPEN: Loaded plugin %s\n"), path);
		uae_dlopen_patch_common(handle);
	}
	return handle;
}

void uae_dlopen_patch_common(UAE_DLHANDLE handle)
{
	void *ptr;
	ptr = uae_dlsym(handle, "uae_log");
	if (ptr) {
		write_log(_T("DLOPEN: Patching common functions\n"));
//		*((uae_log_function *)ptr) = &uae_log;
	}
}
