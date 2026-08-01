#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace android_touch_mouse {

using TouchId = std::uint64_t;
using FingerId = std::uint64_t;
using Timestamp = std::uint64_t;

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

enum class ContactPhase {
	down,
	motion,
	up,
	cancel
};

struct TouchFact {
	TouchKey key;
	ContactPhase phase = ContactPhase::down;
	Timestamp timestamp_ns = 0;
	double gesture_x_dp = 0.0;
	double gesture_y_dp = 0.0;
	double relative_x = 0.0;
	double relative_y = 0.0;
	double normalized_y = 0.0;
};

enum class State {
	idle,
	one_finger,
	second_tap,
	left_drag,
	two_finger_pending,
	swipe_only,
	right_drag,
	gui_consumed,
	drain_until_all_up
};

enum class MouseButton {
	none,
	left,
	right
};

enum class ActionType {
	relative_delta,
	click_pulse,
	button_down,
	button_up,
	open_gui
};

struct Action {
	ActionType type = ActionType::relative_delta;
	MouseButton button = MouseButton::none;
	int delta_x = 0;
	int delta_y = 0;
};

struct GestureProfile {
	Timestamp hold_time_ns = 400000000ULL;
	Timestamp double_tap_min_ns = 40000000ULL;
	Timestamp double_tap_max_ns = 300000000ULL;
	double movement_slop_dp = 8.0;
	double double_tap_distance_dp = 100.0;
	double gui_swipe_fraction = 0.15;
};

class Recognizer {
public:
	explicit Recognizer(GestureProfile profile = {})
		: profile_(profile)
	{
	}

	std::vector<Action> handle(const TouchFact& fact)
	{
		switch (fact.phase) {
		case ContactPhase::down:
			return handle_down(fact);
		case ContactPhase::motion:
			return handle_motion(fact);
		case ContactPhase::up:
			return handle_up(fact);
		case ContactPhase::cancel:
			return owns(fact.key) ? cancel() : std::vector<Action>{};
		}
		return {};
	}

	std::vector<Action> tick(Timestamp now_ns)
	{
		expire_recent_tap(now_ns);
		if (contacts_.empty())
			return {};

		if (state_ == State::one_finger) {
			if (elapsed(now_ns, contacts_.front().down_timestamp)
				>= profile_.hold_time_ns)
				contacts_.front().tap_eligible = false;
			return {};
		}

		if (state_ == State::second_tap
			&& elapsed(now_ns, contacts_.front().down_timestamp)
				>= profile_.hold_time_ns)
			return activate_drag(MouseButton::left, State::left_drag);

		if (state_ == State::two_finger_pending
			&& elapsed(now_ns, pair_started_at_) >= profile_.hold_time_ns)
			return activate_drag(MouseButton::right, State::right_drag);

		return {};
	}

	std::vector<Action> neutralize()
	{
		std::vector<Action> actions;
		if (state_ == State::left_drag)
			actions.push_back(button_action(ActionType::button_up, MouseButton::left));
		else if (state_ == State::right_drag)
			actions.push_back(button_action(ActionType::button_up, MouseButton::right));
		reset_all();
		return actions;
	}

	std::vector<Action> cancel()
	{
		return neutralize();
	}

	std::vector<Action> mapping_lost()
	{
		return neutralize();
	}

	State state() const
	{
		return state_;
	}

	bool owns(TouchKey key) const
	{
		return find_contact(key) != contacts_.end();
	}

	std::size_t tracked_contacts() const
	{
		return contacts_.size();
	}

	bool has_recent_tap() const
	{
		return recent_tap_.has_value();
	}

private:
	struct Contact {
		TouchKey key;
		Timestamp down_timestamp = 0;
		double origin_x = 0.0;
		double origin_y = 0.0;
		double current_x = 0.0;
		double current_y = 0.0;
		double origin_normalized_y = 0.0;
		double current_normalized_y = 0.0;
		bool tap_eligible = true;
	};

	struct RecentTap {
		Timestamp up_timestamp = 0;
		double x = 0.0;
		double y = 0.0;
	};

