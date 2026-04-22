#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <unordered_map>
#ifndef LIBRETRO
#include <fstream>
#endif
#include <algorithm>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

// Retroarch mapping reference: https://docs.libretro.com/guides/controller-autoconfiguration/

static const char* const retroarch_kb_button_list[] = {
	"left", "right", "up", "down", "a", "b", "x", "y", "l", "r", "start"
};

static const char* const retroarch_button_list[] = {
	"input_b_btn", "input_a_btn", "input_y_btn", "input_x_btn",
	"input_select_btn", "input_menu_toggle_btn",	"input_start_btn",
	"input_l3_btn", "input_r3_btn", "input_l_btn", "input_r_btn",
	"input_up_btn",	"input_down_btn", "input_left_btn", "input_right_btn"
};

static const char* const retroarch_axis_list[] = {
	"input_l_x_plus_axis", "input_l_y_plus_axis", "input_r_x_plus_axis", "input_r_y_plus_axis",
	"input_l2_axis", "input_r2_axis"
};

// Platform-agnostic line reader: FILE* for libretro (no <fstream>), ifstream for host
class config_reader
{
public:
	explicit config_reader(const std::string& path)
	{
#ifdef LIBRETRO
		fp_ = fopen(path.c_str(), "rb");
#else
		ifs_.open(path);
#endif
	}
	~config_reader()
	{
#ifdef LIBRETRO
		if (fp_) fclose(fp_);
#endif
	}
	config_reader(const config_reader&) = delete;
	config_reader& operator=(const config_reader&) = delete;

	bool is_open() const
	{
#ifdef LIBRETRO
		return fp_ != nullptr;
#else
		return ifs_.is_open();
#endif
	}

	bool getline(std::string& line)
	{
#ifdef LIBRETRO
		char buf[1024];
		if (!fgets(buf, sizeof(buf), fp_))
			return false;
		line = buf;
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
			line.pop_back();
		return true;
#else
		return static_cast<bool>(std::getline(ifs_, line));
#endif
	}

private:
#ifdef LIBRETRO
	FILE* fp_ = nullptr;
#else
	std::ifstream ifs_;
#endif
};

using config_map = std::unordered_map<std::string, std::string>;

static config_map parse_retroarch_config(const std::string& retroarch_file)
{
	config_map result;
	config_reader read_file(retroarch_file);
	if (!read_file.is_open())
	{
		write_log("Controller Detection: Could not open config file: %s\n", retroarch_file.c_str());
		return result;
	}

	std::string line;
	const std::string delimiter = " = ";

	while (read_file.getline(line))
	{
		if (line.length() <= 1)
			continue;

		const auto delimiter_pos = line.find(delimiter);
		if (delimiter_pos == std::string::npos)
			continue;

		auto key = line.substr(0, delimiter_pos);
		auto value = line.substr(delimiter_pos + delimiter.length());

		// Remove quotes
		value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
		// Trim whitespace
		value.erase(0, value.find_first_not_of(" \t\r\n"));
		if (!value.empty())
		{
			const auto last_non_ws = value.find_last_not_of(" \t\r\n");
			if (last_non_ws != std::string::npos && last_non_ws + 1 < value.size())
				value.erase(last_non_ws + 1);
		}

		if (!value.empty())
			result[key] = value;
	}

	return result;
}

