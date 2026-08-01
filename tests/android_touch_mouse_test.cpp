#include "android_touch_mouse.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace android_touch_mouse;

static int failures = 0;

static void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		failures++;
	}
}

static Timestamp ns(std::uint64_t milliseconds)
{
	return milliseconds * 1000000ULL;
}

static TouchFact down(TouchKey key, std::uint64_t milliseconds,
	double x = 0.0, double y = 0.0, double normalized_y = 0.0)
{
	return {key, ContactPhase::down, ns(milliseconds), x, y, 0.0, 0.0,
		normalized_y};
}

static TouchFact motion(TouchKey key, std::uint64_t milliseconds,
	double x, double y, double relative_x, double relative_y,
	double normalized_y = 0.0)
{
	return {key, ContactPhase::motion, ns(milliseconds), x, y,
		relative_x, relative_y, normalized_y};
}

static TouchFact up(TouchKey key, std::uint64_t milliseconds,
	double x = 0.0, double y = 0.0, double normalized_y = 0.0)
{
	return {key, ContactPhase::up, ns(milliseconds), x, y, 0.0, 0.0,
		normalized_y};
}

static bool is_action(const Action& action, ActionType type,
	MouseButton button = MouseButton::none)
{
	return action.type == type && action.button == button;
}

static void expect_no_actions(const std::vector<Action>& actions,
	const std::string& message)
{
	expect(actions.empty(), message);
}

static void expect_single_action(const std::vector<Action>& actions,
	ActionType type, MouseButton button, const std::string& message)
{
	expect(actions.size() == 1 && is_action(actions.front(), type, button), message);
}

static void complete_tap(Recognizer& recognizer, TouchKey key,
	std::uint64_t down_ms = 0, double x = 0.0, double y = 0.0)
{
	expect_no_actions(recognizer.handle(down(key, down_ms, x, y)),
		"tap down must remain pending");
	expect_single_action(recognizer.handle(up(key, down_ms + 20, x, y)),
		ActionType::click_pulse, MouseButton::left,
		"stationary short tap must emit a left click pulse");
}

static void test_identity_and_reordered_events()
{
	Recognizer recognizer;
	const TouchKey first{37, 91};
	const TouchKey same_finger_other_device{38, 91};
	const TouchKey second_same_device{37, 407};

	expect_no_actions(recognizer.handle(motion(first, 1, 1.0, 0.0, 1.0, 0.0)),
		"motion arriving before down must be ignored");
	expect_no_actions(recognizer.handle(up(first, 2)),
		"up arriving before down must be ignored");
	recognizer.handle(down(first, 10));
	expect(recognizer.owns(first),
		"arbitrary nonzero touch and finger IDs must be tracked exactly");
	expect_no_actions(recognizer.handle(down(same_finger_other_device, 15)),
		"same finger ID on another device must not join the active gesture");
	expect(!recognizer.owns(same_finger_other_device)
		&& recognizer.state() == State::one_finger,
		"device identity must be part of the contact key");
	expect_no_actions(recognizer.handle(down(second_same_device, 20)),
		"a second arbitrary finger on the same device must pair");
	expect(recognizer.owns(second_same_device)
		&& recognizer.state() == State::two_finger_pending,
		"same-device pairing must not depend on ordinal finger IDs");
	const auto tracked_keys = recognizer.tracked_keys();
	expect(tracked_keys.size() == 2 && tracked_keys[0] == first
		&& tracked_keys[1] == second_same_device,
		"captured composite identities must be observable in acquisition order");
	expect_no_actions(recognizer.handle(up({37, 999}, 21)),
		"a mismatched terminal ID must not release tracked contacts");
	expect(recognizer.tracked_contacts() == 2,
		"mismatched terminal events must preserve the pair");
}

static State state_for_second_down(std::uint64_t interval_ms, double distance)
{
	Recognizer recognizer;
	const TouchKey key{101, 701};
	complete_tap(recognizer, key, 0, 0.0, 0.0);
	recognizer.handle(down(key, 20 + interval_ms, distance, 0.0));
	return recognizer.state();
}

