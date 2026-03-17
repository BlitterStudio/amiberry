#ifndef _NFD_SDL3_H
#define _NFD_SDL3_H

#include <stddef.h>
#include <stdint.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <nfd.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool NFD_SetDisplayPropertiesFromSDL3Window(SDL_Window* sdlWindow) {
#if defined(__linux__) || defined(__FreeBSD__)
	SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
	if (!props) return true;

	void* wl_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
	if (!wl_display) return true;

	return NFD_SetWaylandDisplay(wl_display) == NFD_OKAY;
#else
	(void)sdlWindow;
	return true;
#endif
}

static inline bool NFD_GetNativeWindowFromSDL3Window(SDL_Window* sdlWindow, nfdwindowhandle_t* nativeWindow) {
	SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
	if (!props) return false;

	void* cocoa = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
	if (cocoa) {
		nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_COCOA;
		nativeWindow->handle = cocoa;
		return true;
	}

	void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (hwnd) {
		nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
		nativeWindow->handle = hwnd;
		return true;
	}

	void* wl_surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
	if (wl_surface) {
		nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_WAYLAND;
		nativeWindow->handle = wl_surface;
		return true;
	}

	Sint64 x11_window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
	if (x11_window) {
		nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_X11;
		nativeWindow->handle = (void*)(uintptr_t)x11_window;
		return true;
	}

	return false;
}

#ifdef __cplusplus
}
#endif

#endif
