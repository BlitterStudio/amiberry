// Run from the repository root on Linux with a linkable static Libretro core
// (build-essential, zlib1g-dev, nlohmann-json3-dev):
//   tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
//   make -C libretro -j8 STATIC_LINKING=1 NO_JIT=1 \
//     OBJDIR="$tmp/obj" NLOHMANN_JSON_INCLUDE=/usr/include \
//     EXTRA_SOURCES=../tests/libretro_controller_content_test.cpp \
//     TARGET="$tmp/core.a" "$tmp/core.a"
//   c++ -Wl,--gc-sections -o "$tmp/test" "$tmp/core.a" -lz -ldl -lm -pthread -lutil
//   mkdir "$tmp/run"
//   (cd "$tmp/run" && env -u AMIBERRY_CONFIG_DIR AMIBERRY_HOME_DIR="$PWD" \
//     HOME="$PWD" XDG_CONFIG_HOME="$PWD" XDG_DATA_HOME="$PWD" ../test)

#include "libretro.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::string work_dir;
retro_disk_control_ext_callback disk_control{};
unsigned cd32_device;
unsigned joystick_device;
const char* deadzone = "33";
bool options_updated;
unsigned pad;

void expect(unsigned actual, unsigned expected, const char* message)
{
	if (actual != expected) {
		std::fprintf(stderr, "%s: expected %#x, got %#x\n", message, expected, actual);
		std::exit(EXIT_FAILURE);
	}
}

bool environment(unsigned cmd, void* data)
{
	switch (cmd) {
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
	case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:
		*static_cast<const char**>(data) = work_dir.c_str();
		return true;
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
		return *static_cast<const retro_pixel_format*>(data) == RETRO_PIXEL_FORMAT_XRGB8888;
	case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
		disk_control = *static_cast<const retro_disk_control_ext_callback*>(data);
		return true;
	case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: {
		const auto& port = *static_cast<const retro_controller_info*>(data);
		for (unsigned i = 0; i < port.num_types; ++i) {
			const auto& device = port.types[i];
			if (std::strcmp(device.desc, "CD32 Pad") == 0)
				cd32_device = device.id;
			else if (std::strcmp(device.desc, "Joystick") == 0)
				joystick_device = device.id;
		}
		return true;
	}
	case RETRO_ENVIRONMENT_GET_VARIABLE: {
		auto& var = *static_cast<retro_variable*>(data);
		var.value = nullptr;
		if (std::strcmp(var.key, "amiberry_model") == 0)
			var.value = "CD32";
		else if (std::strcmp(var.key, "amiberry_port0_device") == 0)
			var.value = "mouse";
		else if (std::strcmp(var.key, "amiberry_port1_device") == 0)
			var.value = "joystick";
		else if (std::strcmp(var.key, "amiberry_joyport_order") == 0)
			var.value = "auto";
		else if (std::strcmp(var.key, "amiberry_joy_deadzone") == 0)
			var.value = deadzone;
		return var.value != nullptr;
	}
	case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
		*static_cast<bool*>(data) = options_updated;
		options_updated = false;
		return true;
	default:
		return false;
	}
}

constexpr uaecptr ciaa_pra = 0xbfe001;
constexpr uaecptr ciaa_ddra = 0xbfe201;
constexpr uaecptr potgor = 0xdff016;
constexpr uaecptr potgo = 0xdff034;

unsigned port1_serial()
{
	// Same register sequence as ipc_joystick_live_test.py: charging P5
	// resets the shifter; then pull P5 low and clock the active-low bits.
	put_word(potgo, 0xff00);
	for (int i = 0; i < 8; ++i)
		get_word(potgor);
	put_byte(ciaa_pra, 0x02);
	put_byte(ciaa_ddra, 0x80);
	put_word(potgo, 0x2f00);
	for (int i = 0; i < 8; ++i)
		get_word(potgor);
	unsigned packet = 0;
	for (unsigned bit = 0; bit < 9; ++bit) {
		if (get_word(potgor) & 0x4000)
			packet |= 1U << bit;
		put_byte(ciaa_pra, 0x82);
		put_byte(ciaa_pra, 0x02);
	}
	put_byte(ciaa_ddra, 0);
	return packet;
}

void expect_cd32_play(const char* message)
{
	pad = 1U << RETRO_DEVICE_ID_JOYPAD_START;
	retro_run();
	expect(port1_serial(), 0xbf, message); // Play is bit 6; CD32 ID is bits 7–8.
	pad = 0;
	retro_run();
	expect(port1_serial(), 0xff, "released CD32 Play and unchanged pad ID");
}

} // namespace

