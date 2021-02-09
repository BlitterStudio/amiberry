#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include <fstream>
#include <algorithm>

// Retroarch mapping reference: https://docs.libretro.com/guides/joypad-autoconfiguration/

const string retroarch_kb_button_list[] = {
	"left", "right", "up", "down", "a", "b", "x", "y", "l", "r", "start"
};

const string retroarch_button_list[] = {
	"input_b_btn", "input_a_btn", "input_y_btn", "input_x_btn",
	"input_select_btn", "input_menu_toggle_btn",	"input_start_btn",
	"input_l3_btn", "input_r3_btn", "input_l_btn", "input_r_btn",
	"input_up_btn",	"input_down_btn", "input_left_btn", "input_right_btn"
};

const string retroarch_axis_list[] = {
	"input_l_x_plus_axis", "input_l_y_plus_axis", "input_r_x_plus_axis", "input_r_y_plus_axis",
	"input_l2_axis", "input_r2_axis"
};

const string controller_button_list[] = {
	"a", "b", "x", "y",
	"back", "guide", "start",
	"leftstick", "rightstick", "leftshoulder", "rightshoulder",
	"dpup", "dpdown",  "dpleft", "dpright"
};

const string controller_axis_list[] = {
	"leftx", "lefty", "rightx", "righty",
	"lefttrigger", "righttrigger"
};

int find_retroarch(const TCHAR* find_setting, char* retroarch_file)
{
	// opening file and parsing
	std::ifstream read_file(retroarch_file);
	std::string line;
	std::string delimiter = " = ";
	auto button = -1;

	// read each line in
	while (std::getline(read_file, line))
	{
		if (line.length() > 1)
		{
			if (strncmp(find_setting, "count_hats", 10) == 0)
			{
				auto param = line.substr(line.find(delimiter) + delimiter.length(), line.length());
				// remove leading "
				if (param.at(0) == '"')
					param.erase(0, 1);
				// remove trailing "
				if (param.at(param.length() - 1) == '"')
					param.erase(param.length() - 1, 1);

				if (param.find('h') == 0)
				{
					button = 1;
					break;
				}
			}

			const auto option = line.substr(0, line.find(delimiter));
			// exit if we got no result from splitting the string
			if (option != line)
			{
				if (option != find_setting)
					continue;

				// using the " = " to work out which is the option, and which is the parameter.
				auto param = line.substr(line.find(delimiter) + delimiter.length(), line.length());

				// remove leading "
				if (param.at(0) == '"')
					param.erase(0, 1);

				// remove trailing "
				if (param.at(param.length() - 1) == '"')
					param.erase(param.length() - 1, 1);

				//  time to get the output number
				if (param.find('h') != 0) // check it isn't some kind of hat starting 'h' (so if D-pad uses buttons)
				{
					button = abs(atol(param.c_str()));
				}

				if (strcmp(option.c_str(), find_setting) == 0)
					break;
			}
		}
	}
	write_log("Controller Detection: %s : %d\n", find_setting, button);
	return button;
}

bool find_retroarch_polarity(const TCHAR* find_setting, char* retroarch_file)
{
	// opening file and parsing
	std::ifstream read_file(retroarch_file);
	std::string line;
	std::string delimiter = " = ";
	auto button = false;

	// read each line in
	while (std::getline(read_file, line))
	{
		if (line.length() > 1)
		{
			const auto option = line.substr(0, line.find(delimiter));

			if (option != line) // exit if we got no result from splitting the string
			{
				// using the " = " to work out which is the option, and which is the parameter.
				auto param = line.substr(line.find(delimiter) + delimiter.length(), line.length());

				// remove leading "
				if (param.at(0) == '"')
					param.erase(0, 1);

				// remove trailing "
				if (param.at(param.length() - 1) == '"')
					param.erase(param.length() - 1, 1);

				// ok, this is the 'normal' storing of values
				if (strcmp(option.c_str(), find_setting) == 0)
				{
					//  time to get the output value
					if (param.at(0) == '-')
					{
						button = true;
					}
					break;
				}
			}
		}
	}
	return button;
}

