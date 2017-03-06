#pragma once
extern SDL_Window* sdlWindow;
extern SDL_Renderer* renderer;
extern SDL_Texture* texture;
extern SDL_Surface* screen;

extern SDL_Surface* gui_screen;
extern SDL_Texture* gui_texture;

extern SDL_DisplayMode sdlMode;

extern void check_error_sdl(bool check, const char* message);
extern void updatedisplayarea();