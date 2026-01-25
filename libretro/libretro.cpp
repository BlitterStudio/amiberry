#include "libretro.h"
#include "libretro_shared.h"
#ifdef LIBRETRO
#include "sdl_compat.h"
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "uae.h"
#include "memory.h"
#include "savestate.h"
#include "zfile.h"

cothread_t main_fiber;
cothread_t core_fiber;

retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
retro_log_printf_t log_cb;

static char game_path[1024];
static bool core_started = false;
static bool input_log_enabled = true;
static uint8_t key_state[RETROK_LAST + 1];
static constexpr size_t kJoypadMax = RETRO_DEVICE_ID_JOYPAD_R3 + 1;
static int16_t last_joypad[2][kJoypadMax];
static int16_t last_analog[2][4];
static int16_t last_trigger[2][2];
static int16_t last_mouse_buttons[3];
static int16_t last_mouse_x;
static int16_t last_mouse_y;
static unsigned libretro_port_device[2] = { RETRO_DEVICE_MOUSE, RETRO_DEVICE_JOYPAD };
static int libretro_joyport_order[2] = { 0, 1 };
static bool libretro_options_dirty = true;
static bool last_swap_ports = false;
static int last_deadzone = -1;
static int last_sensitivity = -1;
static bool last_input_log_file = false;
static FILE* input_log_file = nullptr;
static bool fastforward_forced = false;
static bool libretro_analog_enabled = true;
static bool last_analog_enabled = true;
static int last_vsync_setting = -1;
static float last_refresh_rate = -1.0f;

extern float vblank_hz;

static void input_log_file_write(const char* fmt, ...);

static void log_input_button(unsigned port, const char* name, int state)
{
	if (log_cb && input_log_enabled)
		log_cb(RETRO_LOG_INFO, "input p%u %s %s\n", port + 1, name, state ? "down" : "up");
	input_log_file_write("input p%u %s %s\n", port + 1, name, state ? "down" : "up");
}

static void log_input_axis(unsigned port, const char* name, int16_t value, int16_t* last)
{
	if (!log_cb || !input_log_enabled) {
		*last = value;
		if (value != 0)
			input_log_file_write("input p%u %s %d\n", port + 1, name, value);
		return;
	}
	if ((value == 0 && *last != 0) || (value != 0 && *last == 0))
		log_cb(RETRO_LOG_INFO, "input p%u %s %d\n", port + 1, name, value);
	if ((value == 0 && *last != 0) || (value != 0 && *last == 0))
		input_log_file_write("input p%u %s %d\n", port + 1, name, value);
	*last = value;
}

static void log_mouse_button(unsigned button, int state)
{
	if (log_cb && input_log_enabled)
		log_cb(RETRO_LOG_INFO, "input mouse button%u %s\n", button, state ? "down" : "up");
	input_log_file_write("input mouse button%u %s\n", button, state ? "down" : "up");
}

static void log_mouse_motion(int16_t dx, int16_t dy)
{
	if (!log_cb || !input_log_enabled)
	{
		if (dx != 0 || dy != 0)
			input_log_file_write("input mouse motion dx=%d dy=%d\n", dx, dy);
		return;
	}
	if ((dx != 0 || dy != 0) && (last_mouse_x == 0 && last_mouse_y == 0))
		log_cb(RETRO_LOG_INFO, "input mouse motion dx=%d dy=%d\n", dx, dy);
	if ((dx != 0 || dy != 0) && (last_mouse_x == 0 && last_mouse_y == 0))
		input_log_file_write("input mouse motion dx=%d dy=%d\n", dx, dy);
}

static const struct retro_variable variables[] = {
	{ "amiberry_model", "Amiga Model; A500|A500+|A600|A1200|CD32|A4000|CDTV" },
	{ "amiberry_port0_device", "Port 1 Device; mouse|joystick" },
	{ "amiberry_port1_device", "Port 2 Device; joystick|mouse" },
	{ "amiberry_swap_ports", "Swap Ports; disabled|enabled" },
	{ "amiberry_joyport_order", "Joyport Order; auto|21|12" },
	{ "amiberry_joy_deadzone", "Joystick Deadzone (%); 33|25|20|15|10|5|0|40|50" },
	{ "amiberry_analog_sensitivity", "Analog Sensitivity; 18|15|20|25|30|10" },
	{ "amiberry_analog", "Analog Input; enabled|disabled" },
	{ "amiberry_internal_vsync", "Internal VSync; disabled|standard|standard_50" },
	{ "amiberry_joy_as_mouse", "Joystick As Mouse; disabled|port1|port2|both" },
	{ "amiberry_input_log", "Input Log File; disabled|enabled" },
	{ NULL, NULL }
};

extern int amiberry_main(int argc, char** argv);

