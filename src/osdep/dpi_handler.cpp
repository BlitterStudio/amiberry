#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "dpi_handler.hpp"

namespace
{
	constexpr int display_index{0};
	constexpr float default_dpi{96.0F};

	float read_scale_from_dpi()
	{
		float ddpi{0.0F};
		float hdpi{0.0F};
		float vdpi{0.0F};

		if (SDL_GetDisplayDPI(display_index, &ddpi, &hdpi, &vdpi) != 0)
			return 0.0F;

		const float max_dpi = std::max(ddpi, std::max(hdpi, vdpi));
		if (max_dpi <= 0.0F)
			return 0.0F;

		return max_dpi / default_dpi;
	}

	float read_scale_from_display_mode()
	{
		SDL_DisplayMode mode{};
		SDL_Rect bounds{};

		if (SDL_GetDesktopDisplayMode(display_index, &mode) != 0)
			return 0.0F;
		if (SDL_GetDisplayBounds(display_index, &bounds) != 0)
			return 0.0F;
		if (bounds.w <= 0 || bounds.h <= 0)
			return 0.0F;

		const float scale_x = static_cast<float>(mode.w) / static_cast<float>(bounds.w);
		const float scale_y = static_cast<float>(mode.h) / static_cast<float>(bounds.h);

		return std::max(scale_x, scale_y);
	}

#if defined(__linux__)
	float read_scale_from_env(const char* name)
	{
		const char* value = SDL_getenv(name);
		if (!value || !*value)
			return 0.0F;

		char* end{nullptr};
		const float scale = std::strtof(value, &end);
		if (end == value || !std::isfinite(scale))
			return 0.0F;

		return scale;
	}
#endif
}

float DPIHandler::get_scale()
{
	float scale = 1.0F;

	const float dpi_scale = read_scale_from_dpi();
	if (dpi_scale > 0.0F)
		scale = std::max(scale, dpi_scale);

	const float mode_scale = read_scale_from_display_mode();
	if (mode_scale > 0.0F)
		scale = std::max(scale, mode_scale);

#if defined(__linux__)
	const float gdk_scale = read_scale_from_env("GDK_SCALE");
	if (gdk_scale > 0.0F)
		scale = std::max(scale, gdk_scale);

	const float qt_scale = read_scale_from_env("QT_SCALE_FACTOR");
	if (qt_scale > 0.0F)
		scale = std::max(scale, qt_scale);
#endif

	return std::max(scale, 1.0F);
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
#elif defined(__linux__)
    return get_scale();
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

int DPIHandler::scale_window_dimension(const int value)
{
	if (value <= 0)
		return value;

#if defined(__linux__)
	const float scale = get_scale();
	if (scale > 1.0f)
		return std::max(1, static_cast<int>(std::lround(static_cast<float>(value) * scale)));
#endif

	return value;
}

int DPIHandler::unscale_window_dimension(const int value)
{
	if (value <= 0)
		return value;

#if defined(__linux__)
	const float scale = get_scale();
	if (scale > 1.0f)
		return std::max(1, static_cast<int>(std::lround(static_cast<float>(value) / scale)));
#endif

	return value;
}

void DPIHandler::set_render_scale([[maybe_unused]] SDL_Renderer* renderer) {
    // SDL renderer scale is only needed on macOS where drawable pixels differ from window points.
#ifdef __MACH__
    auto scale{std::max(1.0f, std::round(get_scale()))};
    SDL_RenderSetScale(renderer, scale, scale);
#endif
}
