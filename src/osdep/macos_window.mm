#include "macos_window.h"

#if defined(AMIBERRY_MACOS)

#import <AppKit/AppKit.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

void macos_raise_window(SDL_Window* window)
{
	if (!window)
		return;

	[NSApp activateIgnoringOtherApps:YES];

	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	if (!props)
		return;

	NSWindow* nswindow = static_cast<NSWindow*>(
		SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr));
	if (!nswindow)
		return;

	[nswindow makeKeyAndOrderFront:nil];
	[nswindow orderFrontRegardless];
}

#endif