static bool file_readable(const char* path)
{
	return path && *path && access(path, R_OK) == 0;
}

static bool read_kickstart_path(char* out, size_t out_size)
{
	const char* conf_dir = getenv("AMIBERRY_CONFIG_DIR");
	const char* home_dir = getenv("HOME");
	char conf_path[1024] = {0};

	if (conf_dir && *conf_dir) {
		snprintf(conf_path, sizeof(conf_path), "%s/amiberry.conf", conf_dir);
	} else if (home_dir && *home_dir) {
		snprintf(conf_path, sizeof(conf_path), "%s/.config/amiberry/amiberry.conf", home_dir);
	} else {
		return false;
	}

	FILE* f = fopen(conf_path, "r");
	if (!f) {
		if (log_cb)
			log_cb(RETRO_LOG_WARN, "amiberry.conf not found: %s\n", conf_path);
		return false;
	}

	char line[2048];
	while (fgets(line, sizeof(line), f)) {
		char* p = line;
		while (*p == ' ' || *p == '\t')
			p++;
		if (strncmp(p, "kickstart_rom_file=", 19) == 0) {
			char* val = p + 19;
			char* end = val + strlen(val);
			while (end > val && (*(end - 1) == '\n' || *(end - 1) == '\r' || *(end - 1) == ' ' || *(end - 1) == '\t'))
				*--end = '\0';
			strncpy(out, val, out_size - 1);
			out[out_size - 1] = '\0';
			fclose(f);
			return file_readable(out);
		}
	}

	fclose(f);
	if (log_cb)
		log_cb(RETRO_LOG_WARN, "kickstart_rom_file not set in %s\n", conf_path);
	return false;
}

static void input_log_file_write(const char* fmt, ...)
{
	if (!input_log_file)
		return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(input_log_file, fmt, ap);
	va_end(ap);
	fflush(input_log_file);
}

static void update_input_log_file(bool enabled)
{
	if (enabled) {
		if (!input_log_file) {
			input_log_file = fopen("/tmp/amiberry_libretro_input.log", "a");
			if (input_log_file)
				input_log_file_write("amiberry libretro input log start\n");
		}
	} else if (input_log_file) {
		fclose(input_log_file);
		input_log_file = nullptr;
	}
}

void libretro_yield(void)
{
	if (main_fiber)
		co_switch(main_fiber);
}

static const char* get_option_value(const char* key)
{
	struct retro_variable var = { key, nullptr };
	if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		return var.value;
	return nullptr;
}

static unsigned device_from_option(const char* value, unsigned fallback)
{
	if (!value)
		return fallback;
	if (strcmp(value, "mouse") == 0)
		return RETRO_DEVICE_MOUSE;
	if (strcmp(value, "joystick") == 0 || strcmp(value, "joypad") == 0)
		return RETRO_DEVICE_JOYPAD;
	if (strcmp(value, "none") == 0)
		return RETRO_DEVICE_NONE;
	return fallback;
}

static bool option_enabled(const char* value)
{
	return value && (strcmp(value, "enabled") == 0 || strcmp(value, "on") == 0 || strcmp(value, "1") == 0);
}

static int option_to_int(const char* value, int fallback)
{
	if (!value || !*value)
		return fallback;
	return atoi(value);
}

static int vsync_setting_from_option(const char* value)
{
	if (!value || strcmp(value, "disabled") == 0)
		return 0;
	if (strcmp(value, "standard") == 0)
		return 1;
	if (strcmp(value, "standard_50") == 0)
		return 2;
	return 0;
}

static float get_core_refresh_rate()
{
	if (vblank_hz > 1.0f)
		return vblank_hz;
	return currprefs.ntscmode ? 60.0f : 50.0f;
}

static int mousemap_from_option(const char* value)
{
	if (!value)
		return 0;
	if (strcmp(value, "port1") == 0)
		return 1;
	if (strcmp(value, "port2") == 0)
		return 2;
	if (strcmp(value, "both") == 0)
		return 3;
	return 0;
}

static void apply_port_device(struct uae_prefs* prefs, unsigned port, unsigned device, int joy_index)
{
	const TCHAR* name = _T("none");
	int mode = -1;
	if (device == RETRO_DEVICE_MOUSE) {
		name = _T("mouse");
		mode = JSEM_MODE_MOUSE;
	} else if (device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_ANALOG) {
		if (joy_index < 0 || joy_index > 3)
			joy_index = static_cast<int>(port);
		TCHAR joy_name[8];
		_sntprintf(joy_name, sizeof joy_name, _T("joy%d"), joy_index);
		name = joy_name;
		mode = JSEM_MODE_JOYSTICK;
	}
	inputdevice_joyport_config(prefs, name, nullptr, static_cast<int>(port), mode, -1, 0, true);
}

