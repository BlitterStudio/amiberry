#include <iostream>

#include "amiberry_gui_pacing.h"

static int failures;

static void expect_uint_eq(const uint32_t actual, const uint32_t expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

static void test_high_refresh_display_is_capped_to_sixty()
{
	// The regression: a 240 Hz (or VRR) panel let the GUI redraw 240 times a
	// second because presentation was the only throttle.
	expect_uint_eq(amiberry_gui_frame_delay_ms(1.0f, 240.0f), 15,
		"a 1 ms frame on a 240 Hz display must sleep out the rest of a 60 fps frame");
	expect_uint_eq(amiberry_gui_frame_delay_ms(1.0f, 144.0f), 15,
		"144 Hz must be capped to 60 fps as well");
	expect_uint_eq(amiberry_gui_frame_delay_ms(0.0f, 60.0f), 16,
		"an instant frame on a 60 Hz display must sleep a whole frame");
}

static void test_unknown_refresh_rate_falls_back_to_sixty()
{
	expect_uint_eq(amiberry_gui_frame_delay_ms(1.0f, 0.0f), 15,
		"an unreported refresh rate must pace at 60 fps, not spin");
	expect_uint_eq(amiberry_gui_frame_delay_ms(1.0f, -1.0f), 15,
		"a negative refresh rate must pace at 60 fps, not spin");
}

static void test_low_refresh_display_paces_to_its_own_rate()
{
	expect_uint_eq(amiberry_gui_frame_delay_ms(1.0f, 50.0f), 19,
		"a 50 Hz display must get a 20 ms frame budget");
	expect_uint_eq(amiberry_gui_frame_delay_ms(1.0f, 30.0f), 19,
		"below 50 Hz the GUI must not be paced slower than 50 fps");
}

static void test_slow_frames_are_never_delayed()
{
	expect_uint_eq(amiberry_gui_frame_delay_ms(20.0f, 240.0f), 0,
		"a frame that overran its budget must not sleep");
	expect_uint_eq(amiberry_gui_frame_delay_ms(5000.0f, 60.0f), 0,
		"a very slow frame must not produce a wrapped-around delay");
}

int main()
{
	test_high_refresh_display_is_capped_to_sixty();
	test_unknown_refresh_rate_falls_back_to_sixty();
	test_low_refresh_display_paces_to_its_own_rate();
	test_slow_frames_are_never_delayed();

	if (failures > 0) {
		std::cerr << failures << " GUI pacing assertion(s) failed\n";
		return 1;
	}

	std::cout << "GUI pacing tests passed\n";
	return 0;
}
