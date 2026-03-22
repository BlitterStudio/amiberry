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
	// do the loop
	for (auto i = 0; i < *num_joystick; i++)
	{
		struct didata* did = &di_joystick[i];
		const SDL_JoystickID joy_id = joysticks[i];

		// Check if joystick supports SDL's gamepad interface (a mapping is available)
		if (SDL_IsGamepad(joy_id))
		{
			open_as_game_controller(did, joy_id);
			setup_controller_mappings(did, i);
		}
		// Gamepad interface not supported, try to open as joystick
		else
		{
			open_as_joystick(did, joy_id);
			write_log("Joystick #%i does not have a mapping available\n", did->joystick_id);
		}

		fix_didata(did);
		setup_mapping(did, controllers, i);
	}
	SDL_free(joysticks);

	// Register the built-in on-screen joystick only when enabled
	if (currprefs.onscreen_joystick && *num_joystick < MAX_INPUT_DEVICES) {
		osj_device_index = *num_joystick;
		struct didata* did = &di_joystick[osj_device_index];
		cleardid(did);
		did->name = "On-Screen Joystick";
		did->joystick_name = "On-Screen Joystick";
		did->is_controller = false;
		did->controller = nullptr;
		did->joystick = nullptr;
		did->joystick_id = -1;
		// 2 axes (X, Y) + 2 buttons (fire, 2nd fire)
		did->axles = 2;
		did->buttons = 2;
		did->buttons_real = 2;
		// Set up axis metadata
		did->axismappings[0] = 0;
		did->axisname[0] = "X Axis";
		did->axissort[0] = 0;
		did->axistype[0] = AXISTYPE_NORMAL;
		did->axismappings[1] = 1;
		did->axisname[1] = "Y Axis";
		did->axissort[1] = 1;
		did->axistype[1] = AXISTYPE_NORMAL;
		// Set up button metadata
		did->buttonmappings[0] = 0;
		did->buttonname[0] = "Fire";
		did->buttonsort[0] = 0;
		did->buttonmappings[1] = 1;
		did->buttonname[1] = "2nd Fire";
		did->buttonsort[1] = 1;
		// Create +/- button entries for axes (needed by the input mapping system)
		fixthings(did);
		(*num_joystick)++;
		write_log("On-Screen Joystick registered as JOY%d\n", osj_device_index);
	}
}
