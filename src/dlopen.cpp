#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/api.h"
#include "uae/dlopen.h"
#include "uae/log.h"

#ifdef _WIN32
#include "windows.h"
#else
#include <dlfcn.h>
#endif
#ifdef WINUAE
#include "od-win32/win32.h"
#endif

#ifdef AMIBERRY
#include "uae.h"
#endif
#ifdef __MACH__
#include <mach-o/dyld.h>
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
#if 0
	if (handle == NULL) {
		return NULL;
	}
#endif
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

UAE_DLHANDLE uae_dlopen_plugin(const TCHAR *name)
{
#if defined(FSUAE)
	const TCHAR *path = NULL;
	if (plugin_lookup) {
		path = plugin_lookup(name);
	}
	if (path == NULL or path[0] == _T('\0')) {
		write_log(_T("DLOPEN: Could not find plugin \"%s\"\n"), name);
		return NULL;
	}
	UAE_DLHANDLE handle = uae_dlopen(path);
#elif defined(WINUAE)
	TCHAR path[MAX_DPATH];
	_tcscpy(path, name);
	_tcscat(path, LT_MODULE_EXT);
	UAE_DLHANDLE handle = WIN32_LoadLibrary(path);
#else
	TCHAR path[MAX_DPATH];
	std::string directory = get_plugins_path();
	_tcscpy(path, directory.append(name).c_str());
#ifdef _WIN64
	_tcscat(path, _T("_x64"));
#endif
	if (_tcscmp(path + _tcslen(path) - _tcslen(LT_MODULE_EXT), LT_MODULE_EXT) != 0) {
		_tcscat(path, LT_MODULE_EXT);
	}
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
		*((uae_log_function *)ptr) = &write_log;
	}
}
