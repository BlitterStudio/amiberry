#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace osj_capture {

using TouchId = std::uint64_t;
using FingerId = std::uint64_t;

enum class Control {
	none,
	dpad,
	button1,
	button2,
	keyboard
};

enum class State {
	captured,
	contender
};

enum class AcquireResult {
	invalid,
	captured,
	contender,
	already_tracked
};

struct TouchKey {
	TouchId touch_id = 0;
	FingerId finger_id = 0;
};

inline bool operator==(const TouchKey& lhs, const TouchKey& rhs)
{
	return lhs.touch_id == rhs.touch_id && lhs.finger_id == rhs.finger_id;
}

inline bool operator!=(const TouchKey& lhs, const TouchKey& rhs)
{
	return !(lhs == rhs);
}

struct Capture {
	TouchKey key;
	Control control = Control::none;
	State state = State::captured;
};

class Registry {
public:
	AcquireResult acquire(TouchKey key, Control control)
	{
		if (!valid(key) || control == Control::none)
			return AcquireResult::invalid;
		if (lookup(key))
			return AcquireResult::already_tracked;

		const State state = occupied(control) ? State::contender : State::captured;
		captures_.push_back({key, control, state});
		return state == State::captured
			? AcquireResult::captured : AcquireResult::contender;
	}

	const Capture* lookup(TouchKey key) const
	{
		const auto it = std::find_if(captures_.begin(), captures_.end(),
			[key](const Capture& capture) { return capture.key == key; });
		return it == captures_.end() ? nullptr : &*it;
	}

	const Capture* owner(Control control) const
	{
		const auto it = std::find_if(captures_.begin(), captures_.end(),
			[control](const Capture& capture) {
				return capture.control == control && capture.state == State::captured;
			});
		return it == captures_.end() ? nullptr : &*it;
	}

	bool occupied(Control control) const
	{
		return owner(control) != nullptr;
	}

	std::optional<Capture> release(TouchKey key)
	{
		const auto it = std::find_if(captures_.begin(), captures_.end(),
			[key](const Capture& capture) { return capture.key == key; });
		if (it == captures_.end())
			return std::nullopt;

		const Capture released = *it;
		captures_.erase(it);
		return released;
	}

	std::vector<Control> release_stale_captures(TouchId touch_id,
		const std::vector<FingerId>& active_fingers)
	{
		std::vector<Control> released_controls;
		for (auto it = captures_.begin(); it != captures_.end();) {
			if (it->key.touch_id != touch_id) {
				++it;
				continue;
			}
			const bool still_active = std::find(active_fingers.begin(), active_fingers.end(),
				it->key.finger_id) != active_fingers.end();
			if (!still_active) {
				if (it->state == State::captured)
					released_controls.push_back(it->control);
				it = captures_.erase(it);
			} else {
				++it;
			}
		}
		return released_controls;
	}

	void clear()
	{
		captures_.clear();
	}

	bool empty() const
	{
		return captures_.empty();
	}

	std::size_t size() const
	{
		return captures_.size();
	}

private:
	static bool valid(TouchKey key)
	{
		return key.touch_id != 0 && key.finger_id != 0;
	}

	std::vector<Capture> captures_;
};

} // namespace osj_capture
