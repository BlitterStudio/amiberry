#ifndef PANDORA_GFX_H
#define PANDORA_GFX_H

extern SDL_Window* sdlWindow;
extern SDL_Renderer* renderer;
extern SDL_Texture *texture;
extern SDL_Surface *amigaSurface;

extern void check_error_sdl(bool check, const char* message);
#endif