static void test_double_tap_time_and_distance_boundaries()
{
	expect(state_for_second_down(39, 0.0) == State::one_finger,
		"39 ms must be too soon for the second tap");
	expect(state_for_second_down(40, 0.0) == State::second_tap,
		"40 ms must be included in the double-tap interval");
	expect(state_for_second_down(300, 0.0) == State::second_tap,
		"300 ms must be included in the double-tap interval");
	expect(state_for_second_down(301, 0.0) == State::one_finger,
		"301 ms must be outside the double-tap interval");
	expect(state_for_second_down(100, 99.999) == State::second_tap,
		"a second tap just inside 100 dp must match");
	expect(state_for_second_down(100, 100.001) == State::one_finger,
		"a second tap just outside 100 dp must start a new gesture");
}

static void test_one_finger_tap_motion_and_hold()
{
	const TouchKey key{111, 811};
	Recognizer tap;
	complete_tap(tap, key);
	expect(tap.has_recent_tap(), "a completed tap must retain recent-tap history");

	Recognizer moving;
	moving.handle(down(key, 0));
	auto actions = moving.handle(motion(key, 10, 8.001, 0.0, 2.0, -1.0));
	expect(actions.size() == 1 && actions.front().type == ActionType::relative_delta
		&& actions.front().delta_x == 2 && actions.front().delta_y == -1,
		"ordinary one-finger movement must emit relative pointer movement");
	expect_no_actions(moving.handle(up(key, 20, 8.001, 0.0)),
		"movement just outside tap slop must not click");
	expect(!moving.has_recent_tap(), "a non-tap completion must clear tap history");

	Recognizer inside;
	inside.handle(down(key, 0));
	inside.handle(motion(key, 10, 7.999, 0.0, 0.0, 0.0));
	expect_single_action(inside.handle(up(key, 399, 7.999, 0.0)),
		ActionType::click_pulse, MouseButton::left,
		"399 ms and movement just inside slop must remain a tap");

	Recognizer boundary;
	boundary.handle(down(key, 0));
	expect_no_actions(boundary.handle(up(key, 400)),
		"a one-finger contact ending at 400 ms must not click");

	Recognizer long_hold;
	long_hold.handle(down(key, 0));
	expect_no_actions(long_hold.tick(ns(399)),
		"a lone finger must emit no button before the hold cutoff");
	expect_no_actions(long_hold.tick(ns(400)),
		"a lone finger reaching the hold cutoff must not press a button");
	expect_no_actions(long_hold.handle(up(key, 500)),
		"a lone long hold must finish without a button action");
}

static void test_fractional_pointer_residue()
{
	Recognizer recognizer;
	const TouchKey key{121, 821};
	recognizer.handle(down(key, 0));
	expect_no_actions(recognizer.handle(motion(key, 10, 0.25, 0.0, 0.25, 0.0)),
		"fractional pointer movement must wait for a whole unit");
	auto actions = recognizer.handle(motion(key, 20, 1.0, 0.0, 0.75, 0.0));
	expect(actions.size() == 1 && actions.front().type == ActionType::relative_delta
		&& actions.front().delta_x == 1 && actions.front().delta_y == 0,
		"fractional pointer movement must be retained until it forms an integer delta");
}

static void test_second_tap_click_and_movement_drag_order()
{
	const TouchKey key{131, 831};
	Recognizer second_click;
	complete_tap(second_click, key, 0);
	second_click.handle(down(key, 100));
	expect_single_action(second_click.handle(up(key, 120)),
		ActionType::click_pulse, MouseButton::left,
		"a short stationary second tap must emit an ordinary second click");

	Recognizer drag;
	complete_tap(drag, key, 0);
	drag.handle(down(key, 100));
	expect_no_actions(drag.handle(motion(key, 110, 7.999, 0.0, 2.0, 0.0)),
		"second-contact movement inside slop must stay buffered");
	auto actions = drag.handle(motion(key, 120, 8.001, 0.0, 3.0, 0.0));
	expect(actions.size() == 2
		&& is_action(actions[0], ActionType::button_down, MouseButton::left)
		&& actions[1].type == ActionType::relative_delta
		&& actions[1].delta_x == 5,
		"crossing second-tap slop must press left before flushing buffered movement");
	expect(drag.state() == State::left_drag,
		"movement outside slop must establish a left drag");
	expect_single_action(drag.handle(up(key, 130, 8.001, 0.0)),
		ActionType::button_up, MouseButton::left,
		"left drag terminal event must release the left button");
}

