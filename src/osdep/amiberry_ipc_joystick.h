#pragma once

#include <charconv>
#include <string>
#include <vector>

namespace Amiberry::IPC {

struct JoystickRequest {
	int port = 0;
	int directions = 0;
	int buttons = 0;
};

inline const char* parse_joystick_request(const std::vector<std::string>& args,
	JoystickRequest& request)
{
	if (args.size() != 3)
		return "Usage: SEND_JOYSTICK <port> <directions> <buttons>";
	int values[3];
	for (int i = 0; i < 3; ++i) {
		const auto& text = args[i];
		if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
			return "Invalid joystick parameters: expected decimal integers";
		const auto parsed = std::from_chars(text.data(), text.data() + text.size(), values[i]);
		if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
			return "Invalid joystick parameters: expected decimal integers";
	}
	if (values[0] > 3)
		return "Port must be 0-3";
	if (values[1] > 15)
		return "Directions must be 0-15";
	if ((values[1] & 3) == 3 || (values[1] & 12) == 12)
		return "Opposite joystick directions are not allowed";
	if (values[2] > 127)
		return "Buttons must be 0-127";
	request = {values[0], values[1], values[2]};
	return nullptr;
}

} // namespace Amiberry::IPC
