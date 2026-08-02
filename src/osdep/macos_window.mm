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

bool macos_get_current_mouse_position(SDL_Window* window, float* x, float* y)
{
	if (!window || !x || !y)
		return false;

	NSEvent* event = [NSApp currentEvent];
	if (!event)
		return false;

	switch ([event type]) {
	case NSEventTypeLeftMouseDown:
	case NSEventTypeLeftMouseUp:
	case NSEventTypeRightMouseDown:
	case NSEventTypeRightMouseUp:
	case NSEventTypeOtherMouseDown:
	case NSEventTypeOtherMouseUp:
		break;
	default:
		return false;
	}

	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	if (!props)
		return false;

	NSWindow* nswindow = static_cast<NSWindow*>(
		SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr));
	if (!nswindow || [event window] != nswindow)
		return false;

	NSView* content_view = [nswindow contentView];
	if (!content_view)
		return false;

	const NSPoint point = [content_view convertPoint:[event locationInWindow] fromView:nil];
	const NSRect bounds = [content_view bounds];
	if (!NSPointInRect(point, bounds))
		return false;

	*x = static_cast<float>(point.x - NSMinX(bounds));
	*y = static_cast<float>(NSMaxY(bounds) - point.y);
	return true;
}

#endif
