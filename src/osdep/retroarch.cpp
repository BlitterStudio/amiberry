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

int find_retroarch(const std::string& find_setting, const std::string& retroarch_file)
{
	std::ifstream read_file(retroarch_file);
	std::string line;
	std::string delimiter = " = ";
	int button = -1;

	while (std::getline(read_file, line))
	{
		if (line.length() <= 1)
			continue;

		auto delimiter_pos = line.find(delimiter);
		auto option = line.substr(0, delimiter_pos);

		if (option == line || option != find_setting)
			continue;

		auto param = line.substr(delimiter_pos + delimiter.length());
		param.erase(std::remove(param.begin(), param.end(), '"'), param.end());

		if (find_setting == "count_hats" && param[0] == 'h')
		{
			button = 1;
			break;
		}

		if (param[0] != 'h') // check it isn't some kind of hat starting 'h' (so if D-pad uses buttons)
		{
			button = abs(std::stoi(param));
		}

		if (option == find_setting)
			break;
	}

	write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
	return button;
}

bool find_retroarch_polarity(const std::string& find_setting, const std::string& retroarch_file)
{
	std::ifstream read_file(retroarch_file);
	std::string line;
	std::string delimiter = " = ";
	bool button = false;

	while (std::getline(read_file, line))
	{
		if (line.length() <= 1)
			continue;

		auto delimiter_pos = line.find(delimiter);
		auto option = line.substr(0, delimiter_pos);

		if (option == line || option != find_setting)
			continue;

		auto param = line.substr(delimiter_pos + delimiter.length());
		param.erase(std::remove(param.begin(), param.end(), '"'), param.end());

		if (param[0] == '-')
		{
			button = true;
			break;
		}
	}

	return button;
}

std::string find_retroarch_key(const std::string& find_setting_prefix, int player, const std::string& suffix, const std::string& retroarch_file)
{
	std::ifstream read_file(retroarch_file);
	std::string line;
	std::string delimiter = " = ";
	std::string output = "nul";

	std::string find_setting = find_setting_prefix;
	if (!suffix.empty())
	{
		find_setting += std::to_string(player) + "_" + suffix;
	}

	while (std::getline(read_file, line))
	{
		if (line.length() <= 1)
			continue;

		auto delimiter_pos = line.find(delimiter);
		auto option = line.substr(0, delimiter_pos);

		if (option == line || option != find_setting)
			continue;

		auto param = line.substr(delimiter_pos + delimiter.length());
		param.erase(std::remove(param.begin(), param.end(), '"'), param.end());

		output = param;
		break;
	}

	return output;
}

int find_string_in_array(const std::vector<std::string>& arr, const std::string& key)
{
	const auto it = std::find_if(arr.begin(), arr.end(),
	                             [&key](const std::string& str) { return strcasecmp(str.c_str(), key.c_str()) == 0; });

	return (it != arr.end()) ? static_cast<int>(std::distance(arr.begin(), it)) : -1;
}

std::string sanitize_retroarch_name(std::string s)
{
	const std::string illegal_chars = "\\/:?\"<>|";
	for (const auto& illegal_char : illegal_chars)
	{
		s.erase(std::remove(s.begin(), s.end(), illegal_char), s.end());
	}
	return s;
}

