#include <iostream>

#include "amiberry_gfx_geometry.h"

static int failures;

static void expect_int_eq(const int actual, const int expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

static void expect_float_near(const float actual, const float expected, const float tolerance, const char* message)
{
	if (actual < expected - tolerance || actual > expected + tolerance) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

static void test_ntsc_integer_scaling_without_aspect_uses_crop_geometry()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_auto_crop_presentation_dimensions(640, 400, true, false, width, height);

	expect_int_eq(width, 640,
		"uncorrected NTSC integer scaling must keep the normalized crop width");
	expect_int_eq(height, 400,
		"uncorrected NTSC integer scaling must not apply 6:5 vertical correction");
	expect_int_eq(amiberry_gfx_native_integer_scale(1440, 1136, width, height), 2,
		"uncorrected NTSC crop must scale uniformly to the largest whole-number fit");
}

static void test_ntsc_aspect_correction_applies_only_when_requested()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_auto_crop_presentation_dimensions(640, 400, true, true, width, height);
	expect_int_eq(width, 640, "corrected NTSC crop width must stay normalized");
	expect_int_eq(height, 480, "corrected NTSC crop height must apply 6:5 correction");

	amiberry_gfx_auto_crop_presentation_dimensions(640, 400, false, true, width, height);
	expect_int_eq(width, 640, "corrected PAL crop width must stay normalized");
	expect_int_eq(height, 400, "corrected PAL crop must not apply the NTSC 6:5 correction");
}

// Turning aspect correction off keeps the crop's own pixel aspect. It must not
// stretch the crop to the output: filling the window is the Stretch scaling
// method's job, and having both do it made the checkbox mean two things.
static void test_uncorrected_crop_keeps_its_own_aspect()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_auto_crop_presentation_dimensions(640, 400, true, false, width, height);
	expect_int_eq(width, 640, "uncorrected non-integer scaling must not stretch to the output width");
	expect_int_eq(height, 400, "uncorrected non-integer scaling must not stretch to the output height");
}

static void test_integer_scaling_never_fractionally_downscales()
{
	expect_int_eq(amiberry_gfx_native_integer_scale(600, 350, 640, 400), 1,
		"integer scaling must retain a 1x source when the render area is smaller");
}

static void test_exclusive_fullscreen_compensates_for_display_mode_stretch()
{
	expect_float_near(amiberry_gfx_fullscreen_framebuffer_aspect(
		4.0f / 3.0f, 800, 600, 1920, 1080), 1.0f, 0.0001f,
		"4:3 content in a stretched 800x600 mode must use a square framebuffer viewport");

	expect_float_near(amiberry_gfx_fullscreen_framebuffer_aspect(
		4.0f / 3.0f, 1280, 720, 1920, 1080), 4.0f / 3.0f, 0.0001f,
		"a fullscreen mode matching the desktop aspect must not change the content aspect");

	expect_float_near(amiberry_gfx_fullscreen_framebuffer_aspect(
		1.25f, 800, 600, 1920, 1080), 0.9375f, 0.0001f,
		"cropped content aspect must receive the same fullscreen stretch compensation");

	expect_float_near(amiberry_gfx_fullscreen_framebuffer_aspect(
		4.0f / 3.0f, 0, 600, 1920, 1080), 4.0f / 3.0f, 0.0001f,
		"invalid fullscreen dimensions must leave the requested aspect unchanged");
}

static void test_corrected_integer_scaling_stays_within_fullscreen_mode()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_correct_aspect_integer_dimensions(
		5120, 1440, 640, 480, 4.0f / 3.0f, width, height);
	expect_int_eq(width, 1920,
		"normalized 640x480 hires crop must retain its full 4:3 integer width");
	expect_int_eq(height, 1440,
		"normalized 640x480 hires crop must fill a 1440-line integer target");

	amiberry_gfx_correct_aspect_integer_dimensions(
		800, 600, 640, 640, 1.0f, width, height);
	expect_int_eq(width, 600,
		"compensated 640-wide content must fit within an 800x600 fullscreen mode");
	expect_int_eq(height, 600,
		"compensated 640-tall presentation must not be clipped by a 600-line mode");

	amiberry_gfx_correct_aspect_integer_dimensions(
		800, 600, 720, 720, 1.0f, width, height);
	expect_int_eq(width, 600,
		"compensated 720-wide content must use the bounded aspect fit");
	expect_int_eq(height, 600,
		"compensated 720-tall presentation must use the bounded aspect fit");

	amiberry_gfx_correct_aspect_integer_dimensions(
		800, 600, 640, 480, 1.0f, width, height);
	expect_int_eq(width, 600,
		"a compensated target narrower than 1x must retain the bounded width");
	expect_int_eq(height, 600,
		"a compensated target narrower than 1x must retain the target aspect");

	amiberry_gfx_correct_aspect_integer_dimensions(
		3840, 2160, 320, 270, 4.0f / 3.0f, width, height);
	expect_int_eq(width, 2880,
		"integer scaling must retain the closest bounded horizontal scale");
	expect_int_eq(height, 2160,
		"integer scaling must retain the largest bounded vertical scale");
}

