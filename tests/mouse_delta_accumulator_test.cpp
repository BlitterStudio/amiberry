#include <cmath>
#include <iostream>
#include <string>

#include "amiberry_mouse_delta.h"

// Pure-header test for the Android/ChromeOS sub-pixel mouse accumulator
// (plan U2 / KTD1). The captured Android mouse path keeps the window
// absolute (#2285), so SDL computes hover-position deltas as floats that can
// be smaller than one pixel per event. handle_mouse_motion_event() must not
// truncate them per event; the accumulator carries the un-emitted fraction
// forward instead. This test exercises the accumulator in isolation, the way
// tests/mouse_capture_mode_test.cpp exercises the capture policy header.

static int failures = 0;

static void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		failures++;
	}
}

static void expect_delta(const amiberry_mouse_delta& actual, int expected_dx,
	int expected_dy, const std::string& message)
{
	if (actual.dx != expected_dx || actual.dy != expected_dy) {
		std::cerr << "FAIL: " << message << ": expected (" << expected_dx
			<< ", " << expected_dy << "), got (" << actual.dx << ", "
			<< actual.dy << ")\n";
		failures++;
	}
}

static void expect_residual(const amiberry_mouse_delta_accumulator& accumulator,
	float expected_x, float expected_y, const std::string& message)
{
	constexpr float epsilon = 1e-6f;
	if (std::fabs(accumulator.residual_x() - expected_x) > epsilon
		|| std::fabs(accumulator.residual_y() - expected_y) > epsilon) {
		std::cerr << "FAIL: " << message << ": expected residual ("
			<< expected_x << ", " << expected_y << "), got ("
			<< accumulator.residual_x() << ", "
			<< accumulator.residual_y() << ")\n";
		failures++;
	}
}

static void expect_residual_bounded(
	const amiberry_mouse_delta_accumulator& accumulator,
	const std::string& message)
{
	expect(accumulator.residual_x() > -1.0f && accumulator.residual_x() < 1.0f,
		message + " (x residual must stay within (-1, 1))");
	expect(accumulator.residual_y() > -1.0f && accumulator.residual_y() < 1.0f,
		message + " (y residual must stay within (-1, 1))");
}

// AE1 logic: a drift slower than one pixel per event must still move the
// guest pointer — nothing until the fraction completes a whole step, then a
// single step at once.
static void test_slow_positive_drift_accumulates()
{
	amiberry_mouse_delta_accumulator accumulator;

	for (int i = 0; i < 4; ++i)
		expect_delta(accumulator.feed(0.2f, 0.0f), 0, 0,
			"sub-pixel drift below one pixel must not emit early " + std::to_string(i));
	expect_delta(accumulator.feed(0.2f, 0.0f), 1, 0,
		"five 0.2px events must complete one guest step");
	expect_residual_bounded(accumulator, "after positive drift");

	// Long-run accounting: total fed equals total emitted plus residual.
	amiberry_mouse_delta_accumulator steady;
	int emitted_x = 0;
	int emitted_y = 0;
	double fed_x = 0.0;
	double fed_y = 0.0;
	for (int i = 0; i < 1000; ++i) {
		const auto whole = steady.feed(0.637f, -0.411f);
		emitted_x += whole.dx;
		emitted_y += whole.dy;
		fed_x += 0.637;
		fed_y += -0.411;
		expect_residual_bounded(steady, "steady mixed drift keeps residual bounded");
	}
	const double accounted_x = emitted_x + steady.residual_x();
	const double accounted_y = emitted_y + steady.residual_y();
	expect(std::fabs(accounted_x - fed_x) < 1e-3 && std::fabs(accounted_y - fed_y) < 1e-3,
		"emitted deltas plus residual must account for every fed pixel");
}