static bool parse_joyport_order(const char* value, int out[2])
{
	out[0] = 0;
	out[1] = 1;
	if (!value || strcmp(value, "auto") == 0)
		return false;
	if (!value[0] || !value[1])
		return false;
	if (value[0] < '1' || value[0] > '4' || value[1] < '1' || value[1] > '4')
		return false;
	if (value[0] == value[1])
		return false;
	out[0] = value[0] - '1';
	out[1] = value[1] - '1';
	return true;
}

static void apply_libretro_input_options(void)
{
	unsigned desired_port_device[2] = { libretro_port_device[0], libretro_port_device[1] };
	const unsigned port0 = device_from_option(get_option_value("amiberry_port0_device"), desired_port_device[0]);
	const unsigned port1 = device_from_option(get_option_value("amiberry_port1_device"), desired_port_device[1]);
	desired_port_device[0] = port0;
	desired_port_device[1] = port1;

	int desired_joyport_order[2] = { libretro_joyport_order[0], libretro_joyport_order[1] };
	const bool joyport_order_explicit = parse_joyport_order(get_option_value("amiberry_joyport_order"), desired_joyport_order);
	if (!joyport_order_explicit) {
		if (desired_port_device[0] == RETRO_DEVICE_JOYPAD && desired_port_device[1] != RETRO_DEVICE_JOYPAD) {
			desired_joyport_order[0] = 0;
			desired_joyport_order[1] = 1;
		} else if (desired_port_device[1] == RETRO_DEVICE_JOYPAD && desired_port_device[0] != RETRO_DEVICE_JOYPAD) {
			desired_joyport_order[0] = 1;
			desired_joyport_order[1] = 0;
		} else {
			desired_joyport_order[0] = 0;
			desired_joyport_order[1] = 1;
		}
	}

	const bool swap_ports = option_enabled(get_option_value("amiberry_swap_ports"));
	const bool input_log_file_enabled = option_enabled(get_option_value("amiberry_input_log"));
	const int deadzone = option_to_int(get_option_value("amiberry_joy_deadzone"), 33);
	const int sensitivity = option_to_int(get_option_value("amiberry_analog_sensitivity"), 18);
	const char* analog_value = get_option_value("amiberry_analog");
	const bool analog_enabled = !analog_value || option_enabled(analog_value);
	const int vsync_setting = vsync_setting_from_option(get_option_value("amiberry_internal_vsync"));
	const int mousemap_mask = mousemap_from_option(get_option_value("amiberry_joy_as_mouse"));
	const int mousemap0 = (mousemap_mask & 1) ? 1 : 0;
	const int mousemap1 = (mousemap_mask & 2) ? 1 : 0;

	if (swap_ports) {
		const unsigned tmp = desired_port_device[0];
		desired_port_device[0] = desired_port_device[1];
		desired_port_device[1] = tmp;
		const int tmp_order = desired_joyport_order[0];
		desired_joyport_order[0] = desired_joyport_order[1];
		desired_joyport_order[1] = tmp_order;
	}

	const bool ports_changed = (desired_port_device[0] != libretro_port_device[0]) ||
		(desired_port_device[1] != libretro_port_device[1]) ||
		(swap_ports != last_swap_ports);
	const bool order_changed = (desired_joyport_order[0] != libretro_joyport_order[0]) ||
		(desired_joyport_order[1] != libretro_joyport_order[1]);
	const bool deadzone_changed = deadzone != last_deadzone;
	const bool sensitivity_changed = sensitivity != last_sensitivity;
	const bool mousemap_changed = mousemap0 != currprefs.jports[0].mousemap ||
		mousemap1 != currprefs.jports[1].mousemap;
	const bool analog_changed = analog_enabled != libretro_analog_enabled;
	const bool vsync_changed = vsync_setting != last_vsync_setting;

	if (input_log_file_enabled != last_input_log_file) {
		update_input_log_file(input_log_file_enabled);
		last_input_log_file = input_log_file_enabled;
	}

	if (ports_changed || order_changed || deadzone_changed || sensitivity_changed || mousemap_changed || analog_changed || vsync_changed) {
		libretro_port_device[0] = desired_port_device[0];
		libretro_port_device[1] = desired_port_device[1];
		libretro_joyport_order[0] = desired_joyport_order[0];
		libretro_joyport_order[1] = desired_joyport_order[1];
		last_swap_ports = swap_ports;
		last_deadzone = deadzone;
		last_sensitivity = sensitivity;
		libretro_analog_enabled = analog_enabled;
		last_vsync_setting = vsync_setting;

		changed_prefs.input_joymouse_deadzone = deadzone;
		changed_prefs.input_joystick_deadzone = deadzone;
		changed_prefs.input_analog_joystick_mult = sensitivity;
		currprefs.input_joymouse_deadzone = deadzone;
		currprefs.input_joystick_deadzone = deadzone;
		currprefs.input_analog_joystick_mult = sensitivity;

		if (vsync_changed) {
			int gfx_vsync = 0;
			int gfx_vsyncmode = 0;
			switch (vsync_setting) {
			case 1:
				gfx_vsync = 1;
				gfx_vsyncmode = 0;
				break;
			case 2:
				gfx_vsync = 2;
				gfx_vsyncmode = 0;
				break;
			default:
				gfx_vsync = 0;
				gfx_vsyncmode = 0;
				break;
			}
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = gfx_vsync;
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode = gfx_vsyncmode;
			changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsync = gfx_vsync;
			changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode = gfx_vsyncmode;
			currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = gfx_vsync;
			currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode = gfx_vsyncmode;
			currprefs.gfx_apmode[APMODE_RTG].gfx_vsync = gfx_vsync;
			currprefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode = gfx_vsyncmode;
			set_config_changed();
		}

		changed_prefs.jports[0].mousemap = mousemap0;
		changed_prefs.jports[1].mousemap = mousemap1;
		currprefs.jports[0].mousemap = mousemap0;
		currprefs.jports[1].mousemap = mousemap1;

		apply_port_device(&changed_prefs, 0, desired_port_device[0], desired_joyport_order[0]);
		apply_port_device(&changed_prefs, 1, desired_port_device[1], desired_joyport_order[1]);
		apply_port_device(&currprefs, 0, desired_port_device[0], desired_joyport_order[0]);
		apply_port_device(&currprefs, 1, desired_port_device[1], desired_joyport_order[1]);

		inputdevice_updateconfig(&changed_prefs, &currprefs);
	}
}

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
	if (keycode < RETROK_LAST) {
		if (key_state[keycode] == down)
			return;
		key_state[keycode] = down;
	}
	if (log_cb && input_log_enabled)
		log_cb(RETRO_LOG_INFO, "input kbd %s key=%u mod=0x%x char=0x%x\n",
			down ? "down" : "up", keycode, mod, character);
	input_log_file_write("input kbd %s key=%u mod=0x%x char=0x%x\n",
		down ? "down" : "up", keycode, mod, character);

	int scancode = 0;
	switch (keycode)
	{
	case RETROK_BACKSPACE: scancode = SDL_SCANCODE_BACKSPACE; break;
	case RETROK_TAB: scancode = SDL_SCANCODE_TAB; break;
	case RETROK_RETURN: scancode = SDL_SCANCODE_RETURN; break;
	case RETROK_PAUSE: scancode = SDL_SCANCODE_PAUSE; break;
	case RETROK_ESCAPE: scancode = SDL_SCANCODE_ESCAPE; break;
	case RETROK_SPACE: scancode = SDL_SCANCODE_SPACE; break;
	case RETROK_COMMA: scancode = SDL_SCANCODE_COMMA; break;
	case RETROK_MINUS: scancode = SDL_SCANCODE_MINUS; break;
	case RETROK_PERIOD: scancode = SDL_SCANCODE_PERIOD; break;
	case RETROK_SLASH: scancode = SDL_SCANCODE_SLASH; break;
	case RETROK_0: scancode = SDL_SCANCODE_0; break;
	case RETROK_1: scancode = SDL_SCANCODE_1; break;
	case RETROK_2: scancode = SDL_SCANCODE_2; break;
	case RETROK_3: scancode = SDL_SCANCODE_3; break;
	case RETROK_4: scancode = SDL_SCANCODE_4; break;
	case RETROK_5: scancode = SDL_SCANCODE_5; break;
	case RETROK_6: scancode = SDL_SCANCODE_6; break;
	case RETROK_7: scancode = SDL_SCANCODE_7; break;
	case RETROK_8: scancode = SDL_SCANCODE_8; break;
	case RETROK_9: scancode = SDL_SCANCODE_9; break;
	case RETROK_SEMICOLON: scancode = SDL_SCANCODE_SEMICOLON; break;
	case RETROK_EQUALS: scancode = SDL_SCANCODE_EQUALS; break;
	case RETROK_LEFTBRACKET: scancode = SDL_SCANCODE_LEFTBRACKET; break;
	case RETROK_BACKSLASH: scancode = SDL_SCANCODE_BACKSLASH; break;
	case RETROK_RIGHTBRACKET: scancode = SDL_SCANCODE_RIGHTBRACKET; break;
	case RETROK_BACKQUOTE: scancode = SDL_SCANCODE_GRAVE; break;
	case RETROK_a: scancode = SDL_SCANCODE_A; break;
	case RETROK_b: scancode = SDL_SCANCODE_B; break;
	case RETROK_c: scancode = SDL_SCANCODE_C; break;
	case RETROK_d: scancode = SDL_SCANCODE_D; break;
	case RETROK_e: scancode = SDL_SCANCODE_E; break;
	case RETROK_f: scancode = SDL_SCANCODE_F; break;
	case RETROK_g: scancode = SDL_SCANCODE_G; break;
	case RETROK_h: scancode = SDL_SCANCODE_H; break;
	case RETROK_i: scancode = SDL_SCANCODE_I; break;
	case RETROK_j: scancode = SDL_SCANCODE_J; break;
	case RETROK_k: scancode = SDL_SCANCODE_K; break;
	case RETROK_l: scancode = SDL_SCANCODE_L; break;
	case RETROK_m: scancode = SDL_SCANCODE_M; break;
	case RETROK_n: scancode = SDL_SCANCODE_N; break;
	case RETROK_o: scancode = SDL_SCANCODE_O; break;
	case RETROK_p: scancode = SDL_SCANCODE_P; break;
	case RETROK_q: scancode = SDL_SCANCODE_Q; break;
	case RETROK_r: scancode = SDL_SCANCODE_R; break;
	case RETROK_s: scancode = SDL_SCANCODE_S; break;
	case RETROK_t: scancode = SDL_SCANCODE_T; break;
	case RETROK_u: scancode = SDL_SCANCODE_U; break;
	case RETROK_v: scancode = SDL_SCANCODE_V; break;
	case RETROK_w: scancode = SDL_SCANCODE_W; break;
	case RETROK_x: scancode = SDL_SCANCODE_X; break;
	case RETROK_y: scancode = SDL_SCANCODE_Y; break;
	case RETROK_z: scancode = SDL_SCANCODE_Z; break;
	case RETROK_DELETE: scancode = SDL_SCANCODE_DELETE; break;
	case RETROK_KP0: scancode = SDL_SCANCODE_KP_0; break;
	case RETROK_KP1: scancode = SDL_SCANCODE_KP_1; break;
	case RETROK_KP2: scancode = SDL_SCANCODE_KP_2; break;
	case RETROK_KP3: scancode = SDL_SCANCODE_KP_3; break;
	case RETROK_KP4: scancode = SDL_SCANCODE_KP_4; break;
	case RETROK_KP5: scancode = SDL_SCANCODE_KP_5; break;
	case RETROK_KP6: scancode = SDL_SCANCODE_KP_6; break;
	case RETROK_KP7: scancode = SDL_SCANCODE_KP_7; break;
	case RETROK_KP8: scancode = SDL_SCANCODE_KP_8; break;
	case RETROK_KP9: scancode = SDL_SCANCODE_KP_9; break;
	case RETROK_KP_PERIOD: scancode = SDL_SCANCODE_KP_PERIOD; break;
	case RETROK_KP_DIVIDE: scancode = SDL_SCANCODE_KP_DIVIDE; break;
	case RETROK_KP_MULTIPLY: scancode = SDL_SCANCODE_KP_MULTIPLY; break;
	case RETROK_KP_MINUS: scancode = SDL_SCANCODE_KP_MINUS; break;
	case RETROK_KP_PLUS: scancode = SDL_SCANCODE_KP_PLUS; break;
	case RETROK_KP_ENTER: scancode = SDL_SCANCODE_KP_ENTER; break;
	case RETROK_UP: scancode = SDL_SCANCODE_UP; break;
	case RETROK_DOWN: scancode = SDL_SCANCODE_DOWN; break;
	case RETROK_RIGHT: scancode = SDL_SCANCODE_RIGHT; break;
	case RETROK_LEFT: scancode = SDL_SCANCODE_LEFT; break;
	case RETROK_INSERT: scancode = SDL_SCANCODE_INSERT; break;
	case RETROK_HOME: scancode = SDL_SCANCODE_HOME; break;
	case RETROK_END: scancode = SDL_SCANCODE_END; break;
	case RETROK_PAGEUP: scancode = SDL_SCANCODE_PAGEUP; break;
	case RETROK_PAGEDOWN: scancode = SDL_SCANCODE_PAGEDOWN; break;
	case RETROK_F1: scancode = SDL_SCANCODE_F1; break;
	case RETROK_F2: scancode = SDL_SCANCODE_F2; break;
	case RETROK_F3: scancode = SDL_SCANCODE_F3; break;
	case RETROK_F4: scancode = SDL_SCANCODE_F4; break;
	case RETROK_F5: scancode = SDL_SCANCODE_F5; break;
	case RETROK_F6: scancode = SDL_SCANCODE_F6; break;
	case RETROK_F7: scancode = SDL_SCANCODE_F7; break;
	case RETROK_F8: scancode = SDL_SCANCODE_F8; break;
	case RETROK_F9: scancode = SDL_SCANCODE_F9; break;
	case RETROK_F10: scancode = SDL_SCANCODE_F10; break;
	case RETROK_F11: scancode = SDL_SCANCODE_F11; break;
	case RETROK_F12: scancode = SDL_SCANCODE_F12; break;
	case RETROK_NUMLOCK: scancode = SDL_SCANCODE_NUMLOCKCLEAR; break;
	case RETROK_CAPSLOCK: scancode = SDL_SCANCODE_CAPSLOCK; break;
	case RETROK_SCROLLOCK: scancode = SDL_SCANCODE_SCROLLLOCK; break;
	case RETROK_RSHIFT: scancode = SDL_SCANCODE_RSHIFT; break;
	case RETROK_LSHIFT: scancode = SDL_SCANCODE_LSHIFT; break;
	case RETROK_RCTRL: scancode = SDL_SCANCODE_RCTRL; break;
	case RETROK_LCTRL: scancode = SDL_SCANCODE_LCTRL; break;
	case RETROK_RALT: scancode = SDL_SCANCODE_RALT; break;
	case RETROK_LALT: scancode = SDL_SCANCODE_LALT; break;
	case RETROK_LSUPER: scancode = SDL_SCANCODE_LGUI; break;
	case RETROK_RSUPER: scancode = SDL_SCANCODE_RGUI; break;
	default: break;
	}

	if (scancode > 0)
		my_kbd_handler(0, scancode, down ? 1 : 0, false);
}

