#pragma once
#include "options.h"

#ifdef AMIBERRY
#define MAX_MAPPINGS 128
#else
#define MAX_MAPPINGS 256
#endif

enum
{
	AXISTYPE_NORMAL = 0,
	AXISTYPE_POV_X = 1,
	AXISTYPE_POV_Y = 2,
	AXISTYPE_SLIDER = 3,
	AXISTYPE_DIAL = 4
};

struct controller_mapping {
	std::array<int, SDL_CONTROLLER_BUTTON_MAX> button;
	std::array<int, SDL_CONTROLLER_AXIS_MAX> axis;

	bool lstick_axis_y_invert{};
	bool lstick_axis_x_invert{};

	bool rstick_axis_y_invert{};
	bool rstick_axis_x_invert{};

	int hotkey_button{};
	int quit_button{};
	int reset_button{};
	int menu_button{};
	int load_state_button{};
	int save_state_button{};
	int vkbd_button{};

	int number_of_hats{};
	int number_of_axis{};

	bool is_retroarch{};

	std::array<int, SDL_CONTROLLER_BUTTON_MAX> amiberry_custom_none;
	std::array<int, SDL_CONTROLLER_BUTTON_MAX> amiberry_custom_hotkey;
	std::array<int, SDL_CONTROLLER_AXIS_MAX> amiberry_custom_axis_none;
	std::array<int, SDL_CONTROLLER_AXIS_MAX> amiberry_custom_axis_hotkey;
};

struct didata {
	int type{};
	int acquired{};
	std::string name{};
	std::string controller_name{};
	std::string joystick_name{};

	std::string guid{};
	bool is_controller{};
	SDL_GameController* controller{};
	SDL_Joystick* joystick{};
	SDL_JoystickID joystick_id{};
	uae_s16 axles{};
	uae_s16 buttons{}, buttons_real{};

	controller_mapping mapping;

	std::array<uae_s16, MAX_MAPPINGS> axismappings;
	std::array<std::string, MAX_MAPPINGS> axisname;
	std::array<uae_s16, MAX_MAPPINGS> axissort;
	std::array<uae_s16, MAX_MAPPINGS> axistype;
	bool analogstick{};

	std::array<uae_s16, MAX_MAPPINGS> buttonmappings;
	std::array<std::string, MAX_MAPPINGS> buttonname;
	std::array<uae_s16, MAX_MAPPINGS> buttonsort;
	std::array<uae_s16, MAX_MAPPINGS> buttonaxisparent;
	std::array<uae_s16, MAX_MAPPINGS> buttonaxisparentdir;
	std::array<uae_s16, MAX_MAPPINGS> buttonaxistype;
};

//Analog joystick dead zone
static int joystick_dead_zone = 8000;
#define REMAP_BUTTONS 32
static int axisold[MAX_INPUT_DEVICES][256], buttonold[MAX_INPUT_DEVICES][256];

extern struct didata di_joystick[MAX_INPUT_DEVICES];

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

extern const TCHAR* find_inputevent_name(int key);
extern int find_inputevent(const TCHAR* key);
extern int find_in_array(const int arr[], int n, int key);
extern const int default_button_mapping[];
extern const int default_axis_mapping[];
extern const int remap_event_list[];
extern const int remap_event_list_size;

extern const int remap_key_map_list[];
extern std::vector<std::string> remap_key_map_list_strings;

//extern bool key_used_by_retroarch_joy(int scancode);
extern int keyhack(int scancode, int pressed, int num);
extern int get_retroarch_kb_num();
extern bool init_kb_from_retroarch(int index, const std::string& retroarch_file);
extern std::string sanitize_retroarch_name(std::string s);
extern int find_retroarch(const std::string& find_setting, const std::string& retroarch_file);
extern bool find_retroarch_polarity(const std::string& find_setting, const std::string& retroarch_file);
extern void map_from_retroarch(controller_mapping& mapping, const std::string& control_config, const int player);

extern void read_joystick_buttons(int id);
extern void read_joystick_axis(int id, int axis, int value);
extern void read_joystick_hat(int id, int hat, int value);

extern void read_controller_button(int id, int button, int state);
extern void read_controller_axis(int id, int axis, int value);

extern void save_controller_mapping_to_file(const controller_mapping& input, const std::string& filename);
extern void read_controller_mapping_from_file(controller_mapping& input, const std::string& filename);

extern bool load_custom_options(uae_prefs* p, const std::string& option, const TCHAR* value);