// Negative drift and direction reversals must never emit a step that opposes
// the accumulated motion (the sign error a floor-style remainder would make).
static void test_negative_drift_and_reversal()
{
	amiberry_mouse_delta_accumulator accumulator;

	expect_delta(accumulator.feed(-0.5f, 0.0f), 0, 0,
		"negative sub-pixel drift must not emit early");
	expect_delta(accumulator.feed(-0.5f, 0.0f), -1, 0,
		"two -0.5px events must complete one negative guest step");

	// Reverse direction: the leftover negative fraction must not turn a
	// positive step negative or vice versa.
	expect_delta(accumulator.feed(0.5f, 0.0f), 0, 0,
		"reversal fraction must not emit the wrong sign");
	expect_delta(accumulator.feed(0.5f, 0.0f), 1, 0,
		"reversed drift must complete a positive guest step");

	// Back and forth with no net motion emits nothing at all.
	amiberry_mouse_delta_accumulator wiggle;
	for (int i = 0; i < 100; ++i) {
		expect_delta(wiggle.feed(-0.7f, 0.35f), 0, 0,
			"canceling wiggle half must stay sub-pixel");
		expect_delta(wiggle.feed(0.7f, -0.35f), 0, 0,
			"canceling wiggle must net to zero motion");
	}
	expect_residual(wiggle, 0.0f, 0.0f,
		"fully canceled motion must leave no residual");

	// A negative fraction must carry as negative (-0.3), not wrap toward a
	// positive remainder the way truncation-toward-negative-infinity would
	// leave behind (+0.7 after emitting a spurious -1).
	amiberry_mouse_delta_accumulator negative;
	expect_delta(negative.feed(-0.3f, 0.0f), 0, 0,
		"fractional negative drift must not emit a phantom step");
	expect_residual(negative, -0.3f, 0.0f,
		"negative residual must keep its sign");
}

// Capture transitions reset the accumulator so a stale fraction never leaks
// into a new capture session.
static void test_reset_clears_residue()
{
	amiberry_mouse_delta_accumulator accumulator;

	expect_delta(accumulator.feed(0.6f, 0.0f), 0, 0,
		"first 0.6px event stays pending");
	expect_residual(accumulator, 0.6f, 0.0f,
		"pending fraction must be carried before reset");

	accumulator.reset();
	expect_residual(accumulator, 0.0f, 0.0f, "reset must clear the residual");
	expect_delta(accumulator.feed(0.6f, 0.0f), 0, 0,
		"post-reset event must start from zero, not inherit 0.6");
	expect_delta(accumulator.feed(0.6f, 0.0f), 1, 0,
		"post-reset accumulation still completes steps normally");
}

// The absolute/tablet dispatch in handle_mouse_motion_event resets instead of
// inheriting: switching modes mid-stream (and back) must behave as if the
// relative session started fresh.
static void test_absolute_switch_resets_cleanly()
{
	amiberry_mouse_delta_accumulator accumulator;

	expect_delta(accumulator.feed(0.9f, -0.9f), 0, 0,
		"0.9px pending before the mode switch");

	// Absolute dispatch: positions go through unaccumulated; the relative
	// remainder is dropped (mirrors the tablet branch reset).
	accumulator.reset();

	expect_delta(accumulator.feed(0.9f, -0.9f), 0, 0,
		"stale relative fraction must not survive the mode switch");
	expect_delta(accumulator.feed(0.9f, -0.9f), 1, -1,
		"resumed relative tracking completes steps per axis");
	expect_residual_bounded(accumulator, "after mode switch");
}

// Whole-pixel deltas pass through untouched, each axis independent of the
// other.
static void test_whole_deltas_and_axis_independence()
{
	amiberry_mouse_delta_accumulator accumulator;

	expect_delta(accumulator.feed(3.0f, -2.0f), 3, -2,
		"whole-pixel deltas must pass through unchanged");
	expect_residual(accumulator, 0.0f, 0.0f,
		"whole-pixel deltas must leave no residual");

	amiberry_mouse_delta_accumulator axes;
	expect_delta(axes.feed(0.0f, 0.6f), 0, 0,
		"first y-only drift event must stay sub-pixel");
	expect_delta(axes.feed(0.0f, 0.6f), 0, 1,
		"second 0.6px y event must step y only");
	expect_delta(axes.feed(0.0f, 0.6f), 0, 0,
		"leftover 0.2px y fraction must not emit again");
	expect_residual(axes, 0.0f, 0.8f,
		"x residual must stay zero while y carries its fraction");
}

int main()
{
	test_slow_positive_drift_accumulates();
	test_negative_drift_and_reversal();
	test_reset_clears_residue();
	test_absolute_switch_resets_cleanly();
	test_whole_deltas_and_axis_independence();
	return failures == 0 ? 0 : 1;
}
