#pragma once

#include <SDL3/SDL.h>

// Host clipboard access through SDL3. Main thread only.

// SDL_EVENT_CLIPBOARD_UPDATE: hand the new host clipboard to the emulation.
void clipboard_host_update_event(const SDL_ClipboardEvent &event);
// Once per event-loop pass: write what the guest copied to the host
// clipboard, and read the host clipboard when the guest asks for it.
void clipboard_host_flush();
