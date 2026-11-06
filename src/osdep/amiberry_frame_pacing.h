#ifndef AMIBERRY_FRAME_PACING_H
#define AMIBERRY_FRAME_PACING_H

#include <algorithm>
#include <cstdint>

class amiberry_frame_pacer
{
public:
	int sleep_us(const int64_t remaining_us) const
	{
		if (remaining_us < guard_us + 50)
			return 0;
		const int64_t available = remaining_us - guard_us;
		return static_cast<int>(std::min<int64_t>(available, 1000));
	}

	void observe_sleep(const int requested_us, const int64_t elapsed_us)
	{
		// Reserve wait-loop overhead and scheduler overshoot; recover slowly after spikes.
		const int overshoot = static_cast<int>(std::clamp<int64_t>(
			elapsed_us - requested_us, 0, 1900));
		guard_us = std::max(std::max(200, guard_us - 1), overshoot + 100);
	}

private:
	int guard_us = 200;
};

#endif
