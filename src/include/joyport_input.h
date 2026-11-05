#pragma once

#include <array>
#include <cstdint>
#include <map>

namespace Amiberry::Input {

// These are core joybutton bit positions, not host controller button numbers.
inline int joystick_button_mask(int buttons, bool cd32)
{
	constexpr int cd32_bits[] = {8, 9, 6, 7, 4, 5, 3};
	int result = 0;
	if (cd32) {
		for (int bit = 0; bit < 7; ++bit) {
			if (buttons & (1 << bit))
				result |= 1 << cd32_bits[bit];
		}
	} else {
		result = buttons & 3;
	}
	return result;
}

struct JoyportState {
	int directions = 0;
	int buttons = 0;
};

// Physical mappings keep their own state: releasing an injected source must
// not release a physical button, and physical releases must not erase IPC holds.
class JoyportInput {
	static constexpr int ports = 4;
	static constexpr int injected_direction_marker = 0x8000;
	std::array<JoyportState, ports> host{};
	std::array<JoyportState, ports> injected{};
	std::map<std::uint64_t, std::array<JoyportState, ports>> clients;

	void merge()
	{
		injected = {};
		for (const auto& client : clients) {
			for (int port = 0; port < ports; ++port) {
				injected[port].directions |= client.second[port].directions;
				injected[port].buttons |= client.second[port].buttons;
			}
		}
	}

public:
	void set_host_button(int port, int button, bool pressed)
	{
		if (pressed)
			host[port].buttons |= 1 << button;
		else
			host[port].buttons &= ~(1 << button);
	}

	void set_host_directions(int port, int directions)
	{
		host[port].directions = directions;
	}

	void restore_host(int port, int directions, int buttons)
	{
		host[port] = {directions, buttons};
	}

	JoyportState save_host(int port) const
	{
		// The recorder stores physical state, never connection-owned holds.
		// An unused direction bit requests counter refresh after restoration.
		return {host[port].directions
			| (injected[port].directions ? injected_direction_marker : 0),
			host[port].buttons};
	}

	bool restore_saved_host(int port, int directions, int buttons)
	{
		restore_host(port, directions & ~injected_direction_marker, buttons);
		return (directions & injected_direction_marker) != 0;
	}

	void set(std::uint64_t client, int port, int directions, int buttons)
	{
		clients[client][port] = {directions, buttons};
		merge();
	}

	void release(std::uint64_t client)
	{
		clients.erase(client);
		merge();
	}

	void clear_injected()
	{
		clients.clear();
		injected = {};
	}

	void reset()
	{
		clear_injected();
		host = {};
	}

	int buttons(int port) const
	{
		return host[port].buttons | injected[port].buttons;
	}

	int directions(int port, bool allow_opposites) const
	{
		if (!injected[port].directions)
			return host[port].directions;
		int result = host[port].directions | injected[port].directions;
		if (!allow_opposites) {
			if (result & 2)
				result &= ~1;
			if (result & 8)
				result &= ~4;
		}
		return result;
	}
};

} // namespace Amiberry::Input