static void poll_input(void)
{
	if (!input_poll_cb || !input_state_cb)
		return;

	input_poll_cb();
	const bool analog_enabled = libretro_analog_enabled;
	const bool clear_analog = !analog_enabled && last_analog_enabled;

	// Joystick input
	struct JoypadMap {
		unsigned retro_id;
		int sdl_button;
		const char* name;
	};
	static const JoypadMap joypad_map[] = {
		{ RETRO_DEVICE_ID_JOYPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_UP, "dpad_up" },
		{ RETRO_DEVICE_ID_JOYPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN, "dpad_down" },
		{ RETRO_DEVICE_ID_JOYPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT, "dpad_left" },
		{ RETRO_DEVICE_ID_JOYPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "dpad_right" },
		{ RETRO_DEVICE_ID_JOYPAD_B, SDL_CONTROLLER_BUTTON_A, "b" },
		{ RETRO_DEVICE_ID_JOYPAD_A, SDL_CONTROLLER_BUTTON_B, "a" },
		{ RETRO_DEVICE_ID_JOYPAD_Y, SDL_CONTROLLER_BUTTON_X, "y" },
		{ RETRO_DEVICE_ID_JOYPAD_X, SDL_CONTROLLER_BUTTON_Y, "x" },
		{ RETRO_DEVICE_ID_JOYPAD_SELECT, SDL_CONTROLLER_BUTTON_BACK, "select" },
		{ RETRO_DEVICE_ID_JOYPAD_START, SDL_CONTROLLER_BUTTON_START, "start" },
		{ RETRO_DEVICE_ID_JOYPAD_L, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "l1" },
		{ RETRO_DEVICE_ID_JOYPAD_R, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "r1" },
		{ RETRO_DEVICE_ID_JOYPAD_L3, SDL_CONTROLLER_BUTTON_LEFTSTICK, "l3" },
		{ RETRO_DEVICE_ID_JOYPAD_R3, SDL_CONTROLLER_BUTTON_RIGHTSTICK, "r3" }
	};

	for (int i = 0; i < 2; i++)
	{
		const int joy = i;
		for (const auto& mapping : joypad_map)
		{
			const int state = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, mapping.retro_id) & 1;
			if (state != last_joypad[i][mapping.retro_id]) {
				last_joypad[i][mapping.retro_id] = state;
				log_input_button(i, mapping.name, state);
			}
			setjoybuttonstate(joy, mapping.sdl_button, state);
		}

		const int l2 = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) & 1;
		const int r2 = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) & 1;
		if (l2 != last_joypad[i][RETRO_DEVICE_ID_JOYPAD_L2]) {
			last_joypad[i][RETRO_DEVICE_ID_JOYPAD_L2] = l2;
			log_input_button(i, "l2", l2);
		}
		if (r2 != last_joypad[i][RETRO_DEVICE_ID_JOYPAD_R2]) {
			last_joypad[i][RETRO_DEVICE_ID_JOYPAD_R2] = r2;
			log_input_button(i, "r2", r2);
		}

		if (analog_enabled) {
			const int16_t lx = (int16_t)input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
			const int16_t ly = (int16_t)input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
			const int16_t rx = (int16_t)input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
			const int16_t ry = (int16_t)input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
			const int16_t lt = l2 ? 32767 : 0;
			const int16_t rt = r2 ? 32767 : 0;

			log_input_axis(i, "lx", lx, &last_analog[i][0]);
			log_input_axis(i, "ly", ly, &last_analog[i][1]);
			log_input_axis(i, "rx", rx, &last_analog[i][2]);
			log_input_axis(i, "ry", ry, &last_analog[i][3]);
			log_input_axis(i, "lt", lt, &last_trigger[i][0]);
			log_input_axis(i, "rt", rt, &last_trigger[i][1]);

			setjoystickstate(joy, SDL_CONTROLLER_AXIS_LEFTX, lx, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_LEFTY, ly, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_RIGHTX, rx, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_RIGHTY, ry, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_TRIGGERLEFT, lt, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, rt, 32767);
		} else if (clear_analog) {
			memset(last_analog[i], 0, sizeof(last_analog[i]));
			memset(last_trigger[i], 0, sizeof(last_trigger[i]));
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_LEFTX, 0, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_LEFTY, 0, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_RIGHTX, 0, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_RIGHTY, 0, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0, 32767);
			setjoystickstate(joy, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0, 32767);
		}
	}

	// Mouse input
	for (int i = 0; i < 1; i++) // Usually only 1 mouse
	{
		int16_t mouse_x = input_state_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
		int16_t mouse_y = input_state_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
		log_mouse_motion(mouse_x, mouse_y);
		if (mouse_x != 0 || mouse_y != 0)
		{
			setmousestate(i, 0, mouse_x, 0);
			setmousestate(i, 1, mouse_y, 0);
		}

		static const int mouse_buttons[] = {
			RETRO_DEVICE_ID_MOUSE_LEFT,
			RETRO_DEVICE_ID_MOUSE_RIGHT,
			RETRO_DEVICE_ID_MOUSE_MIDDLE
		};
		for (int b = 0; b < 3; b++)
		{
			const int16_t state = input_state_cb(i, RETRO_DEVICE_MOUSE, 0, mouse_buttons[b]) & 1;
			if (state != last_mouse_buttons[b]) {
				last_mouse_buttons[b] = state;
				log_mouse_button(b, state);
			}
			setmousebuttonstate(i, b, state);
		}
		last_mouse_x = mouse_x;
		last_mouse_y = mouse_y;
	}
	last_analog_enabled = analog_enabled;
}