static void test_native_content_grid_ignores_configured_pixel_repetition()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_native_content_dimensions(
		640, 512, 1, 1, 0, 0, width, height);
	expect_int_eq(width, 320,
		"low-res content must use its 320-pixel grid inside a hires surface");
	expect_int_eq(height, 256,
		"non-interlaced content must use its 256-line grid inside a doubled surface");

	amiberry_gfx_native_content_dimensions(
		641, 513, 1, 1, 0, 0, width, height);
	expect_int_eq(width, 321,
		"odd crop widths must round up so edge content remains represented");
	expect_int_eq(height, 257,
		"odd crop heights must round up so edge content remains represented");

	amiberry_gfx_native_content_dimensions(
		640, 512, 1, 1, 1, 0, width, height);
	expect_int_eq(width, 640,
		"hires content must retain its horizontal source grid");
	expect_int_eq(height, 256,
		"non-interlaced hires content must still remove configured line doubling");
}

static void test_crop_rect_maps_across_autoswitch_resolutions()
{
	const AmiberryGfxRect hires_crop{ 76, 34, 640, 414 };
	const AmiberryGfxRect lores_crop = amiberry_gfx_scale_crop_rect(
		hires_crop, 1, 1, 0, 0);
	expect_int_eq(lores_crop.x, 38,
		"Autoswitch should map the crop's left edge to the lower-resolution surface");
	expect_int_eq(lores_crop.y, 17,
		"Autoswitch should map the crop's top edge to the lower-resolution surface");
	expect_int_eq(lores_crop.w, 320,
		"Autoswitch should retain the crop width in native content units");
	expect_int_eq(lores_crop.h, 207,
		"Autoswitch should retain the crop height in native content units");

	const AmiberryGfxRect restored = amiberry_gfx_scale_crop_rect(
		lores_crop, 0, 0, 1, 1);
	expect_int_eq(restored.x, hires_crop.x,
		"Mapping an aligned crop back to hires should restore its left edge");
	expect_int_eq(restored.y, hires_crop.y,
		"Mapping an aligned crop back to doubled lines should restore its top edge");
	expect_int_eq(restored.w, hires_crop.w,
		"Mapping an aligned crop back to hires should restore its width");
	expect_int_eq(restored.h, hires_crop.h,
		"Mapping an aligned crop back to doubled lines should restore its height");

	const AmiberryGfxRect odd_crop{ 73, 48, 643, 400 };
	const AmiberryGfxRect odd_lores = amiberry_gfx_scale_crop_rect(
		odd_crop, 1, 1, 0, 0);
	expect_int_eq(odd_lores.x, 36,
		"Downscaling should floor an odd crop origin");
	expect_int_eq(odd_lores.w, 322,
		"Downscaling should round up the far edge so no content is lost");
}

static void test_provisional_crop_preserves_last_real_geometry()
{
	expect_int_eq(amiberry_gfx_should_use_cached_crop(true, true, true), true,
		"A no-bitplane fallback must retain the last real crop during autoswitch");
	expect_int_eq(amiberry_gfx_should_use_cached_crop(false, false, true), true,
		"An invalid crop must retain the last real geometry");
	expect_int_eq(amiberry_gfx_should_use_cached_crop(true, false, true), false,
		"A real current crop must replace the cache");
	expect_int_eq(amiberry_gfx_should_use_cached_crop(true, true, false), false,
		"Startup without a real crop must keep using the provisional fallback");

	const AmiberryGfxRect cached_hires_crop{76, 17, 640, 200};
	const AmiberryGfxRect retained_lores_crop = amiberry_gfx_scale_crop_rect(
		cached_hires_crop, 1, 0, 0, 0);
	expect_int_eq(retained_lores_crop.w, 320,
		"The cached game crop must map to its 320-pixel low-resolution grid");
	expect_int_eq(retained_lores_crop.h, 200,
		"The cached game crop must retain its 200 active lines");

	int display_width = 0;
	int display_height = 0;
	amiberry_gfx_auto_crop_presentation_dimensions(
		retained_lores_crop.w, retained_lores_crop.h, true, true,
		display_width, display_height);
	int scaled_width = 0;
	int scaled_height = 0;
	expect_int_eq(amiberry_gfx_auto_integer_dimensions(
		5120, 1440, display_width, display_width, display_height, false,
		4.0f / 3.0f, 4.0f / 3.0f, scaled_width, scaled_height), true,
		"The retained 320x200 NTSC crop must select integer scaling immediately");
}

