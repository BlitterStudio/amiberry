#include <cmath>
#include "dpi_handler.hpp"

float DPIHandler::get_scale() {
    constexpr int display_index{0};
    constexpr float default_dpi{96.0F};
    float dpi{default_dpi};

    SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

    return floor(dpi / default_dpi);
}

void DPIHandler::set_render_scale([[maybe_unused]] SDL_Renderer* renderer) {
    // Do nothing on Linux
#ifdef __MACH__
    auto scale{get_scale()};
    SDL_RenderSetScale(renderer, scale, scale);
#endif
}

void DPIHandler::set_font_size(TTF_Font* font, float size) {
    constexpr int display_index{0};
    float hdpi{};
    float vdpi{};
    SDL_GetDisplayDPI(display_index, nullptr, &hdpi, &vdpi);
#if SDL_VERSION_ATLEAST(2, 0, 18)
    TTF_SetFontSizeDPI(font,
        static_cast<int>(size / get_scale()),
        static_cast<unsigned int>(hdpi),
        static_cast<unsigned int>(vdpi));
#endif
}