	using ContactIterator = std::vector<Contact>::iterator;
	using ConstContactIterator = std::vector<Contact>::const_iterator;

	static Timestamp elapsed(Timestamp now, Timestamp then)
	{
		return now >= then ? now - then : 0;
	}

	static double distance_squared(double first_x, double first_y,
		double second_x, double second_y)
	{
		const double dx = first_x - second_x;
		const double dy = first_y - second_y;
		return dx * dx + dy * dy;
	}

	static Action button_action(ActionType type, MouseButton button)
	{
		return {type, button, 0, 0};
	}

	Contact make_contact(const TouchFact& fact) const
	{
		return {fact.key, fact.timestamp_ns, fact.gesture_x_dp, fact.gesture_y_dp,
			fact.gesture_x_dp, fact.gesture_y_dp, fact.normalized_y,
			fact.normalized_y, true};
	}

	ContactIterator find_contact(TouchKey key)
	{
		return std::find_if(contacts_.begin(), contacts_.end(),
			[key](const Contact& contact) { return contact.key == key; });
	}

	ConstContactIterator find_contact(TouchKey key) const
	{
		return std::find_if(contacts_.begin(), contacts_.end(),
			[key](const Contact& contact) { return contact.key == key; });
	}

	bool outside_slop(const Contact& contact) const
	{
		const double slop_squared = profile_.movement_slop_dp
			* profile_.movement_slop_dp;
		return distance_squared(contact.current_x, contact.current_y,
			contact.origin_x, contact.origin_y) > slop_squared;
	}

	void update_contact(Contact& contact, const TouchFact& fact)
	{
		contact.current_x = fact.gesture_x_dp;
		contact.current_y = fact.gesture_y_dp;
		contact.current_normalized_y = fact.normalized_y;
		if (outside_slop(contact))
			contact.tap_eligible = false;
	}

	void reset_pair_origins()
	{
		for (auto& contact : contacts_) {
			contact.origin_x = contact.current_x;
			contact.origin_y = contact.current_y;
			contact.origin_normalized_y = contact.current_normalized_y;
			contact.tap_eligible = true;
		}
	}

	void expire_recent_tap(Timestamp now_ns)
	{
		if (!recent_tap_)
			return;
		if (now_ns < recent_tap_->up_timestamp
			|| elapsed(now_ns, recent_tap_->up_timestamp)
				> profile_.double_tap_max_ns)
			recent_tap_.reset();
	}

	bool matches_recent_tap(const TouchFact& fact) const
	{
		if (!recent_tap_ || fact.timestamp_ns < recent_tap_->up_timestamp)
			return false;
		const Timestamp interval = elapsed(fact.timestamp_ns,
			recent_tap_->up_timestamp);
		if (interval < profile_.double_tap_min_ns
			|| interval > profile_.double_tap_max_ns)
			return false;
		const double distance_limit = profile_.double_tap_distance_dp
			* profile_.double_tap_distance_dp;
		return distance_squared(fact.gesture_x_dp, fact.gesture_y_dp,
			recent_tap_->x, recent_tap_->y) <= distance_limit;
	}

	std::vector<Action> handle_down(const TouchFact& fact)
	{
		if (state_ == State::idle) {
			expire_recent_tap(fact.timestamp_ns);
			const bool second_tap = matches_recent_tap(fact);
			if (!second_tap)
				recent_tap_.reset();
			contacts_.push_back(make_contact(fact));
			active_touch_id_ = fact.key.touch_id;
			state_ = second_tap ? State::second_tap : State::one_finger;
			return {};
		}

		if (!active_touch_id_ || fact.key.touch_id != *active_touch_id_
			|| owns(fact.key))
			return {};

		if (state_ == State::one_finger || state_ == State::second_tap) {
			contacts_.push_back(make_contact(fact));
			reset_pair_origins();
			pair_started_at_ = fact.timestamp_ns;
			buffered_x_ = 0.0;
			buffered_y_ = 0.0;
			recent_tap_.reset();
			state_ = State::two_finger_pending;
			return {};
		}

		contacts_.push_back(make_contact(fact));
		std::vector<Action> actions;
		if (state_ == State::left_drag)
			actions.push_back(button_action(ActionType::button_up, MouseButton::left));
		else if (state_ == State::right_drag)
			actions.push_back(button_action(ActionType::button_up, MouseButton::right));
		state_ = State::drain_until_all_up;
		recent_tap_.reset();
		clear_motion_accumulators();
		return actions;
	}

