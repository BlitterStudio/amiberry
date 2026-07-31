#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "on_screen_joystick_layout.h"

using namespace osj_layout;

static int failures;

static void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

static int right(const Rect& rect) { return rect.x + rect.w; }
static int bottom(const Rect& rect) { return rect.y + rect.h; }

static bool contains(const Rect& outer, const Rect& inner)
{
	return inner.x >= outer.x && inner.y >= outer.y
		&& right(inner) <= right(outer) && bottom(inner) <= bottom(outer);
}

static Rect circle_bounds(const Circle& circle)
{
	return {circle.cx - circle.radius, circle.cy - circle.radius,
		circle.radius * 2, circle.radius * 2};
}

static double circle_gap(const Circle& a, const Circle& b)
{
	const double dx = static_cast<double>(a.cx - b.cx);
	const double dy = static_cast<double>(a.cy - b.cy);
	return std::sqrt(dx * dx + dy * dy) - a.radius - b.radius;
}

static void verify_snapshot_invariants(const LayoutSnapshot& layout, const std::string& name)
{
	expect(layout.valid, name + ": layout must be valid");
	const ControlGeometry* controls[] = {
		&layout.joystick, &layout.fire1, &layout.fire2, &layout.keyboard
	};
	for (const auto* control : controls) {
		expect(contains(layout.safe_rect, control->visual),
			name + ": every visual must remain inside the safe rectangle");
		expect(contains(layout.safe_rect, circle_bounds(control->acquisition)),
			name + ": every acquisition circle must remain inside the safe rectangle");
	}
	expect(contains(layout.safe_rect, layout.joystick_group_bounds),
		name + ": joystick group must remain inside the safe rectangle");
	expect(contains(layout.safe_rect, layout.button_group_bounds),
		name + ": button group must remain inside the safe rectangle");

	const int shorter = std::min(layout.safe_rect.w, layout.safe_rect.h);
	const int minimum_gap = std::max(4, static_cast<int>(std::lround(shorter * 0.006)));
	expect(circle_gap(layout.fire1.acquisition, layout.fire2.acquisition) + 0.0001 >= minimum_gap,
		name + ": fire acquisition circles must retain the rounded minimum gap");
	expect(circle_gap(layout.fire1.acquisition, layout.keyboard.acquisition) + 0.0001 >= minimum_gap,
		name + ": fire 1 and keyboard acquisitions must retain the rounded minimum gap");
	expect(circle_gap(layout.fire2.acquisition, layout.keyboard.acquisition) + 0.0001 >= minimum_gap,
		name + ": fire 2 and keyboard acquisitions must retain the rounded minimum gap");
	expect(circle_gap(layout.joystick.acquisition, layout.fire1.acquisition) + 0.0001 >= minimum_gap,
		name + ": left and right acquisition groups must not overlap");
	expect(circle_gap(layout.joystick.acquisition, layout.fire2.acquisition) + 0.0001 >= minimum_gap,
		name + ": joystick and fire 2 acquisitions must not overlap");
	expect(circle_gap(layout.joystick.acquisition, layout.keyboard.acquisition) + 0.0001 >= minimum_gap,
		name + ": joystick and keyboard acquisitions must not overlap");

	expect(layout.joystick.visual.w < static_cast<int>(std::ceil(shorter * 0.38)),
		name + ": joystick visual diameter must be smaller than the old 0.38 baseline");
	expect(layout.joystick.acquisition.radius * 2 > layout.joystick.visual.w,
		name + ": joystick acquisition diameter must exceed its visual diameter");
}

static Rect centered_four_three(const Rect& output)
{
	int height = output.h;
	int width = height * 4 / 3;
	if (width > output.w) {
		width = output.w;
		height = width * 3 / 4;
	}
	return {output.x + (output.w - width) / 2, output.y + (output.h - height) / 2,
		width, height};
}

static void test_phone_and_tablet_matrix()
{
	const std::vector<Rect> outputs = {
		{0, 0, 1080, 2400}, {0, 0, 2400, 1080},
		{0, 0, 1600, 2560}, {0, 0, 2560, 1600}
	};
	uint64_t generation = 1;
	for (const auto& output : outputs) {
		const auto centered = calculate_layout(output, output, centered_four_three(output), generation++);
		verify_snapshot_invariants(centered, "centered 4:3 matrix");
		const auto full = calculate_layout(output, output, output, generation++);
		verify_snapshot_invariants(full, "full-output matrix");
		expect(!full.gutter_mode, "full-output game rectangle must use the lower band");
	}
}

static void test_gutter_requires_both_complete_groups()
{
	const Rect output{0, 0, 2400, 1080};
	const auto gutters = calculate_layout(output, output, {540, 60, 1320, 990}, 20);
	verify_snapshot_invariants(gutters, "wide complete gutters");
	expect(gutters.gutter_mode, "layout must use gutters when both complete groups fit");
	expect(gutters.joystick_group_bounds.x < gutters.game_rect.x,
		"gutter joystick must remain left of the game rectangle");
	expect(gutters.button_group_bounds.x >= right(gutters.game_rect),
		"gutter buttons must remain right of the game rectangle");

	const auto one_too_narrow = calculate_layout(output, output, {430, 60, 1430, 990}, 21);
	verify_snapshot_invariants(one_too_narrow, "one narrow gutter");
	expect(!one_too_narrow.gutter_mode,
		"one fitting gutter is insufficient; both complete groups must fit");
	expect(one_too_narrow.joystick_group_bounds.x == one_too_narrow.safe_rect.x + one_too_narrow.edge_margin,
		"lower-band joystick acquisition bounds must anchor to safe-left");
	expect(right(one_too_narrow.button_group_bounds)
		== right(one_too_narrow.safe_rect) - one_too_narrow.edge_margin,
		"lower-band buttons acquisition bounds must anchor to safe-right");
	expect(bottom(one_too_narrow.joystick_group_bounds)
		== bottom(one_too_narrow.safe_rect) - one_too_narrow.edge_margin,
		"lower-band joystick acquisition bounds must anchor to safe-bottom");
	expect(bottom(one_too_narrow.button_group_bounds)
		== bottom(one_too_narrow.safe_rect) - one_too_narrow.edge_margin,
		"lower-band button acquisition bounds must anchor to safe-bottom");
}

