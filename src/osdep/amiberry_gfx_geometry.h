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

static inline void amiberry_gfx_correct_aspect_integer_dimensions(
	const int render_width, const int render_height, const int source_width,
	const int display_height, const float target_aspect,
	int& dest_width, int& dest_height)
{
	if (render_width <= 0 || render_height <= 0 || source_width <= 0
		|| display_height <= 0 || target_aspect <= 0.0f) {
		dest_width = 1;
		dest_height = 1;
		return;
	}

	// Start with a bounded aspect-correct fit. If even 1x integer scaling does
	// not fit, retain this fractional fallback instead of clipping the viewport.
	dest_width = render_width;
	dest_height = static_cast<int>(render_width / target_aspect);
	if (dest_height > render_height) {
		dest_height = render_height;
		dest_width = static_cast<int>(render_height * target_aspect);
	}
	if (dest_width < 1) dest_width = 1;
	if (dest_height < 1) dest_height = 1;

	const int vertical_scale = dest_height / display_height;
	if (vertical_scale < 1 || source_width > render_width) {
		return;
	}

	const float ideal_width = display_height * vertical_scale * target_aspect;
	const int lower_scale = static_cast<int>(ideal_width / source_width);
	if (lower_scale < 1) {
		return;
	}
	const int upper_scale = lower_scale + 1;
	const float lower_aspect = static_cast<float>(source_width * lower_scale)
		/ (display_height * vertical_scale);
	const float upper_aspect = static_cast<float>(source_width * upper_scale)
		/ (display_height * vertical_scale);
	const float lower_delta = lower_aspect > target_aspect
		? lower_aspect - target_aspect : target_aspect - lower_aspect;
	const float upper_delta = upper_aspect > target_aspect
		? upper_aspect - target_aspect : target_aspect - upper_aspect;

	int horizontal_scale = lower_scale;
	if (source_width * upper_scale <= render_width && upper_delta < lower_delta) {
		horizontal_scale = upper_scale;
	}
	while (source_width * horizontal_scale > render_width && horizontal_scale > 1) {
		horizontal_scale--;
	}

	dest_width = source_width * horizontal_scale;
	dest_height = display_height * vertical_scale;
}

static inline void amiberry_gfx_shader_render_dimensions(
	const int dest_width, const int dest_height,
	const int source_width, const int source_height,
	int& render_width, int& render_height)
{
	render_width = dest_width;
	render_height = dest_height;
	if (dest_width <= 0 || dest_height <= 0 || source_width <= 0 || source_height <= 0
		|| (dest_width >= source_width && dest_height >= source_height)) {
		return;
	}

	const float width_scale = static_cast<float>(source_width) / dest_width;
	const float height_scale = static_cast<float>(source_height) / dest_height;
	const float scale = width_scale > height_scale ? width_scale : height_scale;
	render_width = static_cast<int>(dest_width * scale);
	render_height = static_cast<int>(dest_height * scale);
}

#endif // AMIBERRY_GFX_GEOMETRY_H
