// SDL 2 to SDL 1 interface by rsn8887
// to port uae4all2 to Switch
// created 10/29/2018
#ifdef USE_SDL2
#ifndef SDL2_TO_SDL1_H
#define SDL2_TO_SDL1_H

#include <SDL.h>

extern SDL_Surface *prSDLScreen;

#define SDL_HWSURFACE 0
#define SDL_DOUBLEBUF 0
#define SDL_CreateThread(x,y) SDL_CreateThread(x,"uae_thread",y)
#define SDLKey SDL_Keycode
#define SDLKey SDL_Keycode
#define SDLK_KP0 SDLK_KP_0
#define SDLK_KP1 SDLK_KP_1
#define SDLK_KP2 SDLK_KP_2
#define SDLK_KP3 SDLK_KP_3
#define SDLK_KP4 SDLK_KP_4
#define SDLK_KP5 SDLK_KP_5
#define SDLK_KP6 SDLK_KP_6
#define SDLK_KP7 SDLK_KP_7
#define SDLK_KP8 SDLK_KP_8
#define SDLK_KP9 SDLK_KP_9
#define SDLK_LMETA SDLK_LGUI
#define SDLK_RMETA SDLK_RGUI

#define SDL_keysym SDL_Keysym
#define SDL_DisplayFormat(x) SDL_ConvertSurface(x,prSDLScreen->format,0)
#define SDL_SetAlpha(x,y,z) SDL_SetSurfaceAlphaMod(x,z); SDL_SetSurfaceBlendMode(x, SDL_BLENDMODE_BLEND);

SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, int flags);
void SDL_SetVideoModeScaling(int x, int y, float sw, float sh);
void SDL_SetVideoModeBilinear(int value);
void SDL_Flip(SDL_Surface *surface);
void SDL_SetVideoModeSync(int value);
#endif

#endif