#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>

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
	if (!read_file.is_open())
	{
		write_log("Controller Detection: Could not open config file: %s\n", retroarch_file.c_str());
		return -1; // File not found / not readable
	}

	std::string line;
	std::string delimiter = " = ";
	int button = -1;

	while (std::getline(read_file, line))
	{
		if (line.length() <= 1)
			continue;

		auto delimiter_pos = line.find(delimiter);
		auto option = line.substr(0, delimiter_pos);

		// If delimiter not found or not the setting we want, skip early
		if (option == line || option != find_setting)
			continue;

		// Safe extraction of parameter portion
		std::string param;
		if (delimiter_pos != std::string::npos)
			param = line.substr(delimiter_pos + delimiter.length());
		else
			continue; // Should not happen due to earlier continue, but guard anyway

		// Remove quotes
		param.erase(std::remove(param.begin(), param.end(), '"'), param.end());
		// Trim whitespace
		param.erase(0, param.find_first_not_of(" \t\r\n"));
		if (!param.empty())
		{
			const auto last_non_ws = param.find_last_not_of(" \t\r\n");
			if (last_non_ws != std::string::npos && last_non_ws + 1 < param.size())
				param.erase(last_non_ws + 1);
		}

		if (param.empty())
		{
			write_log("Controller Detection: %s has empty value, skipping\n", find_setting.c_str());
			continue; // Keep searching; maybe another occurrence exists (unlikely but safe)
		}

		// Special handling for hats: value starting with 'h'
		if (find_setting == "count_hats" && param[0] == 'h')
		{
			button = 1;
			break;
		}

		// Ignore hat-style entries for non-hat settings
		if (param[0] == 'h')
		{
			write_log("Controller Detection: %s value '%s' looks like hat descriptor, ignoring numeric parse\n", find_setting.c_str(), param.c_str());
			continue;
		}

		// Validate first character for integer parsing
		if (!(std::isdigit(static_cast<unsigned char>(param[0])) || param[0] == '-' || param[0] == '+'))
		{
			write_log("Controller Detection: %s value '%s' not numeric, skipping\n", find_setting.c_str(), param.c_str());
			continue;
		}

		try
		{
			int value = std::stoi(param); // May throw
			button = std::abs(value);
		}
		catch (const std::exception& e)
		{
			write_log("Controller Detection: %s invalid integer '%s' (%s)\n", find_setting.c_str(), param.c_str(), e.what());
			// Leave button as -1 and continue searching (though typically only one entry exists)
		}

		// If we successfully matched the setting, we can break
		if (option == find_setting)
			break;
	}

	write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
	return button;
}

bool find_retroarch_polarity(const std::string& find_setting, const std::string& retroarch_file)
{
	std::ifstream read_file(retroarch_file);
	if (!read_file.is_open())
		return false;
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

		std::string param;
		if (delimiter_pos != std::string::npos)
			param = line.substr(delimiter_pos + delimiter.length());
		else
			continue;

		param.erase(std::remove(param.begin(), param.end(), '"'), param.end());
		param.erase(0, param.find_first_not_of(" \t\r\n"));
		if (param.empty())
			continue;

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
	if (!read_file.is_open())
		return "nul"; // Fail fast if file can't be opened

	std::string line;
	const std::string delimiter = " = ";
	std::string output = "nul";

	std::string find_setting = find_setting_prefix;
	if (!suffix.empty())
		find_setting += std::to_string(player) + "_" + suffix;

	while (std::getline(read_file, line))
	{
		if (line.length() <= 1)
			continue;

		const auto delimiter_pos = line.find(delimiter);
		if (delimiter_pos == std::string::npos)
			continue; // No delimiter, skip

		const auto option = line.substr(0, delimiter_pos);
		if (option != find_setting)
			continue;

		// Extract parameter safely
		std::string param = line.substr(delimiter_pos + delimiter.length());
		// Remove quotes
		param.erase(std::remove(param.begin(), param.end(), '"'), param.end());
		// Trim leading/trailing whitespace
		param.erase(0, param.find_first_not_of(" \t\r\n"));
		if (!param.empty())
		{
			const auto last_non_ws = param.find_last_not_of(" \t\r\n");
			if (last_non_ws != std::string::npos && last_non_ws + 1 < param.size())
				param.erase(last_non_ws + 1);
		}

		if (param.empty())
		{
			write_log("Controller Detection: %s present but empty in config\n", find_setting.c_str());
			break; // We found the setting but it's empty; stop searching
		}

		output = param;
		break; // Found it
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
	if (index < 0 || index > 3)
	{
		write_log("Controller init_kb_from_retroarch: invalid index %d\n", index);
		return false;
	}
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
		if (x <= 0 || x >= static_cast<int>(remap_key_map_list_strings.size()))
			break;

		valid = true;
		if (idx < 9)
		{
			if (x < static_cast<int>(remap_key_map_list_strings.size()))
			{
				kbs[index][idx] = remap_key_map_list[x];
				kbs_3[index][idx] = remap_key_map_list[x];
				kbs_cd32[index][idx] = remap_key_map_list[x];
			}
		}
		else if (idx == 9 || idx == 11)
		{
			if (x < static_cast<int>(remap_key_map_list_strings.size()))
			{
				kbs[index][idx] = remap_key_map_list[x];
				kbs_3[index][idx + 1] = remap_key_map_list[x];
				kbs_cd32[index][idx + 1] = remap_key_map_list[x];
			}
		}
		else if (idx >= 13 && idx <= 23)
		{
			if (x < static_cast<int>(remap_key_map_list_strings.size()))
				kbs_cd32[index][idx + 1] = remap_key_map_list[x];
		}
		if (idx < 23) idx++;
	}
	
	// Added for keyboard ability to pull up the amiberry menu which most people want!
	if (index == 0)
	{
		key = find_retroarch_key("input_enable_hotkey", player, "", retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x > 0 && x < static_cast<int>(remap_key_map_list_strings.size()))
		{
			// Store hotkey enabling key if needed later (currently not assigned to a global)
			write_log("Controller Detection: hotkey key '%s' index %d\n", key.c_str(), x);
		}
		else if (!key.empty() && key != "nul")
			write_log("Controller Detection: invalid hotkey key '%s'\n", key.c_str());

		key = find_retroarch_key("input_exit_emulator", player, "", retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x > 0 && x < static_cast<int>(remap_key_map_list_strings.size()))
			quit_key.scancode = remap_key_map_list[x];
		else
			write_log("Controller Detection: exit emulator key '%s' not mapped (index %d)\n", key.c_str(), x);

		key = find_retroarch_key("input_menu_toggle", player, "", retroarch_file);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x > 0 && x < static_cast<int>(remap_key_map_list_strings.size()))
			enter_gui_key.scancode = remap_key_map_list[x];
		else
			write_log("Controller Detection: menu toggle key '%s' not mapped (index %d)\n", key.c_str(), x);
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
		if (retroarch_button_list[b].empty()) { mapping.button[b] = -1; continue; }
		mapping.button[b] = find_retroarch(ra_player_input(retroarch_button_list[b], player), control_config);
	}

	// RetroArch supports 6 axes
	for (int a = 0; a < 6; ++a)
	{
		if (retroarch_axis_list[a].empty()) { mapping.axis[a] = -1; continue; }
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
