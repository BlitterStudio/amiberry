#pragma once

#include <cmath>

// Sub-pixel relative mouse motion accumulator for the Android/ChromeOS
// captured-mouse path (capture keeps the window absolute per #2285, so
// SDL computes hover-position deltas). SDL3 delivers those deltas as floats,
// and a slow physical mouse can move less than one pixel per event. Feeding
// them straight into the int parameters of setmousestate truncates every
// event to zero and the guest pointer stalls. One accumulator per mouse
// carries the un-emitted fraction of each axis forward and releases it once
// motion in that direction completes a whole pixel step.
//
// Truncation is toward zero (std::trunc, matching the touch recognizer's
// residual design in android_touch_mouse.h): an emitted step never opposes
// the accumulated motion, and the carried residual always stays within
// (-1, 1) per axis. The owner resets the accumulator on capture transitions,
// GUI open, and absolute/tablet dispatch so a stale fraction never leaks
// into a new session — see handle_mouse_motion_event() in amiberry.cpp.

struct amiberry_mouse_delta {
	int dx = 0;
	int dy = 0;
};

class amiberry_mouse_delta_accumulator {
public:
	// Feeds one motion event's float deltas and returns the whole-pixel
	// deltas to dispatch now; the un-emitted fraction stays pending.
	amiberry_mouse_delta feed(const float dx, const float dy)
	{
		residual_x_ += dx;
		residual_y_ += dy;
		const amiberry_mouse_delta whole{
			static_cast<int>(std::trunc(residual_x_)),
			static_cast<int>(std::trunc(residual_y_))};
		residual_x_ -= whole.dx;
		residual_y_ -= whole.dy;
		return whole;
	}

	// Drops any pending fraction. Call on capture apply/release and when the
	// event stream switches to absolute (tablet) dispatch.
	void reset()
	{
		residual_x_ = 0.0f;
		residual_y_ = 0.0f;
	}

	float residual_x() const { return residual_x_; }
	float residual_y() const { return residual_y_; }

private:
	float residual_x_ = 0.0f;
	float residual_y_ = 0.0f;
};
