#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace osj_layout {

struct Rect {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
};

inline bool operator==(const Rect& lhs, const Rect& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.w == rhs.w && lhs.h == rhs.h;
}

inline bool operator!=(const Rect& lhs, const Rect& rhs)
{
	return !(lhs == rhs);
}

struct Point {
	int x = 0;
	int y = 0;
};

struct Circle {
	int cx = 0;
	int cy = 0;
	int radius = 0;
};

struct ControlGeometry {
	Rect visual;
	Circle acquisition;
};

struct NormalizedTouchTransform {
	float offset_x = 0.0f;
	float offset_y = 0.0f;
	float scale_x = 0.0f;
	float scale_y = 0.0f;

	Point apply(float normalized_x, float normalized_y) const
	{
		return {
			static_cast<int>(std::lround(offset_x + normalized_x * scale_x)),
			static_cast<int>(std::lround(offset_y + normalized_y * scale_y))
		};
	}
};

struct LayoutSnapshot {
	Rect output_bounds;
	Rect safe_rect;
	Rect game_rect;
	ControlGeometry joystick;
	ControlGeometry fire1;
	ControlGeometry fire2;
	ControlGeometry keyboard;
	Rect joystick_group_bounds;
	Rect button_group_bounds;
	NormalizedTouchTransform normalized_touch_to_layout;
	std::uint64_t generation = 0;
	int edge_margin = 0;
	int minimum_acquisition_gap = 0;
	bool gutter_mode = false;
	bool valid = false;
};