static int lookup_retroarch_int(const std::string& find_setting, const config_map& config)
{
	int button = -1;
	const auto it = config.find(find_setting);
	if (it == config.end())
	{
		write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
		return button;
	}

	const auto& param = it->second;

	// Special handling for hats: value starting with 'h'
	if (find_setting == "count_hats" && param[0] == 'h')
	{
		button = 1;
		write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
		return button;
	}

	// Ignore hat-style entries for non-hat settings
	if (param[0] == 'h')
	{
		write_log("Controller Detection: %s value '%s' looks like hat descriptor, ignoring numeric parse\n",
		          find_setting.c_str(), param.c_str());
		write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
		return button;
	}

	// Validate first character for integer parsing
	if (!(std::isdigit(static_cast<unsigned char>(param[0])) || param[0] == '-' || param[0] == '+'))
	{
		write_log("Controller Detection: %s value '%s' not numeric, skipping\n", find_setting.c_str(), param.c_str());
		write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
		return button;
	}

	try
	{
		const int value = std::stoi(param);
		// Guard against std::abs(INT_MIN) which is undefined behavior
		if (value != INT_MIN)
			button = std::abs(value);
		else
			write_log("Controller Detection: %s value overflow, ignoring\n", find_setting.c_str());
	}
	catch (const std::exception& e)
	{
		write_log("Controller Detection: %s invalid integer '%s' (%s)\n", find_setting.c_str(), param.c_str(), e.what());
	}

	write_log("Controller Detection: %s : %d\n", find_setting.c_str(), button);
	return button;
}

static bool lookup_retroarch_polarity(const std::string& find_setting, const config_map& config)
{
	const auto it = config.find(find_setting);
	if (it == config.end())
		return false;

	return !it->second.empty() && it->second[0] == '-';
}

static std::string lookup_retroarch_key(const std::string& find_setting_prefix, int player,
                                        const std::string& suffix, const config_map& config)
{
	std::string find_setting = find_setting_prefix;
	if (!suffix.empty())
		find_setting += std::to_string(player) + "_" + suffix;

	const auto it = config.find(find_setting);
	if (it == config.end())
		return "nul";

	return it->second;
}

int find_retroarch(const std::string& find_setting, const std::string& retroarch_file)
{
	const auto config = parse_retroarch_config(retroarch_file);
	return lookup_retroarch_int(find_setting, config);
}

static int find_string_in_array(const std::vector<std::string>& arr, const std::string& key)
{
	const auto it = std::find_if(arr.begin(), arr.end(),
	                             [&key](const std::string& str) { return strcasecmp(str.c_str(), key.c_str()) == 0; });

	return (it != arr.end()) ? static_cast<int>(std::distance(arr.begin(), it)) : -1;
}

std::string sanitize_retroarch_name(std::string s)
{
	s.erase(std::remove_if(s.begin(), s.end(),
		[](char c) { return std::strchr("\\/:?\"<>|", c) != nullptr; }), s.end());
	return s;
}

bool init_kb_from_retroarch(const int index, const std::string& retroarch_file)
{
	if (index < 0 || index > 3)
	{
		write_log("Controller init_kb_from_retroarch: invalid index %d\n", index);
		return false;
	}

	const auto config = parse_retroarch_config(retroarch_file);
	if (config.empty())
		return false;

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
		key = lookup_retroarch_key("input_player", player, i, config);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x <= 0 || x >= static_cast<int>(remap_key_map_list_strings.size()))
			break;

		valid = true;
		if (idx < 9)
		{
			kbs[index][idx] = remap_key_map_list[x];
			kbs_3[index][idx] = remap_key_map_list[x];
			kbs_cd32[index][idx] = remap_key_map_list[x];
		}
		else
		{
			// idx >= 9: shoulder buttons and start use a +1 offset in
			// the 3-button and CD32 arrays (extra slot at position 9).
			kbs[index][idx] = remap_key_map_list[x];
			kbs_3[index][idx + 1] = remap_key_map_list[x];
			kbs_cd32[index][idx + 1] = remap_key_map_list[x];
		}
		if (idx < 23) idx++;
	}

	// Added for keyboard ability to pull up the amiberry menu which most people want!
	if (index == 0)
	{
		key = lookup_retroarch_key("input_enable_hotkey", player, "", config);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x > 0 && x < static_cast<int>(remap_key_map_list_strings.size()))
		{
			// Store hotkey enabling key if needed later (currently not assigned to a global)
			write_log("Controller Detection: hotkey key '%s' index %d\n", key.c_str(), x);
		}
		else if (!key.empty() && key != "nul")
			write_log("Controller Detection: invalid hotkey key '%s'\n", key.c_str());

		key = lookup_retroarch_key("input_exit_emulator", player, "", config);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x > 0 && x < static_cast<int>(remap_key_map_list_strings.size()))
			quit_key.scancode = remap_key_map_list[x];
		else
			write_log("Controller Detection: exit emulator key '%s' not mapped (index %d)\n", key.c_str(), x);

		key = lookup_retroarch_key("input_menu_toggle", player, "", config);
		x = find_string_in_array(remap_key_map_list_strings, key);
		if (x > 0 && x < static_cast<int>(remap_key_map_list_strings.size()))
			enter_gui_key.scancode = remap_key_map_list[x];
		else
			write_log("Controller Detection: menu toggle key '%s' not mapped (index %d)\n", key.c_str(), x);
	}

	write_log("Controller init_kb_from_retroarch(%i): %s \n", index, valid ? "Found" : "Not found");
	return valid;
}