static void test_native_content_grid_uses_the_largest_integer_fit()
{
	int content_width = 0;
	int content_height = 0;
	amiberry_gfx_native_content_dimensions(
		640, 400, 1, 1, 0, 0, content_width, content_height);

	int width = 0;
	int height = 0;
	amiberry_gfx_native_integer_dimensions(
		5120, 1440, content_width, content_width, content_height,
		true, 8.0f / 5.0f, width, height);
	expect_int_eq(width, 2240,
		"low-res content must scale from the native grid without resolution autoswitch");
	expect_int_eq(height, 1400,
		"native-grid scaling must use the largest whole vertical multiple");
}

static void test_auto_scaling_uses_integer_only_for_a_lossless_fit()
{
	int width = 0;
	int height = 0;
	bool use_integer = amiberry_gfx_auto_integer_dimensions(
		5120, 1440, 640, 640, 480, true,
		4.0f / 3.0f, 4.0f / 3.0f, width, height);
	expect_int_eq(use_integer, true,
		"auto scaling must use integer scaling for an exact 3x native fit");
	expect_int_eq(width, 1920,
		"auto integer scaling must retain the full lossless width");
	expect_int_eq(height, 1440,
		"auto integer scaling must retain the full lossless height");
	use_integer = amiberry_gfx_auto_integer_dimensions(
		5120, 1440, 640, 640, 480, false,
		4.0f / 3.0f, 4.0f / 3.0f, width, height);
	expect_int_eq(use_integer, true,
		"SDL logical presentation must recognize the same exact 3x fit");

	use_integer = amiberry_gfx_auto_integer_dimensions(
		1920, 1080, 640, 640, 480, true,
		4.0f / 3.0f, 4.0f / 3.0f, width, height);
	expect_int_eq(use_integer, false,
		"auto scaling must keep best fit when integer scaling would shrink the image");
	expect_int_eq(width, 1440,
		"fractional auto scaling must fill the available 4:3 width");
	expect_int_eq(height, 1080,
		"fractional auto scaling must fill the available height");
}

static void test_shader_render_size_resolves_to_compensated_viewport()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_shader_render_dimensions(
		600, 600, 640, 480, width, height);
	expect_int_eq(width, 640,
		"shader rendering must retain at least the source width");
	expect_int_eq(height, 640,
		"shader rendering must preserve the compensated viewport aspect");

	amiberry_gfx_shader_render_dimensions(
		1440, 1080, 640, 480, width, height);
	expect_int_eq(width, 1440,
		"shader rendering must keep a sufficiently large destination width");
	expect_int_eq(height, 1080,
		"shader rendering must keep a sufficiently large destination height");
}

static void expect_rect_eq(const AmiberryGfxRect& actual,
	const int x, const int y, const int w, const int h, const char* message)
{
	expect_int_eq(actual.x, x, message);
	expect_int_eq(actual.y, y, message);
	expect_int_eq(actual.w, w, message);
	expect_int_eq(actual.h, h, message);
}

static void test_full_drawable_presentation_geometry_is_unchanged()
{
	const AmiberryGfxRect available_area{0, 0, 1920, 1080};
	int aspect_width = 0;
	int aspect_height = 0;
	amiberry_gfx_aspect_fit_dimensions(
		available_area.w, available_area.h, 4.0f / 3.0f,
		aspect_width, aspect_height);
	const AmiberryGfxRect aspect_fit = amiberry_gfx_final_presentation_rect(
		available_area, aspect_width, aspect_height, 4.0f / 3.0f, false);
	expect_rect_eq(aspect_fit, 240, 0, 1440, 1080,
		"full-drawable aspect fit must remain centered");

	const int scale = amiberry_gfx_native_integer_scale(1920, 1080, 640, 400);
	const AmiberryGfxRect integer_scaled = amiberry_gfx_final_presentation_rect(
		available_area, 640 * scale, 400 * scale, 640.0f / 400.0f, false);
	expect_rect_eq(integer_scaled, 320, 140, 1280, 800,
		"full-drawable integer scaling must retain its whole-number size");
}

