#include <array>
#include <iostream>

#include "amiberry_input_identity.h"

static int failures;

static void expect_eq(const int actual, const int expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

static void expect_true(const bool actual, const char* message)
{
	if (!actual) {
		std::cerr << message << '\n';
		failures++;
	}
}

static amiberry_joystick_identity identity(const char* guid, const char* serial,
	const char* path, const char* name, const bool is_controller = true)
{
	return {guid, serial, path, name, is_controller};
}

template <size_t Count>
static int best_match(const amiberry_joystick_identity& current,
	const std::array<amiberry_joystick_identity, Count>& previous)
{
	int best_index = -1;
	int best_score = -1;
	bool ambiguous = false;
	for (size_t i = 0; i < Count; ++i) {
		const int score = amiberry_joystick_identity_score(previous[i], current);
		if (score > best_score) {
			best_index = static_cast<int>(i);
			best_score = score;
			ambiguous = false;
		} else if (score >= 0 && score == best_score) {
			ambiguous = true;
		}
	}
	return ambiguous ? -1 : best_index;
}

int main()
{
	const auto original = identity("guid-a", "serial-a", "/dev/input/js0", "Gamepad");

	expect_true(amiberry_joystick_identity_equal(
		original, identity("guid-a", "serial-a", "/dev/input/js0", "Gamepad")),
		"An unchanged physical identity must compare equal");
	expect_true(!amiberry_joystick_identity_equal(
		original, identity("guid-a", "serial-a", "/dev/input/js1", "Gamepad")),
		"A changed physical path must not compare exactly equal");
	expect_eq(amiberry_joystick_identity_score(original,
		identity("guid-a", "serial-a", "/dev/input/js4", "Renamed Gamepad")), 400,
		"A serial number must survive a changed path and display name");
	expect_eq(amiberry_joystick_identity_score(original,
		identity("guid-a", "serial-b", "/dev/input/js0", "Gamepad")), -1,
		"Different serial numbers must distinguish otherwise identical devices");
	expect_eq(amiberry_joystick_identity_score(
		identity("guid-a", "", "/dev/input/by-path/controller-a", "Gamepad"),
		identity("guid-a", "", "/dev/input/by-path/controller-a", "Gamepad")), 300,
		"A stable device path must distinguish controllers without serial numbers");
	expect_eq(amiberry_joystick_identity_score(
		identity("guid-a", "", "/dev/input/js0", "Gamepad"),
		identity("guid-a", "", "/dev/input/js1", "Gamepad")), 200,
		"GUID and name must provide a reconnect fallback when a path changes");
	expect_eq(amiberry_joystick_identity_score(original,
		identity("guid-b", "serial-a", "/dev/input/js0", "Gamepad")), -1,
		"A conflicting GUID must reject a serial collision");
	expect_eq(amiberry_joystick_identity_score(original,
		identity("guid-a", "serial-a", "/dev/input/js0", "Gamepad", false)), -1,
		"Gamepad and raw joystick mappings must not be mixed");
	expect_eq(amiberry_joystick_identity_score(
		identity("", "", "", "Legacy Joystick"),
		identity("", "", "", "Legacy Joystick")), 50,
		"The friendly name must remain a fallback for legacy backends");
	expect_eq(amiberry_joystick_identity_score(
		identity("", "", "", "Joystick A"),
		identity("", "", "", "Joystick B")), -1,
		"Devices without a shared identity must not inherit mappings");

	const std::array duplicate_controllers = {
		identity("shared-guid", "", "/dev/input/by-path/controller-a", "Twin Gamepad"),
		identity("shared-guid", "", "/dev/input/by-path/controller-b", "Twin Gamepad")
	};
	expect_eq(best_match(
		identity("shared-guid", "", "/dev/input/by-path/controller-b", "Twin Gamepad"),
		duplicate_controllers), 1,
		"A reordered duplicate controller must retain the mapping for its physical path");

	const std::array indistinguishable_controllers = {
		identity("shared-guid", "", "", "Twin Gamepad"),
		identity("shared-guid", "", "", "Twin Gamepad")
	};
	expect_eq(best_match(
		identity("shared-guid", "", "", "Twin Gamepad"),
		indistinguishable_controllers), -1,
		"An ambiguous identical-controller match must be rejected");

	return failures == 0 ? 0 : 1;
}
