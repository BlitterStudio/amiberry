#pragma once

#ifdef LIBRETRO
#include "sdl_compat.h"
#include "SDL_ttf.h"
#else
#include "SDL.h"
#include "SDL_ttf.h"
#endif

struct WindowSize {
    int width;
    int height;
};

class DPIHandler {
public:
    [[nodiscard]] static float get_scale();

    static void set_render_scale(SDL_Renderer* renderer);
    static void set_font_size(TTF_Font* font, float size);
};
