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
    [[nodiscard]] static int scale_window_dimension(int value);
    [[nodiscard]] static int unscale_window_dimension(int value);

    static void set_render_scale(SDL_Renderer* renderer);
};
