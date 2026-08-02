#ifndef AMIBERRY_GFX_GEOMETRY_H
#define AMIBERRY_GFX_GEOMETRY_H

struct AmiberryGfxRect
{
	int x;
	int y;
	int w;
	int h;
};

static inline void amiberry_gfx_aspect_fit_dimensions(
	const int available_width, const int available_height,
	const float target_aspect, int& width, int& height)
{
	if (available_width <= 0 || available_height <= 0 || target_aspect <= 0.0f) {
		width = 1;
		height = 1;
		return;
	}

	width = available_width;
	height = static_cast<int>(available_width / target_aspect);
	if (height > available_height) {
		height = available_height;
		width = static_cast<int>(available_height * target_aspect);
	}
	if (width < 1) width = 1;
	if (height < 1) height = 1;
}

static inline AmiberryGfxRect amiberry_gfx_final_presentation_rect(
	const AmiberryGfxRect& available_area, const int presentation_width,
	const int presentation_height, const float fallback_aspect,
	const bool use_bounded_fallback)
{
	int width = presentation_width > 0 ? presentation_width : 1;
	int height = presentation_height > 0 ? presentation_height : 1;

	if (use_bounded_fallback
		&& (width > available_area.w || height > available_area.h)
		&& available_area.w > 0 && available_area.h > 0
		&& fallback_aspect > 0.0f) {
		amiberry_gfx_aspect_fit_dimensions(
			available_area.w, available_area.h, fallback_aspect, width, height);
	}

	return {
		available_area.x + (available_area.w - width) / 2,
		available_area.y + (available_area.h - height) / 2,
		width,
		height
	};
}

static inline bool amiberry_gfx_rect_covers_area(
	const AmiberryGfxRect& rect, const AmiberryGfxRect& area)
{
	return rect.x <= area.x && rect.y <= area.y
		&& rect.x + rect.w >= area.x + area.w
		&& rect.y + rect.h >= area.y + area.h;
}

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

static inline int amiberry_gfx_content_grid_dimension(
	const int rendered_dimension, const int rendered_resolution,
	const int content_resolution)
{
	if (rendered_dimension <= 0 || rendered_resolution <= content_resolution) {
		return rendered_dimension;
	}

	const int resolution_delta = rendered_resolution - content_resolution;
	if (resolution_delta >= 30) {
		return 1;
	}
	const int pixel_repeat = 1 << resolution_delta;
	return rendered_dimension / pixel_repeat
		+ (rendered_dimension % pixel_repeat != 0 ? 1 : 0);
}

static inline void amiberry_gfx_native_content_dimensions(
	const int rendered_width, const int rendered_height,
	const int rendered_hres, const int rendered_vres,
	const int content_hres, const int content_vres,
	int& content_width, int& content_height)
{
	content_width = amiberry_gfx_content_grid_dimension(
		rendered_width, rendered_hres, content_hres);
	content_height = amiberry_gfx_content_grid_dimension(
		rendered_height, rendered_vres, content_vres);
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
	amiberry_gfx_aspect_fit_dimensions(
		render_width, render_height, target_aspect, dest_width, dest_height);

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

static inline void amiberry_gfx_native_integer_dimensions(
	const int render_width, const int render_height, const int source_width,
	const int display_width, const int display_height, const bool correct_aspect,
	const float target_aspect, int& dest_width, int& dest_height)
{
	if (correct_aspect) {
		amiberry_gfx_correct_aspect_integer_dimensions(
			render_width, render_height, source_width, display_height,
			target_aspect, dest_width, dest_height);
		return;
	}

	const int scale = amiberry_gfx_native_integer_scale(
		render_width, render_height, display_width, display_height);
	dest_width = display_width * scale;
	dest_height = display_height * scale;
}

static inline bool amiberry_gfx_auto_integer_dimensions(
	const int render_width, const int render_height, const int source_width,
	const int display_width, const int display_height, const bool correct_aspect,
	const float fit_aspect, const float integer_target_aspect,
	int& dest_width, int& dest_height)
{
	amiberry_gfx_aspect_fit_dimensions(
		render_width, render_height, fit_aspect, dest_width, dest_height);

	int integer_width = 0;
	int integer_height = 0;
	amiberry_gfx_native_integer_dimensions(
		render_width, render_height, source_width, display_width, display_height,
		correct_aspect, integer_target_aspect, integer_width, integer_height);

	const int horizontal_unit = correct_aspect ? source_width : display_width;
	const bool valid_integer_fit = horizontal_unit > 0 && display_height > 0
		&& integer_width > 0 && integer_height > 0
		&& integer_width <= render_width && integer_height <= render_height
		&& integer_width % horizontal_unit == 0
		&& integer_height % display_height == 0;
	const int width_delta = integer_width > dest_width
		? integer_width - dest_width : dest_width - integer_width;
	const int height_delta = integer_height > dest_height
		? integer_height - dest_height : dest_height - integer_height;

	// Auto adopts integer scaling only when it produces the same largest
	// aspect-fit rectangle. The one-pixel tolerance absorbs float truncation.
	if (!valid_integer_fit || width_delta > 1 || height_delta > 1) {
		return false;
	}

	dest_width = integer_width;
	dest_height = integer_height;
	return true;
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
