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

#endif
