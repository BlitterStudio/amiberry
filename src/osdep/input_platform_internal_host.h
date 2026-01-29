#pragma once

#include <string>

static inline void input_platform_init_joystick(int* num_joystick, didata* di_joystick)
{
	if (!num_joystick || !di_joystick)
		return;

	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

	*num_joystick = SDL_NumJoysticks();
	if (*num_joystick > MAX_INPUT_DEVICES)
		*num_joystick = MAX_INPUT_DEVICES;

	std::string cfg = get_controllers_path();
	cfg += "gamecontrollerdb.txt";
	SDL_GameControllerAddMappingsFromFile(cfg.c_str());

	std::string controllers = get_controllers_path();
	controllers.append("gamecontrollerdb_user.txt");
	SDL_GameControllerAddMappingsFromFile(controllers.c_str());

	controllers = get_controllers_path();
	for (auto i = 0; i < *num_joystick; i++)
	{
		struct didata* did = &di_joystick[i];

		if (SDL_IsGameController(i))
		{
			open_as_game_controller(did, i);
			setup_controller_mappings(did, i);
		}
		else
		{
			open_as_joystick(did, i);
			write_log("Joystick #%i does not have a mapping available\n", did->joystick_id);
		}

		fix_didata(did);
		setup_mapping(did, controllers, i);
	}
}
