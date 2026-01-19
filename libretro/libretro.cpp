#include "libretro.h"
#include "libretro_shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "uae.h"

cothread_t main_fiber;
cothread_t core_fiber;

retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;

static char game_path[1024];

static const struct retro_variable variables[] = {
	{ "amiberry_model", "Amiga Model; A500|A500+|A600|A1200|CD32|A4000|CDTV" },
	{ NULL, NULL }
};

extern int amiberry_main(int argc, char** argv);

void libretro_yield(void)
{
	if (main_fiber)
		co_switch(main_fiber);
}

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
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
	if (!input_poll_cb)
		return;

	input_poll_cb();

	// Joystick input
	for (int i = 0; i < 2; i++)
	{
		int port = (i == 0) ? 1 : 0; // Libretro P1 -> Amiga Port 1, Libretro P2 -> Amiga Port 0
		
		// Directions
		setjoybuttonstate(port, DIR_UP_BIT, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP));
		setjoybuttonstate(port, DIR_DOWN_BIT, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN));
		setjoybuttonstate(port, DIR_LEFT_BIT, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT));
		setjoybuttonstate(port, DIR_RIGHT_BIT, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT));
		
		// Buttons
		setjoybuttonstate(port, JOYBUTTON_1, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B));
		setjoybuttonstate(port, JOYBUTTON_2, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A));
		setjoybuttonstate(port, JOYBUTTON_3, input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X));
	}

	// Mouse input
	for (int i = 0; i < 1; i++) // Usually only 1 mouse
	{
		int16_t mouse_x = input_state_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
		int16_t mouse_y = input_state_cb(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
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
			int16_t state = input_state_cb(i, RETRO_DEVICE_MOUSE, 0, mouse_buttons[b]);
			setmousebuttonstate(i, b, state);
		}
	}
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
}

void retro_deinit(void)
{
	if (core_fiber)
	{
		co_delete(core_fiber);
		core_fiber = NULL;
	}
}

unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
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
	info->timing.fps            = 50.0;
	info->timing.sample_rate    = 44100.0;
}

void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
	static struct retro_keyboard_callback kb_cb = { keyboard_cb };
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb_cb);
	environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)variables);

	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1 (Fire)" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Button 3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Button 4" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1 (Fire)" },
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
}

void retro_reset(void)
{
	uae_reset(0, 0);
}

void retro_run(void)
{
	poll_input();
	co_switch(core_fiber);
}

bool retro_load_game(const struct retro_game_info *info)
{
	if (info && info->path)
		strncpy(game_path, info->path, sizeof(game_path) - 1);
	else
		game_path[0] = '\0';

	// Switch to core fiber to perform initialisation
	co_switch(core_fiber);

	return true;
}

void retro_unload_game(void)
{
	uae_quit();
}

unsigned retro_get_region(void)
{
	return RETRO_REGION_PAL;
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