const TCHAR* find_retroarch_key(const TCHAR* find_setting_prefix, int player, const TCHAR* suffix, char* retroarch_file)
{
	// opening file and parsing
	std::ifstream read_file(retroarch_file);
	std::string line;
	std::string delimiter = " = ";
	const auto* output = "nul";

	std::string find_setting = find_setting_prefix;
	if (suffix != nullptr)
	{
		// add player and suffix!
		char buffer[10];
		sprintf(buffer, "%i_", player);
		find_setting += buffer;
		find_setting += suffix;
	}

	// read each line in
	while (std::getline(read_file, line))
	{
		if (line.length() > 1)
		{
			const auto option = line.substr(0, line.find(delimiter));

			if (option != line) // exit if we got no result from splitting the string
			{
				// using the " = " to work out which is the option, and which is the parameter.
				auto param = line.substr(line.find(delimiter) + delimiter.length(), line.length());

				if (!param.empty())
				{
					// remove leading "
					if (param.at(0) == '"')
						param.erase(0, 1);

					// remove trailing "
					if (param.at(param.length() - 1) == '"')
						param.erase(param.length() - 1, 1);

					output = &param[0U];

					// ok, this is the 'normal' storing of values
					if (option == find_setting)
						break;

					output = "nul";
				}
			}
		}
	}
	return output;
}

int find_string_in_array(const char* arr[], const int n, const char* key)
{
	auto index = -1;

	for (auto i = 0; i < n; i++)
	{
		if (!_tcsicmp(arr[i], key))
		{
			index = i;
			break;
		}
	}
	return index;
}

std::string sanitize_retroarch_name(std::string s)
{
	char illegal_chars[] = "\\/:?\"<>|";

	for (unsigned int i = 0; i < strlen(illegal_chars); ++i)
	{
		s.erase(remove(s.begin(), s.end(), illegal_chars[i]), s.end());
	}
	return s;
}

bool init_kb_from_retroarch(int index, char* retroarch_file)
{
	auto player = index + 1;
	const TCHAR* key;
	int x;
	int* kbs[] = { kb_retroarch_player1, kb_retroarch_player2, kb_retroarch_player3, kb_retroarch_player4 };
	int* kbs_3[] = { kb_retroarch_player1_3, kb_retroarch_player2_3, kb_retroarch_player3_3, kb_retroarch_player4_3 };
	int* kbs_cd32[] = { kb_cd32_retroarch_player1, kb_cd32_retroarch_player2, kb_cd32_retroarch_player3, kb_cd32_retroarch_player4 };
	auto idx = 0;
	auto valid = false;
	
	for (auto i = 0; i < 11; i++)
	{
		key = find_retroarch_key("input_player", player, retroarch_kb_button_list[i].c_str(), retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, key);
		if (x == -1 || x == 0) break;

		valid = true;
		if (idx < 9)
		{
			kbs[index][idx] = remap_key_map_list[x];
			kbs_3[index][idx] = remap_key_map_list[x];
			kbs_cd32[index][idx] = remap_key_map_list[x];
			if (idx < 8) idx++;
		}
		else if (idx == 9 || idx == 11)
		{
			kbs[index][idx] = remap_key_map_list[x];
			kbs_3[index][idx + 1] = remap_key_map_list[x];
			kbs_cd32[index][idx + 1] = remap_key_map_list[x];
			idx++;
		}
		else if (idx >= 13 && idx <= 23)
		{
			kbs_cd32[index][idx + 1] = remap_key_map_list[x];
			idx++;
		}
		if (idx < 23) idx++;
	}
	
	// Added for keyboard ability to pull up the amiberry menu which most people want!
	if (index == 0)
	{
		key = find_retroarch_key("input_enable_hotkey", player, nullptr, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, key);
		//temp_keyboard_buttons.hotkey_button = remap_key_map_list[x];

		key = find_retroarch_key("input_exit_emulator", player, nullptr, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, key);
		//temp_keyboard_buttons.quit_button = remap_key_map_list[x];
		quit_key.scancode = remap_key_map_list[x];

		key = find_retroarch_key("input_menu_toggle", player, nullptr, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, key);
		//temp_keyboard_buttons.menu_button = remap_key_map_list[x];
		enter_gui_key.scancode = remap_key_map_list[x];
	}

	write_log("Controller init_kb_from_retroarch(%i): %s \n", index, valid ? "Found" : "Not found");
	return valid;
}

