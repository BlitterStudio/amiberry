#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "on_screen_joystick_capture.h"

using namespace osj_capture;

static int failures = 0;

static void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		failures++;
	}
}

static void test_independent_control_owners_and_release_order()
{
	Registry registry;
	const TouchKey dpad{11, 101};
	const TouchKey fire1{11, 102};
	const TouchKey fire2{11, 103};

	expect(registry.acquire(dpad, Control::dpad) == AcquireResult::captured,
		"first D-pad finger must capture the D-pad");
	expect(registry.acquire(fire1, Control::button1) == AcquireResult::captured,
		"first fire-1 finger must capture fire 1 independently");
	expect(registry.acquire(fire2, Control::button2) == AcquireResult::captured,
		"first fire-2 finger must capture fire 2 independently");
	expect(registry.occupied(Control::dpad) && registry.occupied(Control::button1)
		&& registry.occupied(Control::button2),
		"all three controls must have explicit occupied state");

	const auto released_fire1 = registry.release(fire1);
	expect(released_fire1 && released_fire1->state == State::captured
		&& released_fire1->control == Control::button1,
		"matching fire-1 terminal event must release only fire 1");
	expect(registry.occupied(Control::dpad) && registry.occupied(Control::button2),
		"releasing fire 1 must preserve the D-pad and fire-2 owners");
	expect(!registry.occupied(Control::button1),
		"released fire 1 must become unoccupied");

	const auto* dpad_motion = registry.lookup(dpad);
	expect(dpad_motion && dpad_motion->state == State::captured
		&& dpad_motion->control == Control::dpad,
		"matching D-pad motion lookup must retain capture at arbitrary coordinates");
}

static void test_contender_cannot_steal_or_inherit()
{
	Registry registry;
	const TouchKey owner{21, 201};
	const TouchKey contender{21, 202};

	expect(registry.acquire(owner, Control::dpad) == AcquireResult::captured,
		"first D-pad finger must become the owner");
	expect(registry.acquire(contender, Control::dpad) == AcquireResult::contender,
		"competing D-pad finger must be consumed as a contender");
	const auto* current_owner = registry.owner(Control::dpad);
	expect(current_owner && current_owner->key == owner,
		"a live owner must not be replaced by a contender");
	const auto* inert_motion = registry.lookup(contender);
	expect(inert_motion && inert_motion->state == State::contender
		&& inert_motion->control == Control::dpad,
		"contender motion must remain tracked and consumed but inert");

	const auto released_owner = registry.release(owner);
	expect(released_owner && released_owner->state == State::captured,
		"owner terminal event must release the captured owner");
	expect(!registry.occupied(Control::dpad),
		"owner release must leave the control unoccupied, not transfer ownership");
	inert_motion = registry.lookup(contender);
	expect(inert_motion && inert_motion->state == State::contender,
		"held contender must stay inert after the original owner releases");
	expect(registry.acquire(contender, Control::dpad) == AcquireResult::already_tracked,
		"a held contender must not acquire without its own terminal event");

	const auto released_contender = registry.release(contender);
	expect(released_contender && released_contender->state == State::contender,
		"contender terminal event must be consumed without releasing a control");
	expect(registry.acquire(contender, Control::dpad) == AcquireResult::captured,
		"the same identity may capture only after lifting and touching again");
}

static void test_contender_terminal_preserves_live_owner()
{
	Registry registry;
	const TouchKey owner{31, 301};
	const TouchKey contender{31, 302};
	registry.acquire(owner, Control::button1);
	registry.acquire(contender, Control::button1);

	const auto released = registry.release(contender);
	expect(released && released->state == State::contender,
		"contender terminal event must report contender state");
	const auto* current_owner = registry.owner(Control::button1);
	expect(current_owner && current_owner->key == owner,
		"contender terminal event must not release the live owner");
}

static void test_identity_uses_touch_and_finger_pair()
{
	Registry registry;
	const TouchKey first_device{41, 401};
	const TouchKey second_device{42, 401};

	expect(registry.acquire(first_device, Control::dpad) == AcquireResult::captured,
		"first touch device must capture with a nonzero finger ID");
	expect(registry.acquire(second_device, Control::button1) == AcquireResult::captured,
		"the identical finger ID on another touch device must be a distinct owner");
	expect(registry.lookup(first_device) && registry.lookup(second_device),
		"both paired identities must remain independently addressable");

	const auto mismatched = registry.release({41, 499});
	expect(!mismatched,
		"untracked terminal identity must not release another owner");
	expect(registry.occupied(Control::dpad) && registry.occupied(Control::button1),
		"mismatched terminal event must preserve both live owners");
}