static std::string ra_player_input(std::string retroarch_string, const int player)
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
	const auto config = parse_retroarch_config(control_config);

	mapping.is_retroarch = true;
	const std::string button_keys[] = { "input_enable_hotkey_btn", "input_exit_emulator_btn", "input_reset_btn", "input_menu_toggle_btn", "input_osk_toggle_btn" };
	int* button_mapping[] = { &mapping.hotkey_button, &mapping.quit_button, &mapping.reset_button, &mapping.menu_button, &mapping.vkbd_button };

	for (size_t i = 0; i < std::size(button_keys); ++i)
	{
		*button_mapping[i] = lookup_retroarch_int(button_keys[i], config);
	}

	for (size_t b = 0; b < std::size(retroarch_button_list); ++b)
	{
		if (retroarch_button_list[b][0] == '\0') { mapping.button[b] = -1; continue; }
		mapping.button[b] = lookup_retroarch_int(ra_player_input(retroarch_button_list[b], player), config);
	}

	for (size_t a = 0; a < std::size(retroarch_axis_list); ++a)
	{
		if (retroarch_axis_list[a][0] == '\0') { mapping.axis[a] = -1; continue; }
		mapping.axis[a] = lookup_retroarch_int(ra_player_input(retroarch_axis_list[a], player), config);
	}

	// If axes are not found, try alternative keys
	if (mapping.axis[SDL_GAMEPAD_AXIS_LEFTX] == -1)
		mapping.axis[SDL_GAMEPAD_AXIS_LEFTX] = lookup_retroarch_int(ra_player_input("input_right_axis", player), config);

	if (mapping.axis[SDL_GAMEPAD_AXIS_LEFTY] == -1)
		mapping.axis[SDL_GAMEPAD_AXIS_LEFTY] = lookup_retroarch_int(ra_player_input("input_down_axis", player), config);

	// hats
	mapping.number_of_hats = lookup_retroarch_int(ra_player_input("count_hats", player), config);

	// Axis inversion: two axis keys map to the same invert flag (e.g. "input_down_axis"
	// and "input_l_y_plus_axis" both control lstick_axis_y_invert). First match wins.
	const std::string axis_keys[] = { "input_down_axis", "input_l_y_plus_axis", "input_right_axis", "input_l_x_plus_axis", "input_r_x_plus_axis", "input_r_y_plus_axis" };
	bool* axis_inv_mapping[] = { &mapping.lstick_axis_y_invert, &mapping.lstick_axis_y_invert, &mapping.lstick_axis_x_invert, &mapping.lstick_axis_x_invert, &mapping.rstick_axis_x_invert, &mapping.rstick_axis_y_invert };

	for (size_t i = 0; i < std::size(axis_keys); ++i)
	{
		if (!*axis_inv_mapping[i])
			*axis_inv_mapping[i] = lookup_retroarch_polarity(ra_player_input(axis_keys[i], player), config);
	}

	write_log("Controller Detection: invert left  y axis: %d\n", mapping.lstick_axis_y_invert);
	write_log("Controller Detection: invert left  x axis: %d\n", mapping.lstick_axis_x_invert);
	write_log("Controller Detection: invert right y axis: %d\n", mapping.rstick_axis_y_invert);
	write_log("Controller Detection: invert right x axis: %d\n", mapping.rstick_axis_x_invert);
}
