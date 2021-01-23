#pragma once
#include "options.h"

struct host_input_button {
	std::array<int, 15> button;
	std::array<int, 6> axis;
	
	bool lstick_axis_y_invert;
	bool lstick_axis_x_invert;

	bool rstick_axis_y_invert;
	bool rstick_axis_x_invert;

	int hotkey_button;
	int quit_button;
	int reset_button;
	int load_state_button;
	int save_state_button;

	bool is_retroarch;
};

struct host_joystick_input_button {
	int north_button;
	int east_button;
	int south_button;
	int west_button;
	int dpad_left;
	int dpad_right;
	int dpad_up;
	int dpad_down;
	int select_button;
	int start_button;

	int left_shoulder;
	int right_shoulder;
	int left_trigger;
	int right_trigger;

	int lstick_button;
	int lstick_axis_y;
	bool lstick_axis_y_invert;
	int lstick_axis_x;
	bool lstick_axis_x_invert;
	int lstick_left;
	int lstick_right;
	int lstick_up;
	int lstick_down;

	int rstick_button;
	int rstick_axis_y;
	bool rstick_axis_y_invert;
	int rstick_axis_x;
	bool rstick_axis_x_invert;
	int rstick_left;
	int rstick_right;
	int rstick_up;
	int rstick_down;

	int hotkey_button;
	int quit_button;
	int menu_button;
	int reset_button;
	int load_state_button;
	int save_state_button;

	int number_of_hats;
	int number_of_axis;

	bool is_retroarch;
};

extern int kb_retroarch_player1[14];
extern int kb_retroarch_player1_3[15];
extern int kb_cd32_retroarch_player1[23];
extern int kb_retroarch_player2[14];
extern int kb_retroarch_player2_3[15];
extern int kb_cd32_retroarch_player2[23];
extern int kb_retroarch_player3[14];
extern int kb_retroarch_player3_3[15];
extern int kb_cd32_retroarch_player3[23];
extern int kb_retroarch_player4[14];
extern int kb_retroarch_player4_3[15];
extern int kb_cd32_retroarch_player4[23];

extern struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];
extern int find_in_array(const int arr[], int n, int key);
extern const int remap_key_map_list[];
extern const char* remap_key_map_list_strings[];
extern const int remap_key_map_list_size;

//extern bool key_used_by_retroarch_joy(int scancode);
extern int get_retroarch_kb_num();
extern bool init_kb_from_retroarch(int index, char* retroarch_file);
extern std::string sanitize_retroarch_name(std::string s);
extern int find_retroarch(const TCHAR* find_setting, char* retroarch_file);
extern bool find_retroarch_polarity(const TCHAR* find_setting, char* retroarch_file);
extern void map_from_retroarch(int cpt, char* control_config);