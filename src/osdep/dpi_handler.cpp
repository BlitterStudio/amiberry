#include <cmath>
#include <algorithm>
#include "dpi_handler.hpp"

float DPIHandler::get_scale() {
    constexpr int display_index{0};
    constexpr float default_dpi{96.0F};
    float dpi{default_dpi};

    SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

    return floor(dpi / default_dpi);
}

float DPIHandler::get_layout_scale() {
#ifdef __ANDROID__
    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(0, &bounds) == 0) {
        // Use the larger dimension to ensure consistent scaling regardless of orientation
        // This way portrait and landscape start with the same scale factor
        int max_dim = std::max(bounds.w, bounds.h);
        float scaling_factor = static_cast<float>(max_dim) / 800.0f;
        return (scaling_factor < 1.0f) ? 1.0f : scaling_factor;
    }
    return 1.0f;
#else
    return 1.0f;
#endif
}

float DPIHandler::get_touch_scale() {
#ifdef __ANDROID__
    float layout_scale = get_layout_scale();
    // Ensure minimum touch target size of 48dp (Material Design guideline)
    // With BUTTON_HEIGHT base of 30px, need scale >= 48/30 = 1.6
    return std::max(layout_scale, 1.6f);
#else
    return get_layout_scale();
#endif
}

void DPIHandler::set_render_scale([[maybe_unused]] SDL_Renderer* renderer) {
    // Do nothing on Linux
#ifdef __MACH__
    auto scale{get_scale()};
    SDL_RenderSetScale(renderer, scale, scale);
#endif
}
