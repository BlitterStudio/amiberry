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

#endif
