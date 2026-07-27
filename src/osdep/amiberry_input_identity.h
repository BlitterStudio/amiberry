#pragma once

#include <string>

struct amiberry_joystick_identity
{
	std::string guid;
	std::string serial;
	std::string path;
	std::string name;
	bool is_controller{};
};

// SDL instance IDs change whenever a controller reconnects. Prefer physical
// identity data when the backend exposes it, with GUID/name as a deterministic
// fallback for devices that do not report a serial number or stable path.
inline int amiberry_joystick_identity_score(const amiberry_joystick_identity& previous,
	const amiberry_joystick_identity& current)
{
	if (previous.is_controller != current.is_controller)
		return -1;

	if (!previous.guid.empty() && !current.guid.empty() && previous.guid != current.guid)
		return -1;

	if (!previous.serial.empty() && !current.serial.empty())
		return previous.serial == current.serial ? 400 : -1;

	if (!previous.path.empty() && !current.path.empty() && previous.path == current.path)
		return 300;

	if (!previous.guid.empty() && previous.guid == current.guid) {
		if (!previous.name.empty() && previous.name == current.name)
			return 200;
		return 100;
	}

	if (!previous.name.empty() && previous.name == current.name)
		return 50;

	return -1;
}
