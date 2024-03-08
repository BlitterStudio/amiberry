#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include <fstream>
#include <algorithm>

// Retroarch mapping reference: https://docs.libretro.com/guides/controller-autoconfiguration/

const std::string retroarch_kb_button_list[] = {
	"left", "right", "up", "down", "a", "b", "x", "y", "l", "r", "start"
};

const std::string retroarch_button_list[] = {
	"input_b_btn", "input_a_btn", "input_y_btn", "input_x_btn",
	"input_select_btn", "input_menu_toggle_btn",	"input_start_btn",
	"input_l3_btn", "input_r3_btn", "input_l_btn", "input_r_btn",
	"input_up_btn",	"input_down_btn", "input_left_btn", "input_right_btn",
	"", "", "", "", "", ""
};

const std::string retroarch_axis_list[] = {
	"input_l_x_plus_axis", "input_l_y_plus_axis", "input_r_x_plus_axis", "input_r_y_plus_axis",
	"input_l2_axis", "input_r2_axis"
};

const std::string controller_button_list[] = {
	"a", "b", "x", "y",
	"back", "guide", "start",
	"leftstick", "rightstick", "leftshoulder", "rightshoulder",
	"dpup", "dpdown",  "dpleft", "dpright",
	"", "", "", "", "", ""
};

const std::string controller_axis_list[] = {
	"leftx", "lefty", "rightx", "righty",
	"lefttrigger", "righttrigger"
};

int find_retroarch(const std::string& find_setting, std::string retroarch_file)
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
			if (find_setting == "count_hats")
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

				if (option == find_setting)
					break;
			}
		}
	}
	write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
	return button;
}

bool find_retroarch_polarity(const std::string& find_setting, std::string retroarch_file)
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
				if (option == find_setting)
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

const TCHAR* find_retroarch_key(const std::string& find_setting_prefix, int player, const TCHAR* suffix, std::string retroarch_file)
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

bool init_kb_from_retroarch(const int index, std::string retroarch_file)
{
	const auto player = index + 1;
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

std::string ra_player_input(std::string retroarch_string, const int player)
{
	if (player != -1 && retroarch_string.find("input_") == 0)
		retroarch_string.replace(0, 6, "input_player" + to_string(player) + "_");

	return retroarch_string;
}

host_input_button map_from_retroarch(host_input_button mapping, const std::string& control_config, const int player)
{
	mapping.is_retroarch = true;
	mapping.hotkey_button = find_retroarch("input_enable_hotkey_btn", control_config);
	mapping.quit_button = find_retroarch("input_exit_emulator_btn", control_config);
	mapping.reset_button = find_retroarch("input_reset_btn", control_config);
	mapping.menu_button = find_retroarch("input_menu_toggle_btn", control_config);
	mapping.vkbd_button = find_retroarch("input_osk_toggle_btn", control_config);

	// RetroArch supports 15 buttons
	for (auto b = 0; b < 15; b++)
	{
		mapping.button[b] = find_retroarch(ra_player_input(retroarch_button_list[b], player), control_config);
	}

	// RetroArch supports 6 axes
	for (auto a = 0; a < 6; a++)
	{
		mapping.axis[a] = find_retroarch(ra_player_input(retroarch_axis_list[a], player), control_config);
	}
	
	if (mapping.axis[SDL_CONTROLLER_AXIS_LEFTX] == -1)
		mapping.axis[SDL_CONTROLLER_AXIS_LEFTX] = find_retroarch(ra_player_input("input_right_axis", player), control_config);

	if (mapping.axis[SDL_CONTROLLER_AXIS_LEFTY] == -1)
		mapping.axis[SDL_CONTROLLER_AXIS_LEFTY] = find_retroarch(ra_player_input("input_down_axis", player), control_config);

	// hats
	mapping.number_of_hats = find_retroarch(ra_player_input("count_hats", player), control_config);
	
	// invert on axes
	mapping.lstick_axis_y_invert = find_retroarch_polarity(ra_player_input("input_down_axis", player), control_config);
	if (!mapping.lstick_axis_y_invert)
		mapping.lstick_axis_y_invert = find_retroarch_polarity(ra_player_input("input_l_y_plus_axis", player), control_config);

	mapping.lstick_axis_x_invert = find_retroarch_polarity(ra_player_input("input_right_axis", player), control_config);
	if (!mapping.lstick_axis_x_invert)
		mapping.lstick_axis_x_invert = find_retroarch_polarity(ra_player_input("input_l_x_plus_axis", player), control_config);

	mapping.rstick_axis_x_invert = find_retroarch_polarity(ra_player_input("input_r_x_plus_axis", player), control_config);
	mapping.rstick_axis_y_invert = find_retroarch_polarity(ra_player_input("input_r_y_plus_axis", player), control_config);

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
