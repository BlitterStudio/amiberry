#ifndef AMIBERRY_GFX_GEOMETRY_H
#define AMIBERRY_GFX_GEOMETRY_H

static inline void amiberry_gfx_auto_crop_presentation_dimensions(
	int source_width, int source_height, bool is_ntsc, bool correct_aspect,
	bool integer_scaling, int output_width, int output_height,
	int& display_width, int& display_height)
{
	display_width = source_width;
	display_height = source_height;

	if (correct_aspect) {
		if (is_ntsc) {
			display_height = display_height * 6 / 5;
		}
	} else if (!integer_scaling) {
		// Non-integer scaling historically fills the selected output when
		// aspect correction is disabled.
		display_width = output_width;
		display_height = output_height;
	}
}

static inline int amiberry_gfx_native_integer_scale(
	int render_width, int render_height, int display_width, int display_height)
{
	if (render_width <= 0 || render_height <= 0 || display_width <= 0 || display_height <= 0) {
		return 1;
	}

	const int horizontal_scale = render_width / display_width;
	const int vertical_scale = render_height / display_height;
	return horizontal_scale > 0 && vertical_scale > 0
		? (horizontal_scale < vertical_scale ? horizontal_scale : vertical_scale)
		: 1;
}

static inline float amiberry_gfx_fullscreen_framebuffer_aspect(
	const float content_aspect, const int framebuffer_width, const int framebuffer_height,
	const int desktop_width, const int desktop_height)
{
	if (content_aspect <= 0.0f || framebuffer_width <= 0 || framebuffer_height <= 0
		|| desktop_width <= 0 || desktop_height <= 0) {
		return content_aspect;
	}

	// Exclusive modes can be stretched to the desktop's physical output area.
	// Pre-compensate in framebuffer coordinates so the stretched result retains
	// the requested content aspect on the display.
	const float framebuffer_aspect = static_cast<float>(framebuffer_width) / framebuffer_height;
	const float desktop_aspect = static_cast<float>(desktop_width) / desktop_height;
	return content_aspect * framebuffer_aspect / desktop_aspect;
}

#endif // AMIBERRY_GFX_GEOMETRY_H