static void test_second_tap_hold_boundary_and_added_contact()
{
	const TouchKey key{141, 841};
	Recognizer drag;
	complete_tap(drag, key, 0);
	drag.handle(down(key, 100));
	drag.handle(motion(key, 150, 1.0, 0.0, 1.0, 0.0));
	expect_no_actions(drag.tick(ns(499)),
		"a second tap held for 399 ms must remain pending");
	auto actions = drag.tick(ns(500));
	expect(actions.size() == 2
		&& is_action(actions[0], ActionType::button_down, MouseButton::left)
		&& actions[1].type == ActionType::relative_delta
		&& actions[1].delta_x == 1,
		"a second tap held for 400 ms must press left before buffered movement");

	const TouchKey added{141, 999};
	expect_single_action(drag.handle(down(added, 510)),
		ActionType::button_up, MouseButton::left,
		"an added same-device contact must terminate an active left drag");
	expect(drag.state() == State::drain_until_all_up
		&& drag.tracked_contacts() == 2,
		"left-drag contacts must remain inert until every tracked contact lifts");
	expect_no_actions(drag.handle(motion(key, 520, 20.0, 0.0, 10.0, 0.0)),
		"a surviving drained contact must not move the pointer");
}

static void begin_pair(Recognizer& recognizer, TouchKey primary,
	TouchKey secondary, std::uint64_t second_down_ms = 10)
{
	expect_no_actions(recognizer.handle(down(primary, 0, 0.0, 0.0, 0.10)),
		"primary down must remain pending");
	expect_no_actions(recognizer.handle(down(secondary, second_down_ms,
		0.0, 0.0, 0.10)), "secondary down must begin two-finger arbitration");
	expect(recognizer.state() == State::two_finger_pending,
		"same-device second contact must enter two-finger pending state");
}

static void test_two_finger_hold_and_buffer_order()
{
	Recognizer recognizer;
	const TouchKey primary{151, 851};
	const TouchKey secondary{151, 852};
	begin_pair(recognizer, primary, secondary, 10);
	expect_no_actions(recognizer.handle(motion(primary, 20, 1.0, 0.0,
		1.0, 0.0, 0.10)), "pending right hold must buffer primary movement");
	expect_no_actions(recognizer.tick(ns(409)),
		"a two-finger hold at 399 ms must remain pending");
	auto actions = recognizer.tick(ns(410));
	expect(actions.size() == 2
		&& is_action(actions[0], ActionType::button_down, MouseButton::right)
		&& actions[1].type == ActionType::relative_delta
		&& actions[1].delta_x == 1,
		"a 400 ms two-finger hold must press right before buffered primary movement");
	expect(recognizer.state() == State::right_drag,
		"stationary pair at the hold deadline must become a right drag");
}

static void test_gui_swipe_precedes_hold_without_mouse_mapping_gate()
{
	Recognizer recognizer;
	const TouchKey primary{161, 861};
	const TouchKey secondary{161, 862};
	complete_tap(recognizer, primary);
	recognizer.handle(down(primary, 100, 0.0, 0.0, 0.10));
	expect(recognizer.state() == State::second_tap && recognizer.has_recent_tap(),
		"a qualifying second contact must begin with recent-tap history");
	recognizer.handle(down(secondary, 110, 0.0, 0.0, 0.10));
	expect(recognizer.state() == State::two_finger_pending,
		"adding a same-device finger must turn second-tap intent into pair arbitration");
	expect_no_actions(recognizer.handle(motion(primary, 150, 0.0, 20.0,
		0.0, 12.0, 0.25)),
		"one downward finger must not open the GUI alone");
	auto actions = recognizer.handle(motion(secondary, 160, 0.0, 20.0,
		0.0, 12.0, 0.25));
	expect_single_action(actions, ActionType::open_gui, MouseButton::none,
		"two downward contacts crossing 15 percent must open the GUI even though the pure recognizer has no mouse mapping");
	expect(recognizer.state() == State::gui_consumed,
		"a winning GUI swipe must retain ownership of its contacts");
	expect(!recognizer.has_recent_tap(),
		"a winning GUI swipe must clear recent-tap history");
	expect_no_actions(recognizer.handle(motion(primary, 170, 0.0, 30.0,
		5.0, 5.0, 0.40)), "GUI ownership must suppress later pointer and GUI actions");
	expect_no_actions(recognizer.handle(up(primary, 180, 0.0, 30.0, 0.40)),
		"first GUI-swipe lift must remain captured and inert");
	expect(recognizer.state() == State::drain_until_all_up,
		"GUI-swipe survivor must enter drain state");
	expect_no_actions(recognizer.handle(up(secondary, 190, 0.0, 30.0, 0.40)),
		"last GUI-swipe lift must emit no mouse action");
	expect(recognizer.state() == State::idle && recognizer.tracked_contacts() == 0,
		"GUI-swipe ownership must clear only after every tracked contact lifts");
}

