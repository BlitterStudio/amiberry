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
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define access _access
#ifndef R_OK
#define R_OK 4
#endif
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
#include "retro_dirent.h"
#include "streams/file_stream.h"
#include "file/file_path.h"
#ifdef __cplusplus
}
#endif

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "uae.h"
#include "memory.h"
#include "savestate.h"
#include "disk.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "zfile.h"
#include "target.h"

struct AmigaMonitor;
extern struct AmigaMonitor AMonitors[];
extern int gfx_GetWidth(const struct AmigaMonitor* mon);
extern int gfx_GetHeight(const struct AmigaMonitor* mon);

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
static bool input_bitmask_supported = false;
static bool memory_map_set = false;
static bool ff_override_supported = false;
static bool ff_override_active = false;
static int last_geometry_width = -1;
static int last_geometry_height = -1;
bool pixel_format_xrgb8888 = false;

static retro_set_led_state_t led_state_cb = nullptr;
enum RetroLedIndex {
	RETRO_LED_POWER = 0,
	RETRO_LED_DRIVES,
	RETRO_LED_HDCDMD,
	RETRO_LED_DRIVE0,
	RETRO_LED_DRIVE1,
	RETRO_LED_DRIVE2,
	RETRO_LED_DRIVE3,
	RETRO_LED_HD,
	RETRO_LED_CDMD,
	RETRO_LED_NUM
};
static unsigned retro_led_state[RETRO_LED_NUM] = { 0 };

extern float vblank_hz;

static constexpr int DEFAULT_GFX_WIDTH = 640;
static constexpr int DEFAULT_GFX_HEIGHT = 480;
static constexpr int MAX_GFX_WIDTH = 1280;
static constexpr int MAX_GFX_HEIGHT = 1024;
static constexpr size_t CORE_FIBER_STACK_SIZE = 65536 * sizeof(void*);

static void input_log_file_write(const char* fmt, ...);

static std::string system_dir;
static std::string save_dir;
static std::string content_dir;
static std::string whdload_temp_path;
static std::string cached_model;
static std::string cached_kickstart_override;
static std::string cached_cpu_model;
static std::string cached_chipset;
static std::string cached_chipset_aga;
static std::string cached_audio_rate;
static std::string cached_audio_interpol;
static int cached_audio_rate_value = 44100;
static const char* get_option_value(const char* key);

static struct retro_vfs_interface vfs_iface = {};
static bool vfs_available = false;

extern "C" const struct retro_vfs_interface* libretro_get_vfs_interface(void)
{
	return vfs_available ? &vfs_iface : nullptr;
}

extern "C" bool libretro_is_vfs_available(void)
{
	return vfs_available;
}

static FILE* libretro_debug_file = nullptr;

static std::string get_log_directory()
{
	if (!save_dir.empty()) return save_dir;
	if (!system_dir.empty()) return system_dir;
	const char* tmp = getenv("TMPDIR");
	if (!tmp) tmp = getenv("TMP");
	if (!tmp) tmp = getenv("TEMP");
	if (tmp) return tmp;
	return ".";
}

static void libretro_debug_open()
{
	if (libretro_debug_file)
		return;
	const char* env = getenv("AMIBERRY_LIBRETRO_DEBUG");
	if (env && !strcmp(env, "0"))
		return;
	const std::string dir = get_log_directory();
	const std::string path = dir.empty() ? std::string("amiberry_libretro_debug.log")
		: dir + "/amiberry_libretro_debug.log";
	libretro_debug_file = fopen(path.c_str(), "a");
	if (libretro_debug_file)
		setvbuf(libretro_debug_file, nullptr, _IOLBF, 0);
}

static void libretro_debug_close()
{
	if (!libretro_debug_file)
		return;
	fclose(libretro_debug_file);
	libretro_debug_file = nullptr;
}

static void libretro_debug_log(const char* fmt, ...)
{
	if (!libretro_debug_file)
		return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(libretro_debug_file, fmt, ap);
	va_end(ap);
}

struct DiskImage {
	std::string path;
	std::string label;
};

static std::vector<DiskImage> disk_images;
static unsigned disk_index = 0;
static bool disk_ejected = false;
static int initial_disk_index = -1;
static std::string initial_disk_path;
static unsigned last_disk_index = 0;
static bool last_disk_ejected = false;
static std::string disk_command;

struct CheatOp {
	uaecptr addr;
	uae_u32 value;
	uae_u8 size;
};

struct CheatEntry {
	bool enabled = false;
	std::vector<CheatOp> ops;
};

static std::vector<CheatEntry> cheat_entries;

static const char *skip_cheat_separators(const char *p)
{
	while (*p && (isspace((unsigned char)*p) || *p == ':' || *p == '=' || *p == ',' || *p == '|' || *p == '\t'))
		p++;
	return p;
}

