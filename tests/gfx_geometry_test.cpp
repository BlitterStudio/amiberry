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
	amiberry_gfx_auto_crop_presentation_dimensions(
		640, 400, true, false, true, 1728, 1117, width, height);

	expect_int_eq(width, 640,
		"uncorrected NTSC integer scaling must keep the normalized crop width");
	expect_int_eq(height, 400,
		"uncorrected NTSC integer scaling must not apply 6:5 vertical correction");
	expect_int_eq(amiberry_gfx_native_integer_scale(1440, 1136, width, height), 2,
		"uncorrected NTSC crop must scale uniformly to the largest whole-number fit");
}

static void test_ntsc_aspect_correction_and_legacy_stretch_remain_distinct()
{
	int width = 0;
	int height = 0;
	amiberry_gfx_auto_crop_presentation_dimensions(
		640, 400, true, true, true, 1728, 1117, width, height);
	expect_int_eq(width, 640, "corrected NTSC crop width must stay normalized");
	expect_int_eq(height, 480, "corrected NTSC crop height must apply 6:5 correction");

	amiberry_gfx_auto_crop_presentation_dimensions(
		640, 400, true, false, false, 1728, 1117, width, height);
	expect_int_eq(width, 1728,
		"uncorrected non-integer scaling must preserve the historical output-width stretch");
	expect_int_eq(height, 1117,
		"uncorrected non-integer scaling must preserve the historical output-height stretch");
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

int main()
{
	test_ntsc_integer_scaling_without_aspect_uses_crop_geometry();
	test_ntsc_aspect_correction_and_legacy_stretch_remain_distinct();
	test_integer_scaling_never_fractionally_downscales();
	test_exclusive_fullscreen_compensates_for_display_mode_stretch();
	return failures == 0 ? 0 : 1;
}