host_input_button map_from_retroarch(host_input_button mapping, char* control_config)
{
	mapping.is_retroarch = true;
	mapping.hotkey_button = find_retroarch("input_enable_hotkey_btn", control_config);
	mapping.quit_button = find_retroarch("input_exit_emulator_btn", control_config);
	mapping.reset_button = find_retroarch("input_reset_btn", control_config);

	for (auto b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
	{
		mapping.button[b] = find_retroarch(retroarch_button_list[b].c_str(), control_config);
	}

	for (auto a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++)
	{
		mapping.axis[a] = find_retroarch(retroarch_axis_list[a].c_str(), control_config);
	}
	
	if (mapping.axis[SDL_CONTROLLER_AXIS_LEFTX] == -1)
		mapping.axis[SDL_CONTROLLER_AXIS_LEFTX] = find_retroarch("input_right_axis", control_config);

	if (mapping.axis[SDL_CONTROLLER_AXIS_LEFTY] == -1)
		mapping.axis[SDL_CONTROLLER_AXIS_LEFTY] = find_retroarch("input_down_axis", control_config);

	// hats
	mapping.number_of_hats = find_retroarch("count_hats", control_config);
	
	// invert on axes
	mapping.lstick_axis_y_invert = find_retroarch_polarity("input_down_axis", control_config);
	if (!mapping.lstick_axis_y_invert)
		mapping.lstick_axis_y_invert = find_retroarch_polarity("input_l_y_plus_axis", control_config);

	mapping.lstick_axis_x_invert = find_retroarch_polarity("input_right_axis", control_config);
	if (!mapping.lstick_axis_x_invert)
		mapping.lstick_axis_x_invert = find_retroarch_polarity("input_l_x_plus_axis", control_config);

	mapping.rstick_axis_x_invert = find_retroarch_polarity("input_r_x_plus_axis", control_config);
	mapping.rstick_axis_y_invert = find_retroarch_polarity("input_r_y_plus_axis", control_config);

	write_log("Controller Detection: invert left  y axis: %d\n",
		mapping.lstick_axis_y_invert);
	write_log("Controller Detection: invert left  x axis: %d\n",
		mapping.lstick_axis_x_invert);
	write_log("Controller Detection: invert right y axis: %d\n",
		mapping.rstick_axis_y_invert);
	write_log("Controller Detection: invert right x axis: %d\n",
		mapping.rstick_axis_x_invert);

	return mapping;
}

std::string binding_from_retroarch(host_input_button mapping, char* control_config)
{
	string result;
	
	mapping.is_retroarch = true;
	mapping.hotkey_button = find_retroarch("input_enable_hotkey_btn", control_config);
	mapping.quit_button = find_retroarch("input_exit_emulator_btn", control_config);
	mapping.reset_button = find_retroarch("input_reset_btn", control_config);

	for (auto button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
	{
		//TODO fix hat detection!
		const auto retroarch_button = find_retroarch(retroarch_button_list[button].c_str(), control_config);
		if (retroarch_button != -1)
		{
			result += ",";
			result += controller_button_list[button];
			result += ":b";
			result += to_string(retroarch_button);
		}
	}
	for (auto axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		const auto retroarch_axis = find_retroarch(retroarch_axis_list[axis].c_str(), control_config);
		if (retroarch_axis != -1)
		{
			result += ",";
			result += controller_axis_list[axis];
			result += ":b";
			result += to_string(retroarch_axis);
		}
	}

	return result;
}