bool init_kb_from_retroarch(const int index, const std::string& retroarch_file)
{
	const auto player = index + 1;
	std::string key;
	int x;
	int* kbs[] = { kb_retroarch_player1, kb_retroarch_player2, kb_retroarch_player3, kb_retroarch_player4 };
	int* kbs_3[] = { kb_retroarch_player1_3, kb_retroarch_player2_3, kb_retroarch_player3_3, kb_retroarch_player4_3 };
	int* kbs_cd32[] = { kb_cd32_retroarch_player1, kb_cd32_retroarch_player2, kb_cd32_retroarch_player3, kb_cd32_retroarch_player4 };
	auto idx = 0;
	auto valid = false;
	
	for (const auto & i : retroarch_kb_button_list)
	{
		key = find_retroarch_key("input_player", player, i, retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x == -1 || x == 0) break;

		valid = true;
		if (idx < 9)
		{
			kbs[index][idx] = remap_key_map_list[x];
			kbs_3[index][idx] = remap_key_map_list[x];
			kbs_cd32[index][idx] = remap_key_map_list[x];
		}
		else if (idx == 9 || idx == 11)
		{
			kbs[index][idx] = remap_key_map_list[x];
			kbs_3[index][idx + 1] = remap_key_map_list[x];
			kbs_cd32[index][idx + 1] = remap_key_map_list[x];
		}
		else if (idx >= 13 && idx <= 23)
		{
			kbs_cd32[index][idx + 1] = remap_key_map_list[x];
		}
		if (idx < 23) idx++;
	}
	
	// Added for keyboard ability to pull up the amiberry menu which most people want!
	if (index == 0)
	{
		key = find_retroarch_key("input_enable_hotkey", player, "", retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);

		key = find_retroarch_key("input_exit_emulator", player, "", retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);
		quit_key.scancode = remap_key_map_list[x];

		key = find_retroarch_key("input_menu_toggle", player, "", retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);
		enter_gui_key.scancode = remap_key_map_list[x];
	}

	write_log("Controller init_kb_from_retroarch(%i): %s \n", index, valid ? "Found" : "Not found");
	return valid;
}

std::string ra_player_input(std::string retroarch_string, const int player)
{
	const std::string input_prefix = "input_";
	if (player != -1 && retroarch_string.find(input_prefix) == 0)
	{
		const std::string new_prefix = "input_player" + std::to_string(player) + "_";
		retroarch_string.replace(0, input_prefix.length(), new_prefix);
	}

	return retroarch_string;
}

void map_from_retroarch(controller_mapping& mapping, const std::string& control_config, const int player)
{
	mapping.is_retroarch = true;
	const std::string button_keys[] = { "input_enable_hotkey_btn", "input_exit_emulator_btn", "input_reset_btn", "input_menu_toggle_btn", "input_osk_toggle_btn" };
	int* button_mapping[] = { &mapping.hotkey_button, &mapping.quit_button, &mapping.reset_button, &mapping.menu_button, &mapping.vkbd_button };

	for (int i = 0; i < 5; ++i)
	{
		*button_mapping[i] = find_retroarch(button_keys[i], control_config);
	}

	// RetroArch supports 15 buttons
	for (int b = 0; b < 15; ++b)
	{
		mapping.button[b] = find_retroarch(ra_player_input(retroarch_button_list[b], player), control_config);
	}

	// RetroArch supports 6 axes
	for (int a = 0; a < 6; ++a)
	{
		mapping.axis[a] = find_retroarch(ra_player_input(retroarch_axis_list[a], player), control_config);
	}

	// If axes are not found, try alternative keys
	if (mapping.axis[SDL_CONTROLLER_AXIS_LEFTX] == -1)
		mapping.axis[SDL_CONTROLLER_AXIS_LEFTX] = find_retroarch(ra_player_input("input_right_axis", player), control_config);

	if (mapping.axis[SDL_CONTROLLER_AXIS_LEFTY] == -1)
		mapping.axis[SDL_CONTROLLER_AXIS_LEFTY] = find_retroarch(ra_player_input("input_down_axis", player), control_config);

	// hats
	mapping.number_of_hats = find_retroarch(ra_player_input("count_hats", player), control_config);

	// invert on axes
	const std::string axis_keys[] = { "input_down_axis", "input_l_y_plus_axis", "input_right_axis", "input_l_x_plus_axis", "input_r_x_plus_axis", "input_r_y_plus_axis" };
	bool* axis_mapping[] = { &mapping.lstick_axis_y_invert, &mapping.lstick_axis_y_invert, &mapping.lstick_axis_x_invert, &mapping.lstick_axis_x_invert, &mapping.rstick_axis_x_invert, &mapping.rstick_axis_y_invert };

	for (int i = 0; i < 6; ++i)
	{
		if (!*axis_mapping[i])
			*axis_mapping[i] = find_retroarch_polarity(ra_player_input(axis_keys[i], player), control_config);
	}

	write_log("Controller Detection: invert left  y axis: %d\n", mapping.lstick_axis_y_invert);
	write_log("Controller Detection: invert left  x axis: %d\n", mapping.lstick_axis_x_invert);
	write_log("Controller Detection: invert right y axis: %d\n", mapping.rstick_axis_y_invert);
	write_log("Controller Detection: invert right x axis: %d\n", mapping.rstick_axis_x_invert);
}
