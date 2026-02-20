#pragma once

#include <string>

static inline void input_platform_init_joystick(int* num_joystick, didata* di_joystick)
{
	if (!num_joystick || !di_joystick)
		return;

	// This disables the use of gyroscopes as axis device
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

	num_joystick = SDL_NumJoysticks();
	if (num_joystick > MAX_INPUT_DEVICES)
		num_joystick = MAX_INPUT_DEVICES;

	// set up variables / paths etc.
	std::string cfg = get_controllers_path();
	cfg += "gamecontrollerdb.txt";
	SDL_GameControllerAddMappingsFromFile(cfg.c_str());

	std::string controllers = get_controllers_path();
	controllers.append("gamecontrollerdb_user.txt");
	SDL_GameControllerAddMappingsFromFile(controllers.c_str());

	// Possible scenarios:
	// 1 - Controller is an SDL2 Game Controller, no retroarch file: we use the default mapping
	// 2 - Controller is an SDL2 Game Controller, but there's a retroarch file: retroarch overrides default mapping
	// 3 - Controller is not an SDL2 Game Controller, but there's a retroarch file: open it as Joystick, use retroarch mapping
	// 4 - Controller is not an SDL2 Game Controller, no retroarch file: open as Joystick with default map

	controllers = get_controllers_path();
	// do the loop
	for (auto i = 0; i < num_joystick; i++)
	{
		struct didata* did = &di_joystick[i];

		// Check if joystick supports SDL's game controller interface (a mapping is available)
		if (SDL_IsGameController(i))
		{
			open_as_game_controller(did, i);
			setup_controller_mappings(did, i);
		}
		// Controller interface not supported, try to open as joystick
		else
		{
			open_as_joystick(did, i);
			write_log("Joystick #%i does not have a mapping available\n", did->joystick_id);
		}

		fix_didata(did);
		setup_mapping(did, controllers, i);
	}

	// Register the built-in on-screen joystick as a virtual device
	if (num_joystick < MAX_INPUT_DEVICES) {
		osj_device_index = num_joystick;
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
		num_joystick++;
		write_log("On-Screen Joystick registered as JOY%d\n", osj_device_index);
	}
}