static void test_swipe_only_and_right_hold_precedence()
{
	const TouchKey primary{171, 871};
	const TouchKey secondary{171, 872};
	Recognizer swipe_only;
	begin_pair(swipe_only, primary, secondary);
	expect_no_actions(swipe_only.handle(motion(primary, 20, 8.001, 0.0,
		4.0, 0.0, 0.10)), "movement outside pair slop must emit no pointer action");
	expect(swipe_only.state() == State::swipe_only,
		"movement just outside 8 dp must permanently disqualify right hold");
	expect_no_actions(swipe_only.tick(ns(500)),
		"a slop-disqualified pair must never activate right hold");
	expect_no_actions(swipe_only.handle(up(primary, 510, 8.001, 0.0, 0.10)),
		"an incomplete GUI swipe must finish without mouse output");

	Recognizer inside;
	begin_pair(inside, primary, secondary);
	inside.handle(motion(primary, 20, 7.999, 0.0, 0.0, 0.0, 0.10));
	expect_single_action(inside.tick(ns(410)), ActionType::button_down,
		MouseButton::right, "movement just inside pair slop must permit right hold");
	auto actions = inside.handle(motion(primary, 420, 20.0, 20.0,
		3.0, 4.0, 0.30));
	expect(actions.size() == 1 && actions.front().type == ActionType::relative_delta
		&& actions.front().delta_x == 3 && actions.front().delta_y == 4,
		"after right hold wins, primary downward movement must move the pointer");
	expect(inside.state() == State::right_drag,
		"right-drag ownership must permanently block the GUI swipe");
	expect_no_actions(inside.handle(motion(secondary, 430, 0.0, 20.0,
		8.0, 8.0, 0.30)),
		"secondary movement during right drag must be inert");
}

static void test_right_drag_terminal_and_drain()
{
	const TouchKey primary{181, 881};
	const TouchKey secondary{181, 882};
	for (int lifted_index = 0; lifted_index < 2; ++lifted_index) {
		Recognizer recognizer;
		begin_pair(recognizer, primary, secondary);
		recognizer.tick(ns(410));
		const TouchKey lifted = lifted_index == 0 ? primary : secondary;
		const TouchKey survivor = lifted_index == 0 ? secondary : primary;
		expect_single_action(recognizer.handle(up(lifted, 420)),
			ActionType::button_up, MouseButton::right,
			"lifting either member of a right drag must release right");
		expect(recognizer.state() == State::drain_until_all_up
			&& recognizer.owns(survivor),
			"the surviving right-drag contact must remain tracked and inert");
		expect_no_actions(recognizer.handle(motion(survivor, 430, 20.0, 0.0,
			10.0, 0.0)), "right-drag survivor must not be promoted");
		recognizer.handle(up(survivor, 440));
		expect(recognizer.state() == State::idle,
			"right-drag drain must end after its final contact lifts");
	}
}

static void test_third_contact_terminates_right_drag()
{
	Recognizer recognizer;
	const TouchKey primary{191, 891};
	const TouchKey secondary{191, 892};
	const TouchKey third{191, 893};
	const TouchKey other_device{192, 893};
	begin_pair(recognizer, primary, secondary);
	recognizer.tick(ns(410));
	expect_no_actions(recognizer.handle(down(other_device, 415)),
		"a contact on another device must not terminate a held pair");
	expect(recognizer.state() == State::right_drag,
		"same finger ID on another device must not join the held pair");
	expect_single_action(recognizer.handle(down(third, 420)),
		ActionType::button_up, MouseButton::right,
		"a third same-device contact must release a held right button");
	expect(recognizer.state() == State::drain_until_all_up
		&& recognizer.tracked_contacts() == 3,
		"all three same-device contacts must drain without reassignment");
	expect_no_actions(recognizer.handle(motion(primary, 430, 20.0, 0.0,
		10.0, 0.0)), "drained contacts must remain inert");
	recognizer.handle(up(primary, 440));
	recognizer.handle(up(secondary, 450));
	expect(recognizer.state() == State::drain_until_all_up,
		"drain must retain the added contact until it lifts");
	recognizer.handle(up(third, 460));
	expect(recognizer.state() == State::idle,
		"third-contact drain must end only after all tracked contacts lift");
}

