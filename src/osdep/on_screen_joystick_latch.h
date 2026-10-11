#ifndef ON_SCREEN_JOYSTICK_LATCH_H
#define ON_SCREEN_JOYSTICK_LATCH_H

#include <cstdint>

namespace osj_latch {

// Defers a release until at least the next emulated frame. This preserves a
// press/release pair that arrives within one host event-pump iteration.
class ReleaseLatch {
public:
	void press()
	{
		pending_ = false;
	}

	void defer_release(const std::uint64_t frame)
	{
		pending_ = true;
		held_through_frame_ = frame;
	}

	bool release_due(const std::uint64_t frame)
	{
		if (!pending_ || frame <= held_through_frame_)
			return false;
		pending_ = false;
		return true;
	}

	bool pending() const
	{
		return pending_;
	}

	void clear()
	{
		pending_ = false;
	}

private:
	bool pending_ = false;
	std::uint64_t held_through_frame_ = 0;
};

} // namespace osj_latch

#endif // ON_SCREEN_JOYSTICK_LATCH_H
