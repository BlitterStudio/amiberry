#include <cstdint>
#include <iostream>
#include <string>

#include "on_screen_joystick_latch.h"

using osj_latch::ReleaseLatch;

static int failures;

static void expect(const bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << message << '\n';
		++failures;
	}
}

static void test_tap_is_held_through_its_frame()
{
	ReleaseLatch latch;
	latch.press();
	latch.defer_release(17);

	expect(latch.pending(), "tap release should be pending");
	expect(!latch.release_due(17), "tap must remain pressed in its input frame");
	expect(latch.release_due(18), "tap must release at the next frame");
	expect(!latch.pending(), "release should be consumed exactly once");
	expect(!latch.release_due(19), "consumed release must not repeat");
}

static void test_new_press_cancels_deferred_release()
{
	ReleaseLatch latch;
	latch.defer_release(3);
	latch.press();

	expect(!latch.release_due(4), "new press must cancel prior deferred release");
}

static void test_clear_discards_deferred_release()
{
	ReleaseLatch latch;
	latch.defer_release(9);
	latch.clear();

	expect(!latch.release_due(10), "neutralization must discard deferred release");
}

int main()
{
	test_tap_is_held_through_its_frame();
	test_new_press_cancels_deferred_release();
	test_clear_discards_deferred_release();
	return failures == 0 ? 0 : 1;
}