static void expect_cancelled(Recognizer& recognizer, bool expected_release,
	MouseButton button, const std::string& label)
{
	auto actions = recognizer.cancel();
	if (expected_release) {
		expect_single_action(actions, ActionType::button_up, button,
			label + " cancellation must release its held button");
	} else {
		expect_no_actions(actions, label + " cancellation must emit no spurious action");
	}
	expect(recognizer.state() == State::idle && recognizer.tracked_contacts() == 0
		&& !recognizer.has_recent_tap(),
		label + " cancellation must clear contacts, state, and tap history");
	expect_no_actions(recognizer.cancel(),
		label + " repeated cancellation must be idempotent");
}

static void test_cancellation_matrix()
{
	const TouchKey primary{201, 901};
	const TouchKey secondary{201, 902};

	Recognizer one;
	one.handle(down(primary, 0));
	expect_cancelled(one, false, MouseButton::none, "one-finger pending");

	Recognizer second;
	complete_tap(second, primary);
	second.handle(down(primary, 100));
	expect_cancelled(second, false, MouseButton::none, "second-tap pending");

	Recognizer pair;
	begin_pair(pair, primary, secondary);
	expect_cancelled(pair, false, MouseButton::none, "two-finger pending");

	Recognizer swipe;
	begin_pair(swipe, primary, secondary);
	swipe.handle(motion(primary, 20, 8.001, 0.0, 0.0, 0.0));
	expect_cancelled(swipe, false, MouseButton::none, "swipe-only");

	Recognizer left;
	complete_tap(left, primary);
	left.handle(down(primary, 100));
	left.tick(ns(500));
	expect_cancelled(left, true, MouseButton::left, "left drag");

	Recognizer right;
	begin_pair(right, primary, secondary);
	right.tick(ns(410));
	expect_cancelled(right, true, MouseButton::right, "right drag");

	Recognizer gui;
	begin_pair(gui, primary, secondary);
	gui.handle(motion(primary, 20, 0.0, 20.0, 0.0, 0.0, 0.30));
	gui.handle(motion(secondary, 30, 0.0, 20.0, 0.0, 0.0, 0.30));
	expect_cancelled(gui, false, MouseButton::none, "GUI-consumed");

	Recognizer drain;
	begin_pair(drain, primary, secondary);
	drain.handle(up(primary, 20));
	expect_cancelled(drain, false, MouseButton::none, "drain");
}

static void test_mapping_loss_uses_same_neutralization_contract()
{
	Recognizer recognizer;
	const TouchKey primary{211, 911};
	const TouchKey secondary{211, 912};
	begin_pair(recognizer, primary, secondary);
	recognizer.tick(ns(410));
	expect_single_action(recognizer.mapping_lost(), ActionType::button_up,
		MouseButton::right, "mapping loss must release a held right button");
	expect(recognizer.state() == State::idle && !recognizer.has_recent_tap(),
		"mapping loss must clear the pure gesture state");
}

static void test_button_source_composition()
{
	ButtonSourceComposer composer;
	auto transition = composer.set(ButtonSource::physical, MouseButton::left, true);
	expect(transition && transition->button == MouseButton::left
		&& transition->pressed,
		"first physical left press must raise aggregate left");
	expect(!composer.set(ButtonSource::gesture, MouseButton::left, true),
		"gesture press must not duplicate an already-held aggregate left");
	expect(!composer.set(ButtonSource::physical, MouseButton::left, false),
		"physical release must preserve gesture-held aggregate left");
	transition = composer.set(ButtonSource::gesture, MouseButton::left, false);
	expect(transition && transition->button == MouseButton::left
		&& !transition->pressed,
		"last left source release must lower aggregate left");

	transition = composer.set(ButtonSource::pen, MouseButton::right, true);
	expect(transition && transition->button == MouseButton::right
		&& transition->pressed,
		"first pen right press must raise aggregate right");
	expect(!composer.set(ButtonSource::physical, MouseButton::right, true),
		"physical right press must compose with a pen-held right");
	expect(!composer.set(ButtonSource::pen, MouseButton::right, false),
		"pen release must preserve physical-held aggregate right");
	transition = composer.set(ButtonSource::physical, MouseButton::right, false);
	expect(transition && transition->button == MouseButton::right
		&& !transition->pressed,
		"last right source release must lower aggregate right");

	ButtonSourceComposer reverse_right;
	reverse_right.set(ButtonSource::physical, MouseButton::right, true);
	reverse_right.set(ButtonSource::pen, MouseButton::right, true);
	expect(!reverse_right.set(ButtonSource::physical, MouseButton::right, false),
		"physical release must preserve pen-held aggregate right");
	transition = reverse_right.set(ButtonSource::pen, MouseButton::right, false);
	expect(transition && transition->button == MouseButton::right
		&& !transition->pressed,
		"pen must release aggregate right when it is the last source");

	composer.set(ButtonSource::physical, MouseButton::left, true);
	composer.set(ButtonSource::gesture, MouseButton::left, true);
	expect(composer.clear_source(ButtonSource::gesture).empty(),
		"clearing gesture ownership must preserve physical ownership");
	expect(composer.pressed(MouseButton::left),
		"aggregate left must remain held by the physical source");
	auto transitions = composer.clear_source(ButtonSource::physical);
	expect(transitions.size() == 1 && transitions.front().button == MouseButton::left
		&& !transitions.front().pressed,
		"clearing the remaining source must emit the aggregate release");
}