static void test_zero_finger_id_can_capture_and_release()
{
	Registry registry;
	const TouchKey primary_finger{45, 0};

	expect(registry.acquire(primary_finger, Control::dpad) == AcquireResult::captured,
		"Android primary finger ID zero must be allowed to capture a control");
	const auto* capture = registry.lookup(primary_finger);
	expect(capture && capture->state == State::captured
		&& capture->control == Control::dpad,
		"zero-valued finger ID must remain addressable while captured");
	const auto released = registry.release(primary_finger);
	expect(released && released->state == State::captured
		&& released->control == Control::dpad,
		"zero-valued finger ID must release its captured control");
}

static void test_stale_audit_is_scoped_to_matching_touch_device()
{
	Registry registry;
	const TouchKey stale_owner{51, 501};
	const TouchKey present_owner{51, 502};
	const TouchKey other_device_same_finger{52, 501};
	registry.acquire(stale_owner, Control::dpad);
	registry.acquire(present_owner, Control::button1);
	registry.acquire(other_device_same_finger, Control::button2);

	const auto released = registry.release_stale_captures(51, std::vector<FingerId>{502});
	expect(released.size() == 1 && released.front() == Control::dpad,
		"audit must release only a genuinely absent owner on the matching device");
	expect(!registry.lookup(stale_owner),
		"genuinely stale owner must leave the registry");
	expect(registry.lookup(present_owner) && registry.occupied(Control::button1),
		"owner present in the matching SDL touch list must remain captured");
	expect(registry.lookup(other_device_same_finger)
		&& registry.occupied(Control::button2),
		"audit must not compare or release captures from another touch device");

	const TouchKey replacement{51, 503};
	expect(registry.acquire(replacement, Control::dpad) == AcquireResult::captured,
		"new touch may acquire after SDL proves the previous owner stale");
}

static void test_stale_audit_does_not_promote_contender()
{
	Registry registry;
	const TouchKey owner{61, 601};
	const TouchKey contender{61, 602};
	registry.acquire(owner, Control::keyboard);
	registry.acquire(contender, Control::keyboard);

	const auto released = registry.release_stale_captures(61, std::vector<FingerId>{602});
	expect(released.size() == 1 && released.front() == Control::keyboard,
		"absent captured owner must be released even when a contender is present");
	expect(!registry.occupied(Control::keyboard),
		"stale-owner release must not promote the held contender");
	const auto* tracked_contender = registry.lookup(contender);
	expect(tracked_contender && tracked_contender->state == State::contender,
		"stale audit must keep the contender explicitly inert until its terminal event");
}

static void test_stale_audit_discards_only_absent_contender()
{
	Registry registry;
	const TouchKey owner{62, 611};
	const TouchKey absent_contender{62, 612};
	const TouchKey other_device_contender{63, 612};
	registry.acquire(owner, Control::button2);
	registry.acquire(absent_contender, Control::button2);
	registry.acquire({63, 613}, Control::button1);
	registry.acquire(other_device_contender, Control::button1);

	const auto released = registry.release_stale_captures(62, std::vector<FingerId>{611});
	expect(released.empty(),
		"discarding an absent contender must not report or release a control owner");
	expect(!registry.lookup(absent_contender),
		"absent contender on the matching device must not leak registry state");
	expect(registry.lookup(owner) && registry.occupied(Control::button2),
		"present owner must remain captured while an absent contender is discarded");
	expect(registry.lookup(other_device_contender),
		"audit must not discard a contender from another touch device");
}

static void test_clear_removes_all_state()
{
	Registry registry;
	registry.acquire({71, 701}, Control::dpad);
	registry.acquire({71, 702}, Control::dpad);
	registry.acquire({71, 703}, Control::button1);
	registry.clear();

	expect(registry.empty(), "global clear must remove captured owners and contenders");
	expect(!registry.occupied(Control::dpad) && !registry.occupied(Control::button1),
		"global clear must leave every control unoccupied");
}

int main()
{
	test_independent_control_owners_and_release_order();
	test_contender_cannot_steal_or_inherit();
	test_contender_terminal_preserves_live_owner();
	test_identity_uses_touch_and_finger_pair();
	test_zero_finger_id_can_capture_and_release();
	test_stale_audit_is_scoped_to_matching_touch_device();
	test_stale_audit_does_not_promote_contender();
	test_stale_audit_discards_only_absent_contender();
	test_clear_removes_all_state();
	return failures == 0 ? 0 : 1;
}
