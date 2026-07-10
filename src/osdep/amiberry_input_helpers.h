#ifndef AMIBERRY_INPUT_HELPERS_H
#define AMIBERRY_INPUT_HELPERS_H

static inline int amiberry_input_clamp_native_axis(int value, int extent, bool* out_of_bounds)
{
	if (extent <= 0) {
		if (out_of_bounds) {
			*out_of_bounds = true;
		}
		return 0;
	}
	if (value < 0) {
		if (out_of_bounds) {
			*out_of_bounds = true;
		}
		return 0;
	}
	if (value >= extent) {
		if (out_of_bounds) {
			*out_of_bounds = true;
		}
		return extent - 1;
	}
	if (out_of_bounds) {
		*out_of_bounds = false;
	}
	return value;
}

static inline int amiberry_input_mousehack_hotspot_axis(int pointer_offset, int extent, int bias)
{
	if (extent <= 0) {
		return 0;
	}

	const int hotspot = -(pointer_offset + bias);
	if (hotspot < 0) {
		return 0;
	}
	if (hotspot >= extent) {
		return extent - 1;
	}
	return hotspot;
}

static inline int amiberry_input_mousehack_hotspot_residual_axis(int pointer_offset, int extent, int bias)
{
	if (extent <= 0) {
		return 0;
	}

	const int hotspot = amiberry_input_mousehack_hotspot_axis(pointer_offset, extent, bias);
	return pointer_offset + bias + hotspot;
}

static inline void amiberry_input_mousehack_cursor_hotspot(int pointer_x_offset, int pointer_y_offset,
	int cursor_width, int cursor_height, int* hotspot_x, int* hotspot_y)
{
	if (hotspot_x) {
		*hotspot_x = amiberry_input_mousehack_hotspot_axis(pointer_x_offset, cursor_width, 1);
	}
	if (hotspot_y) {
		*hotspot_y = amiberry_input_mousehack_hotspot_axis(pointer_y_offset, cursor_height, 2);
	}
}

static inline int amiberry_input_native_sprite_x(uae_u16 pos, uae_u16 ctl, bool extended_x)
{
	// Mousehack uses hires-granularity coordinates, so AGA's half-step bit cannot be represented here.
	int x = ((pos & 0xff) << 2) | ((ctl & 0x01) << 1);
	if (extended_x && (ctl & 0x10)) {
		x |= 1;
	}
	return x;
}

static inline int amiberry_input_native_sprite_y(uae_u16 pos, uae_u16 ctl, bool extended_y)
{
	int y = pos >> 8;
	if (ctl & 0x04) {
		y |= 0x0100;
	}
	if (extended_y && (ctl & 0x40)) {
		y |= 0x0200;
	}
	return y << 1;
}

struct amiberry_input_cursor_hotspot_tracker
{
	bool have_sample = false;
	bool learned = false;
	int last_pointer_x = 0;
	int last_pointer_y = 0;
	int last_sprite_x = 0;
	int last_sprite_y = 0;
	int candidate_x = 0;
	int candidate_y = 0;
	int stable_samples = 0;
	int hotspot_x = 0;
	int hotspot_y = 0;
};

static inline void amiberry_input_cursor_hotspot_tracker_reset(
	amiberry_input_cursor_hotspot_tracker* tracker)
{
	if (tracker) {
		*tracker = {};
	}
}

static inline bool amiberry_input_cursor_hotspot_tracker_sample(
	amiberry_input_cursor_hotspot_tracker* tracker, bool position_valid,
	int pointer_x, int pointer_y, int sprite_x, int sprite_y,
	int cursor_width, int cursor_height, int required_stable_samples,
	int* hotspot_x, int* hotspot_y)
{
	if (!tracker) {
		return false;
	}

	const int candidate_x = pointer_x - sprite_x;
	const int candidate_y = pointer_y - sprite_y;
	const bool candidate_valid = position_valid && cursor_width > 0 && cursor_height > 0
		&& candidate_x >= 0 && candidate_x < cursor_width
		&& candidate_y >= 0 && candidate_y < cursor_height;

	if (candidate_valid) {
		const bool stationary = tracker->have_sample
			&& pointer_x == tracker->last_pointer_x && pointer_y == tracker->last_pointer_y
			&& sprite_x == tracker->last_sprite_x && sprite_y == tracker->last_sprite_y
			&& candidate_x == tracker->candidate_x && candidate_y == tracker->candidate_y;

		tracker->stable_samples = stationary ? tracker->stable_samples + 1 : 1;
		tracker->have_sample = true;
		tracker->last_pointer_x = pointer_x;
		tracker->last_pointer_y = pointer_y;
		tracker->last_sprite_x = sprite_x;
		tracker->last_sprite_y = sprite_y;
		tracker->candidate_x = candidate_x;
		tracker->candidate_y = candidate_y;

		if (tracker->stable_samples >= required_stable_samples) {
			tracker->learned = true;
			tracker->hotspot_x = candidate_x;
			tracker->hotspot_y = candidate_y;
		}
	} else {
		tracker->have_sample = false;
		tracker->stable_samples = 0;
	}

	if (tracker->learned) {
		if (hotspot_x) {
			*hotspot_x = tracker->hotspot_x;
		}
		if (hotspot_y) {
			*hotspot_y = tracker->hotspot_y;
		}
	}
	return tracker->learned;
}

#endif
