#pragma once

#include "SDL.h"

struct WindowSize {
    int width;
    int height;
};

class DPIHandler {
public:
    [[nodiscard]] static float get_scale();
    [[nodiscard]] static float get_layout_scale();
    [[nodiscard]] static float get_touch_scale();

    static void set_render_scale(SDL_Renderer* renderer);
};