static void test_bezel_area_centers_integer_scaled_presentation()
{
	const AmiberryGfxRect available_area{100, 50, 1000, 700};
	const int scale = amiberry_gfx_native_integer_scale(
		available_area.w, available_area.h, 320, 240);
	const AmiberryGfxRect final_rect = amiberry_gfx_final_presentation_rect(
		available_area, 320 * scale, 240 * scale, 4.0f / 3.0f, true);

	expect_rect_eq(final_rect, 280, 160, 640, 480,
		"integer-scaled presentation must be centered within the bezel hole origin");
}

static void test_rtg_center_remains_source_sized_inside_bezel_area()
{
	const AmiberryGfxRect available_area{250, 120, 640, 480};
	const AmiberryGfxRect final_rect = amiberry_gfx_final_presentation_rect(
		available_area, 800, 600, 4.0f / 3.0f, false);

	expect_rect_eq(final_rect, 170, 60, 800, 600,
		"RTG Center must remain source-sized even when it exceeds the bezel hole");
}

static void test_bezel_bounded_integer_scaling_falls_back_below_one_x()
{
	const AmiberryGfxRect available_area{100, 50, 300, 180};
	const AmiberryGfxRect final_rect = amiberry_gfx_final_presentation_rect(
		available_area, 640, 400, 640.0f / 400.0f, true);

	expect_rect_eq(final_rect, 106, 50, 288, 180,
		"sub-1x integer scaling must use a centered fractional aspect fit inside the bezel hole");
}

static void test_invalid_bounded_fallback_inputs_keep_presentation_size()
{
	AmiberryGfxRect final_rect = amiberry_gfx_final_presentation_rect(
		{10, 20, 0, 180}, 640, 400, 640.0f / 400.0f, true);
	expect_rect_eq(final_rect, -310, -90, 640, 400,
		"a zero-width fallback area must not resize the presentation");

	final_rect = amiberry_gfx_final_presentation_rect(
		{10, 20, 300, -10}, 640, 400, 640.0f / 400.0f, true);
	expect_rect_eq(final_rect, -160, -185, 640, 400,
		"a negative-height fallback area must not resize the presentation");

	final_rect = amiberry_gfx_final_presentation_rect(
		{10, 20, 300, 180}, 640, 400, 0.0f, true);
	expect_rect_eq(final_rect, -160, -90, 640, 400,
		"an invalid fallback aspect must not resize the presentation");
}

static void test_oversized_centered_presentation_covers_drawable()
{
	const AmiberryGfxRect drawable_area{0, 0, 1920, 1080};
	const AmiberryGfxRect final_rect = amiberry_gfx_final_presentation_rect(
		drawable_area, 2000, 1200, 5.0f / 3.0f, false);

	expect_int_eq(amiberry_gfx_rect_covers_area(final_rect, drawable_area), 1,
		"an oversized presentation centered on the drawable must cover every edge");
}

static void test_oversized_offset_presentation_leaves_drawable_uncovered()
{
	const AmiberryGfxRect drawable_area{0, 0, 1920, 1080};
	const AmiberryGfxRect offset_bezel_area{250, 120, 640, 480};
	const AmiberryGfxRect final_rect = amiberry_gfx_final_presentation_rect(
		offset_bezel_area, 2000, 1200, 5.0f / 3.0f, false);

	expect_int_eq(amiberry_gfx_rect_covers_area(final_rect, drawable_area), 0,
		"an oversized presentation shifted by its bezel hole must clear uncovered drawable edges");
}

static float native_source_x(const AmiberryGfxInputOffset& offset, const float x)
{
	// getgfxoffset() hands out 1/mx; get_mouse_position() computes x * fmx - fdx.
	return x * (1.0f / offset.mx) - offset.dx;
}

static float native_source_y(const AmiberryGfxInputOffset& offset, const float y)
{
	return y * (1.0f / offset.my) - offset.dy;
}