static void core_entry(void)
{
	char* argv[10];
	int argc = 0;
	argv[argc++] = strdup("amiberry");

	struct retro_variable var;
	var.key = "amiberry_model";
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		argv[argc++] = strdup("--model");
		argv[argc++] = strdup(var.value);
	}

	argv[argc++] = strdup("-G"); // No GUI

	char kick_path[1024] = {0};
	if (read_kickstart_path(kick_path, sizeof(kick_path))) {
		argv[argc++] = strdup("-r");
		argv[argc++] = strdup(kick_path);
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "Using Kickstart ROM from amiberry.conf: %s\n", kick_path);
	}

	if (game_path[0])
	{
		argv[argc++] = strdup(game_path);
	}
	argv[argc] = NULL;

	amiberry_main(argc, argv);
	
	// If amiberry_main returns, we are done
	libretro_yield();
}

void retro_init(void)
{
	if (!main_fiber)
		main_fiber = co_active();
	
	if (!core_fiber)
		core_fiber = co_create(65536 * sizeof(void*), core_entry);

	core_started = false;
}

void retro_deinit(void)
{
	if (core_fiber)
	{
		co_delete(core_fiber);
		core_fiber = NULL;
	}
	update_input_log_file(false);
	last_input_log_file = false;
	fastforward_forced = false;
	last_refresh_rate = -1.0f;
	core_started = false;
}

unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	if (port < 2) {
		libretro_port_device[port] = device;
		libretro_options_dirty = true;
	}
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name     = "Amiberry";
	info->library_version  = "v5.7.0-dev";
	info->valid_extensions = "adf|uae|ipf|zip|lha|hdf|rp9";
	info->need_fullpath    = true;
	info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	memset(info, 0, sizeof(*info));
	info->geometry.base_width   = 640;
	info->geometry.base_height  = 480;
	info->geometry.max_width    = 1280;
	info->geometry.max_height   = 1024;
	info->geometry.aspect_ratio = 4.0f / 3.0f;
	info->timing.fps            = get_core_refresh_rate();
	info->timing.sample_rate    = 44100.0;
}

void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
	struct retro_log_callback logging;
	if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
		log_cb = logging.log;
	else
		log_cb = NULL;
	{
		enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
		if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt) && log_cb) {
			log_cb(RETRO_LOG_WARN, "Failed to set pixel format XRGB8888; using default.\n");
		}
	}
	static struct retro_keyboard_callback kb_cb = { keyboard_cb };
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb_cb);
	environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)variables);

	static const struct retro_controller_description port_controllers[] = {
		{ "Joypad", RETRO_DEVICE_JOYPAD },
		{ "Mouse", RETRO_DEVICE_MOUSE },
		{ "None", RETRO_DEVICE_NONE }
	};
	static const struct retro_controller_info controller_info[] = {
		{ port_controllers, 3 },
		{ port_controllers, 3 },
		{ nullptr, 0 }
	};
	environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)controller_info);

	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1 (Fire)" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Button 3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Button 4" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "L1" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "R1" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "L3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "R3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
		{ 0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X, "Mouse X" },
		{ 0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y, "Mouse Y" },
		{ 0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT, "Mouse Left" },
		{ 0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT, "Mouse Right" },
		{ 0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE, "Mouse Middle" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Analog X" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Analog Y" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Analog X" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Analog Y" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1 (Fire)" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 2" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Button 3" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Button 4" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "L1" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "R1" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L2" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R2" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "L3" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "R3" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
		{ 1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X, "Mouse X" },
		{ 1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y, "Mouse Y" },
		{ 1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT, "Mouse Left" },
		{ 1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT, "Mouse Right" },
		{ 1, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE, "Mouse Middle" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Analog X" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Analog Y" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Analog X" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Analog Y" },
		{ 0, 0, 0, 0, NULL }
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
	if (log_cb)
		log_cb(RETRO_LOG_INFO, "retro_set_video_refresh=%p\n", (void*)cb);
}