	std::vector<Action> handle_motion(const TouchFact& fact)
	{
		auto contact = find_contact(fact.key);
		if (contact == contacts_.end())
			return {};
		const bool primary = contact == contacts_.begin();
		update_contact(*contact, fact);

		if (state_ == State::one_finger)
			return emit_relative(fact.relative_x, fact.relative_y);

		if (state_ == State::second_tap) {
			buffered_x_ += fact.relative_x;
			buffered_y_ += fact.relative_y;
			if (outside_slop(*contact))
				return activate_drag(MouseButton::left, State::left_drag);
			return {};
		}

		if (state_ == State::left_drag)
			return primary ? emit_relative(fact.relative_x, fact.relative_y)
				: std::vector<Action>{};

		if (state_ == State::two_finger_pending) {
			if (primary) {
				buffered_x_ += fact.relative_x;
				buffered_y_ += fact.relative_y;
			}
			if (gui_swipe_complete())
				return consume_gui_swipe();
			if (pair_outside_slop()) {
				state_ = State::swipe_only;
				clear_motion_accumulators();
			}
			return {};
		}

		if (state_ == State::swipe_only)
			return gui_swipe_complete() ? consume_gui_swipe()
				: std::vector<Action>{};

		if (state_ == State::right_drag)
			return primary ? emit_relative(fact.relative_x, fact.relative_y)
				: std::vector<Action>{};

		return {};
	}

	std::vector<Action> handle_up(const TouchFact& fact)
	{
		auto contact = find_contact(fact.key);
		if (contact == contacts_.end())
			return {};
		update_contact(*contact, fact);
		std::vector<Action> actions;

		if (state_ == State::one_finger || state_ == State::second_tap) {
			const bool tap = contact->tap_eligible
				&& elapsed(fact.timestamp_ns, contact->down_timestamp)
					< profile_.hold_time_ns;
			if (tap) {
				actions.push_back(button_action(ActionType::click_pulse,
					MouseButton::left));
				recent_tap_ = RecentTap{fact.timestamp_ns,
					contact->current_x, contact->current_y};
			} else {
				recent_tap_.reset();
			}
			contacts_.erase(contact);
			finish_active_gesture();
			return actions;
		}

		if (state_ == State::left_drag)
			actions.push_back(button_action(ActionType::button_up, MouseButton::left));
		else if (state_ == State::right_drag)
			actions.push_back(button_action(ActionType::button_up, MouseButton::right));

		contacts_.erase(contact);
		recent_tap_.reset();
		clear_motion_accumulators();
		if (contacts_.empty()) {
			finish_active_gesture();
		} else {
			state_ = State::drain_until_all_up;
		}
		return actions;
	}

	bool pair_outside_slop() const
	{
		return contacts_.size() >= 2
			&& (outside_slop(contacts_[0]) || outside_slop(contacts_[1]));
	}

	bool gui_swipe_complete() const
	{
		if (contacts_.size() < 2)
			return false;
		constexpr double epsilon = 1e-12;
		for (std::size_t index = 0; index < 2; ++index) {
			const Contact& contact = contacts_[index];
			if (contact.current_normalized_y - contact.origin_normalized_y
				+ epsilon < profile_.gui_swipe_fraction)
				return false;
		}
		return true;
	}

	std::vector<Action> consume_gui_swipe()
	{
		state_ = State::gui_consumed;
		recent_tap_.reset();
		clear_motion_accumulators();
		return {{ActionType::open_gui, MouseButton::none, 0, 0}};
	}

	std::vector<Action> activate_drag(MouseButton button, State state)
	{
		state_ = state;
		recent_tap_.reset();
		std::vector<Action> actions{button_action(ActionType::button_down, button)};
		auto movement = emit_relative(buffered_x_, buffered_y_);
		actions.insert(actions.end(), movement.begin(), movement.end());
		buffered_x_ = 0.0;
		buffered_y_ = 0.0;
		return actions;
	}