// Issue #2367: with Manual Crop and offsets the renderer presents only the
// crop rect, so the pointer must map against it rather than the full surface.
static void test_manual_crop_pointer_maps_to_presented_source()
{
	const AmiberryGfxRect source = amiberry_gfx_presented_source_rect(
		{100, 40, 400, 300}, 720, 568);
	expect_int_eq(source.x, 100, "manual crop source must keep its horizontal offset");
	expect_int_eq(source.y, 40, "manual crop source must keep its vertical offset");
	expect_int_eq(source.w, 400, "manual crop source must keep its width");
	expect_int_eq(source.h, 300, "manual crop source must keep its height");

	// 4x presentation, letterboxed 160 px from the left edge.
	const AmiberryGfxRect quad{160, 0, 1600, 1200};
	const AmiberryGfxInputOffset offset = amiberry_gfx_input_offset(quad,
		static_cast<float>(source.x), static_cast<float>(source.y),
		static_cast<float>(source.w), static_cast<float>(source.h), false);

	expect_float_near(native_source_x(offset, 160.0f), 100.0f, 0.001f,
		"the left edge of the presented crop must map to the crop's first source column");
	expect_float_near(native_source_y(offset, 0.0f), 40.0f, 0.001f,
		"the top edge of the presented crop must map to the crop's first source row");
	expect_float_near(native_source_x(offset, 160.0f + 4.0f * 50.0f), 150.0f, 0.001f,
		"host motion inside the crop must advance one source pixel per presented pixel");
	expect_float_near(native_source_x(offset, 1760.0f), 500.0f, 0.001f,
		"the right edge of the presented crop must map to the crop's right source edge");
	expect_float_near(native_source_y(offset, 1200.0f), 340.0f, 0.001f,
		"the bottom edge of the presented crop must map to the crop's bottom source edge");
}

static void test_presented_source_rect_matches_renderer_clamping()
{
	const AmiberryGfxRect overflow = amiberry_gfx_presented_source_rect(
		{600, 500, 400, 300}, 720, 568);
	expect_int_eq(overflow.x, 600, "an overflowing crop keeps its in-surface origin");
	expect_int_eq(overflow.w, 120, "an overflowing crop is clipped to the surface width");
	expect_int_eq(overflow.h, 68, "an overflowing crop is clipped to the surface height");

	const AmiberryGfxRect negative = amiberry_gfx_presented_source_rect(
		{-20, -10, 400, 300}, 720, 568);
	expect_int_eq(negative.x, 0, "a negative crop origin is clamped to the surface");
	expect_int_eq(negative.y, 0, "a negative vertical crop origin is clamped to the surface");
	expect_int_eq(negative.w, 400, "a clamped crop origin keeps the presented width");

	const AmiberryGfxRect empty = amiberry_gfx_presented_source_rect({0, 0, 0, 0}, 720, 568);
	expect_int_eq(empty.w, 720, "an unset crop presents the whole surface width");
	expect_int_eq(empty.h, 568, "an unset crop presents the whole surface height");

	const AmiberryGfxRect outside = amiberry_gfx_presented_source_rect(
		{800, 0, 100, 100}, 720, 568);
	expect_int_eq(outside.x, 0, "a crop entirely outside the surface falls back to the whole surface");
	expect_int_eq(outside.w, 720, "a crop entirely outside the surface presents the whole width");
}

static void test_rtg_pointer_mapping_removes_letterbox()
{
	const AmiberryGfxInputOffset offset = amiberry_gfx_input_offset(
		{240, 0, 1440, 1080}, 0.0f, 0.0f, 640.0f, 480.0f, true);
	// get_mouse_position(): (x * fmx) + (dx * fmx).
	const float x = 240.0f + 2.25f * 320.0f;
	expect_float_near(x / offset.mx + offset.dx / offset.mx, 320.0f, 0.001f,
		"RTG pointer mapping must subtract the letterbox offset before scaling");
}

int main()
{
	test_ntsc_integer_scaling_without_aspect_uses_crop_geometry();
	test_ntsc_aspect_correction_applies_only_when_requested();
	test_uncorrected_crop_keeps_its_own_aspect();
	test_native_content_grid_ignores_configured_pixel_repetition();
	test_crop_rect_maps_across_autoswitch_resolutions();
	test_provisional_crop_preserves_last_real_geometry();
	test_native_content_grid_uses_the_largest_integer_fit();
	test_integer_scaling_never_fractionally_downscales();
	test_exclusive_fullscreen_compensates_for_display_mode_stretch();
	test_corrected_integer_scaling_stays_within_fullscreen_mode();
	test_auto_scaling_uses_integer_only_for_a_lossless_fit();
	test_shader_render_size_resolves_to_compensated_viewport();
	test_full_drawable_presentation_geometry_is_unchanged();
	test_bezel_area_centers_integer_scaled_presentation();
	test_rtg_center_remains_source_sized_inside_bezel_area();
	test_bezel_bounded_integer_scaling_falls_back_below_one_x();
	test_invalid_bounded_fallback_inputs_keep_presentation_size();
	test_oversized_centered_presentation_covers_drawable();
	test_oversized_offset_presentation_leaves_drawable_uncovered();
	test_manual_crop_pointer_maps_to_presented_source();
	test_presented_source_rect_matches_renderer_clamping();
	test_rtg_pointer_mapping_removes_letterbox();
	return failures == 0 ? 0 : 1;
}