namespace detail {

inline int rect_right(const Rect& rect)
{
	return rect.x + rect.w;
}

inline int rect_bottom(const Rect& rect)
{
	return rect.y + rect.h;
}

inline bool valid_rect(const Rect& rect)
{
	return rect.w > 0 && rect.h > 0;
}

inline bool contains(const Rect& outer, const Rect& inner)
{
	return valid_rect(inner) && inner.x >= outer.x && inner.y >= outer.y
		&& rect_right(inner) <= rect_right(outer)
		&& rect_bottom(inner) <= rect_bottom(outer);
}

inline Rect circle_bounds(const Circle& circle)
{
	return {circle.cx - circle.radius, circle.cy - circle.radius,
		circle.radius * 2, circle.radius * 2};
}

inline Rect union_rect(const Rect& lhs, const Rect& rhs)
{
	const int x = std::min(lhs.x, rhs.x);
	const int y = std::min(lhs.y, rhs.y);
	const int right = std::max(rect_right(lhs), rect_right(rhs));
	const int bottom = std::max(rect_bottom(lhs), rect_bottom(rhs));
	return {x, y, right - x, bottom - y};
}

inline bool circles_have_gap(const Circle& lhs, const Circle& rhs, int gap)
{
	const long long dx = static_cast<long long>(lhs.cx) - rhs.cx;
	const long long dy = static_cast<long long>(lhs.cy) - rhs.cy;
	const long long required = static_cast<long long>(lhs.radius) + rhs.radius + gap;
	return dx * dx + dy * dy >= required * required;
}

inline Rect visual_rect(int cx, int cy, int diameter)
{
	return {cx - diameter / 2, cy - diameter / 2, diameter, diameter};
}

inline void translate(ControlGeometry& geometry, int dx, int dy)
{
	geometry.visual.x += dx;
	geometry.visual.y += dy;
	geometry.acquisition.cx += dx;
	geometry.acquisition.cy += dy;
}

inline ControlGeometry make_control(int cx, int cy, int visual_diameter, int acquisition_radius)
{
	return {
		visual_rect(cx, cy, visual_diameter),
		{cx, cy, acquisition_radius}
	};
}

struct ButtonGroup {
	ControlGeometry fire1;
	ControlGeometry fire2;
	ControlGeometry keyboard;
	Rect bounds;
};

inline ButtonGroup make_button_group(int shorter, int minimum_gap)
{
	const int fire_diameter = std::max(1, static_cast<int>(std::lround(shorter * 0.19)));
	const int fire_radius = std::max(1, static_cast<int>(std::lround(shorter * 0.105)));
	const int keyboard_diameter = std::max(1, static_cast<int>(std::lround(shorter * 0.12)));
	const int keyboard_radius = std::max(1, static_cast<int>(std::lround(shorter * 0.066)));

	int fire2_dx = static_cast<int>(std::lround(fire_diameter * 0.70));
	int fire2_dy = static_cast<int>(std::lround(fire_diameter * 0.90));
	const double initial_distance = std::hypot(static_cast<double>(fire2_dx), static_cast<double>(fire2_dy));
	const int required_fire_distance = fire_radius * 2 + minimum_gap;
	if (initial_distance < required_fire_distance && initial_distance > 0.0) {
		const double scale = required_fire_distance / initial_distance;
		fire2_dx = static_cast<int>(std::lround(fire2_dx * scale));
		fire2_dy = static_cast<int>(std::lround(fire2_dy * scale));
	}
	while (static_cast<long long>(fire2_dx) * fire2_dx
		+ static_cast<long long>(fire2_dy) * fire2_dy
		< static_cast<long long>(required_fire_distance) * required_fire_distance) {
		fire2_dy++;
	}

	ButtonGroup group;
	group.fire1 = make_control(0, 0, fire_diameter, fire_radius);
	group.fire2 = make_control(fire2_dx, fire2_dy, fire_diameter, fire_radius);
	const int keyboard_x = static_cast<int>(std::lround(fire2_dx * 0.5));
	int keyboard_y = fire2_dy + static_cast<int>(std::lround(
		fire_diameter * 0.5 + shorter * 0.025 + shorter * 0.06));
	group.keyboard = make_control(keyboard_x, keyboard_y, keyboard_diameter, keyboard_radius);
	while (!circles_have_gap(group.fire1.acquisition, group.keyboard.acquisition, minimum_gap)
		|| !circles_have_gap(group.fire2.acquisition, group.keyboard.acquisition, minimum_gap)) {
		keyboard_y++;
		group.keyboard = make_control(keyboard_x, keyboard_y, keyboard_diameter, keyboard_radius);
	}

	group.bounds = union_rect(circle_bounds(group.fire1.acquisition), circle_bounds(group.fire2.acquisition));
	group.bounds = union_rect(group.bounds, circle_bounds(group.keyboard.acquisition));
	return group;
}

inline void place_button_group(ButtonGroup& group, int x, int y)
{
	const int dx = x - group.bounds.x;
	const int dy = y - group.bounds.y;
	translate(group.fire1, dx, dy);
	translate(group.fire2, dx, dy);
	translate(group.keyboard, dx, dy);
	group.bounds.x = x;
	group.bounds.y = y;
}

inline int centered_and_clamped(int outer_start, int outer_size, int item_size, int margin)
{
	const int minimum = outer_start + margin;
	const int maximum = outer_start + outer_size - margin - item_size;
	return std::clamp(outer_start + (outer_size - item_size) / 2, minimum, maximum);
}

} // namespace detail