static bool parse_hex_token(const char *&p, uae_u32 &out)
{
	p = skip_cheat_separators(p);
	if (*p == '$')
		p++;
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
		p += 2;

	uae_u32 value = 0;
	int digits = 0;
	while (*p && isxdigit((unsigned char)*p)) {
		char c = *p;
		uae_u32 v = 0;
		if (c >= '0' && c <= '9')
			v = static_cast<uae_u32>(c - '0');
		else if (c >= 'a' && c <= 'f')
			v = static_cast<uae_u32>(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			v = static_cast<uae_u32>(c - 'A' + 10);
		value = (value << 4) | v;
		p++;
		digits++;
	}
	if (digits == 0)
		return false;

	out = value;
	return true;
}

static bool parse_size_token(const char *&p, uae_u8 &out_size)
{
	const char *orig = p;
	p = skip_cheat_separators(p);
	if (!*p || *p == '+' || *p == ';')
		return false;

	char *end = nullptr;
	unsigned long sz = strtoul(p, &end, 0);
	if (end == p) {
		p = orig;
		return false;
	}
	p = end;
	if (sz == 8 || sz == 16 || sz == 32)
		sz /= 8;
	if (sz != 1 && sz != 2 && sz != 4)
		return false;
	out_size = static_cast<uae_u8>(sz);
	return true;
}

static bool parse_cheat_code(const char *code, std::vector<CheatOp> &ops)
{
	if (!code)
		return false;

	const char *p = code;
	while (*p) {
		p = skip_cheat_separators(p);
		if (!*p)
			break;
		if (*p == '+' || *p == ';') {
			p++;
			continue;
		}

		uae_u32 addr = 0;
		uae_u32 value = 0;
		if (!parse_hex_token(p, addr))
			return false;
		if (!parse_hex_token(p, value))
			return false;

		uae_u8 size = 0;
		const char *size_start = p;
		if (!parse_size_token(p, size))
			p = size_start;

		if (size == 0) {
			if (value <= 0xFF)
				size = 1;
			else if (value <= 0xFFFF)
				size = 2;
			else
				size = 4;
		}

		ops.push_back({ static_cast<uaecptr>(addr), value, size });

		p = skip_cheat_separators(p);
		if (*p == '+' || *p == ';')
			p++;
	}
	return !ops.empty();
}

static void apply_cheats()
{
	if (cheat_entries.empty())
		return;

	for (const auto &cheat : cheat_entries) {
		if (!cheat.enabled)
			continue;
		for (const auto &op : cheat.ops) {
			if (!valid_address(op.addr, op.size))
				continue;
			switch (op.size) {
				case 1:
					put_byte_compatible(op.addr, op.value);
					break;
				case 2:
					put_word_compatible(op.addr, op.value);
					break;
				case 4:
					put_long_compatible(op.addr, op.value);
					break;
				default:
					break;
			}
		}
	}
}

static void update_memory_map()
{
	if (!environ_cb || memory_map_set)
		return;

	struct retro_memory_descriptor memdesc[20];
	unsigned count = 0;

	auto add_mem = [&](uint64_t flags, uae_u8* base, uae_u32 start, uae_u32 size, const char* name) {
		if (!base || !size || count >= sizeof(memdesc) / sizeof(memdesc[0]))
			return;
		memdesc[count++] = { flags, base, 0, start, 0, 0, size, name };
	};

	add_mem(RETRO_MEMDESC_SYSTEM_RAM, chipmem_bank.baseaddr, chipmem_bank.start, chipmem_bank.allocated_size, "CHIP");
	add_mem(RETRO_MEMDESC_SYSTEM_RAM, bogomem_bank.baseaddr, bogomem_bank.start, bogomem_bank.allocated_size, "SLOW");
	for (int i = 0; i < MAX_RAM_BOARDS; ++i) {
		add_mem(RETRO_MEMDESC_SYSTEM_RAM, fastmem_bank[i].baseaddr, fastmem_bank[i].start,
			fastmem_bank[i].allocated_size, "FAST");
	}
	for (int i = 0; i < MAX_RAM_BOARDS; ++i) {
		add_mem(RETRO_MEMDESC_SYSTEM_RAM, z3fastmem_bank[i].baseaddr, z3fastmem_bank[i].start,
			z3fastmem_bank[i].allocated_size, "Z3FAST");
	}
	add_mem(RETRO_MEMDESC_SYSTEM_RAM, z3chipmem_bank.baseaddr, z3chipmem_bank.start,
		z3chipmem_bank.allocated_size, "Z3CHIP");
	add_mem(RETRO_MEMDESC_CONST, kickmem_bank.baseaddr, kickmem_bank.start, kickmem_bank.allocated_size, "KICK");
	add_mem(RETRO_MEMDESC_CONST, extendedkickmem_bank.baseaddr, extendedkickmem_bank.start,
		extendedkickmem_bank.allocated_size, "EXTROM");
	add_mem(RETRO_MEMDESC_CONST, extendedkickmem2a_bank.baseaddr, extendedkickmem2a_bank.start,
		extendedkickmem2a_bank.allocated_size, "EXTROM2A");
	add_mem(RETRO_MEMDESC_CONST, extendedkickmem2b_bank.baseaddr, extendedkickmem2b_bank.start,
		extendedkickmem2b_bank.allocated_size, "EXTROM2B");

	if (count == 0)
		return;

	const struct retro_memory_map mmap = { memdesc, count };
	environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void*)&mmap);
	memory_map_set = true;
}

static void update_led_interface()
{
	if (!led_state_cb)
		return;

	unsigned led_state[RETRO_LED_NUM] = { 0 };
	led_state[RETRO_LED_POWER] = gui_data.powerled ? 1 : 0;

	for (int i = 0; i < 4; i++) {
		if (gui_data.drives[i].df[0]) {
			if (!led_state[RETRO_LED_DRIVES])
				led_state[RETRO_LED_DRIVES] = gui_data.drives[i].drive_motor ? 1 : 0;
			switch (i) {
				case 0: led_state[RETRO_LED_DRIVE0] = gui_data.drives[i].drive_motor ? 1 : 0; break;
				case 1: led_state[RETRO_LED_DRIVE1] = gui_data.drives[i].drive_motor ? 1 : 0; break;
				case 2: led_state[RETRO_LED_DRIVE2] = gui_data.drives[i].drive_motor ? 1 : 0; break;
				case 3: led_state[RETRO_LED_DRIVE3] = gui_data.drives[i].drive_motor ? 1 : 0; break;
				default: break;
			}
		}
	}

	if (gui_data.hd >= 0) {
		led_state[RETRO_LED_HD] = gui_data.hd ? 1 : 0;
		if (!led_state[RETRO_LED_HDCDMD])
			led_state[RETRO_LED_HDCDMD] = led_state[RETRO_LED_HD];
	}
	if (gui_data.cd >= 0) {
		led_state[RETRO_LED_CDMD] = (gui_data.cd & (LED_CD_ACTIVE | LED_CD_AUDIO)) ? 1 : 0;
		if (!led_state[RETRO_LED_HDCDMD])
			led_state[RETRO_LED_HDCDMD] = led_state[RETRO_LED_CDMD];
	}
	if (gui_data.md >= 0) {
		led_state[RETRO_LED_CDMD] = gui_data.md ? 1 : 0;
		if (!led_state[RETRO_LED_HDCDMD])
			led_state[RETRO_LED_HDCDMD] = led_state[RETRO_LED_CDMD];
	}

	for (unsigned i = 0; i < RETRO_LED_NUM; i++) {
		if (retro_led_state[i] != led_state[i]) {
			retro_led_state[i] = led_state[i];
			led_state_cb(i, led_state[i]);
		}
	}
}

static void set_fastforward_override(bool enabled)
{
	if (!ff_override_supported || !environ_cb)
		return;
	if (ff_override_active == enabled)
		return;

	struct retro_fastforwarding_override ff = {};
	ff.ratio = 0.0f;
	ff.fastforward = enabled;
	ff.notification = enabled;
	ff.inhibit_toggle = false;
	environ_cb(RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE, &ff);
	ff_override_active = enabled;
}

static void update_geometry()
{
	if (!environ_cb)
		return;

	const int width = gfx_GetWidth(&AMonitors[0]);
	const int height = gfx_GetHeight(&AMonitors[0]);
	if (width <= 0 || height <= 0)
		return;

	if (width == last_geometry_width && height == last_geometry_height)
		return;

	struct retro_game_geometry geom;
	geom.base_width = width;
	geom.base_height = height;
	geom.max_width = std::max(width, MAX_GFX_WIDTH);
	geom.max_height = std::max(height, MAX_GFX_HEIGHT);
	geom.aspect_ratio = (float)width / (float)height;
	environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geom);
	last_geometry_width = width;
	last_geometry_height = height;
}

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
	if ((value == 0 && *last != 0) || (value != 0 && *last == 0)) {
		log_cb(RETRO_LOG_INFO, "input p%u %s %d\n", port + 1, name, value);
		input_log_file_write("input p%u %s %d\n", port + 1, name, value);
	}
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
	if ((dx != 0 || dy != 0) && (last_mouse_x == 0 && last_mouse_y == 0)) {
		log_cb(RETRO_LOG_INFO, "input mouse motion dx=%d dy=%d\n", dx, dy);
		input_log_file_write("input mouse motion dx=%d dy=%d\n", dx, dy);
	}
}

