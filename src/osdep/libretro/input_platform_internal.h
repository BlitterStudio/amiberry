#pragma once

#include <string>

static inline void input_platform_init_joystick(int* num_joystick, didata* di_joystick)
{
	if (!num_joystick || !di_joystick)
		return;

	*num_joystick = 4;
	for (int i = 0; i < *num_joystick; i++)
	{
		struct didata* did = &di_joystick[i];
		did->name = std::string("Libretro Joystick ").append(std::to_string(i + 1));
		did->is_controller = true;
		did->joystick_id = i;
		did->buttons = 16;
		did->buttons_real = did->buttons;
		did->axles = 8;
		did->analogstick = did->axles >= 2;
		fill_default_controller(did->mapping);
		did->mapping.amiberry_custom_none.fill(0);
		did->mapping.amiberry_custom_hotkey.fill(0);
		did->mapping.amiberry_custom_axis_none.fill(0);
		did->mapping.amiberry_custom_axis_hotkey.fill(0);
		for (uae_s16 b = 0; b < did->buttons; b++) {
			did->buttonmappings[b] = b;
			did->buttonsort[b] = b;
			did->buttonaxisparent[b] = -1;
			did->buttonaxisparentdir[b] = 0;
			did->buttonaxistype[b] = AXISTYPE_NORMAL;
			did->buttonname[b] = std::string("Button ").append(std::to_string(b));
		}
		for (uae_s16 a = 0; a < did->axles; a++) {
			did->axismappings[a] = a;
			did->axissort[a] = a;
			did->axisname[a] = std::string("Axis ").append(std::to_string(a));
			if (a == SDL_CONTROLLER_AXIS_LEFTX || a == SDL_CONTROLLER_AXIS_RIGHTX)
				did->axistype[a] = AXISTYPE_POV_X;
			else if (a == SDL_CONTROLLER_AXIS_LEFTY || a == SDL_CONTROLLER_AXIS_RIGHTY)
				did->axistype[a] = AXISTYPE_POV_Y;
			else
				did->axistype[a] = AXISTYPE_NORMAL;
		}
	}
}