inline LayoutSnapshot calculate_layout(Rect output_bounds, Rect safe_rect, Rect game_rect,
	std::uint64_t generation, NormalizedTouchTransform normalized_touch_to_layout)
{
	LayoutSnapshot result;
	if (!detail::valid_rect(output_bounds))
		return result;

	if (!detail::contains(output_bounds, safe_rect))
		safe_rect = output_bounds;
	if (!detail::contains(output_bounds, game_rect))
		game_rect = output_bounds;

	result.output_bounds = output_bounds;
	result.safe_rect = safe_rect;
	result.game_rect = game_rect;
	result.generation = generation;
	result.normalized_touch_to_layout = normalized_touch_to_layout;

	const int shorter = std::min(safe_rect.w, safe_rect.h);
	result.edge_margin = std::max(0, static_cast<int>(std::lround(shorter * 0.025)));
	result.minimum_acquisition_gap = std::max(4,
		static_cast<int>(std::lround(shorter * 0.006)));

	const int joystick_diameter = std::max(1, static_cast<int>(std::lround(shorter * 0.32)));
	const int joystick_radius = std::max(1, static_cast<int>(std::lround(shorter * 0.18)));
	result.joystick = detail::make_control(0, 0, joystick_diameter, joystick_radius);
	result.joystick_group_bounds = detail::circle_bounds(result.joystick.acquisition);
	auto buttons = detail::make_button_group(shorter, result.minimum_acquisition_gap);

	const int safe_right = detail::rect_right(safe_rect);
	const int game_right = detail::rect_right(game_rect);
	const int left_region_right = std::clamp(game_rect.x, safe_rect.x, safe_right);
	const int right_region_start = std::clamp(game_right, safe_rect.x, safe_right);
	const int left_region_width = left_region_right - safe_rect.x;
	const int right_region_width = safe_right - right_region_start;
	const int margin_twice = result.edge_margin * 2;
	const bool joystick_fits_left = left_region_width >= result.joystick_group_bounds.w + margin_twice
		&& safe_rect.h >= result.joystick_group_bounds.h + margin_twice;
	const bool buttons_fit_right = right_region_width >= buttons.bounds.w + margin_twice
		&& safe_rect.h >= buttons.bounds.h + margin_twice;
	result.gutter_mode = joystick_fits_left && buttons_fit_right;

	int joystick_x;
	int joystick_y;
	int buttons_x;
	int buttons_y;
	if (result.gutter_mode) {
		joystick_x = safe_rect.x + (left_region_width - result.joystick_group_bounds.w) / 2;
		buttons_x = right_region_start + (right_region_width - buttons.bounds.w) / 2;
		joystick_y = detail::centered_and_clamped(safe_rect.y, safe_rect.h,
			result.joystick_group_bounds.h, result.edge_margin);
		buttons_y = detail::centered_and_clamped(safe_rect.y, safe_rect.h,
			buttons.bounds.h, result.edge_margin);
	} else {
		joystick_x = safe_rect.x + result.edge_margin;
		buttons_x = safe_right - result.edge_margin - buttons.bounds.w;
		joystick_y = detail::rect_bottom(safe_rect) - result.edge_margin
			- result.joystick_group_bounds.h;
		buttons_y = detail::rect_bottom(safe_rect) - result.edge_margin - buttons.bounds.h;
	}

	const int joystick_dx = joystick_x - result.joystick_group_bounds.x;
	const int joystick_dy = joystick_y - result.joystick_group_bounds.y;
	detail::translate(result.joystick, joystick_dx, joystick_dy);
	result.joystick_group_bounds.x = joystick_x;
	result.joystick_group_bounds.y = joystick_y;
	detail::place_button_group(buttons, buttons_x, buttons_y);
	result.fire1 = buttons.fire1;
	result.fire2 = buttons.fire2;
	result.keyboard = buttons.keyboard;
	result.button_group_bounds = buttons.bounds;
	result.valid = true;
	return result;
}

inline LayoutSnapshot calculate_layout(Rect output_bounds, Rect safe_rect, Rect game_rect,
	std::uint64_t generation)
{
	return calculate_layout(output_bounds, safe_rect, game_rect, generation, {
		static_cast<float>(output_bounds.x), static_cast<float>(output_bounds.y),
		static_cast<float>(output_bounds.w), static_cast<float>(output_bounds.h)
	});
}

inline Rect scale_rect_between_spaces(const Rect& rect, const Rect& from, const Rect& to)
{
	if (!detail::valid_rect(rect) || !detail::valid_rect(from) || !detail::valid_rect(to))
		return {};
	const double scale_x = static_cast<double>(to.w) / from.w;
	const double scale_y = static_cast<double>(to.h) / from.h;
	const int x1 = to.x + static_cast<int>(std::lround((rect.x - from.x) * scale_x));
	const int y1 = to.y + static_cast<int>(std::lround((rect.y - from.y) * scale_y));
	const int x2 = to.x + static_cast<int>(std::lround((detail::rect_right(rect) - from.x) * scale_x));
	const int y2 = to.y + static_cast<int>(std::lround((detail::rect_bottom(rect) - from.y) * scale_y));
	return {x1, y1, x2 - x1, y2 - y1};
}

} // namespace osj_layout