// NOTE: This v1 options array is a fallback for old frontends that don't support v2.
// When modifying options, keep this in sync with option_defs[] below.
static const struct retro_variable variables[] = {
	{ "amiberry_model", "Amiga Model; A500|A500+|A600|A1200|CD32|A4000|CDTV" },
	{ "amiberry_kickstart", "Kickstart ROM; auto|kick.rom|kick13.rom|kick20.rom|kick31.rom|kick205.rom|kick40068.A1200|kick40068.A4000|cd32.rom|cdtv.rom" },
	{ "amiberry_cpu_model", "CPU Model; auto|68000|68010|68020|68030" },
	{ "amiberry_chipset", "Chipset; auto|ocs|ecs" },
	{ "amiberry_chipset_aga", "Chipset (AGA Models); auto|ocs|ecs|aga" },
	{ "amiberry_audio_rate", "Audio Rate (Hz); auto|44100|48000" },
	{ "amiberry_audio_interpolation", "Audio Interpolation; auto|none|anti|sinc" },
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

static struct retro_core_option_v2_category option_cats[] = {
	{ "system", "System", "System and ROM settings." },
	{ "input", "Input", "Controller and mouse settings." },
	{ "video", "Video", "Video and sync settings." },
	{ "audio", "Audio", "Audio settings." },
	{ NULL, NULL, NULL }
};

static struct retro_core_option_v2_definition option_defs[] = {
	{
		"amiberry_model",
		"System Model",
		"Model",
		"Select the base Amiga model preset. Core restart required.",
		NULL,
		"system",
		{
			{ "A500", "A500" },
			{ "A500+", "A500+" },
			{ "A600", "A600" },
			{ "A1200", "A1200" },
			{ "CD32", "CD32" },
			{ "A4000", "A4000" },
			{ "CDTV", "CDTV" },
			{ NULL, NULL }
		},
		"A500"
	},
	{
		"amiberry_kickstart",
		"Kickstart ROM",
		"Kickstart ROM",
		"Override the Kickstart ROM file. Uses system directory unless an absolute path is provided. Core restart required.",
		NULL,
		"system",
		{
			{ "auto", "Automatic" },
			{ "kick.rom", "kick.rom" },
			{ "kick13.rom", "kick13.rom" },
			{ "kick20.rom", "kick20.rom" },
			{ "kick31.rom", "kick31.rom" },
			{ "kick205.rom", "kick205.rom" },
			{ "kick40068.A1200", "kick40068.A1200" },
			{ "kick40068.A4000", "kick40068.A4000" },
			{ "cd32.rom", "cd32.rom" },
			{ "cdtv.rom", "cdtv.rom" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_cpu_model",
		"CPU Model",
		"CPU Model",
		"Override the CPU model. Uses model defaults when set to Auto. Core restart required.",
		NULL,
		"system",
		{
			{ "auto", "Auto" },
			{ "68000", "68000" },
			{ "68010", "68010" },
			{ "68020", "68020" },
			{ "68030", "68030" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_chipset",
		"Chipset",
		"Chipset",
		"Override the chipset. Uses model defaults when set to Auto. Core restart required.",
		NULL,
		"system",
		{
			{ "auto", "Auto" },
			{ "ocs", "OCS" },
			{ "ecs", "ECS" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_chipset_aga",
		"Chipset (AGA Models)",
		"Chipset (AGA)",
		"Override the chipset on AGA-capable models. Uses model defaults when set to Auto. Core restart required.",
		NULL,
		"system",
		{
			{ "auto", "Auto" },
			{ "ocs", "OCS" },
			{ "ecs", "ECS" },
			{ "aga", "AGA" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_port0_device",
		"Port 1 Device",
		"Port 1 Device",
		"Select the input device for Amiga port 1.",
		NULL,
		"input",
		{
			{ "mouse", "Mouse" },
			{ "joystick", "Joystick" },
			{ NULL, NULL }
		},
		"mouse"
	},
	{
		"amiberry_port1_device",
		"Port 2 Device",
		"Port 2 Device",
		"Select the input device for Amiga port 2.",
		NULL,
		"input",
		{
			{ "joystick", "Joystick" },
			{ "mouse", "Mouse" },
			{ NULL, NULL }
		},
		"joystick"
	},
	{
		"amiberry_swap_ports",
		"Swap Ports",
		"Swap Ports",
		"Swap input devices between port 1 and port 2.",
		NULL,
		"input",
		{
			{ "disabled", NULL },
			{ "enabled", NULL },
			{ NULL, NULL }
		},
		"disabled"
	},
	{
		"amiberry_joyport_order",
		"Joyport Order",
		"Joyport Order",
		"Order for mapping controller ports to Amiga ports.",
		NULL,
		"input",
		{
			{ "auto", "Automatic" },
			{ "21", "2-1" },
			{ "12", "1-2" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_joy_deadzone",
		"Joystick Deadzone",
		"Deadzone",
		"Analog deadzone percentage.",
		NULL,
		"input",
		{
			{ "33", "33%" },
			{ "25", "25%" },
			{ "20", "20%" },
			{ "15", "15%" },
			{ "10", "10%" },
			{ "5", "5%" },
			{ "0", "0%" },
			{ "40", "40%" },
			{ "50", "50%" },
			{ NULL, NULL }
		},
		"33"
	},
	{
		"amiberry_analog_sensitivity",
		"Analog Sensitivity",
		"Sensitivity",
		"Adjust analog sensitivity.",
		NULL,
		"input",
		{
			{ "18", "18" },
			{ "15", "15" },
			{ "20", "20" },
			{ "25", "25" },
			{ "30", "30" },
			{ "10", "10" },
			{ NULL, NULL }
		},
		"18"
	},
	{
		"amiberry_analog",
		"Analog Input",
		"Analog Input",
		"Enable analog input mapping.",
		NULL,
		"input",
		{
			{ "enabled", NULL },
			{ "disabled", NULL },
			{ NULL, NULL }
		},
		"enabled"
	},
	{
		"amiberry_internal_vsync",
		"Internal VSync",
		"Internal VSync",
		"Select internal vsync mode.",
		NULL,
		"video",
		{
			{ "disabled", NULL },
			{ "standard", "Standard" },
			{ "standard_50", "Standard (50Hz)" },
			{ NULL, NULL }
		},
		"disabled"
	},
	{
		"amiberry_audio_rate",
		"Audio Rate (Hz)",
		"Audio Rate",
		"Override audio sample rate. Uses defaults when set to Auto. Core restart required.",
		NULL,
		"audio",
		{
			{ "auto", "Auto" },
			{ "44100", "44100" },
			{ "48000", "48000" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_audio_interpolation",
		"Audio Interpolation",
		"Interpolation",
		"Override audio interpolation. Uses defaults when set to Auto. Core restart required.",
		NULL,
		"audio",
		{
			{ "auto", "Auto" },
			{ "none", "Off" },
			{ "anti", "Anti-alias" },
			{ "sinc", "Sinc" },
			{ NULL, NULL }
		},
		"auto"
	},
	{
		"amiberry_joy_as_mouse",
		"Joystick As Mouse",
		"Joystick As Mouse",
		"Map joystick to mouse input.",
		NULL,
		"input",
		{
			{ "disabled", NULL },
			{ "port1", "Port 1" },
			{ "port2", "Port 2" },
			{ "both", "Both" },
			{ NULL, NULL }
		},
		"disabled"
	},
	{
		"amiberry_input_log",
		"Input Log File",
		"Input Log File",
		"Write input events to a log file in the save directory.",
		NULL,
		"input",
		{
			{ "disabled", NULL },
			{ "enabled", NULL },
			{ NULL, NULL }
		},
		"disabled"
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, { { NULL, NULL } }, NULL }
};

static void set_core_options()
{
	unsigned version = 0;
	if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2) {
		static struct retro_core_options_v2 options_v2 = {
			option_cats,
			option_defs
		};
		if (environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options_v2))
			return;
	}

	if (environ_cb)
		environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)variables);
}

static bool is_aga_model_name(const char* model)
{
	return model && (strcmp(model, "A1200") == 0 || strcmp(model, "A4000") == 0 || strcmp(model, "CD32") == 0);
}

static bool update_core_option_visibility(void)
{
	if (!environ_cb)
		return false;
	const char* model = get_option_value("amiberry_model");
	const bool aga_model = is_aga_model_name(model);

	struct retro_core_option_display disp;
	disp.key = "amiberry_chipset";
	disp.visible = !aga_model;
	environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &disp);

	disp.key = "amiberry_chipset_aga";
	disp.visible = aga_model;
	environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &disp);

	return true;
}

static bool core_options_update_display_callback(void)
{
	return update_core_option_visibility();
}

extern int amiberry_main(int argc, char** argv);

static bool path_is_absolute(const std::string& path)
{
	if (path.empty())
		return false;
	if (path[0] == '/' || path[0] == '\\')
		return true;
	if (path.size() > 1 && path[1] == ':')
		return true;
	return false;
}

static std::string path_dirname(const std::string& path)
{
	const auto pos = path.find_last_of("/\\");
	if (pos == std::string::npos)
		return std::string();
	return path.substr(0, pos);
}

static std::string path_basename(const std::string& path)
{
	const auto pos = path.find_last_of("/\\");
	if (pos == std::string::npos)
		return path;
	return path.substr(pos + 1);
}

static std::string path_join(const std::string& dir, const std::string& file)
{
	if (dir.empty())
		return file;
	if (file.empty())
		return dir;
	const char last = dir.back();
	if (last == '/' || last == '\\')
		return dir + file;
	return dir + "/" + file;
}

static std::string to_lower_copy(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
	return value;
}

static std::string path_extension_lower(const std::string& path)
{
	const auto pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return std::string();
	return to_lower_copy(path.substr(pos + 1));
}

static std::string sanitize_filename(const std::string& name)
{
	std::string out = name;
	for (char& ch : out) {
		if (ch == '/' || ch == '\\' || ch == ':')
			ch = '_';
	}
	return out;
}

static bool has_non_ascii(const std::string& value)
{
	for (unsigned char ch : value) {
		if (ch >= 0x80)
			return true;
	}
	return false;
}

static bool vfs_write_file(const char* path, const void* data, size_t size)
{
	if (!path || !*path || !data || size == 0)
		return false;

	if (vfs_available && vfs_iface.open && vfs_iface.write && vfs_iface.close) {
		struct retro_vfs_file_handle* fp = vfs_iface.open(path, RETRO_VFS_FILE_ACCESS_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE);
		if (!fp)
			return false;
		const int64_t written = vfs_iface.write(fp, data, size);
		vfs_iface.close(fp);
		return written == static_cast<int64_t>(size);
	}

	FILE* f = fopen(path, "wb");
	if (!f)
		return false;
	const size_t written = fwrite(data, 1, size, f);
	fclose(f);
	return written == size;
}

static bool vfs_read_all(const char* path, std::vector<uint8_t>& out)
{
	if (!vfs_available || !vfs_iface.open || !vfs_iface.read || !vfs_iface.close)
		return false;

	struct retro_vfs_file_handle* fp = vfs_iface.open(path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
	if (!fp)
		return false;

	int64_t size = -1;
	if (vfs_iface.size)
		size = vfs_iface.size(fp);

	if (size > 0) {
		out.resize(static_cast<size_t>(size));
		const int64_t read = vfs_iface.read(fp, out.data(), static_cast<uint64_t>(size));
		vfs_iface.close(fp);
		if (read != size)
			return false;
		return true;
	}

	const size_t chunk = 1 << 16;
	std::vector<uint8_t> buffer(chunk);
	out.clear();
	while (true) {
		const int64_t read = vfs_iface.read(fp, buffer.data(), buffer.size());
		if (read <= 0)
			break;
		out.insert(out.end(), buffer.begin(), buffer.begin() + static_cast<size_t>(read));
	}
	vfs_iface.close(fp);
	return !out.empty();
}

static void setup_whdload_paths()
{
	if (!system_dir.empty())
		set_whdbootpath(system_dir);
	else if (!save_dir.empty())
		set_whdbootpath(save_dir);
	else if (!content_dir.empty())
		set_whdbootpath(content_dir);

	const char* whdboot_env = nullptr;
	if (!system_dir.empty())
		whdboot_env = system_dir.c_str();
	else if (!save_dir.empty())
		whdboot_env = save_dir.c_str();
	else if (!content_dir.empty())
		whdboot_env = content_dir.c_str();
	if (whdboot_env && *whdboot_env) {
#ifdef _WIN32
		_putenv_s("AMIBERRY_WHDBOOT_PATH", whdboot_env);
#else
		setenv("AMIBERRY_WHDBOOT_PATH", whdboot_env, 1);
#endif
	}

	if (!save_dir.empty()) {
#ifdef _WIN32
		_putenv_s("WHDBOOT_SAVE_DATA", save_dir.c_str());
		_putenv_s("AMIBERRY_WHDBOOT_SAVE_DATA", save_dir.c_str());
#else
		setenv("WHDBOOT_SAVE_DATA", save_dir.c_str(), 1);
		setenv("AMIBERRY_WHDBOOT_SAVE_DATA", save_dir.c_str(), 1);
#endif
	}
}

static std::string trim_copy(const char* input)
{
	if (!input)
		return std::string();
	const char* start = input;
	while (*start && isspace(static_cast<unsigned char>(*start)))
		start++;
	const char* end = start + strlen(start);
	while (end > start && isspace(static_cast<unsigned char>(*(end - 1))))
		end--;
	return std::string(start, end - start);
}

static bool file_readable(const char* path)
{
	return path && *path && access(path, R_OK) == 0;
}

static bool dir_exists(const std::string& path)
{
	if (path.empty())
		return false;
	struct stat st {};
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool find_kickstart_in_dir(const std::string& dir, const char* name, char* out, size_t out_size)
{
	if (dir.empty() || !name || !*name)
		return false;
	const std::string full = path_join(dir, name);
	if (!file_readable(full.c_str()))
		return false;
	snprintf(out, out_size, "%s", full.c_str());
	return true;
}

static bool pick_whdload_kickstart(const std::string& dir, char* out, size_t out_size)
{
	if (find_kickstart_in_dir(dir, "kick205.rom", out, out_size))
		return true;
	if (find_kickstart_in_dir(dir, "kick31.rom", out, out_size))
		return true;
	return false;
}

static bool find_kickstart_in_system_dir(const char* model, char* out, size_t out_size)
{
	if (system_dir.empty())
		return false;

	const char* candidates_generic[] = {
		"kick.rom",
		"kick13.rom",
		"kick20.rom",
		"kick31.rom"
	};
	const char* candidates_a1200[] = { "kick31.rom", "kick40068.A1200", "kick.rom" };
	const char* candidates_a4000[] = { "kick31.rom", "kick40068.A4000", "kick.rom" };
	const char* candidates_a600[] = { "kick20.rom", "kick205.rom", "kick.rom", "kick13.rom" };
	const char* candidates_cd32[] = { "cd32.rom", "kick31.rom", "kick.rom" };
	const char* candidates_cdtv[] = { "cdtv.rom", "kick13.rom", "kick.rom" };

	const char** candidates = candidates_generic;
	size_t count = sizeof(candidates_generic) / sizeof(candidates_generic[0]);

	if (model) {
		if (!strcmp(model, "A1200")) {
			candidates = candidates_a1200;
			count = sizeof(candidates_a1200) / sizeof(candidates_a1200[0]);
		} else if (!strcmp(model, "A4000")) {
			candidates = candidates_a4000;
			count = sizeof(candidates_a4000) / sizeof(candidates_a4000[0]);
		} else if (!strcmp(model, "A600")) {
			candidates = candidates_a600;
			count = sizeof(candidates_a600) / sizeof(candidates_a600[0]);
		} else if (!strcmp(model, "CD32")) {
			candidates = candidates_cd32;
			count = sizeof(candidates_cd32) / sizeof(candidates_cd32[0]);
		} else if (!strcmp(model, "CDTV")) {
			candidates = candidates_cdtv;
			count = sizeof(candidates_cdtv) / sizeof(candidates_cdtv[0]);
		}
	}

	for (size_t i = 0; i < count; i++) {
		const std::string candidate = path_join(system_dir, candidates[i]);
		if (file_readable(candidate.c_str())) {
			strncpy(out, candidate.c_str(), out_size - 1);
			out[out_size - 1] = '\0';
			return true;
		}
	}

	return false;
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

static bool parse_m3u(const char* path, std::vector<DiskImage>& out_images)
{
	if (!path || !*path)
		return false;
	FILE* f = fopen(path, "r");
	if (!f)
		return false;

	std::string base_dir = path_dirname(path);
	std::string pending_label;
	char line[2048];
	while (fgets(line, sizeof(line), f)) {
		char* p = line;
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == '#') {
			if (strncmp(p, "#COMMAND:", 9) == 0) {
				disk_command = trim_copy(p + 9);
			} else if (strncmp(p, "#SAVEDISK:", 10) == 0) {
				if (log_cb)
					log_cb(RETRO_LOG_WARN, "M3U savedisk tag not supported in libretro core\n");
			} else if (strncmp(p, "#EXTINF:", 8) == 0) {
				char* comma = strchr(p, ',');
				if (comma)
					pending_label = trim_copy(comma + 1);
			} else if (strncmp(p, "#LABEL:", 7) == 0) {
				pending_label = trim_copy(p + 7);
			} else if (strncmp(p, "#DISK:", 6) == 0) {
				pending_label = trim_copy(p + 6);
			}
			continue;
		}
		if (*p == '\0' || *p == '\n' || *p == '\r')
			continue;
		char* end = p + strlen(p);
		while (end > p && (*(end - 1) == '\n' || *(end - 1) == '\r' || *(end - 1) == ' ' || *(end - 1) == '\t'))
			*--end = '\0';
		if (*p == '\0')
			continue;

		std::string entry;
		std::string label;
		bool label_set = false;

		char* delim = strchr(p, '|');
		if (delim) {
			entry.assign(p, delim - p);
			label = trim_copy(delim + 1);
			label_set = true;
		} else {
			entry = p;
		}
		if (!path_is_absolute(entry))
			entry = path_join(base_dir, entry);
		DiskImage image;
		image.path = entry;
		if (label_set) {
			image.label = label;
		} else if (!pending_label.empty()) {
			image.label = pending_label;
		}
		pending_label.clear();
		out_images.push_back(image);
	}
	fclose(f);
	return !out_images.empty();
}

static bool libretro_get_image_label(unsigned index, char* s, size_t len);

static void show_message(const char* text)
{
	if (!environ_cb || !text || !*text)
		return;
	struct retro_message msg;
	msg.msg = text;
	msg.frames = 120;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
}

static void notify_disk_change()
{
	if (disk_ejected) {
		if (!last_disk_ejected)
			show_message("Disk ejected");
		last_disk_ejected = true;
		return;
	}

	if (disk_index >= disk_images.size())
		return;
	if (disk_images[disk_index].path.empty())
		return;

	if (last_disk_ejected || disk_index != last_disk_index) {
		char label[256];
		const char* label_ptr = nullptr;
		if (libretro_get_image_label(disk_index, label, sizeof(label)))
			label_ptr = label;
		char msg[320];
		if (label_ptr && *label_ptr)
			snprintf(msg, sizeof(msg), "Disk %u: %s inserted", disk_index + 1, label_ptr);
		else
			snprintf(msg, sizeof(msg), "Disk %u inserted", disk_index + 1);
		show_message(msg);
	}
	last_disk_index = disk_index;
	last_disk_ejected = false;
}

static void disk_control_apply()
{
	if (!core_started)
		return;

	if (disk_ejected) {
		disk_eject(0);
		notify_disk_change();
		return;
	}
	if (disk_index >= disk_images.size())
		return;
	if (disk_images[disk_index].path.empty()) {
		disk_eject(0);
		notify_disk_change();
		return;
	}
	TCHAR tpath[MAX_DPATH];
	uae_tcslcpy(tpath, disk_images[disk_index].path.c_str(), MAX_DPATH);
	disk_eject(0);
	disk_insert(0, tpath);
	notify_disk_change();
}

static bool libretro_set_eject_state(bool ejected)
{
	disk_ejected = ejected;
	disk_control_apply();
	return true;
}

static bool libretro_get_eject_state(void)
{
	return disk_ejected;
}

static unsigned libretro_get_image_index(void)
{
	return disk_index;
}

static bool libretro_set_image_index(unsigned index)
{
	if (index >= disk_images.size())
		return false;
	disk_index = index;
	disk_control_apply();
	return true;
}

static unsigned libretro_get_num_images(void)
{
	return (unsigned)disk_images.size();
}

static bool libretro_replace_image_index(unsigned index, const struct retro_game_info* info)
{
	if (index >= disk_images.size())
		return false;
	if (info && info->path) {
		disk_images[index].path = info->path;
		disk_images[index].label.clear();
	} else {
		disk_images[index].path.clear();
		disk_images[index].label.clear();
	}
	if (index == disk_index)
		disk_control_apply();
	return true;
}

static bool libretro_add_image_index(void)
{
	disk_images.emplace_back();
	return true;
}

static bool libretro_set_initial_image(unsigned index, const char* path)
{
	initial_disk_index = static_cast<int>(index);
	if (path && *path)
		initial_disk_path = path;
	else
		initial_disk_path.clear();

	if (!disk_images.empty()) {
		if (index >= disk_images.size())
			return false;
		if (!initial_disk_path.empty()) {
			for (size_t i = 0; i < disk_images.size(); i++) {
				if (disk_images[i].path == initial_disk_path) {
					initial_disk_index = static_cast<int>(i);
					return true;
				}
			}
			return false;
		}
	}
	return true;
}

static bool libretro_get_image_path(unsigned index, char* s, size_t len)
{
	if (!s || len == 0 || index >= disk_images.size())
		return false;
	if (disk_images[index].path.empty())
		return false;
	strncpy(s, disk_images[index].path.c_str(), len - 1);
	s[len - 1] = '\0';
	return true;
}

static bool libretro_get_image_label(unsigned index, char* s, size_t len)
{
	if (!s || len == 0 || index >= disk_images.size())
		return false;
	std::string label = disk_images[index].label;
	if (label.empty()) {
		if (disk_images[index].path.empty())
			return false;
		label = path_basename(disk_images[index].path);
		const auto dot = label.find_last_of('.');
		if (dot != std::string::npos)
			label = label.substr(0, dot);
	}
	if (label.empty())
		return false;
	strncpy(s, label.c_str(), len - 1);
	s[len - 1] = '\0';
	return true;
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
			const std::string dir = get_log_directory();
			const std::string path = dir.empty() ? std::string("amiberry_libretro_input.log")
				: dir + "/amiberry_libretro_input.log";
			input_log_file = fopen(path.c_str(), "a");
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

static int parse_audio_rate_value(const char* value)
{
	if (!value || strcmp(value, "auto") == 0)
		return 44100;
	const int rate = atoi(value);
	return rate > 0 ? rate : 44100;
}

static int audio_rate_for_av_info()
{
	if (!cached_audio_rate.empty())
		return cached_audio_rate_value;
	return parse_audio_rate_value(get_option_value("amiberry_audio_rate"));
}

static void snapshot_core_options()
{
	const char* model = get_option_value("amiberry_model");
	cached_model = model ? model : "";
	const char* kick = get_option_value("amiberry_kickstart");
	cached_kickstart_override = kick ? kick : "";
	const char* cpu_model = get_option_value("amiberry_cpu_model");
	cached_cpu_model = cpu_model ? cpu_model : "";
	const char* chipset = get_option_value("amiberry_chipset");
	cached_chipset = chipset ? chipset : "";
	const char* chipset_aga = get_option_value("amiberry_chipset_aga");
	cached_chipset_aga = chipset_aga ? chipset_aga : "";
	const char* audio_rate = get_option_value("amiberry_audio_rate");
	cached_audio_rate = audio_rate ? audio_rate : "";
	cached_audio_rate_value = parse_audio_rate_value(audio_rate);
	const char* audio_interpol = get_option_value("amiberry_audio_interpolation");
	cached_audio_interpol = audio_interpol ? audio_interpol : "";
}

static const char* cached_chipset_value()
{
	const bool aga_model = is_aga_model_name(cached_model.empty() ? nullptr : cached_model.c_str());
	const std::string& value = aga_model ? cached_chipset_aga : cached_chipset;
	if (value.empty())
		return nullptr;
	return value.c_str();
}

static bool resolve_kickstart_override_value(const char* opt, char* out, size_t out_size)
{
	if (!opt || !*opt || strcmp(opt, "auto") == 0)
		return false;

	std::string path = opt;
	if (!path_is_absolute(path)) {
		if (system_dir.empty())
			return false;
		path = path_join(system_dir, path);
	}
	if (!file_readable(path.c_str())) {
		if (log_cb)
			log_cb(RETRO_LOG_WARN, "Kickstart override not found: %s\n", path.c_str());
		return false;
	}
	strncpy(out, path.c_str(), out_size - 1);
	out[out_size - 1] = '\0';
	return true;
}

static bool resolve_kickstart_override(char* out, size_t out_size)
{
	return resolve_kickstart_override_value(get_option_value("amiberry_kickstart"), out, out_size);
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
		unsigned joypad_mask = 0;
		if (input_bitmask_supported)
			joypad_mask = (unsigned)input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
		for (const auto& mapping : joypad_map)
		{
			const int state = input_bitmask_supported
				? ((joypad_mask >> mapping.retro_id) & 1)
				: (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, mapping.retro_id) & 1);
			if (state != last_joypad[i][mapping.retro_id]) {
				last_joypad[i][mapping.retro_id] = state;
				log_input_button(i, mapping.name, state);
			}
			setjoybuttonstate(joy, mapping.sdl_button, state);
		}

		const int l2 = input_bitmask_supported
			? ((joypad_mask >> RETRO_DEVICE_ID_JOYPAD_L2) & 1)
			: (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) & 1);
		const int r2 = input_bitmask_supported
			? ((joypad_mask >> RETRO_DEVICE_ID_JOYPAD_R2) & 1)
			: (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) & 1);
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
	libretro_debug_open();
	libretro_debug_log("core_entry start: game_path='%s'\n", game_path);
	std::vector<char*> argv;
	argv.reserve(32);
	argv.push_back(strdup("amiberry"));
	std::string game_ext;
	if (game_path[0])
		game_ext = path_extension_lower(game_path);
	const bool is_whdload = (game_ext == "lha" || game_ext == "lzh");
	const bool user_kick_override = !cached_kickstart_override.empty() && cached_kickstart_override != "auto";

	const char* model = cached_model.empty() ? nullptr : cached_model.c_str();
	if (model)
	{
		argv.push_back(strdup("--model"));
		argv.push_back(strdup(model));
	}

	auto push_s_option = [&argv](const std::string& value) {
		argv.push_back(strdup("-s"));
		argv.push_back(strdup(value.c_str()));
	};

	const char* cpu_model = cached_cpu_model.empty() ? nullptr : cached_cpu_model.c_str();
	if (cpu_model && strcmp(cpu_model, "auto") != 0) {
		push_s_option(std::string("cpu_model=") + cpu_model);
	}

	const char* chipset = cached_chipset_value();
	if (chipset && strcmp(chipset, "auto") != 0) {
		push_s_option(std::string("chipset=") + chipset);
	}

	const char* audio_rate = cached_audio_rate.empty() ? nullptr : cached_audio_rate.c_str();
	if (audio_rate && strcmp(audio_rate, "auto") != 0) {
		push_s_option(std::string("sound_frequency=") + audio_rate);
	}

	const char* audio_interpol = cached_audio_interpol.empty() ? nullptr : cached_audio_interpol.c_str();
	if (audio_interpol && strcmp(audio_interpol, "auto") != 0) {
		push_s_option(std::string("sound_interpol=") + audio_interpol);
	}

	argv.push_back(strdup("-G")); // No GUI

	std::string rom_path_value;
	if (!system_dir.empty()) {
		if (!save_dir.empty()) {
			const std::string kick_dir = path_join(save_dir, "Kickstarts");
			if (dir_exists(kick_dir))
				rom_path_value = kick_dir;
		}
		if (rom_path_value.empty()) {
			const std::string kick_dir = path_join(system_dir, "Kickstarts");
			if (dir_exists(kick_dir))
				rom_path_value = kick_dir;
		}
		if (rom_path_value.empty()) {
			const std::string kick_dir = path_join(system_dir, "save-data/Kickstarts");
			if (dir_exists(kick_dir))
				rom_path_value = kick_dir;
		}
		if (rom_path_value.empty())
			rom_path_value = system_dir;
		const std::string rom_path = "rom_path=" + rom_path_value;
		push_s_option(rom_path);
	}

	{
		char kick_path[1024] = {0};
		bool have_kick = false;

		if (user_kick_override) {
			have_kick = resolve_kickstart_override_value(cached_kickstart_override.c_str(), kick_path, sizeof(kick_path)) ||
				find_kickstart_in_system_dir(model, kick_path, sizeof(kick_path));
		} else if (is_whdload) {
			if (!rom_path_value.empty())
				have_kick = pick_whdload_kickstart(rom_path_value, kick_path, sizeof(kick_path));
		} else {
			have_kick = find_kickstart_in_system_dir(model, kick_path, sizeof(kick_path));
		}

		if (have_kick) {
			argv.push_back(strdup("-r"));
			argv.push_back(strdup(kick_path));
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "Using Kickstart ROM: %s\n", kick_path);
			libretro_debug_log("kickstart override: %s\n", kick_path);
		}
	}
	if (!save_dir.empty()) {
		const std::string cfg_path = "config_path=" + save_dir;
		const std::string saveimage_path = "saveimage_dir=" + save_dir;
		const std::string savestate_path = "savestate_dir=" + save_dir;
		const std::string statefile_path = "statefile_path=" + save_dir;
		argv.push_back(strdup("-s"));
		argv.push_back(strdup(cfg_path.c_str()));
		argv.push_back(strdup("-s"));
		argv.push_back(strdup(saveimage_path.c_str()));
		argv.push_back(strdup("-s"));
		argv.push_back(strdup(savestate_path.c_str()));
		argv.push_back(strdup("-s"));
		argv.push_back(strdup(statefile_path.c_str()));
	}

	if (game_path[0])
	{
		if (is_whdload) {
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "WHDLoad autoload: %s\n", game_path);
			argv.push_back(strdup("--autoload"));
			argv.push_back(strdup(game_path));
		} else {
			argv.push_back(strdup(game_path));
		}
	}
	argv.push_back(nullptr);

	if (libretro_debug_file) {
		libretro_debug_log("core_entry argv:");
		for (size_t i = 0; i + 1 < argv.size(); i++)
			libretro_debug_log(" [%s]", argv[i] ? argv[i] : "");
		libretro_debug_log("\n");
	}
	amiberry_main((int)argv.size() - 1, argv.data());

	// Free strdup'd argv strings
	for (auto p : argv)
		free(p);
	argv.clear();

	// If amiberry_main returns, we are done
	libretro_yield();
}

void retro_init(void)
{
	libretro_debug_open();
	if (!main_fiber)
		main_fiber = co_active();
	
	if (!core_fiber)
		core_fiber = co_create(CORE_FIBER_STACK_SIZE, core_entry);

	if (log_cb)
		log_cb(RETRO_LOG_INFO, "retro_init: main_fiber=%p core_fiber=%p\n", (void*)main_fiber, (void*)core_fiber);
	libretro_debug_log("retro_init: main_fiber=%p core_fiber=%p\n", (void*)main_fiber, (void*)core_fiber);

	core_started = false;
	memory_map_set = false;
	ff_override_active = false;
	last_geometry_width = -1;
	last_geometry_height = -1;
}

void retro_deinit(void)
{
	if (core_fiber)
	{
		co_delete(core_fiber);
		core_fiber = NULL;
	}
	libretro_debug_close();
	update_input_log_file(false);
	last_input_log_file = false;
	fastforward_forced = false;
	last_refresh_rate = -1.0f;
	core_started = false;
	memory_map_set = false;
	ff_override_active = false;
	cheat_entries.clear();
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
	info->library_version  = "v" AMIBERRY_VERSION;
	info->valid_extensions = "adf|uae|ipf|zip|lha|hdf|rp9";
	info->need_fullpath    = true;
	info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	memset(info, 0, sizeof(*info));
	info->geometry.base_width   = DEFAULT_GFX_WIDTH;
	info->geometry.base_height  = DEFAULT_GFX_HEIGHT;
	info->geometry.max_width    = MAX_GFX_WIDTH;
	info->geometry.max_height   = MAX_GFX_HEIGHT;
	info->geometry.aspect_ratio = 4.0f / 3.0f;
	info->timing.fps            = get_core_refresh_rate();
	info->timing.sample_rate    = static_cast<double>(audio_rate_for_av_info());
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
		pixel_format_xrgb8888 = environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
		if (!pixel_format_xrgb8888 && log_cb)
			log_cb(RETRO_LOG_WARN, "Failed to set pixel format XRGB8888; using default.\n");
	}
	static struct retro_keyboard_callback kb_cb = { keyboard_cb };
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb_cb);
	set_core_options();
	{
		struct retro_core_options_update_display_callback cb_info = { core_options_update_display_callback };
		environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &cb_info);
		update_core_option_visibility();
	}
	{
		bool achievements = true;
		environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);
	}
	{
		const char* dir = nullptr;
		if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
			system_dir = dir;
		if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
			save_dir = dir;
		if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &dir) && dir)
			content_dir = dir;
		if (save_dir.empty())
			save_dir = system_dir;
		libretro_debug_open();
		libretro_debug_log("env system_dir='%s' save_dir='%s' content_dir='%s'\n",
			system_dir.c_str(), save_dir.c_str(), content_dir.c_str());
	}

	{
		struct retro_vfs_interface_info vfs_info = {};
		vfs_info.required_interface_version = 3;
		if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_info) && vfs_info.iface) {
			vfs_iface = *vfs_info.iface;
			vfs_available = true;
			libretro_debug_log("env vfs_available=1\n");
			dirent_vfs_init(&vfs_info);
			filestream_vfs_init(&vfs_info);
			path_vfs_init(&vfs_info);
		} else {
			memset(&vfs_iface, 0, sizeof(vfs_iface));
			vfs_available = false;
			libretro_debug_log("env vfs_available=0\n");
		}
	}

	{
		static const struct retro_system_content_info_override overrides[] = {
			{ "lha", true, false },
			{ nullptr, false, false }
		};
		environ_cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void*)overrides);
	}

	ff_override_supported = environ_cb(RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE, NULL);

	{
		struct retro_led_interface led_interface = {};
		if (environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &led_interface))
			led_state_cb = led_interface.set_led_state;
		else
			led_state_cb = nullptr;
	}

	input_bitmask_supported = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);

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
	{
		struct retro_disk_control_ext_callback disk_ext = {};
		disk_ext.set_eject_state = libretro_set_eject_state;
		disk_ext.get_eject_state = libretro_get_eject_state;
		disk_ext.get_image_index = libretro_get_image_index;
		disk_ext.set_image_index = libretro_set_image_index;
		disk_ext.get_num_images = libretro_get_num_images;
		disk_ext.replace_image_index = libretro_replace_image_index;
		disk_ext.add_image_index = libretro_add_image_index;
		disk_ext.set_initial_image = libretro_set_initial_image;
		disk_ext.get_image_path = libretro_get_image_path;
		disk_ext.get_image_label = libretro_get_image_label;
		if (!environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &disk_ext)) {
			struct retro_disk_control_callback disk_cb = {};
			disk_cb.set_eject_state = libretro_set_eject_state;
			disk_cb.get_eject_state = libretro_get_eject_state;
			disk_cb.get_image_index = libretro_get_image_index;
			disk_cb.set_image_index = libretro_set_image_index;
			disk_cb.get_num_images = libretro_get_num_images;
			disk_cb.replace_image_index = libretro_replace_image_index;
			disk_cb.add_image_index = libretro_add_image_index;
			environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &disk_cb);
		}
	}

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
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "retro_run: starting core, core_fiber=%p\n", (void*)core_fiber);
		co_switch(core_fiber);
		core_started = true;
		update_memory_map();
		return;
	}
	if (environ_cb) {
		bool updated = false;
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
			libretro_options_dirty = true;
	}
	if (libretro_options_dirty) {
		apply_libretro_input_options();
		update_core_option_visibility();
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
			set_fastforward_override(fastforwarding);
		}
	}
	poll_input();
	co_switch(core_fiber);
	apply_cheats();
	update_memory_map();
	update_led_interface();
	update_geometry();
}

