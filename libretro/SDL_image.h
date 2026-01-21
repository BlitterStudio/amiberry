#ifndef LIBRETRO_SDL_IMAGE_H
#define LIBRETRO_SDL_IMAGE_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

SDL_Surface* IMG_Load(const char* file);

#ifdef __cplusplus
}
#endif

#endif