void retro_reset(void)
{
	uae_reset(0, 0);
}

void retro_run(void)
{
	if (!core_started) {
		co_switch(core_fiber);
		core_started = true;
		return;
	}
	if (environ_cb) {
		bool updated = false;
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
			libretro_options_dirty = true;
	}
	if (libretro_options_dirty) {
		apply_libretro_input_options();
		libretro_options_dirty = false;
	}
	if (environ_cb) {
		const float refresh_rate = get_core_refresh_rate();
		if (fabsf(refresh_rate - last_refresh_rate) > 0.01f) {
			struct retro_system_av_info info;
			retro_get_system_av_info(&info);
			environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
			last_refresh_rate = refresh_rate;
		}
	}
	if (environ_cb) {
		bool fastforwarding = false;
		if (environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &fastforwarding)) {
			if (fastforwarding) {
				if (!currprefs.turbo_emulation) {
					warpmode(1);
					fastforward_forced = true;
				}
			} else if (fastforward_forced) {
				warpmode(0);
				fastforward_forced = false;
			}
		}
	}
	poll_input();
	co_switch(core_fiber);
}

bool retro_load_game(const struct retro_game_info *info)
{
	if (info && info->path)
		strncpy(game_path, info->path, sizeof(game_path) - 1);
	else
		game_path[0] = '\0';

	return true;
}

