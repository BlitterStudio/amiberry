#pragma once
#include "options.h"

#define MAX_MAPPINGS 256

#define AXISTYPE_NORMAL 0
#define AXISTYPE_POV_X 1
#define AXISTYPE_POV_Y 2
#define AXISTYPE_SLIDER 3
#define AXISTYPE_DIAL 4

struct host_input_button {
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

	int number_of_hats{};
	int number_of_axis{};
	
	bool is_retroarch{};
};

struct didata {
	int type{};
	int acquired{};
	std::string name{};
	std::string controller_name{};
	std::string joystick_name{};

	int mousemap{};
	bool is_controller{};
	SDL_GameController* controller{};
	SDL_Joystick* joystick{};
	SDL_JoystickID joystick_id{};
	host_input_button mapping;
	uae_s16 axles{};
	uae_s16 buttons{}, buttons_real{};

	uae_s16 axismappings[MAX_MAPPINGS]{};
	TCHAR* axisname[MAX_MAPPINGS]{};
	uae_s16 axissort[MAX_MAPPINGS]{};
	uae_s16 axistype[MAX_MAPPINGS]{};
	bool analogstick{};

	uae_s16 buttonmappings[MAX_MAPPINGS]{};
	TCHAR* buttonname[MAX_MAPPINGS]{};
	uae_s16 buttonsort[MAX_MAPPINGS]{};
	uae_s16 buttonaxisparent[MAX_MAPPINGS]{};
	uae_s16 buttonaxisparentdir[MAX_MAPPINGS]{};
	uae_s16 buttonaxistype[MAX_MAPPINGS]{};
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
extern int find_inputevent(TCHAR* key);
extern int find_in_array(const int arr[], int n, int key);
extern const int default_button_mapping[];
extern const int default_axis_mapping[];
extern const int remap_event_list[];
extern const int remap_event_list_size;

extern const int remap_key_map_list[];
extern const char* remap_key_map_list_strings[];
extern const int remap_key_map_list_size;

//extern bool key_used_by_retroarch_joy(int scancode);
extern int keyhack(int scancode, int pressed, int num);
extern int get_retroarch_kb_num();
extern bool init_kb_from_retroarch(int index, char* retroarch_file);
extern std::string sanitize_retroarch_name(std::string s);
extern int find_retroarch(const TCHAR* find_setting, char* retroarch_file);
extern bool find_retroarch_polarity(const TCHAR* find_setting, char* retroarch_file);
extern host_input_button map_from_retroarch(host_input_button mapping, char* control_config, int player);

extern void read_joystick_button(int id, int button, int state);
extern void read_joystick_axis(int id, int axis, int value);
extern void read_joystick_hat(int id, int hat, int value);

extern void read_controller_button(int id, int button, int state);
extern void read_controller_axis(int id, int axis, int value);