int main()
{
	work_dir = std::filesystem::current_path().string();
	const auto rom_path = work_dir + "/idle.rom";
	const auto config_path = work_dir + "/game.UAE";
	{
		// Reset vectors, then MOVE #$2700,SR; BRA * in ROM. No guest program
		// touches the joystick registers between frontend frames.
		constexpr unsigned char boot[] = {
			0x00, 0x07, 0xf0, 0x00, 0x00, 0xf8, 0x00, 0x08,
			0x46, 0xfc, 0x27, 0x00, 0x60, 0xfe
		};
		std::ofstream rom(rom_path, std::ios::binary);
		rom.write(reinterpret_cast<const char*>(boot), sizeof boot);
		rom.seekp(512 * 1024 - 1);
		rom.put('\0');
		rom.close();
		expect(rom.good(), true, "write synthetic idle ROM");
	}
	{
		std::ofstream config(config_path);
		config << "config_version=8.5.0\nconfig_hardware=true\nconfig_host=true\n"
			"chipset_compatible=A500\nchipset=ocs\ncpu_model=68000\n"
			"cd32cd=false\ncd32c2p=false\ncd32nvram=false\n"
			"ntsc=true\nuse_gui=yes\njoyport1mode=djoy\nkickstart_rom_file=" << rom_path << '\n';
		config.close();
		expect(config.good(), true, "write uppercase .UAE fixture");
	}

	retro_set_environment(environment);
	retro_set_video_refresh([](const void*, unsigned, unsigned, size_t) {});
	retro_set_audio_sample_batch([](const int16_t*, size_t frames) { return frames; });
	retro_set_input_poll([] {});
	retro_set_input_state([](unsigned port, unsigned device, unsigned, unsigned id) -> int16_t {
		return port == 0 && device == RETRO_DEVICE_JOYPAD && ((pad >> id) & 1);
	});
	retro_init();
	expect(disk_control.get_num_images != nullptr, true, "disk-control interface advertised");
	expect(cd32_device != 0 && joystick_device != 0 && cd32_device != joystick_device,
		true, "frontend port 0 advertises both controller subtypes");
	retro_set_controller_port_device(0, cd32_device);
	const retro_game_info content{config_path.c_str(), nullptr, 0, nullptr};
	expect(retro_load_game(&content), true, "load .UAE with CD32 selected in core options");
	expect(disk_control.get_num_images(), 0, "configuration is not a disk-control image");
	for (int i = 0; i < 3; ++i)
		retro_run();
	expect(retro_get_region(), RETRO_REGION_NTSC, "configuration applied its NTSC preference");
	expect(currprefs.start_gui, false, "core stays headless even when configuration requests GUI");
	expect(currprefs.cdslots[0].name[0], 0, "configuration was not mounted as a CD image");
	expect(disk_control.get_num_images(), 0, "no configuration image after boot");
	expect_cd32_play("saved frontend CD32 selection reaches the Amiga port at boot");

	retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
	retro_run();
	expect(port1_serial(), 0x1ff, "generic RetroPad uses the standard joystick from the configuration");

	put_byte(ciaa_ddra, 0);
	pad = 1U << RETRO_DEVICE_ID_JOYPAD_B;
	retro_run();
	expect(get_byte(ciaa_pra) & 0xc0, 0x40, "frontend 0 fire goes to Amiga 1, not Amiga 0");
	pad = 0;
	retro_run();

	retro_set_controller_port_device(0, cd32_device);
	expect_cd32_play("frontend 0 CD32 Play reaches Amiga 1's serial pad after live change");
	deadzone = "40";
	options_updated = true;
	expect_cd32_play("CD32 subtype survives dirty core options");
	expect(currprefs.input_joystick_deadzone, 40, "dirty joystick deadzone was applied");

	retro_set_controller_port_device(0, joystick_device);
	retro_run();
	expect(port1_serial(), 0x1ff, "explicit Joystick removes the CD32 serial pad");

	retro_unload_game();

	{
		std::ofstream config(config_path, std::ios::app);
		config << "joyport1mode=cd32joy\n";
		config.close();
		expect(config.good(), true, "configure the Amiga joystick port for CD32");
	}
	retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
	expect(retro_load_game(&content), true, "load a configuration that requests a CD32 pad");
	for (int i = 0; i < 3; ++i)
		retro_run();
	expect_cd32_play("generic RetroPad uses the configured CD32 pad");
	retro_set_controller_port_device(0, joystick_device);
	retro_run();
	expect(port1_serial(), 0x1ff, "explicit Joystick overrides the configured CD32 pad");
	retro_unload_game();
	retro_deinit();
	std::puts("Libretro controller and direct .UAE regression passed");
}
