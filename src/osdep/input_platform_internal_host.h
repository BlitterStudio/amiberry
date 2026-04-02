#pragma once

#include <string>

static inline void input_platform_init_joystick(int* num_joystick, didata* di_joystick)
{
	if (!num_joystick || !di_joystick)
		return;

	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
	if (joysticks == nullptr) {
		write_log("SDL_GetJoysticks failed: %s\n", SDL_GetError());
		*num_joystick = 0;
		return;
	}
	write_log("SDL_GetJoysticks: %d device(s) found\n", count);
	*num_joystick = count;
	if (*num_joystick > MAX_INPUT_DEVICES)
		*num_joystick = MAX_INPUT_DEVICES;

	// set up variables / paths etc.
	std::string cfg = get_controllers_path();
	cfg += "gamecontrollerdb.txt";
	SDL_AddGamepadMappingsFromFile(cfg.c_str());

	std::string controllers = get_controllers_path();
	controllers.append("gamecontrollerdb_user.txt");
	SDL_AddGamepadMappingsFromFile(controllers.c_str());

	// Possible scenarios:
	// 1 - Controller is an SDL Gamepad, no retroarch file: we use the default mapping
	// 2 - Controller is an SDL Gamepad, but there's a retroarch file: retroarch overrides default mapping
	// 3 - Controller is not an SDL Gamepad, but there's a retroarch file: open it as Joystick, use retroarch mapping
	// 4 - Controller is not an SDL Gamepad, no retroarch file: open as Joystick with default map

	controllers = get_controllers_path();
	// do the loop, filtering out non-joystick devices (e.g. keyboards exposed via evdev)
	int valid_count = 0;
	for (int i = 0; i < count && valid_count < MAX_INPUT_DEVICES; i++)
	{
		const SDL_JoystickID joy_id = joysticks[i];

		// Gamepads are always valid
		if (SDL_IsGamepad(joy_id))
		{
			struct didata* did = &di_joystick[valid_count];
			open_as_game_controller(did, joy_id);
			setup_controller_mappings(did, valid_count);
			fix_didata(did);
			setup_mapping(did, controllers, valid_count);
			valid_count++;
			continue;
		}

		// For non-gamepad devices, check the joystick type and capabilities
		SDL_JoystickType jtype = SDL_GetJoystickTypeForID(joy_id);

		// Known joystick types are always valid
		if (jtype != SDL_JOYSTICK_TYPE_UNKNOWN)
		{
			struct didata* did = &di_joystick[valid_count];
			open_as_joystick(did, joy_id);
			write_log("Joystick #%i does not have a mapping available\n", did->joystick_id);
			fix_didata(did);
			setup_mapping(did, controllers, valid_count);
			valid_count++;
			continue;
		}

		// Unknown type: open temporarily to check capabilities
		SDL_Joystick* temp_joy = SDL_OpenJoystick(joy_id);
		if (temp_joy)
		{
			int axes = SDL_GetNumJoystickAxes(temp_joy);
			int buttons = SDL_GetNumJoystickButtons(temp_joy);
			const char* dev_name = SDL_GetJoystickName(temp_joy);

			if (axes == 0 && buttons <= 1)
			{
				// No axes and at most 1 button — not a real joystick (likely a keyboard or other HID device)
				write_log("Skipping non-joystick device: \"%s\" (axes=%d, buttons=%d)\n",
					dev_name ? dev_name : "?", axes, buttons);
				SDL_CloseJoystick(temp_joy);
				continue;
			}
			SDL_CloseJoystick(temp_joy);
		}
		else
		{
			// Can't open at all — skip it
			const char* dev_name = SDL_GetJoystickNameForID(joy_id);
			write_log("Skipping unopenable joystick device: \"%s\"\n", dev_name ? dev_name : "?");
			continue;
		}

		// Passed capability check — register it
		struct didata* did = &di_joystick[valid_count];
		open_as_joystick(did, joy_id);
		write_log("Joystick #%i does not have a mapping available\n", did->joystick_id);
		fix_didata(did);
		setup_mapping(did, controllers, valid_count);
		valid_count++;
	}
	*num_joystick = valid_count;
	SDL_free(joysticks);

	// Register the built-in on-screen joystick only when enabled
	if (currprefs.onscreen_joystick) {
		ensure_onscreen_joystick_registered();
	}
}