static void test_pump_coordinator_click_and_deadline_ordering()
{
	const TouchKey primary{221, 921};
	const TouchKey secondary{221, 922};
	PumpCoordinator coordinator;

	expect_no_actions(coordinator.begin_pump(),
		"the first pump must not retire a click");
	expect_no_actions(coordinator.handle(down(primary, 0)),
		"tap down must remain pending in the coordinator");
	expect_single_action(coordinator.handle(up(primary, 20)),
		ActionType::button_down, MouseButton::left,
		"a click pulse must become an observable left down");
	expect_single_action(coordinator.begin_pump(),
		ActionType::button_up, MouseButton::left,
		"the click release must wait until the next pump boundary");

	coordinator.handle(down(primary, 100, 0.0, 0.0, 0.10));
	coordinator.handle(down(secondary, 110, 0.0, 0.0, 0.10));
	expect_no_actions(coordinator.handle(up(primary, 509)),
		"a queued terminal event must retire the pair without a button");
	expect_no_actions(coordinator.tick(ns(510)),
		"a later deadline tick must not activate a retired right hold");
}

static void test_pump_coordinator_neutralizes_pending_click()
{
	const TouchKey key{231, 931};
	PumpCoordinator coordinator;
	coordinator.handle(down(key, 0));
	coordinator.handle(up(key, 20));
	expect_single_action(coordinator.neutralize(), ActionType::button_up,
		MouseButton::left,
		"neutralization must release a click that is waiting for the next pump");
	expect_no_actions(coordinator.begin_pump(),
		"a neutralized click must not release twice on the next pump");
}

static void test_nonowning_added_contact_terminates_before_overlay_capture()
{
	const TouchKey primary{241, 941};
	const TouchKey overlay_contact{241, 942};
	PumpCoordinator coordinator;
	coordinator.handle(down(primary, 0));
	coordinator.handle(up(primary, 20));
	coordinator.begin_pump();
	coordinator.handle(down(primary, 100));
	coordinator.tick(ns(500));

	expect_single_action(
		coordinator.terminate_for_nonowning_contact(overlay_contact),
		ActionType::button_up, MouseButton::left,
		"an overlay candidate must release an active left drag first");
	expect(coordinator.state() == State::drain_until_all_up
		&& coordinator.owns(primary) && !coordinator.owns(overlay_contact),
		"the added overlay contact must remain nonowning while trackpad survivors drain");
	expect_no_actions(coordinator.handle(motion(primary, 510, 20.0, 0.0,
		10.0, 0.0)),
		"a surviving trackpad contact must remain inert after overlay takeover");
}

int main()
{
	test_identity_and_reordered_events();
	test_double_tap_time_and_distance_boundaries();
	test_one_finger_tap_motion_and_hold();
	test_fractional_pointer_residue();
	test_second_tap_click_and_movement_drag_order();
	test_second_tap_hold_boundary_and_added_contact();
	test_two_finger_hold_and_buffer_order();
	test_gui_swipe_precedes_hold_without_mouse_mapping_gate();
	test_swipe_only_and_right_hold_precedence();
	test_right_drag_terminal_and_drain();
	test_third_contact_terminates_right_drag();
	test_cancellation_matrix();
	test_mapping_loss_uses_same_neutralization_contract();
	test_button_source_composition();
	test_pump_coordinator_click_and_deadline_ordering();
	test_pump_coordinator_neutralizes_pending_click();
	test_nonowning_added_contact_terminates_before_overlay_capture();
	return failures == 0 ? 0 : 1;
}