	std::vector<Action> emit_relative(double x, double y)
	{
		residual_x_ += x;
		residual_y_ += y;
		const int integer_x = static_cast<int>(std::trunc(residual_x_));
		const int integer_y = static_cast<int>(std::trunc(residual_y_));
		residual_x_ -= integer_x;
		residual_y_ -= integer_y;
		if (integer_x == 0 && integer_y == 0)
			return {};
		return {{ActionType::relative_delta, MouseButton::none,
			integer_x, integer_y}};
	}

	void clear_motion_accumulators()
	{
		buffered_x_ = 0.0;
		buffered_y_ = 0.0;
		residual_x_ = 0.0;
		residual_y_ = 0.0;
	}

	void finish_active_gesture()
	{
		contacts_.clear();
		active_touch_id_.reset();
		pair_started_at_ = 0;
		state_ = State::idle;
		clear_motion_accumulators();
	}

	void reset_all()
	{
		finish_active_gesture();
		recent_tap_.reset();
	}

	GestureProfile profile_;
	State state_ = State::idle;
	std::vector<Contact> contacts_;
	std::optional<TouchId> active_touch_id_;
	std::optional<RecentTap> recent_tap_;
	Timestamp pair_started_at_ = 0;
	double buffered_x_ = 0.0;
	double buffered_y_ = 0.0;
	double residual_x_ = 0.0;
	double residual_y_ = 0.0;
};

enum class ButtonSource {
	physical,
	pen,
	gesture
};

struct ButtonTransition {
	MouseButton button = MouseButton::none;
	bool pressed = false;
};

class ButtonSourceComposer {
public:
	std::optional<ButtonTransition> set(ButtonSource source,
		MouseButton button, bool pressed)
	{
		const auto index = button_index(button);
		if (!index)
			return std::nullopt;
		const bool was_pressed = aggregate(*index);
		sources_[source_index(source)][*index] = pressed;
		const bool is_pressed = aggregate(*index);
		if (was_pressed == is_pressed)
			return std::nullopt;
		return ButtonTransition{button, is_pressed};
	}

	std::vector<ButtonTransition> clear_source(ButtonSource source)
	{
		std::vector<ButtonTransition> transitions;
		for (std::size_t index = 0; index < button_count; ++index) {
			const bool was_pressed = aggregate(index);
			sources_[source_index(source)][index] = false;
			const bool is_pressed = aggregate(index);
			if (was_pressed != is_pressed)
				transitions.push_back({button_for_index(index), is_pressed});
		}
		return transitions;
	}

	std::vector<ButtonTransition> clear_all()
	{
		std::vector<ButtonTransition> transitions;
		for (std::size_t index = 0; index < button_count; ++index) {
			const bool was_pressed = aggregate(index);
			for (auto& source : sources_)
				source[index] = false;
			if (was_pressed)
				transitions.push_back({button_for_index(index), false});
		}
		return transitions;
	}

	bool pressed(MouseButton button) const
	{
		const auto index = button_index(button);
		return index && aggregate(*index);
	}

	bool source_pressed(ButtonSource source, MouseButton button) const
	{
		const auto index = button_index(button);
		return index && sources_[source_index(source)][*index];
	}

private:
	static constexpr std::size_t source_count = 3;
	static constexpr std::size_t button_count = 2;

	static std::size_t source_index(ButtonSource source)
	{
		return static_cast<std::size_t>(source);
	}

	static std::optional<std::size_t> button_index(MouseButton button)
	{
		if (button == MouseButton::left)
			return 0;
		if (button == MouseButton::right)
			return 1;
		return std::nullopt;
	}

	static MouseButton button_for_index(std::size_t index)
	{
		return index == 0 ? MouseButton::left : MouseButton::right;
	}

	bool aggregate(std::size_t button) const
	{
		return std::any_of(sources_.begin(), sources_.end(),
			[button](const auto& source) { return source[button]; });
	}

	std::array<std::array<bool, button_count>, source_count> sources_{};
};

} // namespace android_touch_mouse
