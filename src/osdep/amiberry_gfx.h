#pragma once
#include <SDL.h>

#ifdef USE_SDL2
extern SDL_Window* sdlWindow;
extern SDL_Renderer* renderer;
extern SDL_Texture* texture;
#endif

extern SDL_Cursor* cursor;
extern SDL_Surface* gui_screen;

#ifdef USE_SDL2
extern SDL_Texture* gui_texture;
extern SDL_DisplayMode sdlMode;
extern const char* sdl_video_driver;
#endif

extern bool can_have_linedouble;
extern void check_error_sdl(bool check, const char* message);
extern void toggle_fullscreen();