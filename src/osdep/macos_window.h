#pragma once

#include "sysconfig.h"
#include <SDL3/SDL.h>

#if defined(AMIBERRY_MACOS)
void macos_raise_window(SDL_Window* window);
#else
inline void macos_raise_window(SDL_Window*) {}
#endif
