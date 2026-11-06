#include "amiberry_ipc_joystick.h"
#include "joyport_input.h"

#include <cassert>
#include <iostream>

using Amiberry::IPC::JoystickRequest;
using Amiberry::IPC::parse_joystick_request;
using Amiberry::Input::JoyportInput;
using Amiberry::Input::joystick_button_mask;

int main()
{
	JoystickRequest request;
	for (int port = 0; port < 4; ++port) {
		for (int directions = 0; directions < 16; ++directions) {
			for (int buttons = 0; buttons < 128; ++buttons) {
				const bool opposite = (directions & 3) == 3 || (directions & 12) == 12;
				const auto error = parse_joystick_request(
					{std::to_string(port), std::to_string(directions), std::to_string(buttons)}, request);
				assert((error != nullptr) == opposite);
				if (!error) {
					assert(request.port == port);
					assert(request.directions == directions);
					assert(request.buttons == buttons);
				}
			}
		}
	}
	for (const auto& args : std::vector<std::vector<std::string>>{
		{}, {"0"}, {"0", "0"}, {"0", "0", "0", "0"},
		{"4", "0", "0"}, {"0", "16", "0"}, {"0", "0", "128"},
		{"-1", "0", "0"}, {"0", "-1", "0"}, {"0", "0", "-1"},
		{"0x1", "0", "0"}, {"0", "1junk", "0"}, {"0", "0", "1.0"},
		{"", "0", "0"}, {"0", "", "0"}, {"0", "0", ""},
		{" 0", "0", "0"}, {"0", "+1", "0"}, {"0", "0", "1 "},
		{"999999999999999999999999", "0", "0"},
		{"0", "999999999999999999999999", "0"},
		{"0", "0", "999999999999999999999999"}}) {
		request = {2, 4, 8};
		assert(parse_joystick_request(args, request));
		assert(request.port == 2 && request.directions == 4 && request.buttons == 8);
	}
	assert(!parse_joystick_request({"01", "04", "064"}, request));
	assert(request.port == 1 && request.directions == 4 && request.buttons == 64);

	// Core bit positions consumed by the existing CIA/POT and CD32 shifter.
	const int core_buttons[] = {8, 9, 6, 7, 4, 5, 3};
	const int serial_bits[] = {1, 0, 3, 2, 5, 4, 6};
	for (int buttons = 0; buttons < 128; ++buttons) {
		int expected = 0;
		int serial_pressed = 0;
		for (int bit = 0; bit < 7; ++bit) {
			if (buttons & (1 << bit)) {
				expected |= 1 << core_buttons[bit];
				serial_pressed |= 1 << serial_bits[bit];
			}
		}
		const int mapped = joystick_button_mask(buttons, true);
		assert(mapped == expected);
		// In serial mode the shifter visits core bits 9..3, then ID 1, 0.
		int serial = 0x80;
		for (int bit = 0; bit < 7; ++bit) {
			if (!(mapped & (1 << (9 - bit))))
				serial |= 1 << bit;
		}
		assert(serial == (0xff ^ serial_pressed));
		assert(!(mapped & 3)); // Generic button 2 would ground all serial bits.
	}
	for (int buttons = 0; buttons < 4; ++buttons)
		assert(joystick_button_mask(buttons, false) == buttons);

	JoyportInput input;
	for (int port = 0; port < 4; ++port) {
		assert(input.buttons(port) == 0 && input.directions(port, false) == 0);
		input.set(1, port, 5, 1 << port);
		assert(input.buttons(port) == (1 << port));
		assert(input.directions(port, false) == 5);
		input.set(1, port, 10, 0);
		assert(input.buttons(port) == 0 && input.directions(port, false) == 10);
		input.set(1, port, 0, 0);
		assert(input.buttons(port) == 0 && input.directions(port, false) == 0);
	}
	input.set_host_button(0, 0, true);
	input.set_host_directions(0, 5);
	input.set(1, 0, 10, 3);
	assert(input.buttons(0) == 3 && input.directions(0, false) == 10);
	assert(input.directions(0, true) == 15);
	input.set_host_button(0, 0, false);
	input.set_host_directions(0, 0);
	assert(input.buttons(0) == 3 && input.directions(0, false) == 10);
	input.set_host_button(0, 0, true);
	input.set_host_directions(0, 5);
	input.release(1);
	assert(input.buttons(0) == 1 && input.directions(0, false) == 5);
	assert(input.buttons(1) == 0 && input.directions(1, false) == 0);

	input.set(1, 1, 1, 256);
	input.set(2, 1, 4, 512);
	input.release(1);
	assert(input.buttons(1) == 512 && input.directions(1, false) == 4);
	input.set(1, 1, 1, 256);
	input.clear_injected();
	assert(input.buttons(0) == 1 && input.directions(0, false) == 5);
	assert(input.buttons(1) == 0 && input.directions(1, false) == 0);
	input.set(1, 2, 2, 1);
	input.release(2); // A disconnect after reset cannot resurrect another hold.
	assert(input.buttons(1) == 0 && input.directions(1, false) == 0);
	input.reset();
	for (int port = 0; port < 4; ++port)
		assert(input.buttons(port) == 0 && input.directions(port, false) == 0);
	input.restore_host(3, 15, 3);
	assert(input.buttons(3) == 3 && input.directions(3, false) == 15);
	input.release(999); // An unrelated controller does not change host state.
	assert(input.buttons(3) == 3 && input.directions(3, false) == 15);
	std::cout << "IPC joystick parser, mapping, source merging and release tests passed\n";
}