static void test_asymmetric_safe_area_and_invalid_fallbacks()
{
	const Rect output{0, 0, 2400, 1080};
	const Rect cutout_safe{140, 18, 2180, 1042};
	const auto cutout = calculate_layout(output, cutout_safe, {700, 100, 1100, 825}, 30);
	verify_snapshot_invariants(cutout, "asymmetric cutout");
	expect(cutout.safe_rect.x == 140 && cutout.safe_rect.w == 2180,
		"valid asymmetric safe rectangle must be preserved");

	const auto invalid_safe = calculate_layout(output, {-10, 0, 0, 1080}, output, 31);
	verify_snapshot_invariants(invalid_safe, "invalid safe fallback");
	expect(invalid_safe.safe_rect == output,
		"invalid safe rectangle must fall back to the full output bounds");

	const auto empty_game = calculate_layout(output, output, {0, 0, 0, 0}, 32);
	verify_snapshot_invariants(empty_game, "empty game fallback");
	expect(empty_game.game_rect == output,
		"empty game rectangle must fall back to the full output bounds");
	expect(!empty_game.gutter_mode,
		"empty game fallback must not invent side gutters");
}

static void test_gutter_groups_are_vertically_centered()
{
	const Rect output{0, 0, 2560, 1600};
	const Rect safe{40, 80, 2480, 1440};
	const auto layout = calculate_layout(output, safe, {700, 140, 1160, 990}, 40);
	verify_snapshot_invariants(layout, "vertically centered gutters");
	expect(layout.gutter_mode, "tablet side regions must activate gutter mode");
	const int safe_mid = safe.y + safe.h / 2;
	const int joystick_mid = layout.joystick_group_bounds.y + layout.joystick_group_bounds.h / 2;
	const int buttons_mid = layout.button_group_bounds.y + layout.button_group_bounds.h / 2;
	expect(std::abs(joystick_mid - safe_mid) <= 1,
		"complete joystick group must be vertically centered in safe space");
	expect(std::abs(buttons_mid - safe_mid) <= 1,
		"complete button group must be vertically centered in safe space");
}

static void test_explicit_normalized_touch_transform()
{
	// The SDL logical window is 1080x2400 while GLES draws at 2160x4800.
	// Normalized touch must map into the snapshot's drawable-pixel space.
	const Rect drawable{0, 0, 2160, 4800};
	const auto layout = calculate_layout(drawable, {0, 120, 2160, 4560}, drawable, 50);
	const Point center = layout.normalized_touch_to_layout.apply(0.5f, 0.5f);
	expect(center.x == 1080 && center.y == 2400,
		"normalized touch transform must target drawable coordinates, not logical dimensions");
	expect(layout.generation == 50,
		"snapshot generation must be published unchanged");

	const Rect logical{0, 0, 1080, 2400};
	const Rect logical_safe{50, 100, 980, 2200};
	const auto scaled_safe = scale_rect_between_spaces(logical_safe, logical, drawable);
	expect(scaled_safe == Rect{100, 200, 1960, 4400},
		"logical safe area must scale exactly into drawable-pixel space");

	// A 1440x1080 logical presentation is letterboxed in a 2400x1080 window.
	// SDL's window-to-render conversion maps the normalized window edges to
	// logical x=-480 and x=1920, which the snapshot stores explicitly.
	const Rect fallback_output{0, 0, 1440, 1080};
	const NormalizedTouchTransform fallback_transform{-480.0f, 0.0f, 2400.0f, 1080.0f};
	const auto fallback = calculate_layout(fallback_output, fallback_output,
		fallback_output, 51, fallback_transform);
	const Point fallback_center = fallback.normalized_touch_to_layout.apply(0.5f, 0.5f);
	expect(fallback_center.x == 720 && fallback_center.y == 540,
		"fallback snapshot must retain SDL's window-to-render transform");
	expect(fallback.normalized_touch_to_layout.apply(0.0f, 0.0f).x == -480,
		"fallback transform must preserve logical-presentation letterbox offset");
}

static void test_invalid_output_has_no_snapshot()
{
	const auto layout = calculate_layout({0, 0, 0, 1080}, {}, {}, 60);
	expect(!layout.valid, "invalid output bounds must not publish a valid layout");
}

int main()
{
	test_phone_and_tablet_matrix();
	test_gutter_requires_both_complete_groups();
	test_asymmetric_safe_area_and_invalid_fallbacks();
	test_gutter_groups_are_vertically_centered();
	test_explicit_normalized_touch_transform();
	test_invalid_output_has_no_snapshot();
	return failures == 0 ? 0 : 1;
}