void retro_unload_game(void)
{
	uae_quit();
}

unsigned retro_get_region(void)
{
	return currprefs.ntscmode ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	return false;
}

size_t retro_serialize_size(void)
{
	return currprefs.chipmem.size + currprefs.bogomem.size + currprefs.fastmem[0].size + currprefs.z3fastmem[0].size + 2 * 1024 * 1024;
}

bool retro_serialize(void *data, size_t size)
{
	struct zfile *f = zfile_fopen_empty(NULL, _T("libretro"));
	if (!f) return false;

	save_state_internal(f, _T("Libretro Savestate"), 0, false);

	size_t actual_size = (size_t)zfile_size(f);
	if (actual_size > size)
	{
		zfile_fclose(f);
		return false;
	}

	size_t len;
	uae_u8 *ptr = zfile_get_data_pointer(f, &len);
	if (ptr)
	{
		memcpy(data, ptr, actual_size);
		zfile_fclose(f);
		return true;
	}

	zfile_fclose(f);
	return false;
}

bool retro_unserialize(const void *data, size_t size)
{
	struct zfile *f = zfile_fopen_data(_T("libretro"), (uae_u64)size, (const uae_u8*)data);
	if (!f) return false;

	restore_state_file(f);
	zfile_fclose(f);
	return true;
}

extern addrbank chipmem_bank;

void *retro_get_memory_data(unsigned id)
{
	switch (id)
	{
		case RETRO_MEMORY_SYSTEM_RAM:
			return chipmem_bank.baseaddr;
		default:
			return NULL;
	}
}

size_t retro_get_memory_size(unsigned id)
{
	switch (id)
	{
		case RETRO_MEMORY_SYSTEM_RAM:
			return currprefs.chipmem.size;
		default:
			return 0;
	}
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}
