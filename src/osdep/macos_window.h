#pragma once

#include "sysconfig.h"
#include <SDL3/SDL.h>

#if defined(AMIBERRY_MACOS)
void macos_raise_window(SDL_Window* window);
bool macos_get_current_mouse_position(SDL_Window* window, float* x, float* y);
#else
inline void macos_raise_window(SDL_Window*) {}
inline bool macos_get_current_mouse_position(SDL_Window*, float*, float*) { return false; }
#endif