bool retro_load_game(const struct retro_game_info *info)
{
	libretro_debug_open();
	const struct retro_game_info_ext* info_ext = nullptr;
	if (environ_cb)
		environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_ext);
	snapshot_core_options();

	std::string path;
	if (info_ext && info_ext->full_path && *info_ext->full_path)
		path = info_ext->full_path;
	else if (info && info->path)
		path = info->path;

	const std::string ext = info_ext && info_ext->ext ? info_ext->ext : path_extension_lower(path);
	const bool is_whdload = (ext == "lha" || ext == "lzh");
	libretro_debug_log("retro_load_game: path='%s' ext='%s' is_whdload=%d\n",
		path.c_str(), ext.c_str(), is_whdload ? 1 : 0);

	if (is_whdload) {
		if (log_cb) {
			log_cb(RETRO_LOG_INFO, "WHDLoad content detected: path='%s' ext='%s'\n",
				path.c_str(), ext.c_str());
			if (info_ext)
				log_cb(RETRO_LOG_INFO, "WHDLoad info_ext: full_path='%s' archive_path='%s' archive_file='%s' file_in_archive=%d size=%zu\n",
					info_ext->full_path ? info_ext->full_path : "",
					info_ext->archive_path ? info_ext->archive_path : "",
					info_ext->archive_file ? info_ext->archive_file : "",
					info_ext->file_in_archive ? 1 : 0,
					info_ext->size);
		}
		setup_whdload_paths();

		std::string whd_path = path;
		bool extracted = false;

		if (info_ext && info_ext->data && info_ext->size > 0 &&
			(info_ext->file_in_archive || !info_ext->full_path || !*info_ext->full_path)) {
			const std::string base = info_ext->name ? info_ext->name : "whdload";
			const std::string out_dir = !save_dir.empty() ? save_dir : (!system_dir.empty() ? system_dir : (!content_dir.empty() ? content_dir : "."));
			const std::string out_file = "whdload_" + sanitize_filename(base) + "." + (info_ext->ext ? info_ext->ext : "lha");
			const std::string out_path = path_join(out_dir, out_file);
			if (vfs_write_file(out_path.c_str(), info_ext->data, info_ext->size)) {
				whd_path = out_path;
				extracted = true;
			} else if (log_cb) {
				log_cb(RETRO_LOG_WARN, "Failed to write WHDLoad data to %s\n", out_path.c_str());
			}
		} else if (info_ext && info_ext->file_in_archive && info_ext->archive_path && info_ext->archive_file) {
			std::string vfs_path = std::string(info_ext->archive_path) + "#" + info_ext->archive_file;
			std::vector<uint8_t> data;
			if (vfs_read_all(vfs_path.c_str(), data)) {
				const std::string base = info_ext->name ? info_ext->name : path_basename(info_ext->archive_file);
				const std::string out_dir = !save_dir.empty() ? save_dir : (!system_dir.empty() ? system_dir : (!content_dir.empty() ? content_dir : "."));
				const std::string out_file = "whdload_" + sanitize_filename(base) + "." + ext;
				const std::string out_path = path_join(out_dir, out_file);
				if (vfs_write_file(out_path.c_str(), data.data(), data.size())) {
					whd_path = out_path;
					extracted = true;
				} else if (log_cb) {
					log_cb(RETRO_LOG_WARN, "Failed to write WHDLoad file to %s\n", out_path.c_str());
				}
			}
		}

		if (!extracted && (has_non_ascii(whd_path) || !file_readable(whd_path.c_str()))) {
			std::vector<uint8_t> data;
			if (vfs_read_all(whd_path.c_str(), data)) {
				const std::string base = path_basename(whd_path);
				const std::string out_dir = !save_dir.empty() ? save_dir : (!system_dir.empty() ? system_dir : (!content_dir.empty() ? content_dir : "."));
				const std::string out_file = "whdload_" + sanitize_filename(base);
				const std::string out_path = path_join(out_dir, out_file);
				if (vfs_write_file(out_path.c_str(), data.data(), data.size())) {
					whd_path = out_path;
					extracted = true;
				} else if (log_cb) {
					log_cb(RETRO_LOG_WARN, "Failed to write WHDLoad temp file to %s\n", out_path.c_str());
				}
			} else if (log_cb) {
				log_cb(RETRO_LOG_WARN, "Failed to read WHDLoad path via VFS: %s\n", whd_path.c_str());
			}
		}

		if (!whd_path.empty()) {
			whdload_temp_path = extracted ? whd_path : std::string();
			libretro_debug_log("WHDLoad using path: %s (extracted=%d)\n", whd_path.c_str(), extracted ? 1 : 0);
			disk_images.clear();
			disk_index = 0;
			disk_ejected = false;
			last_disk_index = 0;
			last_disk_ejected = false;
			DiskImage image;
			image.path = whd_path;
			disk_images.push_back(image);
			strncpy(game_path, whd_path.c_str(), sizeof(game_path) - 1);
			game_path[sizeof(game_path) - 1] = '\0';
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "WHDLoad using path: %s\n", game_path);
			return true;
		}
	}

	if (!path.empty())
	{
		disk_images.clear();
		disk_index = 0;
		disk_ejected = false;
		last_disk_index = 0;
		last_disk_ejected = false;

		const auto ext_pos = path.find_last_of('.');
		const std::string path_ext = (ext_pos == std::string::npos) ? "" : path.substr(ext_pos);
		if (path_ext == ".m3u" || path_ext == ".m3u8") {
			if (!parse_m3u(path.c_str(), disk_images)) {
				if (log_cb)
					log_cb(RETRO_LOG_WARN, "Failed to parse m3u, using content path\n");
				DiskImage image;
				image.path = path;
				disk_images.push_back(image);
			}
		} else {
			DiskImage image;
			image.path = path;
			disk_images.push_back(image);
		}

		if (!disk_images.empty()) {
			if (!initial_disk_path.empty()) {
				for (size_t i = 0; i < disk_images.size(); i++) {
					if (disk_images[i].path == initial_disk_path) {
						disk_index = static_cast<unsigned>(i);
						break;
					}
				}
			} else if (initial_disk_index >= 0 && static_cast<unsigned>(initial_disk_index) < disk_images.size()) {
				disk_index = static_cast<unsigned>(initial_disk_index);
			}
			strncpy(game_path, disk_images[disk_index].path.c_str(), sizeof(game_path) - 1);
			game_path[sizeof(game_path) - 1] = '\0';
		}
		else
			game_path[0] = '\0';
	}
	else
		game_path[0] = '\0';

	// No content is valid: Amiga can boot to Workbench with just a Kickstart ROM
	if (game_path[0] == '\0' && log_cb)
		log_cb(RETRO_LOG_INFO, "No game content provided; booting to Workbench.\n");

	return true;
}

void retro_unload_game(void)
{
	uae_quit();
	cheat_entries.clear();
	if (!whdload_temp_path.empty()) {
		if (vfs_available && vfs_iface.remove)
			vfs_iface.remove(whdload_temp_path.c_str());
		else
			remove(whdload_temp_path.c_str());
		whdload_temp_path.clear();
	}
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
	cheat_entries.clear();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	if (index >= cheat_entries.size())
		cheat_entries.resize(index + 1);

	CheatEntry &cheat = cheat_entries[index];
	cheat.enabled = enabled;
	cheat.ops.clear();

	if (!code || !*code)
		return;

	if (!parse_cheat_code(code, cheat.ops)) {
		cheat.enabled = false;
		if (log_cb)
			log_cb(RETRO_LOG_WARN, "Failed to parse cheat code: %s\n", code);
		return;
	}

	if (log_cb)
		log_cb(RETRO_LOG_INFO, "Cheat %u %s: %s\n", index, enabled ? "enabled" : "disabled", code);
}
