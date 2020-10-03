#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include <fstream>
#include <algorithm>

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
	struct host_keyboard_button temp_keyboard_buttons{};
	auto player = index + 1;

	const auto* tempkey = find_retroarch_key("input_player", player, "y", retroarch_file);
	auto x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.north_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "a", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.east_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "b", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.south_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "x", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.west_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "left", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.dpad_left = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "right", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.dpad_right = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "up", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.dpad_up = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "down", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.dpad_down = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "l", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.left_shoulder = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "r", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.right_shoulder = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "select", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.select_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "start", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.start_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "l2", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.lstick_button = remap_key_map_list[x];

	tempkey = find_retroarch_key("input_player", player, "r2", retroarch_file);
	x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
	temp_keyboard_buttons.rstick_button = remap_key_map_list[x];

	// Added for keyboard ability to pull up the amiberry menu which most people want!
	if (index == 0)
	{
		tempkey = find_retroarch_key("input_enable_hotkey", player, nullptr, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
		temp_keyboard_buttons.hotkey_button = remap_key_map_list[x];

		tempkey = find_retroarch_key("input_exit_emulator", player, nullptr, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
		temp_keyboard_buttons.quit_button = remap_key_map_list[x];

		tempkey = find_retroarch_key("input_menu_toggle", player, nullptr, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, remap_key_map_list_size, tempkey);
		temp_keyboard_buttons.menu_button = remap_key_map_list[x];
	}
	else
	{
		temp_keyboard_buttons.hotkey_button = -1;
		temp_keyboard_buttons.quit_button = -1;
		temp_keyboard_buttons.menu_button = -1;
	}

	// only accept this IF it has been setup, exit once one is missing!
	// basically if one is set, assume it is setup for a player
	const auto valid = temp_keyboard_buttons.north_button != -1
		|| temp_keyboard_buttons.east_button != -1
		|| temp_keyboard_buttons.south_button != -1
		|| temp_keyboard_buttons.west_button != -1
		|| temp_keyboard_buttons.dpad_left != -1
		|| temp_keyboard_buttons.dpad_right != -1
		|| temp_keyboard_buttons.dpad_up != -1
		|| temp_keyboard_buttons.dpad_down != -1
		|| temp_keyboard_buttons.left_shoulder != -1
		|| temp_keyboard_buttons.right_shoulder != -1
		|| temp_keyboard_buttons.select_button != -1
		|| temp_keyboard_buttons.start_button != -1
		|| temp_keyboard_buttons.lstick_button != -1
		|| temp_keyboard_buttons.rstick_button != -1;

	if (valid)
	{
		//
		temp_keyboard_buttons.is_retroarch = true;

		// set it!
		host_keyboard_buttons[index] = temp_keyboard_buttons;

		// and max!
		num_keys_as_joys = player > num_keys_as_joys ? player : num_keys_as_joys;
	}

	write_log("Controller init_kb_from_retroarch(%i): %s \n", index, valid ? "Found" : "Not found");
	return valid;
}

bool key_used_by_retroarch_joy(int scancode)
{
	auto key_used = false;
	if (amiberry_options.input_keyboard_as_joystick_stop_keypresses)
	{
		//currprefs.jports[port]
		for (auto joy_id = 0; joy_id < MAX_JPORTS && !key_used; joy_id++)
		{
			// First handle retroarch (or default) keys as Joystick...
			if (currprefs.jports[joy_id].id >= JSEM_JOYS && currprefs.jports[joy_id].id < JSEM_JOYS +
				num_keys_as_joys)
			{
				const auto host_key_id = currprefs.jports[joy_id].id - JSEM_JOYS;
				const auto kb = host_key_id;
				key_used = host_keyboard_buttons[kb].south_button == scancode
					|| host_keyboard_buttons[kb].east_button == scancode
					|| host_keyboard_buttons[kb].north_button == scancode
					|| host_keyboard_buttons[kb].west_button == scancode
					|| host_keyboard_buttons[kb].left_shoulder == scancode
					|| host_keyboard_buttons[kb].right_shoulder == scancode
					|| host_keyboard_buttons[kb].start_button == scancode
					|| host_keyboard_buttons[kb].dpad_up == scancode
					|| host_keyboard_buttons[kb].dpad_down == scancode
					|| host_keyboard_buttons[kb].dpad_left == scancode
					|| host_keyboard_buttons[kb].dpad_right == scancode
					|| host_keyboard_buttons[kb].lstick_button == scancode
					|| host_keyboard_buttons[kb].rstick_button == scancode
					|| host_keyboard_buttons[kb].select_button == scancode;
			}
		}
	}
	return key_used;
}

void map_from_retroarch(int cpt, char* control_config)
{
	host_input_buttons[cpt].is_retroarch = true;

	host_input_buttons[cpt].hotkey_button = find_retroarch("input_enable_hotkey_btn", control_config);

	host_input_buttons[cpt].quit_button = find_retroarch("input_exit_emulator_btn", control_config);
	host_input_buttons[cpt].menu_button = find_retroarch("input_menu_toggle_btn", control_config);
	host_input_buttons[cpt].reset_button = find_retroarch("input_reset_btn", control_config);

	host_input_buttons[cpt].east_button = find_retroarch("input_a_btn", control_config);
	host_input_buttons[cpt].south_button = find_retroarch("input_b_btn", control_config);
	host_input_buttons[cpt].north_button = find_retroarch("input_x_btn", control_config);
	host_input_buttons[cpt].west_button = find_retroarch("input_y_btn", control_config);

	host_input_buttons[cpt].dpad_left = find_retroarch("input_left_btn", control_config);
	host_input_buttons[cpt].dpad_right = find_retroarch("input_right_btn", control_config);
	host_input_buttons[cpt].dpad_up = find_retroarch("input_up_btn", control_config);
	host_input_buttons[cpt].dpad_down = find_retroarch("input_down_btn", control_config);

	host_input_buttons[cpt].select_button = find_retroarch("input_select_btn", control_config);
	host_input_buttons[cpt].start_button = find_retroarch("input_start_btn", control_config);

	host_input_buttons[cpt].left_shoulder = find_retroarch("input_l_btn", control_config);
	host_input_buttons[cpt].right_shoulder = find_retroarch("input_r_btn", control_config);
	host_input_buttons[cpt].left_trigger = find_retroarch("input_l2_btn", control_config);
	host_input_buttons[cpt].right_trigger = find_retroarch("input_r2_btn", control_config);
	host_input_buttons[cpt].lstick_button = find_retroarch("input_l3_btn", control_config);
	host_input_buttons[cpt].rstick_button = find_retroarch("input_r3_btn", control_config);

	host_input_buttons[cpt].lstick_axis_x = find_retroarch("input_l_x_plus_axis", control_config);
	if (host_input_buttons[cpt].lstick_axis_x == -1)
		host_input_buttons[cpt].lstick_axis_x = find_retroarch("input_right_axis", control_config);

	host_input_buttons[cpt].lstick_axis_y = find_retroarch("input_l_y_plus_axis", control_config);
	if (host_input_buttons[cpt].lstick_axis_y == -1)
		host_input_buttons[cpt].lstick_axis_y = find_retroarch("input_down_axis", control_config);

	host_input_buttons[cpt].rstick_axis_x = find_retroarch("input_r_x_plus_axis", control_config);
	host_input_buttons[cpt].rstick_axis_y = find_retroarch("input_r_y_plus_axis", control_config);

	// specials
	// hats
	host_input_buttons[cpt].number_of_hats = find_retroarch("count_hats", control_config);

	// invert on axes
	host_input_buttons[cpt].lstick_axis_y_invert = find_retroarch_polarity(
		"input_down_axis", control_config);
	if (!host_input_buttons[cpt].lstick_axis_y_invert)
		host_input_buttons[cpt].lstick_axis_y_invert = find_retroarch_polarity(
			"input_l_y_plus_axis", control_config);

	host_input_buttons[cpt].lstick_axis_x_invert = find_retroarch_polarity(
		"input_right_axis", control_config);
	if (!host_input_buttons[cpt].lstick_axis_x_invert)

		host_input_buttons[cpt].lstick_axis_x_invert = find_retroarch_polarity(
			"input_l_x_plus_axis", control_config);

	host_input_buttons[cpt].rstick_axis_x_invert = find_retroarch_polarity(
		"input_r_x_plus_axis", control_config);
	host_input_buttons[cpt].rstick_axis_y_invert = find_retroarch_polarity(
		"input_r_y_plus_axis", control_config);

	write_log("Controller Detection: invert left  y axis: %d\n",
	          host_input_buttons[cpt].lstick_axis_y_invert);
	write_log("Controller Detection: invert left  x axis: %d\n",
	          host_input_buttons[cpt].lstick_axis_x_invert);
	write_log("Controller Detection: invert right y axis: %d\n",
	          host_input_buttons[cpt].rstick_axis_y_invert);
	write_log("Controller Detection: invert right x axis: %d\n",
	          host_input_buttons[cpt].rstick_axis_x_invert);
}
