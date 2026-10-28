// Harness for the shared settings layer (src/osdep/settings). Built by
// tests/lib/settings_layer_build.sh against the layer sources and the core's
// fixup_cpu(), sliced out of src/main.cpp.
//
// Without arguments it checks descriptors, rules, sessions and validation.
// With --emit-roundtrip it prints configurations built through the layer for
// tests/test_settings_roundtrip.sh, which feeds them to a real binary.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "uae/ppc.h"
#include "memory.h"
#include "newcpu.h"
#include "cpuboard.h"
#include "autoconf.h"
#include "custom.h"
#include "disk.h"
#include "blkdev.h"
#include "x86.h"
#include "gfxboard.h"
#include "rtgmodes.h"
#include "ethernet.h"
#include "rommgr.h"
#include "amiberry_gfx_mode.h"
#include "amiberry_gfx.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "sounddep/sound.h"

#include "settings/amiberry_settings.h"
#include "settings/settings_definitions.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "settings/settings_actions_queue.h"
#include "amiga_constants.h"
#include "display_catalog.h"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

// The globals and core functions the layer and fixup_cpu() use. The
// stand-in preview runs the real CPU fixup, the part of fixup_prefs() the
// CPU options depend on.
uae_prefs changed_prefs;
uae_prefs currprefs;
int emulating;
// uae_prefs is too large for the stack.
static uae_prefs test_prefs;

// Besides the real fixup_cpu(), the stand-in applies fixup_prefs()'s rule
// that a 24-bit address space removes Z3 RAM, so commit is tested on an
// adjustment outside the CPU options.
void fixup_prefs_preview(uae_prefs* p)
{
	fixup_cpu(p);
	if (p->address_space_24)
		p->z3fastmem[0].size = 0;
}
void error_log(const TCHAR*, ...) {}
void write_log(const TCHAR*, ...) {}
// Controlled by the tests: whether the selected accelerator board is 32-bit
// only, and the address of a UAE boot ROM the configuration needs (0: none).
static bool board_is_32bit;
static uaecptr boot_rom_address;
int uae_boot_rom_type;
bool cpuboard_32bit(uae_prefs*) { return board_is_32bit; }
bool cpuboard_is_ppc_accelerator(uae_prefs* p)
{
	return cpuboard_is_ppc_accelerator_subtype(p->cpuboard_type, p->cpuboard_subtype);
}
void cpuboard_setboard(uae_prefs* p, int type, int subtype)
{
	p->cpuboard_type = type;
	p->cpuboard_subtype = subtype;
}
// Like the real one, records the boot ROM type in a global.
uaecptr need_uae_boot_rom(uae_prefs*)
{
	uae_boot_rom_type = boot_rom_address ? 1 : 0;
	return boot_rom_address;
}
bool is_ppc_cpu(uae_prefs*) { return false; }
 int is_x86_cpu(uae_prefs*) { return 0; }

// Host state the display, sound and input domains read: one test
// joystick, one test mouse, two test audio devices, no extra monitors.
bool kmsdrm_detected;
struct MultiDisplay Displays[MAX_DISPLAYS + 1];
int max_uae_width = 8192, max_uae_height = 8192;
static int joystick_total = 1, mouse_total = 1;
int inputdevice_get_device_total(int type)
{
	return type == IDTYPE_JOYSTICK ? joystick_total : type == IDTYPE_MOUSE ? mouse_total : 0;
}
const TCHAR* inputdevice_get_device_name(int, int) { return "Test Device"; }
// Joysticks have unique names (swap_joysticks reorders them, as a hotplug
// can); other devices share one.
static bool swap_joysticks = false;
const TCHAR* inputdevice_get_device_unique_name(int type, int index)
{
	if (type == IDTYPE_JOYSTICK && (index == 0 || index == 1))
		return (index == 0) != swap_joysticks ? "pad-a" : "pad-b";
	return "test-unique";
}
struct didata di_joystick[MAX_INPUT_DEVICES];
bool joystick_refresh_needed;
void inputdevice_validate_jports_prefs_only(uae_prefs*, int) {}
// Host settings: the process-wide options and the content folders.
struct amiberry_options amiberry_options;
static int host_saves = 0;
static bool host_save_result = true;
bool save_amiberry_settings_with_result() { host_saves++; return host_save_result; }
static std::map<std::string, std::string> host_paths;
#define HOST_PATH(get, set, key) \
	std::string get() { return host_paths[key]; } \
	void set(const std::string& path) { host_paths[key] = path; }
HOST_PATH(get_base_content_path, set_base_content_path, "base")
HOST_PATH(get_configuration_path, set_configuration_path, "config")
HOST_PATH(get_controllers_path, set_controllers_path, "controllers")
HOST_PATH(get_retroarch_file, set_retroarch_file, "retroarch")
HOST_PATH(get_whdbootpath, set_whdbootpath, "whdboot")
HOST_PATH(get_whdload_arch_path, set_whdload_arch_path, "whdload")
HOST_PATH(get_floppy_path, set_floppy_path, "floppy")
HOST_PATH(get_harddrive_path, set_harddrive_path, "harddrive")
HOST_PATH(get_cdrom_path, set_cdrom_path, "cdrom")
HOST_PATH(get_logfile_path, set_logfile_path, "logfile")
HOST_PATH(get_rom_path, set_rom_path, "rom")
HOST_PATH(get_rp9_path, set_rp9_path, "rp9")
HOST_PATH(get_floppy_sounds_path, set_floppy_sounds_path, "sounds")
HOST_PATH(get_saveimage_path, set_saveimage_path, "saveimage")
HOST_PATH(get_savestate_path, set_savestate_path, "savestate")
std::string get_savestate_folder() { return host_paths["savestate"]; }
HOST_PATH(get_ripper_path, set_ripper_path, "ripper")
HOST_PATH(get_inputrecordings_path, set_inputrecordings_path, "recordings")
HOST_PATH(get_screenshot_path, set_screenshot_path, "screenshot")
HOST_PATH(get_nvram_path, set_nvram_path, "nvram")
HOST_PATH(get_plugins_override_path, set_plugins_path, "plugins")
HOST_PATH(get_video_path, set_video_path, "video")
HOST_PATH(get_themes_path, set_themes_path, "themes")
HOST_PATH(get_shaders_path, set_shaders_path, "shaders")
HOST_PATH(get_bezels_path, set_bezels_path, "bezels")
#undef HOST_PATH

// Custom controls: three events, and the mappings the input code holds per
// port (for the device it was set for, as the real store keys them).
extern const int remap_event_list[] = { INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_KEY_SPACE, INPUTEVENT_JOY2_UP };
extern const int remap_event_list_size = 3;
static const inputevent test_events[] = {
	{ "JOY2_FIRE_BUTTON", "Joy2 Fire/Mouse2 Left Button" },
	{ "KEY_SPACE", "Space" },
	{ "JOY2_UP", "Joy2 Up" },
};
const inputevent* inputdevice_get_eventinfo(int evt)
{
	for (int i = 0; i < remap_event_list_size; i++) {
		if (remap_event_list[i] == evt)
			return &test_events[i];
	}
	return nullptr;
}
static controller_mapping test_port_mappings[MAX_JPORTS];
static std::string test_port_mapping_devices[MAX_JPORTS];
static int input_config_updates = 0;
bool amiberry_get_port_joystick_custom_mapping(int portnum, const TCHAR*, const TCHAR* configname,
	controller_mapping& mapping)
{
	if (portnum < 0 || portnum >= MAX_JPORTS || test_port_mapping_devices[portnum] != configname)
		return false;
	mapping = test_port_mappings[portnum];
	return true;
}
void amiberry_set_port_joystick_custom_mapping(int portnum, const TCHAR*, const TCHAR* configname,
	const controller_mapping& mapping)
{
	test_port_mappings[portnum] = mapping;
	test_port_mapping_devices[portnum] = configname;
}
void inputdevice_updateconfig(struct uae_prefs*, struct uae_prefs*)
{
	input_config_updates++;
}
static struct sound_device test_sound_devices[2] = {
	{ {}, 0, const_cast<TCHAR*>("Test Speaker"), nullptr, const_cast<TCHAR*>("speaker"), nullptr,
		0, SOUND_DEVICE_SDL },
	{ {}, 1, const_cast<TCHAR*>("Test Headset"), nullptr, const_cast<TCHAR*>("headset"), nullptr,
		0, SOUND_DEVICE_SDL },
};
struct sound_device* sound_devices[MAX_SOUND_DEVICES] = { &test_sound_devices[0],
	&test_sound_devices[1] };
// A refresh can reorder the host's devices; swap_sound_devices simulates a
// device ahead of the selected one going away and coming back after it.
static bool swap_sound_devices = false;
void refresh_sound_devices()
{
	if (swap_sound_devices) {
		std::swap(sound_devices[0], sound_devices[1]);
		std::swap(record_devices[0], record_devices[1]);
	}
}
static int sound_device_count = 2;
int enumerate_sound_devices() { return sound_device_count; }
// Two FloppyBridge drivers: the first supports everything, the second
// needs a serial port and supports nothing else.
static int drawbridge_drivers = 2;
static bool drawbridge_shugart = true;
int drawbridge_driver_count(bool) { return drawbridge_drivers; }
const char* drawbridge_driver_name(int index) { return index == 0 ? "DrawBridge" : "Greaseweazle"; }
unsigned int drawbridge_driver_options(int index)
{
	if (index < 0 || index >= drawbridge_drivers)
		return 0;
	return index == 0 ? (drawbridge_shugart ? 0x3f : 0x1f) : DRAWBRIDGE_OPTION_COMPORT;
}

// The memory window the memory domain clamps against; the real value is
// host-dependent, the tests use the common 512 MB reservation.
size_t max_z3fastmem = 512 * 1024 * 1024;

// The accelerator board table, laid out like the core's (expansion.cpp,
// NCR9X builds): board IDs are table indices, Blizzard (9) and CyberStorm
// (10) carry their PPC variants at the core's indices, and the variants the
// round-trip scenarios use have the core's configuration names. Board 1
// stands for the others; boards 2-8 have no variants here.
static const expansionboardsettings test_maprom[] = {
	{"MapROM", "maprom"}, {nullptr}};
static const expansionboardsettings test_maprom_scsikit[] = {
	{"MapROM", "maprom"}, {"SCSI Kit II", "scsikitii"}, {nullptr}};
static const expansionboardsettings test_gvp_jumpers[] = {
	{"Board disable (J5)", "disabled"}, {"SCSI Disable (J6)", "scsidisabled"}, {nullptr}};
// Like the IVS Vector's: a selector, a switch, and an inverted switch one
// bit further up.
static const expansionboardsettings test_vector_jumpers[] = {
	{"Memory (JP12)\0" "4M\0" "8M\0" "16M\0" "32M\0", "memory\0" "4m\0" "8m\0" "16m\0" "32m\0",
		EXPANSIONBOARD_MULTI, false, 0},
	{"Autoboot (JP20)", "autoboot", EXPANSIONBOARD_CHECKBOX, false, 0},
	{"Burst (JP13)", "burst", EXPANSIONBOARD_CHECKBOX, true, 1},
	{nullptr}};
// Like the Apollo 12xx's: a "memory" switch, so the selector above becomes
// "memory_choice".
static const expansionboardsettings test_apollo_jumpers[] = {
	{"SCSI", "scsi"}, {"Memory", "memory"}, {"Autoboot disable", "autoboot"},
	{"Revision\0" "3\0" "4\0", "hwrev\0" "3\0" "4\0", EXPANSIONBOARD_MULTI, false, 0}, {nullptr}};
// The same selector as the Apollo's, with other positions.
static const expansionboardsettings test_z2_jumpers[] = {
	{"Revision\0" "4\0" "5\0", "hwrev\0" "4\0" "5\0", EXPANSIONBOARD_MULTI, false, 0}, {nullptr}};
static const cpuboardsubtype test_dummy_sub[] = {
	{"-", "none", 0, 0, 0, nullptr, 0, 0, 0}, {nullptr}};
static const cpuboardsubtype test_board1_sub[] = {
	{"Test 030", "Test030", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_HIGHMEM, 128 * 1024 * 1024, 0,
		nullptr, nullptr, 0, 0, test_maprom},
	{"Test 040", "Test040", ROMTYPE_CPUBOARD, 0, 4, nullptr, 0, BOARD_MEMORY_HIGHMEM, 64 * 1024 * 1024, 0,
		nullptr, nullptr, 0, 0, test_gvp_jumpers},
	{"Test Z2", "TestZ2", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_Z2, 8 * 1024 * 1024, 0,
		nullptr, nullptr, 0, 0, test_z2_jumpers},
	{"Test 25-bit", "Test25", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_25BITMEM, 128 * 1024 * 1024, 0,
		nullptr, nullptr, 0, 0, test_vector_jumpers},
	{"Test Apollo", "TestApollo", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_HIGHMEM, 32 * 1024 * 1024, 0,
		nullptr, nullptr, 0, 0, test_apollo_jumpers},
	{nullptr}};
static const cpuboardsubtype test_blizzard_sub[] = {
	{"Blizzard 1230 I/II", "Blizzard1230II", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_BLIZZARD_12xx,
		64 * 1024 * 1024, 0, nullptr, nullptr, 0, 0, test_maprom_scsikit},
	{"Blizzard 1230 III", "Blizzard1230III", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_BLIZZARD_12xx,
		32 * 1024 * 1024, 0, nullptr, nullptr, 0, 0, test_maprom},
	{"Blizzard 1230 IV", "Blizzard1230IV", ROMTYPE_CPUBOARD, 0, 3, nullptr, 0, BOARD_MEMORY_BLIZZARD_12xx,
		256 * 1024 * 1024, 0, nullptr, nullptr, 0, 0, test_maprom},
	{"Blizzard 1260", "Blizzard1260", ROMTYPE_CPUBOARD, 0, 6, nullptr, 0, BOARD_MEMORY_BLIZZARD_12xx,
		256 * 1024 * 1024, 0, nullptr, nullptr, 0, 0, test_maprom},
	{"Blizzard 2060", "Blizzard2060", ROMTYPE_CPUBOARD, 0, 6, nullptr, 0, BOARD_MEMORY_HIGHMEM,
		128 * 1024 * 1024, 0, nullptr, nullptr, 0, 0, test_maprom},
	{"Blizzard PPC", "BlizzardPPC", ROMTYPE_CPUBOARD, 0, 4, nullptr, 0, BOARD_MEMORY_BLIZZARD_PPC,
		256 * 1024 * 1024},
	{nullptr}};
static const cpuboardsubtype test_cyberstorm_sub[] = {
	{"CyberStorm MK I", "CyberStormMK1", ROMTYPE_CPUBOARD, 0, 4, nullptr, 0, BOARD_MEMORY_HIGHMEM, 128 * 1024 * 1024},
	{"CyberStorm MK II", "CyberStormMK2", ROMTYPE_CPUBOARD, 0, 4, nullptr, 0, BOARD_MEMORY_HIGHMEM, 128 * 1024 * 1024},
	{"CyberStorm MK III", "CyberStormMK3", ROMTYPE_CPUBOARD, 0, 4, nullptr, 0, BOARD_MEMORY_HIGHMEM, 128 * 1024 * 1024},
	{"CyberStorm PPC", "CyberStormPPC", ROMTYPE_CPUBOARD, 0, 4, nullptr, 0, BOARD_MEMORY_HIGHMEM, 128 * 1024 * 1024},
	{nullptr}};
const cpuboardtype cpuboards[] = {
	{0, "-", test_dummy_sub}, {1, "Test Maker", test_board1_sub},
	{2, "Board 2"}, {3, "Board 3"}, {4, "Board 4"}, {5, "Board 5"}, {6, "Board 6"}, {7, "Board 7"}, {8, "Board 8"},
	{BOARD_BLIZZARD, "Phase 5 - Blizzard", test_blizzard_sub},
	{BOARD_CYBERSTORM, "Phase 5 - CyberStorm", test_cyberstorm_sub},
	{0, nullptr}};

// The host's serial ports (a device and the network entries the core always
// lists), MIDI ports and audio inputs; enumerations count.
std::vector<std::string> serial_ports = {"/dev/cu.usbserial", "TCP://0.0.0.0:1234"};
std::vector<std::string> midi_in_ports = {"Keyboard In"};
std::vector<std::string> midi_out_ports = {"Munt MT-32", "Synth Out"};
static int io_enumerations;
int enumserialports() { io_enumerations++; return static_cast<int>(serial_ports.size()); }
// Like the real one: the lists are rebuilt, not appended to.
int enummidiports()
{
	io_enumerations++;
	midi_in_ports.clear();
	midi_out_ports.clear();
	midi_in_ports.emplace_back("Keyboard In");
	midi_out_ports.emplace_back("Munt MT-32");
	midi_out_ports.emplace_back("Synth Out");
	return 2;
}
static struct sound_device test_record_devices[2] = {
	{ {}, 0, const_cast<TCHAR*>("Test Microphone"), nullptr, const_cast<TCHAR*>("microphone"), nullptr, 0, SOUND_DEVICE_SDL },
	{ {}, 1, const_cast<TCHAR*>("Test Line In"), nullptr, const_cast<TCHAR*>("linein"), nullptr, 0, SOUND_DEVICE_SDL },
};
struct sound_device* record_devices[MAX_SOUND_DEVICES] = { &test_record_devices[0], &test_record_devices[1] };

// The shaders and bezels folders: two built-in shaders and a preset, one
// bezel image; an invalidate counts as a rescan.
static int display_catalog_rescans;
const std::vector<std::string>& get_available_shader_names()
{
	static const std::vector<std::string> names = {"none", "tv", "pc", "crt/crt-geom.glslp"};
	return names;
}
void invalidate_shader_catalog() { display_catalog_rescans++; }
const std::vector<std::string>& get_available_bezel_names()
{
	static const std::vector<std::string> names = {"none", "frames/arcade.png"};
	return names;
}
void invalidate_bezel_catalog() { display_catalog_rescans++; }

// A few entries of the core's expansion board table (expansion.cpp), laid
// out like it: the accelerator (no expansion board), an A2091 with a 24-bit
// DMA variant, a Zorro III SCSI board with an ID jumper and a switch, a
// selector and a text setting, a PCMCIA card without a ROM, and the
// built-in IDE controller.
static const expansionsubromtype test_a2091_sub[] = {
	{"DMAC-01", "dmac01", ROMTYPE_A2091, 0, 0, 0, false, 0},
	{"DMAC-02", "dmac02", ROMTYPE_A2091, 0, 0, 0, false, EXPANSIONTYPE_DMA24},
	{nullptr}};
static const expansionboardsettings test_scsi_settings[] = {
	{"Fast SCSI", "fast"},
	{"Speed\0" "Slow\0" "Fast\0", "speed\0" "slow\0" "fast\0", EXPANSIONBOARD_MULTI, false, 0},
	{"Name", "name", EXPANSIONBOARD_STRING, false, 0},
	{nullptr}};
const expansionromtype expansionroms[] = {
	{"cpuboard", "Accelerator", "Accelerator", nullptr, nullptr, nullptr, nullptr, ROMTYPE_CPUBOARD, 0, 0, 0, true,
		nullptr, 0, false, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE},
	{"a2091", "A590/A2091", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_A2091, 0, 0, 2, false,
		test_a2091_sub, 0, true, EXPANSIONTYPE_SCSI},
	{"testscsi", "Test SCSI", "Tester", nullptr, nullptr, nullptr, nullptr, ROMTYPE_A4091, 0, 0, 3, true,
		nullptr, 0, false, EXPANSIONTYPE_SCSI, 0, 0, 0, false, nullptr, true, 0, test_scsi_settings},
	{"testpcmcia", "Test PCMCIA", nullptr, nullptr, nullptr, nullptr, nullptr, ROMTYPE_NE2KPCMCIA | ROMTYPE_NOT, 0, 0, 0,
		true, nullptr, 0, false, EXPANSIONTYPE_NET | EXPANSIONTYPE_PCMCIA},
	{"ide_mb", "A600/A1200/A4000 IDE", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_MB_IDE | ROMTYPE_NOT,
		0, 0, BOARD_NONAUTOCONFIG_BEFORE, true, nullptr, 0, false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_IDE},
	{"scsi_a3000", "A3000 SCSI", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_SCSI_A3000 | ROMTYPE_NOT,
		0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true, nullptr, 0, false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_SCSI},
	{"scsi_a4000t", "A4000T SCSI", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_SCSI_A4000T | ROMTYPE_NOT,
		0, 0, BOARD_NONAUTOCONFIG_BEFORE, true, nullptr, 0, false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_SCSI},
	{"cd32fmv", "CD32 FMV", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_CD32CART, 0, 0,
		BOARD_AUTOCONFIG_Z2, true, nullptr, 0, false, EXPANSIONTYPE_INTERNAL},
	{"cdtvdmac", "CDTV DMAC", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_CDTVDMAC | ROMTYPE_NOT, 0, 0,
		BOARD_AUTOCONFIG_Z2, true, nullptr, 0, false, EXPANSIONTYPE_INTERNAL},
	{"cdtvcr", "CDTV-CR", "Commodore", nullptr, nullptr, nullptr, nullptr, ROMTYPE_CDTVCR | ROMTYPE_NOT, 0, 0,
		BOARD_NONAUTOCONFIG_AFTER_Z2, true, nullptr, 0, false, EXPANSIONTYPE_INTERNAL},
	{nullptr}};
void ethernet_updateselection() {}

// The board ROM lookups, over the configuration's board list like the
// core's (rommgr.cpp), for boards without a parent ROM type.
boardromconfig* get_device_rom(uae_prefs* p, int romtype, int devnum, int* index)
{
	if (index)
		*index = 0;
	for (boardromconfig& brc : p->expansionboard) {
		if (brc.device_type && (brc.device_type & ROMTYPE_MASK) == (romtype & ROMTYPE_MASK) && brc.device_num == devnum)
			return &brc;
	}
	return nullptr;
}
void clear_device_rom(uae_prefs* p, int romtype, int devnum, bool delete_device)
{
	int index = 0;
	boardromconfig* brc = get_device_rom(p, romtype, devnum, &index);
	if (!brc)
		return;
	if (delete_device)
		std::memset(static_cast<void*>(brc), 0, sizeof *brc);
	else
		std::memset(static_cast<void*>(&brc->roms[index]), 0, sizeof brc->roms[index]);
}
// The built-in controllers' entries follow their chipset fields, as the
// core's cfgfile_compatibility_romtype() keeps them (addbcromtype): IDE on
// cs_ide, A3000 or A4000T SCSI on cs_mbdmac 1 or 2 (neither for any other
// value), the CDTV DMAC on cs_cdtvcd without cs_cdtvcr, the CDTV-CR on
// cs_cdtvcr, the CD32 FMV cartridge on cs_cd32fmv, which then follows the
// entry.
static void addbcromtype(uae_prefs* p, int romtype, bool add, const TCHAR* romfile)
{
	if (!add) {
		clear_device_rom(p, romtype, 0, true);
		return;
	}
	boardromconfig* brc = get_device_rom_new(p, romtype, 0, nullptr);
	if (brc && !brc->roms[0].romfile[0])
		std::strcpy(brc->roms[0].romfile, romfile ? romfile : ":ENABLED");
}
void cfgfile_compatibility_romtype(uae_prefs* p)
{
	addbcromtype(p, ROMTYPE_MB_IDE, p->cs_ide != 0, nullptr);
	addbcromtype(p, ROMTYPE_SCSI_A3000, p->cs_mbdmac == 1, nullptr);
	addbcromtype(p, ROMTYPE_SCSI_A4000T, p->cs_mbdmac == 2, nullptr);
	addbcromtype(p, ROMTYPE_CDTVDMAC, p->cs_cdtvcd && !p->cs_cdtvcr, nullptr);
	addbcromtype(p, ROMTYPE_CDTVCR, p->cs_cdtvcr, nullptr);
	if (p->cs_cd32fmv)
		addbcromtype(p, ROMTYPE_CD32CART, true, p->cartfile);
	p->cs_cd32fmv = get_device_rom(p, ROMTYPE_CD32CART, 0, nullptr) != nullptr;
}
boardromconfig* get_device_rom_new(uae_prefs* p, int romtype, int devnum, int* index)
{
	if (boardromconfig* brc = get_device_rom(p, romtype, devnum, index))
		return brc;
	for (boardromconfig& brc : p->expansionboard) {
		if (!brc.device_type) {
			std::memset(&brc, 0, sizeof brc);
			brc.device_type = romtype;
			brc.device_num = devnum;
			for (romconfig& rom : brc.roms) {
				rom.back = &brc; // as device_rom_defaults()
				rom.device_id = 7;
			}
			return &brc;
		}
	}
	return nullptr;
}

// A few entries of the core's board table, as gfxboard.cpp has them: the
// two UAE boards, two hardware boards with a monitor switcher and one
// without.
struct test_board {
	int id;
	const char* name;
	const char* maker;
	const char* config;
	int vram_min, vram_max;
	bool switcher;
	int bus; // configtype: 2 Zorro II, 3 Zorro III
};
static const test_board test_boards[] = {
	{GFXBOARD_UAE_Z2, "UAE [Zorro II]", nullptr, "ZorroII", -1, -1, true, 2},
	{GFXBOARD_UAE_Z3, "UAE [Zorro III]", nullptr, "ZorroIII", -1, -1, true, 3},
	{GFXBOARD_ID_PICASSO2, "Picasso II [Zorro II]", "Village Tronic", "PicassoII", 0x100000, 0x200000, true, 2},
	{GFXBOARD_ID_ZZ9000_Z3, "ZZ9000 [Zorro III]", "MNT Research", "ZZ9000_Z3", 0x8000000, 0x8000000, true, 3},
	{GFXBOARD_ID_GD5446_PCI, "GD5446 [PCI]", "Cirrus Logic", "GD5446_PCI", 0x400000, 0x400000, false, BOARD_PCI},
};
static const test_board* find_test_board(int id)
{
	for (const auto& b : test_boards) {
		if (b.id == id)
			return &b;
	}
	return nullptr;
}
int gfxboard_get_id_from_index(int index)
{
	return index >= 0 && index < static_cast<int>(sizeof test_boards / sizeof test_boards[0])
		? test_boards[index].id : -1;
}
const TCHAR* gfxboard_get_name(int id) { const test_board* b = find_test_board(id); return b ? b->name : nullptr; }
const TCHAR* gfxboard_get_manufacturername(int id) { const test_board* b = find_test_board(id); return b ? b->maker : nullptr; }
const TCHAR* gfxboard_get_configname(int id) { const test_board* b = find_test_board(id); return b ? b->config : nullptr; }
bool gfxboard_get_switcher(rtgboardconfig* rbc) { const test_board* b = find_test_board(rbc->rtgmem_type); return !b || b->switcher; }
int gfxboard_get_configtype_of(int type) { const test_board* b = find_test_board(type); return b ? b->bus : 0; }
bool gfxboard_get_vram_range(int type, int* vmin, int* vmax)
{
	const test_board* b = find_test_board(type);
	if (!b || type < GFXBOARD_HARDWARE)
		return false;
	*vmin = b->vram_min;
	*vmax = b->vram_max;
	return true;
}
// The host network driver every build has.
static netdriverdata test_slirp = [] {
	netdriverdata d{};
	d.name = "slirp";
	d.desc = "SLIRP User Mode NAT";
	return d;
}();
static netdriverdata* test_net_drivers[] = {&test_slirp, nullptr};
netdriverdata** target_ethernet_enumerate() { return test_net_drivers; }

// A scanned ROM list with one cartridge dump, for the cartridge ident.
static romdata test_cart_rom = [] {
	romdata rd{};
	rd.name = "Action Replay III";
	rd.configname = "AR3";
	rd.type = ROMTYPE_AR;
	return rd;
}();
// And an A2091 ROM, the ROM an A2091 enabled without one gets.
static romdata test_a2091_rom = [] {
	romdata rd{};
	rd.name = "A590/A2091 SCSI boot ROM";
	rd.type = ROMTYPE_A2091;
	return rd;
}();
static romlist test_rom_list[] = {{const_cast<TCHAR*>("/roms/ar3.rom"), &test_cart_rom},
	{const_cast<TCHAR*>("/roms/a2091.rom"), &test_a2091_rom}};
int romlist_count() { return 2; }
static int rom_scans = 0;
int scan_roms(int) { return ++rom_scans, 0; }
static std::string loaded_key;
void addkeyfile(const TCHAR* path) { loaded_key = path; }
romlist* romlist_getit() { return test_rom_list; }
// Like the core, getarcadiaroms() lists every known ALG game (type 1), or
// every Arcadia BIOS and game (type 0, BIOS first), installed or not;
// getromlistbyromdata() finds the installed file. The BIOS is installed;
// the games are unless a test removes them.
static romdata test_arcade_rom(const char* name, int type)
{
	romdata rd{};
	rd.name = name;
	rd.type = type;
	return rd;
}
static romdata test_alg_game = test_arcade_rom("ALG game", ROMTYPE_ALG);
static romdata test_arcadia_bios = test_arcade_rom("Arcadia BIOS", ROMTYPE_ARCADIABIOS);
static romdata test_arcadia_game = test_arcade_rom("Arcadia game", ROMTYPE_ARCADIAGAME);
static romlist test_arcade_list[] = {
	{const_cast<TCHAR*>("/roms/alg.rom"), &test_alg_game},
	{const_cast<TCHAR*>("/roms/arcadia-bios.rom"), &test_arcadia_bios},
	{const_cast<TCHAR*>("/roms/arcadia-game.rom"), &test_arcadia_game},
};
static bool arcadia_game_installed = true;
romlist** getarcadiaroms(int type)
{
	auto** list = static_cast<romlist**>(std::calloc(3, sizeof(romlist*)));
	if (list && type) {
		list[0] = &test_arcade_list[0];
	} else if (list) {
		list[0] = &test_arcade_list[1];
		list[1] = &test_arcade_list[2];
	}
	return list;
}
romlist* getromlistbyromdata(const romdata* rd)
{
	for (romlist& rl : test_arcade_list) {
		if (rl.rd == rd && (rd == &test_arcadia_bios || arcadia_game_installed))
			return &rl;
	}
	return nullptr;
}

// Preset engine stand-in: the harness cannot link cfgfile.cpp, so it
// records the request and applies a few marker fields the Quickstart
// constraints are known to react to, plus one field outside every option
// domain (keyboard_leds[0]) as a fresh configuration's defaults would set
// one.
// Both the layer and the frozen reference implementation below run against
// this same stand-in, so the test still proves the constraint move changed
// nothing.
static int stub_preset_model = -1, stub_preset_config = -1, stub_preset_compa = -1;
static int stub_preview_calls;
int built_in_prefs(uae_prefs* p, int model, int config, int compa, int romcheck)
{
	(void)romcheck;
	stub_preset_model = model;
	stub_preset_config = config;
	stub_preset_compa = compa;
	p->floppyslots[0].dfxtype = DRV_35_HD; // constraints must force DD on A500-class
	p->floppyslots[1].dfxtype = DRV_35_DD;
	p->cdslots[0].inuse = true;
	p->cdslots[0].type = SCSI_UNIT_DEFAULT;
	p->jports[1].mode = 3;
	p->keyboard_leds[0] = 2;
	return 1;
}
int built_in_prefs_preview(uae_prefs* p, int model, int config, int compa)
{
	stub_preview_calls++;
	return built_in_prefs(p, model, config, compa, 0);
}

// Mount list stand-ins: the harness cannot link cfgfile.cpp, so entries
// use a reduced uaehfN spelling, "dir,rw,DEVICE:PATH", and the adder keeps
// the hard drive panel's device-name and capacity checks. The session logic
// around them (copies, placement, merging, apply class) is the layer's own;
// the IPC test drives the real parser and writer.
static size_t stub_mount_line_max = 200;
bool cfgfile_parse_mount_value(uae_prefs*, const TCHAR* value, uaedev_config_info* ci)
{
	const char* prefix = "dir,rw,";
	if (!value || std::strncmp(value, prefix, std::strlen(prefix)) != 0)
		return false;
	const char* device = value + std::strlen(prefix);
	const char* colon = std::strchr(device, ':');
	if (!colon || colon == device || static_cast<size_t>(colon - device) >= sizeof ci->devname
		|| std::strlen(colon + 1) >= sizeof ci->rootdir)
		return false;
	std::memset(ci, 0, sizeof *ci);
	ci->type = UAEDEV_DIR;
	std::memcpy(ci->devname, device, static_cast<size_t>(colon - device));
	std::strcpy(ci->rootdir, colon + 1);
	return true;
}
uaedev_config_data* add_filesys_config(uae_prefs* p, int index, uaedev_config_info* ci)
{
	if (index >= 0)
		return nullptr; // the layer only appends
	for (int i = 0; i < p->mountitems; i++) {
		if (!std::strcmp(p->mountconfig[i].ci.devname, ci->devname))
			return nullptr;
	}
	if (p->mountitems >= MOUNT_CONFIG_SIZE)
		return nullptr;
	uaedev_config_data* uci = &p->mountconfig[p->mountitems++];
	uci->configoffset = -1;
	uci->unitnum = -1;
	std::memcpy(&uci->ci, ci, sizeof *ci);
	return uci;
}
bool cfgfile_mount_value(uae_prefs* p, int index, TCHAR* out, size_t size)
{
	if (index < 0 || index >= p->mountitems)
		return false;
	std::snprintf(out, size, "dir,rw,%s:%s", p->mountconfig[index].ci.devname, p->mountconfig[index].ci.rootdir);
	return true;
}
TCHAR* validatedevicename(TCHAR* s, const TCHAR*)
{
	if (std::strlen(s) > 30)
		s[30] = 0; // the core's striplength(s, 30)
	return s;
}
bool cfgfile_mount_entry_fits(uae_prefs* p, int index)
{
	return std::strlen(p->mountconfig[index].ci.rootdir) < stub_mount_line_max;
}

namespace settings = amiberry_settings;

namespace {

int failures;

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

const settings::context idle{false};


// The CPU, display, sound, input and on-screen fields of default_prefs()
// (src/cfgfile.cpp) with the default amiberry.conf options.
void reset_prefs(uae_prefs& p)
{
	std::memset(static_cast<void*>(&p), 0, sizeof p);
	p.cpu_model = 68000;
	p.cpu_compatible = true;
	p.address_space_24 = true;
	p.cpu_cycle_exact = true;
	p.cpu_memory_cycle_exact = true;
	p.blitter_cycle_exact = true;
	p.compnf = true;
	p.comp_constjump = true;
	p.comp_catchfault = true;
#ifdef USE_JIT_FPU
	p.compfpu = true;
#endif
	// The machine fields of default_prefs() (src/cfgfile.cpp), resolved the
	// way fixup_prefs() leaves them: with the Generic chipset extra the
	// built-in chipset table rewrites the A500-like options (the default
	// 512 KB slow RAM carries an RTC), and the resolved values are what the
	// emulator settles on.
	p.chipset_mask = CSMASK_ECS_AGNUS;
	p.collision_level = 3;
	p.keyboard_nkro = true;
	p.cs_compatible = CP_GENERIC;
	p.cs_rtc = 1;
	p.cs_ciaoverlay = true;
	p.cs_ksmirror_e0 = true;
	p.cs_df0idhw = false;
	p.cs_resetwarning = false;
	p.cs_ciatodbug = true;
	p.cs_memorypatternfill = true;
	p.cs_fatgaryrev = -1;
	p.cs_ramseyrev = -1;
	p.cs_agnusrev = -1;
	p.cs_deniserev = -1;
	p.chipmem.size = 0x80000;
	p.bogomem.size = 0x80000;

	// Display (default_prefs plus target_default_options defaults).
	p.gfx_monitor[0].gfx_size_win.width = 720;
	p.gfx_monitor[0].gfx_size_win.height = 568;
	p.gfx_resolution = 1;
	p.gfx_vresolution = 1;
	p.gfx_iscanlines = 1;
	p.gfx_overscanmode = 3;
	p.gfx_autoresolution_vga = true;
	p.gfx_windowed_resize = true;
	p.gfx_correct_aspect = 1;
	p.scaling_method = -1;
	// The display filters (target_default_options, default application
	// settings): no shader, no bezel, the manual crop window the whole
	// frame at the render resolution.
	strcpy(p.shader, "none");
	strcpy(p.shader_rtg, "none");
	strcpy(p.custom_bezel, "none");
	p.gfx_manual_crop_width = AMIGA_WIDTH_MAX << p.gfx_resolution;
	p.gfx_manual_crop_height = AMIGA_HEIGHT_MAX << p.gfx_vresolution;
	p.cr_selected = -1;
	// IO ports (default_prefs, target_default_options): serial flow control
	// and status lines on, no sampler.
	p.serial_hwctsrts = true;
	p.serial_rtsctsdtrdtecd = true;
	p.samplersoundcard = -1;
	// default_prefs() describes a fresh configuration.
	std::strcpy(p.description, "UAE default configuration");
	for (int i = 0; i < 2; i++) {
		struct chipset_refresh& cr = p.cr[i == 0 ? CHIPSET_REFRESH_PAL : CHIPSET_REFRESH_NTSC];
		cr.index = i == 0 ? CHIPSET_REFRESH_PAL : CHIPSET_REFRESH_NTSC;
		cr.rate = i == 0 ? 50.0f : 60.0f;
		cr.horiz = cr.vert = cr.lace = cr.vsync = cr.framelength = -1;
		cr.resolution = 7;
		cr.ntsc = i;
		cr.inuse = true;
		strcpy(cr.label, i == 0 ? "PAL" : "NTSC");
	}

	// Sound.
	p.produce_sound = 3;
	p.sound_stereo = 1;
	p.sound_stereo_separation = 7;
	p.sound_freq = 44100;
	p.sound_maxbsiz = 3072; // amiberry_options.default_sound_buffer, applied by target_default_options()
	p.sound_interpol = 1;
	p.sound_filter = 1;
	p.sound_volume_cd = 20;
	p.soundcard_default = true;
	for (int i = 0; i < 4; i++) {
		p.dfxclickvolume_disk[i] = 33;
		p.dfxclickvolume_empty[i] = 33;
	}

	// Input (inputdevice_default_prefs plus target defaults).
	p.input_joymouse_multiplier = 25;
	p.input_joystick_deadzone = 33;
	p.input_joymouse_speed = 10;
	p.input_autofire_linecnt = 600;
	p.input_autoswitch = true;
	p.input_joystick_up_button = true;
	p.input_mouse_speed = 100;
	p.input_mouse_untrap = MOUSEUNTRAP_MIDDLEBUTTON;
	p.jports[0].id = JSEM_MICE;
	p.jports[1].id = JSEM_JOYS;
	p.jports[2].id = JPORT_NONE;
	p.jports[3].id = JPORT_NONE;

	// On-screen controls (default_vkbd_transparency is 0: the 85 % default).
	p.vkbd_touch_button = true;
	p.vkbd_transparency = 0;
	strcpy(p.vkbd_language, "US");
	strcpy(p.vkbd_toggle, "guide");

	// Floppies (default_prefs) and DrawBridge (target_default_options).
	p.floppyslots[0].dfxtype = DRV_35_DD;
	p.floppyslots[1].dfxtype = DRV_35_DD;
	p.floppyslots[2].dfxtype = DRV_NONE;
	p.floppyslots[3].dfxtype = DRV_NONE;
	p.floppy_speed = 100;
	p.drawbridge_serial_auto = true;
	p.cd_speed = 100;
	p.filesys_custom_uaefsdb = true;

	// Miscellaneous and priority (default_prefs, target_default_options
	// and the built-in amiberry_options hotkeys).
	p.start_gui = true;
	p.capture_always = true;
	p.ctrl_alt_release = true;
	strcpy(p.open_gui, "F12");
	strcpy(p.action_replay, "Pause");
	p.kbd_led_num = p.kbd_led_scr = p.kbd_led_cap = -1;
	p.active_capture_priority = 1;
	p.minimized_pause = true;
	p.minimized_nosound = true;
	p.use_retroarch_quit = true;
	p.use_retroarch_menu = true;

	// RTG (default_prefs and target_default_options).
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		p.rtgboards[i].rtg_index = i;
		p.rtgboards[i].autoswitch = true;
	}
	p.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
	p.rtg_zerocopy = true;
	p.rtg_hardwaresprite = true;
	p.picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_R8G8B8A8;
	p.gf[GF_RTG].gfx_filter_autoscale = RTG_MODE_SCALE;
	p.rtgscaleaspectratio = -1;
	p.gfx_apmode[APMODE_RTG].gfx_backbuffers = 1;
	strcpy(p.zz9000net_name, "slirp");
}

std::string value_of(const uae_prefs& p, const char* id)
{
	char text[settings::max_value_length];
	CHECK(settings::get(settings::require(id), p, text, sizeof text) == AMIBERRY_SETTINGS_OK);
	return text;
}

amiberry_settings_status set(uae_prefs& p, const char* id, const char* text,
	const settings::context& ctx = idle)
{
	return settings::set(settings::require(id), p, ctx, text);
}

bool available(const uae_prefs& p, const char* id, const settings::context& ctx = idle)
{
	return settings::is_available(settings::require(id), p, ctx);
}

bool choice_available(const uae_prefs& p, const char* id, const char* choice)
{
	return settings::is_choice_available(settings::require(id), p, idle, choice);
}

bool jit_compiled()
{
#ifdef JIT
	return true;
#else
	return false;
#endif
}

// Every descriptor is complete and its default is a value it accepts.
// Options with host-dependent choices enumerate their list instead of a
// static one.
void check_descriptors()
{
	uae_prefs& defaults = test_prefs;
	reset_prefs(defaults);
	CHECK(amiberry_settings_count() == settings::definition_count());
	CHECK(amiberry_settings_at(amiberry_settings_count()) == nullptr);
	CHECK(amiberry_settings_find("no_such_option") == nullptr);
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		CHECK(d && d->id && d->group && d->label && d->help && d->default_value);
		if (!d)
			continue;
		CHECK(amiberry_settings_find(d->id) == d);
		switch (d->type) {
		case AMIBERRY_SETTING_BOOL:
			CHECK(!std::strcmp(d->default_value, "true") || !std::strcmp(d->default_value, "false"));
			break;
		case AMIBERRY_SETTING_INT:
		case AMIBERRY_SETTING_REAL: {
			CHECK(d->min < d->max && d->step > 0);
			const double v = std::atof(d->default_value);
			CHECK(v >= d->min && v <= d->max);
			break;
		}
		case AMIBERRY_SETTING_PATH:
		case AMIBERRY_SETTING_STRING:
		case AMIBERRY_SETTING_HOTKEY:
			// Free-form values only need a non-null default string.
			break;
		case AMIBERRY_SETTING_CHOICE: {
			const settings::definition& def = settings::definition_at(i);
			if (!def.dynamic_choices)
				CHECK(d->choices && d->choice_count > 0);
			else
				CHECK(!d->choices && d->choice_count == 0);
			const settings::choice_list choices = settings::choices_of(def, defaults);
			CHECK(choices.count > 0);
			bool found = false;
			for (size_t c = 0; c < choices.count; c++) {
				CHECK(choices.items[c].value && choices.items[c].label);
				found = found || !std::strcmp(choices.items[c].value, d->default_value);
			}
			CHECK(found);
			break;
		}
		}
		}
	// The defaults are the values default_prefs() produces.
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const settings::definition& d = settings::definition_at(i);
		const std::string v = value_of(p, d.descriptor.id);
		if (v != d.descriptor.default_value)
			std::fprintf(stderr, "default of %s: prefs %s, descriptor %s\n", d.descriptor.id, v.c_str(),
				d.descriptor.default_value);
		CHECK(v == d.descriptor.default_value);
	}
}

// AE1: availability follows the CPU. A 68000 keeps 24-bit addressing and has
// no JIT, MMU or data cache; the 32-bit CPUs gain them.
void check_availability()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(available(p, "cpu_24bit_addressing"));
	CHECK(!available(p, "cachesize"));
	CHECK(!available(p, "mmu_model"));
	CHECK(!available(p, "cpu_compatible")); // memory cycle-exact forces it on
	CHECK(!available(p, "cpu_data_cache"));
	CHECK(!available(p, "cpu_threaded"));
	CHECK(!available(p, "fpu_strict"));
	CHECK(choice_available(p, "fpu_model", "0"));
	CHECK(!choice_available(p, "fpu_model", "68881")); // 68000 in More compatible mode
	CHECK(!choice_available(p, "fpu_model", "68040"));

	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.cpu_compatible = false;
	CHECK(available(p, "cpu_compatible"));
	CHECK(choice_available(p, "fpu_model", "68881")); // possible without More compatible

	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "cpu_24bit_addressing"));
	CHECK(value_of(p, "cpu_24bit_addressing") == "false");
	CHECK(available(p, "cachesize") == jit_compiled());
	CHECK(available(p, "mmu_model"));
	CHECK(choice_available(p, "mmu_model", "68030"));
	CHECK(choice_available(p, "mmu_model", "68ec030"));
	CHECK(!choice_available(p, "mmu_model", "68040"));
	CHECK(available(p, "cpu_threaded"));
	CHECK(!available(p, "cpu_threaded", settings::context{true}));

	CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "cpu_24bit_addressing"));
	CHECK(!choice_available(p, "fpu_model", "68881"));
	CHECK(choice_available(p, "fpu_model", "68040"));
	CHECK(!choice_available(p, "fpu_model", "68060"));
	CHECK(!available(p, "cpu_no_unimplemented"));

	CHECK(set(p, "mmu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "cachesize")); // no JIT with MMU emulation
	CHECK(!available(p, "cpu_threaded")); // MMU emulation without the JIT

	// The speed adjustment can only slow the CPU down at fastest speed.
	double min, max;
	CHECK(set(p, "cpu_speed", "max") == AMIBERRY_SETTINGS_OK);
	settings::get_range(settings::require("cpu_throttle"), p, min, max);
	CHECK(min == -900 && max == 0);
	CHECK(set(p, "cpu_throttle", "500") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpu_speed", "real") == AMIBERRY_SETTINGS_OK);
	settings::get_range(settings::require("cpu_throttle"), p, min, max);
	CHECK(max == 5000);
	CHECK(set(p, "cpu_throttle", "500") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpu_throttle") == "500.0");
}

void check_dependent_changes()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// A 68040 keeps an FPU as its internal one; a 68020 gets a 68881 back.
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fpu_model", "68882") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpu_multiplier") == "8");
	CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "68040");
	CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "68881");
	CHECK(value_of(p, "cpu_multiplier") == "4");
	CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "0"); // More compatible
	CHECK(value_of(p, "cpu_24bit_addressing") == "true");
	CHECK(value_of(p, "cpu_multiplier") == "2");

	// Without More compatible a 68000 keeps an external FPU, and an internal
	// one carried over from a 68040 becomes a 68881.
	p.cpu_compatible = false;
	CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fpu_model", "68040") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "68881");

	// Switching the CPU resets the MMU, but setting the same CPU again keeps
	// it.
	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "mmu_model", "68ec030") == AMIBERRY_SETTINGS_OK);
	CHECK(p.mmu_model == 68030 && p.mmu_ec);
	CHECK(value_of(p, "mmu_model") == "68ec030");
	CHECK(set(p, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "mmu_model") == "68ec030");
	CHECK(set(p, "cpu_model", "68060") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "mmu_model") == "0");

	// Choosing the current speed mode or memory access mode again
	// re-applies it, as a re-clicked radio button did in the panel.
	p.m68k_speed = -1;
	p.m68k_speed_throttle = -500;
	CHECK(set(p, "cpu_speed", "max") == AMIBERRY_SETTINGS_OK);
	CHECK(p.m68k_speed_throttle == 0);
	p.m68k_speed = 3 * CYCLE_UNIT; // cpu_speed=3
	CHECK(value_of(p, "cpu_speed") == "3");
	CHECK(set(p, "cpu_speed", "real") == AMIBERRY_SETTINGS_OK);
	CHECK(p.m68k_speed == 0);

	// Turning the JIT on leaves cycle-exact mode. A 68040/060 JIT greys out
	// More compatible but keeps its value for when the JIT is off again.
	if (jit_compiled()) {
		reset_prefs(p);
		CHECK(set(p, "cpu_model", "68040") == AMIBERRY_SETTINGS_OK);
		CHECK(p.cpu_compatible);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(!p.cpu_cycle_exact && !p.cpu_memory_cycle_exact && !p.blitter_cycle_exact);
		CHECK(!available(p, "cpu_compatible"));
		CHECK(set(p, "cachesize", "0") == AMIBERRY_SETTINGS_OK);
		CHECK(available(p, "cpu_compatible") && p.cpu_compatible);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "comp_trustbyte", "indirect") == AMIBERRY_SETTINGS_OK);
		CHECK(p.comptrustbyte == 1 && p.comptrustword == 1 && p.comptrustlong == 1 && p.comptrustnaddr == 1);
		p.comptrustlong = 0; // a configuration can set them separately
		CHECK(set(p, "comp_trustbyte", "indirect") == AMIBERRY_SETTINGS_OK);
		CHECK(p.comptrustlong == 1);
	}

	// More compatible stays on in memory cycle-exact mode and drops the
	// FPU of a 68000.
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.cpu_compatible = false;
	CHECK(set(p, "fpu_model", "68881") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpu_compatible", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "fpu_model") == "0");

	// x86 bridgeboard speed is 0 at fastest CPU speed.
	reset_prefs(p);
	p.m68k_speed = -1;
	CHECK(set(p, "cpu_x86_throttle", "500") == AMIBERRY_SETTINGS_UNAVAILABLE); // no bridgeboard
}

// Every option offered as available must survive fixup_cpu(): a value the
// core would clear at once is unavailable, so neither the panel nor an API
// client can choose it.
void check_fixup_consistency()
{
	uae_prefs& p = test_prefs;
	auto survives = [&p](const char* id) {
		uae_prefs* copy = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
		std::memcpy(static_cast<void*>(copy), static_cast<const void*>(&p), sizeof p);
		fixup_cpu(copy);
		const bool same = value_of(*copy, id) == value_of(p, id);
		std::free(copy);
		return same;
	};

	// 24-bit addressing with a 32-bit-only accelerator board.
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.cpu_model = 68030;
	p.address_space_24 = false;
	p.cpuboard_type = 1;
	board_is_32bit = true;
	CHECK(!available(p, "cpu_24bit_addressing"));
	board_is_32bit = false;
	CHECK(available(p, "cpu_24bit_addressing"));
	CHECK(set(p, "cpu_24bit_addressing", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("cpu_24bit_addressing"));
	p.cpuboard_type = 0;
	p.address_space_24 = false;

	// Data cache emulation needs no UAE boot ROM or the indirect one, and the
	// check must not change the boot ROM type the running emulation uses.
	uae_boot_rom_type = 7;
	boot_rom_address = 0xf00000;
	CHECK(!available(p, "cpu_data_cache"));
	CHECK(uae_boot_rom_type == 7);
	p.uaeboard = 3;
	CHECK(available(p, "cpu_data_cache"));
	CHECK(set(p, "cpu_data_cache", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("cpu_data_cache"));
	p.uaeboard = 0;
	boot_rom_address = 0;
	CHECK(available(p, "cpu_data_cache"));
	CHECK(survives("cpu_data_cache"));

	// A 68000 in memory cycle-exact mode cannot keep an external FPU even
	// without More compatible.
	reset_prefs(p);
	p.cpu_compatible = false;
	CHECK(!choice_available(p, "fpu_model", "68881"));

	// A JIT left behind by a switch to a 68000 is cleared, so it cannot keep
	// softfloat and the other JIT-gated options unavailable.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		p.cpu_compatible = false;
		CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_24bit_addressing", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "fpu_model", "68881") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(p.compfpu);
		// The JIT FPU refuses softfloat but keeps host floating point
		// selectable.
		CHECK(available(p, "fpu_softfloat"));
		CHECK(!choice_available(p, "fpu_softfloat", "true"));
		CHECK(choice_available(p, "fpu_softfloat", "false"));
		CHECK(set(p, "fpu_softfloat", "true") == AMIBERRY_SETTINGS_INVALID_VALUE);
		CHECK(set(p, "fpu_softfloat", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
		CHECK(p.cachesize == 0 && p.fpu_model == 68881);
		CHECK(choice_available(p, "fpu_softfloat", "true"));
		CHECK(set(p, "fpu_softfloat", "true") == AMIBERRY_SETTINGS_OK);
		CHECK(survives("fpu_softfloat"));

		// Turning the JIT FPU on switches softfloat off, as fixup_cpu() would.
		CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_24bit_addressing", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(!p.compfpu); // softfloat kept the JIT FPU off
		CHECK(set(p, "compfpu", "true") == AMIBERRY_SETTINGS_OK);
		CHECK(p.fpu_mode == 0);
	}

	// A speed set with cpu_speed=N reads back as N, not as "real", and
	// writing it back is refused rather than silently changing the speed.
	reset_prefs(p);
	p.m68k_speed = 5 * CYCLE_UNIT;
	CHECK(value_of(p, "cpu_speed") == "5");
	CHECK(set(p, "cpu_speed", "5") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(p.m68k_speed == 5 * CYCLE_UNIT);

	// An EC flag without an MMU model reads as no MMU.
	p.mmu_model = 0;
	p.mmu_ec = true;
	CHECK(value_of(p, "mmu_model") == "0");

	// JIT caches below 128 KB are turned off by fixup_cpu(), so they are
	// refused; the JIT inhibit flag makes the JIT unavailable.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		p.cpu_model = 68020;
		p.address_space_24 = false;
		CHECK(set(p, "cachesize", "64") == AMIBERRY_SETTINGS_INVALID_VALUE);
		CHECK(set(p, "cachesize", "128") == AMIBERRY_SETTINGS_OK);
		CHECK(survives("cachesize"));
		p.cachesize_inhibit = true;
		CHECK(!available(p, "cachesize"));
	}

	// Turning softfloat off leaves the host 80-bit mode of MSVC long double
	// builds alone.
	reset_prefs(p);
	p.fpu_model = 68881;
	p.fpu_mode = -1;
	CHECK(set(p, "fpu_softfloat", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fpu_mode == -1);
	CHECK(set(p, "fpu_softfloat", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fpu_softfloat", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fpu_mode == 0);
}

void check_rejections()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(settings::find("no_such_option") == nullptr);
	CHECK(set(p, "cpu_model", "68050") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpu_24bit_addressing", "yes") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.cpu_idle", "20") == AMIBERRY_SETTINGS_UNAVAILABLE); // real speed
	p.m68k_speed = -1;
	CHECK(set(p, "amiberry.cpu_idle", "20") == AMIBERRY_SETTINGS_INVALID_VALUE); // not a step
	CHECK(set(p, "amiberry.cpu_idle", "180") == AMIBERRY_SETTINGS_INVALID_VALUE); // above range
	CHECK(set(p, "amiberry.cpu_idle", "30x") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.cpu_idle", "30") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "mmu_model", "68030") == AMIBERRY_SETTINGS_UNAVAILABLE); // 68000
	p.cpu_model = 68030;
	CHECK(set(p, "mmu_model", "68040") == AMIBERRY_SETTINGS_UNAVAILABLE); // other CPU's MMU
	CHECK(value_of(p, "mmu_model") == "0");
#ifdef WITH_PPC
	// Manual PPC setups come from an accelerator board configuration; the
	// option itself only switches automatic PPC on and off.
	p.cpu_model = 68060;
	CHECK(set(p, "ppc_model", "manual") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// The choices map to the stored values; PearPC is no longer offered.
	CHECK(set(p, "ppc_implementation", "qemu") == AMIBERRY_SETTINGS_OK);
	CHECK(p.ppc_implementation == PPC_IMPLEMENTATION_QEMU && value_of(p, "ppc_implementation") == "qemu");
	CHECK(!choice_available(p, "ppc_implementation", "pearpc"));
#endif
	char small[3];
	CHECK(settings::get(settings::require("cpu_model"), p, small, sizeof small) == AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
}

// AE2 for the CPU domain: validation reports what fixup_cpu() would change
// without changing the session, and commit stores the adjusted values.
void check_session()
{
	reset_prefs(changed_prefs);
	changed_prefs.cpu_model = 68040;
	changed_prefs.address_space_24 = true;

	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(session);
	amiberry_settings_adjustment adjustments[4];
	CHECK(amiberry_settings_validate(session, adjustments, 4) == 1);
	CHECK(!std::strcmp(adjustments[0].id, "cpu_24bit_addressing"));
	CHECK(!std::strcmp(adjustments[0].requested, "true"));
	CHECK(!std::strcmp(adjustments[0].adjusted, "false"));
	CHECK(adjustments[0].reason && std::strstr(adjustments[0].reason, "68040"));
	CHECK(amiberry_settings_validate(session, nullptr, 0) == 1); // counts without storage
	char text[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(amiberry_settings_get(session, "cpu_24bit_addressing", text, sizeof text) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(text, "true"));

	// 24-bit addressing only reaches a running emulation at a hard reset,
	// so storing its adjustment needs one.
	bool changed = false;
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed && changed_prefs.cpu_model == 68040 && !changed_prefs.address_space_24);
	CHECK(amiberry_settings_validate(session, adjustments, 4) == 0);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(!changed);

	// Nothing reaches the pending configuration before commit; a new CPU
	// applies live.
	CHECK(amiberry_settings_set(session, "cpu_model", "68030") == AMIBERRY_SETTINGS_OK);
	CHECK(changed_prefs.cpu_model == 68040);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.cpu_model == 68030);

	// Fields the session did not change keep what other code set since.
	changed_prefs.cpu_idle = 45;
	CHECK(amiberry_settings_set(session, "fpu_model", "68882") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.fpu_model == 68882 && changed_prefs.cpu_idle == 45);

	// Changes made elsewhere are resolved together with the session's: an
	// external switch to a 68040 turns the session's 68881 into the internal
	// FPU, and validation says so before commit.
	CHECK(amiberry_settings_set(session, "fpu_model", "68881") == AMIBERRY_SETTINGS_OK);
	changed_prefs.cpu_model = 68040;
	std::memset(adjustments, 0xa5, sizeof adjustments);
	CHECK(amiberry_settings_validate(session, adjustments, 4) == 1);
	CHECK(!std::strcmp(adjustments[0].id, "fpu_model"));
	CHECK(!std::strcmp(adjustments[0].requested, "68881"));
	CHECK(!std::strcmp(adjustments[0].adjusted, "68040"));
	// No stack bytes leak through the fixed-size buffers.
	for (size_t i = std::strlen(adjustments[0].adjusted); i < sizeof adjustments[0].adjusted; i++)
		CHECK(adjustments[0].adjusted[i] == 0);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.cpu_model == 68040 && changed_prefs.fpu_model == 68040);
	CHECK(amiberry_settings_get(session, "fpu_model", text, sizeof text) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(text, "68040")); // the session continues from what it stored

	// Dependent changes outside the option list are committed too.
	if (jit_compiled()) {
		CHECK(amiberry_settings_set(session, "cachesize", "4096") == AMIBERRY_SETTINGS_OK);
		CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
		CHECK(changed_prefs.cachesize == 4096 && !changed_prefs.cpu_cycle_exact
			&& !changed_prefs.blitter_cycle_exact);
	}
	amiberry_settings_session_close(session);

	// Re-selecting the current JIT memory access mode aligns the other
	// modes; that is a change even though the option's own value is not.
	if (jit_compiled()) {
		changed_prefs.comptrustbyte = 1;
		changed_prefs.comptrustlong = 0;
		session = amiberry_settings_session_open();
		CHECK(amiberry_settings_set(session, "comp_trustbyte", "indirect") == AMIBERRY_SETTINGS_OK);
		CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
		CHECK(changed && changed_prefs.comptrustlong == 1);
		amiberry_settings_session_close(session);
	}

	// The strongest apply class among the changed options wins; an option
	// that can only change while stopped is unavailable during emulation.
	reset_prefs(changed_prefs);
	changed_prefs.cpu_cycle_exact = changed_prefs.cpu_memory_cycle_exact = changed_prefs.blitter_cycle_exact = false;
	changed_prefs.cpu_model = 68030;
	changed_prefs.address_space_24 = false;
	session = amiberry_settings_session_open();
	CHECK(amiberry_settings_set(session, "cpu_compatible", "false") == AMIBERRY_SETTINGS_OK);
	emulating = 1;
	CHECK(!amiberry_settings_is_available(session, "cpu_threaded"));
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_UNAVAILABLE);
	emulating = 0;
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	CHECK(changed && changed_prefs.cpu_thread);

	// Re-submitting an unchanged restart-only option with a live change
	// does not ask for a restart.
	CHECK(amiberry_settings_set(session, "cpu_threaded", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "amiberry.cpu_idle", "0") == AMIBERRY_SETTINGS_UNAVAILABLE); // real speed
	CHECK(amiberry_settings_set(session, "cpu_speed", "max") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed && changed_prefs.m68k_speed < 0);

	// Commit stores what the fixups adjust outside the session's options:
	// 24-bit addressing removes Z3 RAM.
	CHECK(amiberry_settings_set(session, "cpu_threaded", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART);
	changed_prefs.z3fastmem[0].size = 16 * 1024 * 1024;
	CHECK(amiberry_settings_set(session, "cpu_24bit_addressing", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed && changed_prefs.address_space_24 && changed_prefs.z3fastmem[0].size == 0);

	double min, max;
	CHECK(amiberry_settings_get_range(session, "cpu_throttle", &min, &max));
	CHECK(!amiberry_settings_get_range(session, "cpu_model", &min, &max));
	CHECK(amiberry_settings_choice_is_available(session, "mmu_model", "68030"));
	CHECK(!amiberry_settings_choice_is_available(session, "mmu_model", "nonsense"));
	CHECK(amiberry_settings_get(session, "no_such_option", text, sizeof text) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	amiberry_settings_session_close(session);
}

// Prints a configuration for the round-trip test: the cycle-exact mode the
// CPU options depend on, then every option in table order (which keeps
// cpu_model ahead of fpu_model, whose value it resets when parsed). Only
// values that survive the configuration checks are expected back: the
// writer omits some unavailable options (the PPC options while PPC
// emulation is off), and a held value the layer would refuse (the RTC of a
// machine preset another option has since replaced) is not the value a
// reload produces. Sub-1 MB RAM boards are represented by their *_size_k
// key only; a zero one would reset a megabyte board the writer keeps in
// <name>_size, a combination no saved configuration contains.
// Options whose choices are the host's devices (ports, displays, sound
// cards) are not expected back either: the binary resolves them against
// the devices of the machine it runs on (a CI runner has no joystick),
// not the harness's.
bool host_dependent(const settings::definition& d)
{
	return d.dynamic_choices != nullptr;
}

// "key:part" options edit a part of a packed entry; the entry is checked.
bool part_of_entry(const settings::definition& d)
{
	return std::strchr(d.descriptor.id, ':') != nullptr;
}

void emit(const char* name, const uae_prefs& p)
{
	std::printf("S %s\n", name);
	const char* cycle_exact = p.cpu_cycle_exact ? "true" : p.cpu_memory_cycle_exact ? "memory" : "false";
	std::printf("A cycle_exact=%s\n", cycle_exact);
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const settings::definition& d = settings::definition_at(i);
		const std::string v = value_of(p, d.descriptor.id);
		const bool size_k_view = std::strstr(d.descriptor.id, "_size_k") != nullptr;
		if ((size_k_view && v == "0") || part_of_entry(d))
			continue;
		std::printf("A %s=%s\n", d.descriptor.id, v.c_str());
		if (!settings::is_available(d, p, idle) || host_dependent(d))
			continue;
		if ((d.descriptor.type == AMIBERRY_SETTING_CHOICE || d.descriptor.type == AMIBERRY_SETTING_BOOL)
			&& !settings::is_choice_available(d, p, idle, v))
			continue;
		std::printf("E %s=%s\t%s\n", d.descriptor.id, v.c_str(), d.descriptor.default_value);
	}
}

void emit_roundtrip()
{
	std::printf("S defaults\n");
	for (size_t i = 0; i < settings::definition_count(); i++) {
		const settings::definition& d = settings::definition_at(i);
		if (host_dependent(d) || part_of_entry(d))
			continue;
		std::printf("E %s=%s\t%s\n", d.descriptor.id, d.descriptor.default_value, d.descriptor.default_value);
	}

	// A failed set would quietly test a different configuration.
	uae_prefs& p = test_prefs;
	auto apply = [&p](const char* id, const char* v) {
		if (set(p, id, v) != AMIBERRY_SETTINGS_OK) {
			std::fprintf(stderr, "round-trip scenario: setting %s=%s failed\n", id, v);
			failures++;
		}
	};
	reset_prefs(p);
	apply("cpu_model", "68020");
	apply("config_description", "My A1200 setup, cycle-exact");
	apply("config_category", "Games");
	emit("a1200_cycle_exact", p);

	reset_prefs(p);
	apply("cpu_model", "68030");
	apply("mmu_model", "68ec030");
	apply("fpu_model", "68882");
	apply("cpu_data_cache", "true");
	apply("cpu_multiplier", "8");
	emit("a3000_ec030", p);

	reset_prefs(p);
	apply("cpu_model", "68060");
	apply("fpu_model", "68060");
	apply("mmu_model", "68060");
	apply("fpu_softfloat", "true");
	apply("fpu_strict", "true");
	apply("cpu_no_unimplemented", "true");
	apply("fpu_no_unimplemented", "true");
	emit("a4000_060_softfloat", p);

	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("cpu_model", "68020");
	apply("cpu_24bit_addressing", "false");
	apply("cpu_multiplier", "4");
	apply("cpu_throttle", "500");
	emit("a1200_faster", p);

	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("cpu_model", "68020");
	apply("cpu_24bit_addressing", "false");
	apply("cpu_compatible", "false");
	apply("cpu_threaded", "true");
	apply("cpu_speed", "max");
	apply("amiberry.cpu_idle", "60");
	emit("threaded_020", p);

	if (jit_compiled()) {
		reset_prefs(p);
		apply("cpu_model", "68040");
		apply("fpu_model", "68040");
		apply("cachesize", "8192");
		apply("comp_trustbyte", "indirect");
		apply("comp_flushmode", "hard");
		apply("comp_nf", "false");
		apply("comp_catchfault", "false");
		apply("comp_constjump", "false");
		apply("cpu_speed", "max");
		emit("a4000_040_jit", p);
	}

#ifdef WITH_PPC
	reset_prefs(p);
	apply("cpu_model", "68060");
	apply("ppc_model", "automatic");
	apply("ppc_implementation", "qemu");
	apply("ppc_cpu_idle", "max");
	emit("ppc", p);
#endif

	// Machine scenarios: chipset, memory and ROM option classes.
	reset_prefs(p);
	apply("chipset", "aga");
	apply("collision_level", "sprites");
	apply("keyboard_type", "a1200_6805");
	apply("keyboard_nkro", "false");
	apply("display_optimizations", "none");
	apply("hvcsync", "csync");
	apply("ntsc", "true");
	emit("aga_ntsc", p);

	reset_prefs(p);
	apply("cpu_model", "68020");
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("cycle_exact", "memory");
	apply("immediate_blits", "true");
	emit("cycle_exact_memory", p);

	reset_prefs(p);
	apply("chipset_compatible", "A500");
	emit("a500_compatible", p);

	// Miscellaneous and priority options away from their defaults: each
	// must be read back as written (the writer and the parser agree).
	reset_prefs(p);
	apply("use_gui", "no");
	apply("power_led_dim", "128");
	apply("show_leds_rtg", "true");
	apply("amiberry.inactive_minimize", "true");
	apply("amiberry.use_retroarch_quit", "false");
	apply("amiberry.screenshot", "LCtrl+F10");
	apply("amiberry.open_gui", "");
	apply("amiberry.kbd_led_num", "3");
	apply("amiberry.minimized_input", "4");
	apply("amiberry.minimized_priority", "2");
	apply("amiberry.active_not_captured_nosound", "true");
	apply("amiberry.active_not_captured_pause", "true");
	emit("misc_priority", p);

	// An RTG board with non-default options (packed entries included).
	reset_prefs(p);
	apply("gfxcard_size", "4");
	apply("gfxcard_type", "PicassoII");
	apply("gfxcard_options:monitor", "1");
	apply("gfxcard_options:initial", "true");
	emit("rtg_hardware", p);
	std::printf("E gfxcard_type=PicassoII\tZorroIII\n");

	reset_prefs(p);
	apply("cycle_exact", "false");
	apply("cpu_model", "68020");
	apply("cpu_24bit_addressing", "false");
	apply("gfxcard_size", "32");
	apply("rtg_modes:24bit", "B8G8R8");
	apply("rtg_modes:16bit", "all");
	apply("gfxcard_multithread", "true");
	apply("gfx_filter_autoscale_rtg", "integer");
	apply("gfx_filter_autoscale_limit_rtg", "1/4");
	apply("amiberry.rtg_vblank", "60");
	apply("amiberry.rtg_scale_aspect_ratio", "0:0");
	apply("gfx_backbuffers_rtg", "2");
	emit("rtg_uae", p);

	// An accelerator board with a jumper and memory, and the
	// miscellaneous expansions.
	reset_prefs(p);
	apply("cpuboard_type", "Blizzard1230IV");
	apply("cpuboard_settings:maprom", "true");
	apply("cpuboardmem1_size", "64");
	apply("cpuboard_rom_file", "/roms/blizzard1230iv.rom");
	apply("bsdsocket_emu", "true");
	apply("scsi", "scsi");
	apply("sana2", "true");
	emit("accelerator", p);

	// Display filters: a manual crop window, shaders and the built-in bezel.
	reset_prefs(p);
	apply("amiberry.gfx_manual_crop", "true");
	apply("amiberry.gfx_manual_crop_width", "640");
	apply("amiberry.gfx_manual_crop_height", "512");
	apply("amiberry.gfx_horizontal_offset", "-12");
	apply("amiberry.gfx_vertical_offset", "8");
	apply("amiberry.shader", "tv");
	apply("amiberry.shader_rtg", "pc");
	apply("amiberry.use_bezel", "true");
	emit("filter", p);
	// The shaders are host-dependent choices, skipped by emit(); the
	// built-in ones are on every host.
	std::printf("E amiberry.shader=tv\tnone\n");
	std::printf("E amiberry.shader_rtg=pc\tnone\n");

	// IO: print to a folder, a serial port with its options, a dongle.
	reset_prefs(p);
	apply("amiberry.parallel_port:folder", "/tmp/prints");
	apply("serial_port", "TCP://0.0.0.0:1234");
	apply("serial_on_demand", "true");
	apply("serial_hardware_ctsrts", "false");
	apply("serial_ri", "true");
	apply("serial_translate", "crlf_cr");
	apply("uaeserial", "true");
	apply("dongle", "leaderboard");
	emit("io", p);
	// The serial port is a host-dependent choice; the network entry is on
	// every host.
	std::printf("E serial_port=TCP://0.0.0.0:1234\tnone\n");

	reset_prefs(p);
	// The A600 extra enables the machine's IDE controller (ide=a600/a1200
	// is written by the core); the layer's ide option itself stays greyed
	// out under a chipset extra, exactly like the old panel.
	apply("chipset_compatible", "A600");
	apply("chipmem_size", "2");
	emit("a600_compatible", p);

	reset_prefs(p);
	apply("cpu_model", "68030");
	apply("cpu_24bit_addressing", "false");
	apply("cycle_exact", "false");
	apply("cpu_compatible", "false");
	apply("chipmem_size", "4");
	apply("bogomem_size", "0");
	apply("fastmem_size", "8");
	apply("z3mem_size", "128");
	apply("z3mem2_size", "16");
	apply("a3000mem_size", "8");
	apply("mbresmem_size", "32");
	apply("megachipmem_size", "32");
	apply("z3mapping", "real");
	emit("bigbox_ram", p);

	reset_prefs(p);
	apply("fastmem_size_k", "512");
	emit("fastmem_kb", p);

	reset_prefs(p);
	apply("cpu_model", "68030");
	apply("cpu_24bit_addressing", "false");
	apply("cycle_exact", "false");
	apply("cpu_compatible", "false");
	apply("maprom", "0x0f000000");
	apply("kickshifter", "true");
	apply("uaeboard", "full");
	emit("maprom_uaeboard", p);

	reset_prefs(p);
	apply("genlock", "true");
	apply("genlockmode", "testcard");
	apply("genlock_mix", "75");
	apply("genlock_alpha", "true");
	apply("genlock_aspect", "true");
	emit("genlock", p);

	// Values next to the fixup rules the layer mirrors: each must come back
	// from the real fixup_prefs() unchanged.
	reset_prefs(p);
	apply("chipset_compatible", "A1200");
	apply("bogomem_size", "6");
	emit("a1200_slow_ram", p);

	reset_prefs(p);
	apply("chipset_compatible", "-");
	apply("cpu_model", "68030");
	apply("ide", "a4000");
	apply("ramsey", "13");
	emit("custom_a4000_ide", p);

	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	apply("waiting_blits", "automatic");
	apply("cycle_exact", "true");
	emit("cycle_exact_68000_blitter", p);

	reset_prefs(p);
	apply("uaeboard", "full_off");
	apply("fastmem2_size_k", "128");
	emit("uaeboard_nodiag", p);

	if (jit_compiled()) {
		reset_prefs(p);
		apply("cpu_model", "68040");
		apply("cycle_exact", "false");
		apply("cpu_compatible", "false");
		apply("cachesize", "8192");
		apply("chipmem_size", "4");
		apply("bogomem_size", "4");
		emit("jit_ram", p);
	}
}

// ---------------------------------------------------------------- machine

// The availability rules the panels used to enforce with disabled widgets
// (and, in three cases, with writes on every frame).
void check_machine_availability()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	// Memory cycle-exact on a 68000 keeps the blitter waiting option off.
	CHECK(!available(p, "waiting_blits"));
	set(p, "cycle_exact", "false");
	CHECK(available(p, "waiting_blits"));
	CHECK(!available(p, "keyboard_nkro")); // default keyboard is UAE
	set(p, "keyboard_type", "a500_6570-036");
	CHECK(available(p, "keyboard_nkro"));
	set(p, "keyboard_type", "a2000_8039");
	CHECK(!available(p, "keyboard_nkro"));
	set(p, "keyboard_type", "disconnected");
	CHECK(available(p, "keyboard_nkro"));

	set(p, "immediate_blits", "true");
	CHECK(!available(p, "waiting_blits"));
	set(p, "immediate_blits", "false");
	CHECK(available(p, "waiting_blits"));

	// Advanced chipset options are greyed out under a chipset extra.
	CHECK(!available(p, "ciaatod"));
	CHECK(!available(p, "ide"));
	CHECK(!available(p, "pcmcia"));
	CHECK(!available(p, "ramsey"));
	// The RTC radios stay usable under an extra, but only the chip of the
	// selected machine survives the configuration checks (fixup_prefs
	// re-runs the built-in chipset table); the default 512 KB slow RAM
	// carries an RTC, so Generic resolves to MSM6242B.
	CHECK(available(p, "rtc"));
	CHECK(choice_available(p, "rtc", "MSM6242B"));
	CHECK(!choice_available(p, "rtc", "RP5C01A"));
	CHECK(value_of(p, "rtc") == "MSM6242B");
	CHECK(set(p, "rtc", "RP5C01A") == AMIBERRY_SETTINGS_UNAVAILABLE);
	set(p, "chipset_compatible", "-");
	CHECK(available(p, "ciaatod"));
	CHECK(available(p, "ide"));
	CHECK(available(p, "ramsey"));
	CHECK(choice_available(p, "rtc", "RP5C01A"));

	// The 1M chip jumper only exists below 1 MB chip RAM.
	CHECK(available(p, "1mchipjumper"));
	set(p, "chipmem_size", "2");
	CHECK(!available(p, "1mchipjumper"));

	// Z3 memory needs 32-bit addressing.
	CHECK(!available(p, "z3mem_size"));
	CHECK(!available(p, "megachipmem_size"));
	CHECK(!available(p, "z3mapping"));
	CHECK(!choice_available(p, "z3mem_size", "512"));
	set(p, "cpu_model", "68030"); // clears 24-bit addressing
	CHECK(value_of(p, "cpu_24bit_addressing") == "false");
	CHECK(available(p, "z3mem_size"));
	CHECK(choice_available(p, "z3mem_size", "512"));
	CHECK(!choice_available(p, "z3mem_size", "1024")); // above the 512 MB window

	// MapROM conflicts with an accelerator board; the UAE board needs a
	// stopped emulator.
	CHECK(available(p, "maprom"));
	CHECK(available(p, "uaeboard"));
	p.cpuboard_type = 1;
	CHECK(!available(p, "maprom"));
	p.cpuboard_type = 0;
	CHECK(!available(p, "uaeboard", settings::context{true}));

	// The genlock group needs a genlock (or genlock effects).
	CHECK(!available(p, "genlockmode"));
	CHECK(!available(p, "genlock_mix"));
	set(p, "genlock", "true");
	CHECK(available(p, "genlockmode"));
	CHECK(available(p, "genlock_mix"));
	CHECK(choice_available(p, "genlockmode", "testcard"));
	CHECK(!choice_available(p, "genlockmode", "video")); // VIDEOGRAB-only
	set(p, "genlockmode", "image");
	CHECK(available(p, "genlock_image"));
	CHECK(!available(p, "genlock_video"));
}

// The dependent changes the RAM and Chipset panels made on interaction.
void check_machine_dependents()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// AGA implies both ECS chips; ECS Agnus follows chip RAM above 512 KB.
	set(p, "chipset", "aga");
	CHECK((p.chipset_mask & CSMASK_AGA) && (p.chipset_mask & CSMASK_ECS_AGNUS)
		&& (p.chipset_mask & CSMASK_ECS_DENISE));
	reset_prefs(p);
	set(p, "chipset", "ocs");
	CHECK(p.chipset_mask == CSMASK_OCS);
	set(p, "chipmem_size", "2");
	CHECK(p.chipset_mask & CSMASK_ECS_AGNUS);

	// Chip RAM above 2 MB pushes Z2 fast RAM out; the other direction too.
	// (chipmem_size counts 512 KB units: "8" is 4 MB, "16" is 8 MB.)
	p.fastmem[0].size = 8 * 1024 * 1024;
	set(p, "chipmem_size", "8");
	CHECK(p.fastmem[0].size <= 262144);
	set(p, "chipmem_size", "16");
	CHECK(p.chipmem.size == 0x800000);
	p.fastmem[0].size = 4 * 1024 * 1024;
	set(p, "fastmem_size", "8");
	CHECK(p.chipmem.size == 0x200000);

	// Z3 memory above the reserved window is unavailable (the panel used
	// to clamp the slider silently); inside it, it stores verbatim.
	set(p, "cpu_model", "68030");
	max_z3fastmem = 256 * 1024 * 1024;
	CHECK(set(p, "z3mem_size", "512") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "z3mem_size", "256") == AMIBERRY_SETTINGS_OK);
	CHECK(p.z3fastmem[0].size == 256 * 1024 * 1024);
	CHECK(set(p, "megachipmem_size", "512") == AMIBERRY_SETTINGS_UNAVAILABLE);
	max_z3fastmem = 512 * 1024 * 1024;
	set(p, "z3mem_size", "512");
	CHECK(p.z3fastmem[0].size == 512 * 1024 * 1024);

	// NTSC moves the output refresh rate, but only on a real change.
	set(p, "ntsc", "true");
	CHECK(p.ntscmode && p.chipset_refreshrate == 60.0f);
	p.chipset_refreshrate = 57.0f;
	set(p, "ntsc", "true"); // re-clicked
	CHECK(p.chipset_refreshrate == 57.0f);
	set(p, "ntsc", "false");
	CHECK(!p.ntscmode && p.chipset_refreshrate == 50.0f);

	// Blitter waiting and immediate blits exclude each other.
	set(p, "immediate_blits", "true");
	CHECK(p.waiting_blits == 0);
	set(p, "immediate_blits", "false");
	set(p, "waiting_blits", "always");
	CHECK(!p.immediate_blits && p.waiting_blits == 3);

	// Cycle exactness: memory on a 68020 applies the WinUAE dependents; off
	// clears everything. (Memory exact needs a 68020: below that it is
	// indistinguishable from full cycle exact and unavailable.)
	reset_prefs(p);
	set(p, "cpu_model", "68020");
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	p.gfx_framerate = 2;
	set(p, "cycle_exact", "memory");
	CHECK(!p.cpu_cycle_exact && p.cpu_memory_cycle_exact && p.blitter_cycle_exact);
	CHECK(p.m68k_speed == 0 && p.cpu_compatible && !p.immediate_blits && p.cachesize == 0
		&& p.gfx_framerate == 1);
	set(p, "cycle_exact", "true");
	CHECK(p.cpu_cycle_exact && p.cpu_memory_cycle_exact && p.blitter_cycle_exact);
	set(p, "cycle_exact", "false");
	CHECK(!p.cpu_cycle_exact && !p.cpu_memory_cycle_exact && !p.blitter_cycle_exact);
	CHECK(!choice_available(p, "cycle_exact", "memory") || p.cpu_model >= 68020);

	// A chipset extra rewrites the advanced chipset options, and the RTC
	// radios normalise to the machine's chip.
	reset_prefs(p);
	set(p, "chipset_compatible", "A500");
	CHECK(!p.cs_df0idhw && !p.cs_resetwarning);
	CHECK(p.cs_rtc == 1 && value_of(p, "rtc") == "MSM6242B");
	set(p, "chipset_compatible", "A3000");
	CHECK(value_of(p, "rtc") == "RP5C01A");

	// Keyboards without N-key rollover force the option on (the panel
	// wrote it every frame while the control was greyed out).
	reset_prefs(p);
	set(p, "keyboard_nkro", "false");
	set(p, "keyboard_type", "UAE");
	CHECK(p.keyboard_nkro);
	set(p, "keyboard_type", "a500_6570-036");
	CHECK(available(p, "keyboard_nkro"));
	set(p, "keyboard_nkro", "false");
	CHECK(!p.keyboard_nkro);

	// Processor slot RAM syncs the accelerator board copy.
	reset_prefs(p);
	p.cpuboard_type = 1;
	set(p, "mbresmem_size", "32");
	CHECK(p.cpuboardmem1.size == 32 * 1024 * 1024);

	// The automatic PPC configuration installs the accelerator board the
	// fixups would (with its RAM mirrored into the processor slot), and
	// leaving it removes the board again.
#ifdef WITH_PPC
	reset_prefs(p);
	set(p, "cpu_model", "68060");
	set(p, "ppc_model", "automatic");
	CHECK(p.ppc_mode == 1 && p.cpuboardmem1.size == 8 * 1024 * 1024);
	CHECK(p.mbresmem_high.size == 8 * 1024 * 1024);
	CHECK(value_of(p, "mbresmem_size") == "8");
	set(p, "ppc_model", "");
	CHECK(p.ppc_mode == 0 && p.cpuboard_type == 0 && p.mbresmem_high.size == 0);
#endif
	// The UAE board combo stores board and boot ROM as the panel did.
	set(p, "uaeboard", "full");
	CHECK(p.uaeboard == 2 && p.boot_rom == 0);
	CHECK(value_of(p, "boot_rom_uae") == "automatic");
}

void check_machine_rejections()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(set(p, "chipset", "ocs_agnus") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "chipmem_size", "5") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "bogomem_size", "3") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "collision_level", "4") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "rtc", "RP5C01") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "hvcsync", "nope") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// STRING values are validated by the setter: an unparseable maprom
	// token is accepted but leaves the address unchanged.
	set(p, "maprom", "0x00e00000");
	set(p, "maprom", "not-hex");
	CHECK(value_of(p, "maprom") == "0xe00000");
	set(p, "genlock", "true");
	CHECK(set(p, "genlock_mix", "30") == AMIBERRY_SETTINGS_INVALID_VALUE); // not a step
	CHECK(set(p, "z3mem_size", "8") == AMIBERRY_SETTINGS_UNAVAILABLE); // 24-bit
	CHECK(set(p, "keyboard_nkro", "false") == AMIBERRY_SETTINGS_UNAVAILABLE); // UAE keyboard
	CHECK(set(p, "ide", "a4000") == AMIBERRY_SETTINGS_UNAVAILABLE); // chipset extra active
	CHECK(set(p, "ciaatod", "50hz") == AMIBERRY_SETTINGS_UNAVAILABLE);
}

// The Quickstart constraints, frozen as they were in
// src/osdep/imgui/quickstart.cpp before the move: the layer must reproduce
// them exactly for every model, configuration and compatibility level.
void reference_constraints(uae_prefs& p, int model, int conf, int compa)
{
	if (compa < 0)
		compa = 0;
	if (compa > ((model == 4 || model == 8) ? 4 : 3))
		compa = (model == 4 || model == 8) ? 4 : 3;
	built_in_prefs(&p, model, conf, compa, 0);
	if (model <= 4) {
		if (p.floppyslots[0].dfxtype == DRV_35_HD)
			p.floppyslots[0].dfxtype = DRV_35_DD;
	}
	switch (model) {
	case 0: case 1: case 2: case 3: case 4: case 5:
		if (p.floppyslots[0].dfxtype == DRV_NONE)
			p.floppyslots[0].dfxtype = DRV_35_DD;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = false;
		p.cdslots[0].type = SCSI_UNIT_DISABLED;
		p.jports[1].mode = 0;
		break;
	case 6: case 7: case 12:
		if (p.floppyslots[0].dfxtype == DRV_NONE)
			p.floppyslots[0].dfxtype = DRV_35_HD;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = false;
		p.cdslots[0].type = SCSI_UNIT_DISABLED;
		p.jports[1].mode = 0;
		break;
	case 8: case 9: case 10: case 11:
		p.floppyslots[0].dfxtype = DRV_NONE;
		p.floppyslots[1].dfxtype = DRV_NONE;
		p.cdslots[0].inuse = true;
		p.cdslots[0].type = SCSI_UNIT_DEFAULT;
		p.gfx_monitor[0].gfx_size.width = 720;
		p.gfx_monitor[0].gfx_size.height = 568;
		p.jports[1].mode = 7;
		break;
	default:
		break;
	}
}

// Values the fixups would change are not offered, setters make the changes
// the fixups would, and the mirrors of fixup tables agree with the real ones.
void check_machine_fixup_rules()
{
	uae_prefs& p = test_prefs;
	static uae_prefs q; // uae_prefs is too large for the stack

	// Slow RAM 1.8 MB is capped for the Gayle/Gary conflict after the
	// chipset extra's table ran; the layer's mirror must agree with the real
	// table for every extra, CPU class and IDE controller.
	for (int cs = 0; cs <= CP_DRACO; cs++) {
		for (const int cpu : {68000, 68020}) {
			for (const int ide : {0, IDE_A600A1200}) {
				reset_prefs(p);
				p.cs_compatible = cs;
				p.cpu_model = cpu;
				p.cs_ide = ide;
				p.cs_fatgaryrev = p.cs_ramseyrev = -1;
				std::memcpy(static_cast<void*>(&q), static_cast<const void*>(&p), sizeof q);
				built_in_chipset_prefs(&q);
				q.cs_ide = ide; // fixup_prefs keeps the configured controller
				const bool capped = q.cs_ide || q.cs_fatgaryrev >= 0 || q.cs_ramseyrev >= 0;
				if (choice_available(p, "bogomem_size", "7") == capped) {
					std::fprintf(stderr, "bogomem_size 7: chipset %d cpu %d ide %d disagrees with the table\n",
						cs, cpu, ide);
					failures++;
				}
			}
		}
	}
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_ide = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(choice_available(p, "bogomem_size", "7"));
	p.cs_ramseyrev = 0x0f;
	CHECK(!choice_available(p, "bogomem_size", "7"));

	// The JIT cannot map 1.5 MB chip or slow RAM.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		p.cpu_model = 68020;
		p.cachesize = 8192;
		CHECK(!choice_available(p, "chipmem_size", "3"));
		CHECK(!choice_available(p, "bogomem_size", "6"));
		CHECK(choice_available(p, "chipmem_size", "4"));
		p.cachesize = 0;
		CHECK(choice_available(p, "chipmem_size", "3"));
		CHECK(choice_available(p, "bogomem_size", "6"));
	}

	// Chip RAM above 2 MB and a Zorro II RTG board exclude each other.
	reset_prefs(p);
	p.rtgboards[0].rtgmem_size = 0x400000;
	p.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z2;
	CHECK(!choice_available(p, "chipmem_size", "8"));
	CHECK(choice_available(p, "chipmem_size", "4"));
	p.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
	CHECK(choice_available(p, "chipmem_size", "8"));

	// An accelerator whose RAM is the Z2 board follows the chip RAM shrink,
	// so the fixup's copy back does not undo it.
	reset_prefs(p);
	p.cpuboard_type = 1;
	p.cpuboard_subtype = 2; // Test Z2
	p.fastmem[0].size = p.cpuboardmem1.size = 0x400000;
	CHECK(set(p, "chipmem_size", "8") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fastmem[0].size <= 0x40000 && p.cpuboardmem1.size == p.fastmem[0].size);

	// Z2 boards below 1 MB must be a power of two of at least 64 KB.
	reset_prefs(p);
	CHECK(set(p, "fastmem_size_k", "192") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "fastmem2_size_k", "896") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "fastmem3_size_k", "128") == AMIBERRY_SETTINGS_OK && p.fastmem[2].size == 0x20000);
	CHECK(set(p, "fastmem_size_k", "256") == AMIBERRY_SETTINGS_OK && p.fastmem[0].size == 0x40000);
	// Boards up to 256 KB fit next to chip RAM above 2 MB; bigger ones in
	// the first two slots push it back to 2 MB.
	reset_prefs(p);
	p.address_space_24 = false;
	CHECK(set(p, "chipmem_size", "8") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "fastmem_size_k", "256") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "chipmem_size") == "8");
	CHECK(set(p, "fastmem3_size", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "chipmem_size") == "8");
	CHECK(set(p, "fastmem2_size_k", "512") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "chipmem_size") == "4");
	// Raising chip RAM above 2 MB shrinks only the two conflicting boards.
	reset_prefs(p);
	p.address_space_24 = false;
	p.fastmem[0].size = 0x400000;
	p.fastmem[2].size = 0x100000;
	p.fastmem[3].size = 0x200000;
	CHECK(set(p, "chipmem_size", "8") == AMIBERRY_SETTINGS_OK);
	CHECK(p.fastmem[0].size == 262144);
	CHECK(p.fastmem[2].size == 0x100000 && p.fastmem[3].size == 0x200000);

	// Wait for Blitter goes with 68000/010 cycle-exact modes, from either
	// side.
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	CHECK(set(p, "waiting_blits", "automatic") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cycle_exact", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "waiting_blits") == "disabled");
	reset_prefs(p);
	p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
	CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cycle_exact", "memory") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "waiting_blits", "automatic") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpu_model", "68000") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "waiting_blits") == "disabled");

	// The A4000 IDE controller keeps Gary and Ramsey on a custom chipset.
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(set(p, "ide", "a4000") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cs_fatgaryrev == 0 && p.cs_ramseyrev == 0x0f);
	CHECK(set(p, "ramsey", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "fatgary", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "ramsey", "13") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "ide", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "ramsey", "-1") == AMIBERRY_SETTINGS_OK);

	// Genlock cannot be on with A2024 or Graffiti emulation.
	reset_prefs(p);
	p.monitoremu = 1;
	CHECK(!choice_available(p, "genlock", "true"));
	CHECK(set(p, "genlock", "true") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "genlock", "false") == AMIBERRY_SETTINGS_OK);

	// The cartridge ident follows the cartridge file.
	reset_prefs(p);
	CHECK(set(p, "cart_file", "/roms/ar3.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.cartident, ":AR3") == 0);
	CHECK(set(p, "cart_file", "/roms/unknown.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cartident[0] == 0);
	// Writing back the current path keeps an ident typed for a custom dump.
	std::snprintf(p.cartident, sizeof p.cartident, ":DeMoNv1");
	CHECK(set(p, "cart_file", "/roms/unknown.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.cartident, ":DeMoNv1") == 0);

	// Options that make a RAM size impossible change it the way the fixup
	// would: the JIT turns 1.5 MB chip into 2 MB and 1.5 MB slow into 1 MB,
	// and a chipset extra with Gayle/Gary caps 1.8 MB slow RAM at 1.5 MB.
	if (jit_compiled()) {
		reset_prefs(p);
		p.cpu_cycle_exact = p.cpu_memory_cycle_exact = p.blitter_cycle_exact = false;
		CHECK(set(p, "cpu_24bit_addressing", "false") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cpu_model", "68020") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "chipmem_size", "3") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "bogomem_size", "6") == AMIBERRY_SETTINGS_OK);
		CHECK(set(p, "cachesize", "8192") == AMIBERRY_SETTINGS_OK);
		CHECK(value_of(p, "chipmem_size") == "4");
		CHECK(value_of(p, "bogomem_size") == "4");
	}
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_ide = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(set(p, "bogomem_size", "7") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "chipset_compatible", "A1200") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "bogomem_size") == "6");
	reset_prefs(p);
	p.cs_compatible = 0;
	p.cs_ide = 0;
	p.cs_fatgaryrev = p.cs_ramseyrev = -1;
	CHECK(set(p, "bogomem_size", "7") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "ramsey", "15") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "bogomem_size") == "6");


	// The UAE board reports and stores the no-diagnostics spellings.
	reset_prefs(p);
	p.uaeboard = 2;
	p.uaeboard_nodiag = true;
	CHECK(value_of(p, "uaeboard") == "full_off");
	CHECK(set(p, "uaeboard", "min") == AMIBERRY_SETTINGS_OK && p.uaeboard == 1 && !p.uaeboard_nodiag);
	CHECK(set(p, "uaeboard", "full+indirect_off") == AMIBERRY_SETTINGS_OK
		&& p.uaeboard == 3 && p.uaeboard_nodiag);

	// MapROM takes one hexadecimal address.
	reset_prefs(p);
	CHECK(set(p, "maprom", "zz") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "maprom", "0x") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "maprom", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "maprom", "e00000") == AMIBERRY_SETTINGS_OK && p.maprom == 0xe00000);
	CHECK(set(p, "maprom", "0x00f00000") == AMIBERRY_SETTINGS_OK && p.maprom == 0xf00000);
}

void check_presets()
{
	// Preset metadata: the panel's model list, configurations and
	// compatibility tiers.
	CHECK(amiberry_settings_preset_count() == 13);
	CHECK(amiberry_settings_preset_at(13) == nullptr);
	CHECK(amiberry_settings_preset_find("a1200") == amiberry_settings_preset_at(4));
	CHECK(amiberry_settings_preset_find("nope") == nullptr);
	const amiberry_settings_preset* a1200 = amiberry_settings_preset_find("a1200");
	CHECK(a1200 && a1200->compatibility_max == 4);
	CHECK(amiberry_settings_preset_find("a500")->compatibility_max == 3);
	// Only the levels each builder implements.
	CHECK(amiberry_settings_preset_find("a3000")->compatibility_max == 1);
	CHECK(amiberry_settings_preset_find("a4000")->compatibility_max == 0);
	CHECK(amiberry_settings_preset_find("a4000t")->compatibility_max == 0);
	CHECK(amiberry_settings_preset_find("macrosystem")->compatibility_max == 0);
	CHECK(amiberry_settings_preset_config_label(a1200, -1) == nullptr);
	CHECK(amiberry_settings_preset_config_label(a1200, 1)
		&& std::strstr(amiberry_settings_preset_config_label(a1200, 1), "4 MB Fast"));
	CHECK(amiberry_settings_preset_compatibility_label(4)
		&& std::strstr(amiberry_settings_preset_compatibility_label(4), "JIT"));
	CHECK(amiberry_settings_preset_compatibility_label(5) == nullptr);
	CHECK(settings::preset_model_index("cd32") == 8);
	CHECK(std::strcmp(settings::preset_id(8), "cd32") == 0);

	// Every preset applied through the layer equals the frozen Quickstart
	// reference, byte for byte, for every configuration and tier.
	static uae_prefs reference; // uae_prefs is too large for the stack
	for (size_t i = 0; i < amiberry_settings_preset_count(); i++) {
		const amiberry_settings_preset* preset = amiberry_settings_preset_at(i);
		for (int conf = 0; conf < preset->config_count; conf++) {
			for (int compa = 0; compa <= preset->compatibility_max; compa++) {
				reset_prefs(test_prefs);
				reset_prefs(reference);
				settings::apply_preset(test_prefs, settings::preset_model_index(preset->id), conf, compa, false);
				reference_constraints(reference, settings::preset_model_index(preset->id), conf, compa);
				if (std::memcmp(&test_prefs, &reference, sizeof(uae_prefs)) != 0) {
					std::fprintf(stderr, "preset %s config %d compa %d differs from the Quickstart reference\n",
						preset->id, conf, compa);
					failures++;
				}
			}
		}
		// Every model has a configuration (ALG and Arcadia apply the first
		// game ROM, as Quickstart did) and the engine saw the model.
		CHECK(preset->config_count > 0);
		CHECK(stub_preset_model == settings::preset_model_index(preset->id));
	}
	CHECK(amiberry_settings_preset_find("alg")->config_count == 1);
	CHECK(amiberry_settings_preset_find("arcadia")->config_count == 1);
	// Out-of-range compatibility is clamped, not rejected, at this level.
	reset_prefs(test_prefs);
	settings::apply_preset(test_prefs, 4, 0, 99, false);
	CHECK(stub_preset_compa == 4);
	// Live application (Quickstart, --apply-preset) runs the engine as a
	// configuration load does; preview never does.
	const int preview_calls = stub_preview_calls;
	settings::apply_preset(test_prefs, 4, 0, 1, false);
	CHECK(stub_preview_calls == preview_calls);
	settings::apply_preset(test_prefs, 4, 0, 1, true);
	CHECK(stub_preview_calls == preview_calls + 1);

	// ROM availability cannot be tested against a real ROM scan here; it
	// must reject unknown arguments, use the engine's preview (which
	// leaves global state alone) and never touch the config.
	reset_prefs(changed_prefs);
	CHECK(!amiberry_settings_preset_rom_installed("nope", 0));
	CHECK(!amiberry_settings_preset_rom_installed("a500", 99));
	CHECK(amiberry_settings_preset_rom_installed("arcadia", 0));
	CHECK(stub_preview_calls == preview_calls + 2);
	// The ALG/Arcadia builders succeed with only the BIOS; no game means
	// the preset is not installed.
	arcadia_game_installed = false;
	CHECK(!amiberry_settings_preset_rom_installed("arcadia", 0));
	CHECK(!amiberry_settings_preset_rom_installed("alg", 0));
	// Applied without a game, the builder is asked for no game at all
	// rather than for the first known entry (an Arcadia BIOS).
	settings::apply_preset(test_prefs, 11, 0, 1, false);
	CHECK(stub_preset_config == -1);
	arcadia_game_installed = true;
	// With one, the configuration is the first installed game's index in
	// getarcadiaroms(), past the Arcadia BIOS entries.
	settings::apply_preset(test_prefs, 11, 0, 1, false);
	CHECK(stub_preset_config == 1);
	settings::apply_preset(test_prefs, 10, 0, 1, false);
	CHECK(stub_preset_config == 0);
	CHECK(settings::arcade_game_config(4) == -1);
	CHECK(changed_prefs.cpu_model == 68000 && changed_prefs.keyboard_leds[0] == 0);
}

// The accelerator board and the miscellaneous expansions.
void check_expansions_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// No board: only the board choice, and none of what a board brings.
	CHECK(value_of(p, "cpuboard_type") == "none");
	CHECK(!available(p, "cpuboard_settings") && !available(p, "cpuboard_settings:maprom"));
	CHECK(!available(p, "cpuboardmem1_size") && !available(p, "cpuboard_rom_file"));
	CHECK(set(p, "cpuboard_type", "NoSuchBoard") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpuboard_type", "Blizzard1230IV") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpuboard_type") == "Blizzard1230IV" && p.cpuboard_type == BOARD_BLIZZARD);
#ifdef WITH_PPC
	CHECK(choice_available(p, "cpuboard_type", "BlizzardPPC"));
#else
	CHECK(!choice_available(p, "cpuboard_type", "BlizzardPPC"));
	CHECK(!choice_available(p, "cpuboard_type", "CyberStormPPC"));
#endif

	// A board raises the CPU to the one it carries and leaves 24-bit
	// addressing; its jumpers start off.
	reset_prefs(p);
	p.cpu_model = 68000;
	p.address_space_24 = true;
	CHECK(set(p, "cpuboard_type", "Test030") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboard_type == 1 && p.cpuboard_subtype == 0);
	CHECK(p.cpu_model == 68030 && !p.address_space_24);
	CHECK(value_of(p, "cpuboard_settings") == "");
	CHECK(available(p, "cpuboard_settings:maprom") && !available(p, "cpuboard_settings:scsikitii"));
	CHECK(set(p, "cpuboard_settings:maprom", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboard_settings == 1 && value_of(p, "cpuboard_settings") == "maprom");
	CHECK(value_of(p, "cpuboard_settings:maprom") == "true");
	// Choosing the board again (a re-clicked control) keeps its jumpers.
	CHECK(set(p, "cpuboard_type", "Test030") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboard_settings == 1);

	// The packed entry takes the variant's jumpers only.
	CHECK(set(p, "cpuboard_settings", "maprom,bogus") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpuboard_settings", "scsikitii") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpuboard_settings", std::string(300, 'm').c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpuboard_settings", "") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 0);
	CHECK(set(p, "cpuboard_settings", "MAPROM") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 1);

	// Two jumpers set one at a time (exact token match: "disabled" is not
	// "scsidisabled").
	CHECK(set(p, "cpuboard_type", "Test040") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboard_settings == 0 && p.cpu_model == 68040);
	CHECK(set(p, "cpuboard_settings:scsidisabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpuboard_settings:disabled") == "false");
	CHECK(set(p, "cpuboard_settings:disabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpuboard_settings") == "disabled,scsidisabled");
	CHECK(set(p, "cpuboard_settings:scsidisabled", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "cpuboard_settings") == "disabled");

	// Board memory: up to the board's most, mirrored into the memory slot
	// the board uses; a smaller board cuts it down.
	reset_prefs(p);
	CHECK(set(p, "cpuboard_type", "Test030") == AMIBERRY_SETTINGS_OK);
	CHECK(choice_available(p, "cpuboardmem1_size", "128") && !choice_available(p, "cpuboardmem1_size", "256"));
	CHECK(set(p, "cpuboardmem1_size", "256") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "cpuboardmem1_size", "128") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboardmem1.size == 128 * 1024 * 1024 && p.mbresmem_high.size == 128 * 1024 * 1024);
	CHECK(set(p, "cpuboard_type", "Test040") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboardmem1.size == 64 * 1024 * 1024 && p.mbresmem_high.size == 64 * 1024 * 1024);

	// The boot ROM: a new file drops the old file's ROM ident; the current
	// one written back keeps it.
	CHECK(set(p, "cpuboard_rom_file", "/roms/board.rom") == AMIBERRY_SETTINGS_OK);
	int index = 0;
	boardromconfig* brc = get_device_rom(&p, ROMTYPE_CPUBOARD, 0, &index);
	CHECK(brc && !std::strcmp(brc->roms[index].romfile, "/roms/board.rom"));
	if (brc) {
		std::strcpy(brc->roms[index].romident, ":BOARDROM");
		CHECK(set(p, "cpuboard_rom_file", "/roms/board.rom") == AMIBERRY_SETTINGS_OK);
		CHECK(!std::strcmp(brc->roms[index].romident, ":BOARDROM"));
		CHECK(set(p, "cpuboard_rom_file", "/roms/other.rom") == AMIBERRY_SETTINGS_OK);
		CHECK(brc->roms[index].romident[0] == 0 && value_of(p, "cpuboard_rom_file") == "/roms/other.rom");
	}

	// No board again: no jumpers.
	CHECK(set(p, "cpuboard_type", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cpuboard_type == 0 && p.cpuboard_subtype == 0 && p.cpuboard_settings == 0);

#ifdef WITH_PPC
	// A PPC board is the manual PPC configuration; another board leaves it,
	// and the automatic one too (which would put its own board back).
	reset_prefs(p);
	CHECK(set(p, "cpuboard_type", "BlizzardPPC") == AMIBERRY_SETTINGS_OK && p.ppc_mode == 2);
	CHECK(set(p, "cpuboard_type", "Blizzard1260") == AMIBERRY_SETTINGS_OK && p.ppc_mode == 0);
	p.ppc_mode = 1;
	CHECK(set(p, "cpuboard_type", "Blizzard1230IV") == AMIBERRY_SETTINGS_OK && p.ppc_mode == 0);
#endif

	// Every switch and selector of every board has its part.
	for (int t = 1; cpuboards[t].name; t++) {
		for (int s = 0; cpuboards[t].subtypes && cpuboards[t].subtypes[s].name; s++) {
			const expansionboardsettings* table = cpuboards[t].subtypes[s].settings;
			for (int i = 0; table && table[i].name; i++) {
				const std::string config = table[i].configname;
				const settings::definition* d = settings::find("cpuboard_settings:" + config);
				if (table[i].type == EXPANSIONBOARD_MULTI && d && d->descriptor.type != AMIBERRY_SETTING_CHOICE)
					d = settings::find("cpuboard_settings:" + config + "_choice");
				CHECK(d && d->descriptor.type == (table[i].type == EXPANSIONBOARD_MULTI
					? AMIBERRY_SETTING_CHOICE : AMIBERRY_SETTING_BOOL));
			}
		}
	}

	// A selector and an inverted switch, set part by part and as the entry
	// (the core's codec: 4 positions take 2 bits; burst sits one bit above
	// autoboot and reads inverted).
	reset_prefs(p);
	CHECK(set(p, "cpuboard_type", "Test25") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "cpuboard_settings:memory_choice") && !available(p, "cpuboard_settings:memory"));
	CHECK(value_of(p, "cpuboard_settings:memory_choice") == "4m");
	CHECK(value_of(p, "cpuboard_settings:burst") == "true");
	CHECK(set(p, "cpuboard_settings:memory_choice", "16m") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 2);
	CHECK(value_of(p, "cpuboard_settings") == "16m,burst");
	CHECK(set(p, "cpuboard_settings:burst", "false") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 0x12);
	CHECK(value_of(p, "cpuboard_settings") == "16m");
	CHECK(set(p, "cpuboard_settings", "8m,autoboot,burst") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 5);
	CHECK(value_of(p, "cpuboard_settings:memory_choice") == "8m" && value_of(p, "cpuboard_settings:autoboot") == "true");
	CHECK(value_of(p, "cpuboard_settings") == "8m,autoboot,burst"); // its own spelling is accepted back
	CHECK(set(p, "cpuboard_settings", "memory") == AMIBERRY_SETTINGS_INVALID_VALUE); // the selector, not a position
	CHECK(set(p, "cpuboard_settings", "64m") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "cpuboard_settings", "") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 0); // no entry
	CHECK(value_of(p, "cpuboard_settings:burst") == "true");
	CHECK(set(p, "cpuboard_type", "TestApollo") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "cpuboard_settings:memory") && !available(p, "cpuboard_settings:memory_choice"));
	// A switch boards label differently is named after itself.
	CHECK(!std::strcmp(settings::require("cpuboard_settings:autoboot").descriptor.label, "Autoboot jumper"));
	CHECK(!std::strcmp(settings::require("cpuboard_settings:scsi").descriptor.label, "SCSI"));

	// A selector two boards share, each with its own positions.
	CHECK(value_of(p, "cpuboard_settings:hwrev") == "3");
	CHECK(set(p, "cpuboard_settings:hwrev", "4") == AMIBERRY_SETTINGS_OK && value_of(p, "cpuboard_settings:hwrev") == "4");
	CHECK(set(p, "cpuboard_type", "TestZ2") == AMIBERRY_SETTINGS_OK);
	CHECK(!choice_available(p, "cpuboard_settings:hwrev", "3"));
	CHECK(value_of(p, "cpuboard_settings:hwrev") == "4"); // its first position
	CHECK(set(p, "cpuboard_settings:hwrev", "5") == AMIBERRY_SETTINGS_OK && p.cpuboard_settings == 1);
	CHECK(value_of(p, "cpuboard_settings:hwrev") == "5" && value_of(p, "cpuboard_settings") == "5");

	// The old board's RAM copy goes when the new board keeps its RAM
	// elsewhere (the 25-bit slot has no option of its own).
	reset_prefs(p);
	CHECK(set(p, "cpuboard_type", "Test25") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "cpuboardmem1_size", "32") == AMIBERRY_SETTINGS_OK && p.mem25bit.size == 32 * 1024 * 1024);
	CHECK(set(p, "cpuboard_type", "Test030") == AMIBERRY_SETTINGS_OK);
	CHECK(p.mem25bit.size == 0 && p.mbresmem_high.size == 32 * 1024 * 1024);
	CHECK(set(p, "cpuboard_type", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(p.mbresmem_high.size == 0 && p.mem25bit.size == 0);

	// A ROM entry a session adds points at its board in changed_prefs, not
	// into the session's freed copy.
	reset_prefs(changed_prefs);
	{
		amiberry_settings_session* session = amiberry_settings_session_open();
		CHECK(session);
		CHECK(amiberry_settings_set(session, "cpuboard_type", "Test030") == AMIBERRY_SETTINGS_OK);
		CHECK(amiberry_settings_set(session, "cpuboard_rom_file", "/roms/board.rom") == AMIBERRY_SETTINGS_OK);
		bool changed = false;
		amiberry_settings_commit(session, &changed);
		CHECK(changed);
		amiberry_settings_session_close(session);
		int index = 0;
		boardromconfig* brc = get_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, &index);
		CHECK(brc && brc->roms[index].back == brc);

		// With a board in changed_prefs, a session that changes nothing
		// commits nothing, and leaves the boards as they are.
		static boardromconfig before[MAX_EXPANSION_BOARDS];
		std::memcpy(before, changed_prefs.expansionboard, sizeof before);
		session = amiberry_settings_session_open();
		changed = true;
		amiberry_settings_commit(session, &changed);
		CHECK(!changed);
		CHECK(!std::memcmp(before, changed_prefs.expansionboard, sizeof before));
		amiberry_settings_session_close(session);

		// A board added elsewhere while a session is open survives the
		// session's accelerator ROM change: boards merge one by one.
		session = amiberry_settings_session_open();
		CHECK(amiberry_settings_set(session, "cpuboard_rom_file", "/roms/other.rom") == AMIBERRY_SETTINGS_OK);
		boardromconfig* added = get_device_rom_new(&changed_prefs, ROMTYPE_A2091, 0, &index);
		CHECK(added != nullptr);
		if (added)
			std::strcpy(added->roms[0].romfile, "/roms/a2091.rom");
		amiberry_settings_commit(session, &changed);
		amiberry_settings_session_close(session);
		brc = get_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, &index);
		CHECK(brc && !std::strcmp(brc->roms[index].romfile, "/roms/other.rom"));
		boardromconfig* kept = get_device_rom(&changed_prefs, ROMTYPE_A2091, 0, &index);
		CHECK(kept && !std::strcmp(kept->roms[0].romfile, "/roms/a2091.rom"));
	}

	// The miscellaneous expansions.
	reset_prefs(p);
	CHECK(value_of(p, "scsi") == "false" && value_of(p, "bsdsocket_emu") == "false");
	p.automount_cddrives = true;
	CHECK(!choice_available(p, "scsi", "false")); // mapped CD drives need it
	p.automount_cddrives = false;
	CHECK(set(p, "scsi", "scsi") == AMIBERRY_SETTINGS_OK && p.scsi == 2);
	CHECK(set(p, "scsi", "true") == AMIBERRY_SETTINGS_OK && p.scsi == 1);
	CHECK(set(p, "bsdsocket_emu", "true") == AMIBERRY_SETTINGS_OK && p.socket_emu);
	CHECK(set(p, "sana2", "true") == AMIBERRY_SETTINGS_OK && p.sana2);
}

// Expansion boards: the board types the GUI offers, and units read and
// edited by type, unit and field.
std::string board_value(const uae_prefs& p, const char* type, int unit, const char* field)
{
	char text[MAX_DPATH];
	if (settings::board_get(p, type, unit, field, text, sizeof text) != AMIBERRY_SETTINGS_OK)
		return "<error>";
	return text;
}

amiberry_settings_status board_set(uae_prefs& p, const char* type, int unit, const char* field, const char* value)
{
	return settings::board_set(p, settings::context{false}, type, unit, field, value);
}

bool board_available(const uae_prefs& p, const char* type, int unit, const char* field)
{
	return settings::board_field_available(p, settings::context{false}, type, unit, field);
}

std::vector<std::string> board_fields(const char* type)
{
	std::vector<std::string> ids;
	const amiberry_settings_board_type* t = settings::board_type_find(type);
	for (size_t i = 0; t && i < t->field_count; i++)
		ids.emplace_back(settings::board_field_at(t, i)->id);
	return ids;
}

void check_boards()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// The catalog: every expansion board but the accelerator, with its
	// categories, its unit count (Zorro II/III boards that are not single
	// can be fitted several times) and the fields it has.
	CHECK(settings::board_type_count() == 9 && !settings::board_type_find("cpuboard"));
	const amiberry_settings_board_type* a2091 = settings::board_type_find("a2091");
	CHECK(a2091 && a2091->units == MAX_AVAILABLE_DUPLICATE_EXPANSION_BOARDS);
	CHECK(a2091 && a2091->categories == AMIBERRY_BOARD_SCSI && !std::strcmp(a2091->label, "A590/A2091"));
	CHECK(settings::board_type_find("testscsi")->units == 1);
	CHECK(settings::board_type_find("ide_mb")->categories == (AMIBERRY_BOARD_INTERNAL | AMIBERRY_BOARD_IDE));
	CHECK(board_fields("a2091") == (std::vector<std::string>{"enabled", "subtype", "rom_file", "autoboot_disabled", "dma24bit"}));
	CHECK(board_fields("testscsi")
		== (std::vector<std::string>{"enabled", "rom_file", "scsi_id", "setting:fast", "setting:speed", "setting:name"}));
	CHECK(board_fields("testpcmcia") == (std::vector<std::string>{"enabled", "inserted"}));
	CHECK(settings::board_field_at(a2091, 1)->type == AMIBERRY_SETTING_CHOICE
		&& settings::board_field_at(a2091, 1)->choice_count == 2);
	CHECK(settings::board_field_at(a2091, 0)->apply == AMIBERRY_APPLY_RESTART);

	// A unit without a board has nothing but its switch.
	CHECK(board_value(p, "a2091", 0, "enabled") == "false");
	CHECK(!board_available(p, "a2091", 0, "subtype"));
	CHECK(board_set(p, "a2091", 0, "subtype", "dmac02") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(board_set(p, "a2091", 4, "enabled", "true") == AMIBERRY_SETTINGS_UNKNOWN_ID); // units 0-3
	CHECK(board_set(p, "nosuchboard", 0, "enabled", "true") == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(board_set(p, "a2091", 0, "nosuchfield", "true") == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(board_set(p, "a2091", 0, "enabled", "yes") == AMIBERRY_SETTINGS_INVALID_VALUE);

	// Enabled without a ROM file, it takes the scanned ROM of its type (what
	// it loads anyway), so the configuration keeps the board.
	CHECK(board_set(p, "a2091", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(board_value(p, "a2091", 0, "enabled") == "true");
	CHECK(board_value(p, "a2091", 0, "rom_file") == "/roms/a2091.rom");
	CHECK(board_value(p, "a2091", 0, "subtype") == "dmac01");
	// 24-bit DMA comes with the DMAC-02 variant.
	CHECK(!board_available(p, "a2091", 0, "dma24bit"));
	CHECK(board_set(p, "a2091", 0, "subtype", "dmac02") == AMIBERRY_SETTINGS_OK);
	CHECK(board_set(p, "a2091", 0, "dma24bit", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(board_set(p, "a2091", 0, "autoboot_disabled", "true") == AMIBERRY_SETTINGS_OK);
	int index = 0;
	boardromconfig* brc = get_device_rom(&p, ROMTYPE_A2091, 0, &index);
	CHECK(brc && brc->roms[0].subtype == 1 && brc->roms[0].dma24bit && brc->roms[0].autoboot_disabled);
	// No file is a board without its ROM; a new file drops the old ROM ident.
	if (brc) {
		std::strcpy(brc->roms[0].romident, ":A2091");
		CHECK(board_set(p, "a2091", 0, "rom_file", "") == AMIBERRY_SETTINGS_OK);
		CHECK(!std::strcmp(brc->roms[0].romfile, ":NOROM") && brc->roms[0].romident[0] == 0);
		CHECK(board_value(p, "a2091", 0, "rom_file") == "");
	}
	// A second unit is a board of its own.
	CHECK(board_set(p, "a2091", 3, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(board_value(p, "a2091", 3, "subtype") == "dmac01");
	CHECK(board_set(p, "a2091", 0, "enabled", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(!get_device_rom(&p, ROMTYPE_A2091, 0, &index) && get_device_rom(&p, ROMTYPE_A2091, 3, &index));

	// A board without a scanned ROM keeps ":NOROM"; its ID jumper, switch,
	// selector and text settings.
	CHECK(board_set(p, "testscsi", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	brc = get_device_rom(&p, ROMTYPE_A4091, 0, &index);
	CHECK(brc && !std::strcmp(brc->roms[0].romfile, ":NOROM"));
	CHECK(board_value(p, "testscsi", 0, "scsi_id") == "7");
	CHECK(board_set(p, "testscsi", 0, "scsi_id", "6") == AMIBERRY_SETTINGS_OK);
	CHECK(board_set(p, "testscsi", 0, "scsi_id", "8") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(board_set(p, "testscsi", 0, "setting:fast", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(board_set(p, "testscsi", 0, "setting:speed", "fast") == AMIBERRY_SETTINGS_OK);
	CHECK(board_set(p, "testscsi", 0, "setting:name", "Disk one") == AMIBERRY_SETTINGS_OK);
	if (brc) {
		// Switch bit 0, the 2-position selector one bit above it.
		CHECK(brc->roms[0].device_id == 6 && brc->roms[0].device_settings == 3);
		CHECK(!std::strcmp(brc->roms[0].configtext, "Disk one"));
	}
	CHECK(board_value(p, "testscsi", 0, "setting:speed") == "fast" && board_value(p, "testscsi", 0, "setting:name") == "Disk one");
	CHECK(board_set(p, "testscsi", 0, "setting:name", std::string(ROMCONFIG_CONFIGTEXT_LEN, 'x').c_str())
		== AMIBERRY_SETTINGS_INVALID_VALUE);

	// A board without a ROM is enabled as the core marks it.
	CHECK(board_set(p, "testpcmcia", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	brc = get_device_rom(&p, ROMTYPE_NE2KPCMCIA, 0, &index);
	CHECK(brc && !std::strcmp(brc->roms[0].romfile, ":ENABLED"));
	CHECK(board_set(p, "testpcmcia", 0, "inserted", "true") == AMIBERRY_SETTINGS_OK && brc && brc->roms[0].inserted);

	// The built-in IDE controller follows cs_ide on any machine (fixup_prefs
	// keeps the user's over the machine's), as the machine's controller.
	reset_prefs(p);
	p.cs_compatible = CP_A4000;
	CHECK(board_available(p, "ide_mb", 0, "enabled"));
	CHECK(board_set(p, "ide_mb", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cs_ide == IDE_A4000 && get_device_rom(&p, ROMTYPE_MB_IDE, 0, &index));
	CHECK(board_value(p, "ide_mb", 0, "enabled") == "true");
	CHECK(board_set(p, "ide_mb", 0, "enabled", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cs_ide == 0 && !get_device_rom(&p, ROMTYPE_MB_IDE, 0, &index));
	// Not on the Generic chipset, which sets them from the CPU.
	p.cs_compatible = CP_GENERIC;
	CHECK(!board_available(p, "ide_mb", 0, "enabled") && !board_available(p, "scsi_a3000", 0, "enabled"));
	CHECK(board_available(p, "cd32fmv", 0, "enabled"));
	p.cs_compatible = CP_A4000;
	// A3000 or A4000T SCSI, one of the two (the core builds neither for a
	// value other than 1 or 2), and enabled is what gets built.
	CHECK(board_set(p, "scsi_a3000", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK && p.cs_mbdmac == 1);
	CHECK(board_set(p, "scsi_a4000t", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK && p.cs_mbdmac == 2);
	CHECK(board_value(p, "scsi_a3000", 0, "enabled") == "false" && board_value(p, "scsi_a4000t", 0, "enabled") == "true");
	CHECK(board_set(p, "scsi_a3000", 0, "enabled", "false") == AMIBERRY_SETTINGS_OK && p.cs_mbdmac == 2); // not its own
	CHECK(board_set(p, "scsi_a4000t", 0, "enabled", "false") == AMIBERRY_SETTINGS_OK && p.cs_mbdmac == 0);
	p.cs_mbdmac = -1; // a generic chipset's 68000: no SCSI
	cfgfile_compatibility_romtype(&p);
	CHECK(board_value(p, "scsi_a3000", 0, "enabled") == "false" && board_value(p, "scsi_a4000t", 0, "enabled") == "false");
	// The CDTV DMAC or the CDTV-CR (the core builds no DMAC beside the
	// CR), through their chipset options: a custom chipset.
	reset_prefs(p);
	CHECK(!board_available(p, "cdtvdmac", 0, "enabled"));
	p.cs_compatible = 0;
	CHECK(board_set(p, "cdtvcr", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK && p.cs_cdtvcr);
	CHECK(board_set(p, "cdtvdmac", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cs_cdtvcd && !p.cs_cdtvcr && board_value(p, "cdtvdmac", 0, "enabled") == "true");
	CHECK(board_value(p, "cdtvcr", 0, "enabled") == "false");
	// The CD32 FMV cartridge: its flag goes with the entry, or the core
	// would put the cartridge back.
	reset_prefs(p);
	std::strcpy(p.cartfile, "/roms/ar3.rom"); // another cartridge: not the FMV's ROM
	CHECK(board_set(p, "cd32fmv", 0, "enabled", "true") == AMIBERRY_SETTINGS_OK && p.cs_cd32fmv);
	brc = get_device_rom(&p, ROMTYPE_CD32CART, 0, &index);
	CHECK(brc && !std::strcmp(brc->roms[0].romfile, ":NOROM"));
	CHECK(board_set(p, "cd32fmv", 0, "enabled", "false") == AMIBERRY_SETTINGS_OK && !p.cs_cd32fmv);
	cfgfile_compatibility_romtype(&p);
	CHECK(!get_device_rom(&p, ROMTYPE_CD32CART, 0, &index));

	// Through a session: the edits stay in it, and a commit with a changed
	// board needs a restart; one without changes changes nothing.
	reset_prefs(changed_prefs);
	reset_prefs(currprefs);
	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(session);
	CHECK(!amiberry_settings_boards_differ_from_active(session));
	char text[64];
	CHECK(amiberry_settings_board_set(session, "a2091", 1, "enabled", "true") == AMIBERRY_SETTINGS_OK);
	// The running emulation does not have it until it restarts.
	CHECK(amiberry_settings_boards_differ_from_active(session));
	CHECK(amiberry_settings_board_get(session, "a2091", 1, "enabled", text, sizeof text) == AMIBERRY_SETTINGS_OK
		&& !std::strcmp(text, "true"));
	CHECK(amiberry_settings_board_get(session, "a2091", 1, "rom_file", text, 4) == AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
	CHECK(amiberry_settings_board_set(session, "a2091", 1, "enabled", nullptr) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(!get_device_rom(&changed_prefs, ROMTYPE_A2091, 1, &index));
	bool changed = false;
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESTART && changed);
	CHECK(get_device_rom(&changed_prefs, ROMTYPE_A2091, 1, &index));
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE && !changed);
	CHECK(amiberry_settings_boards_differ_from_active(session)); // committed, not yet running
	std::memcpy(static_cast<void*>(&currprefs), static_cast<const void*>(&changed_prefs), sizeof currprefs); // restarted
	CHECK(!amiberry_settings_boards_differ_from_active(session));
	CHECK(!amiberry_settings_boards_differ_from_active(nullptr));
	amiberry_settings_session_close(session);
	reset_prefs(currprefs);
	CHECK(amiberry_settings_board_type_find(nullptr) == nullptr);
	CHECK(amiberry_settings_board_type_at(amiberry_settings_board_type_count()) == nullptr);
}

// Host settings: the Global Settings panel, process-wide, saved explicitly.
void check_host_settings()
{
	CHECK(amiberry_host_settings_count() > 60);
	const amiberry_setting_descriptor* deadzone = amiberry_host_settings_find("default_joystick_deadzone");
	CHECK(deadzone && deadzone->type == AMIBERRY_SETTING_INT && deadzone->min == 0 && deadzone->max == 100);
	CHECK(!amiberry_host_settings_find("nope"));

	char value[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(amiberry_host_settings_get("Quickstart", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "true"));
	CHECK(amiberry_host_settings_set("Quickstart", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(!amiberry_options.quickstart_start);
	CHECK(amiberry_host_settings_set("Quickstart", "yes") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("default_joystick_deadzone", "101") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("default_joystick_deadzone", "4294967336") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_options.default_joystick_deadzone == 33);
	CHECK(amiberry_host_settings_set("default_joystick_deadzone", "40") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_options.default_joystick_deadzone == 40);
	// The legacy exclusive full-screen value is full-window.
	CHECK(amiberry_host_settings_set("default_fullscreen_mode", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_host_settings_get("default_fullscreen_mode", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "2"));
	CHECK(amiberry_host_settings_set("default_fullscreen_mode", "3") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// A GUI scale outside the range is clamped, as the file parser clamps it.
	CHECK(amiberry_host_settings_set("gui_layout_scale", "10") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_options.gui_layout_scale_percent == 50);
	CHECK(amiberry_host_settings_set("gui_layout_scale", "nan") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_options.gui_layout_scale_percent == 100);
	// One line, within the field.
	CHECK(amiberry_host_settings_set("default_open_gui_key", "two\nlines") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("shader", "crt ") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_get("shader", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "none"));
	CHECK(amiberry_host_settings_set("default_vkbd_language", "DE") == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(amiberry_options.default_vkbd_language, "DE"));
	CHECK(amiberry_host_settings_set("default_vkbd_language", "ES") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// A content folder. Another ROMs folder is rescanned, the same one not.
	const int scans = rom_scans;
	CHECK(amiberry_host_settings_set("rom_path", "/roms") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_host_settings_get("rom_path", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "/roms"));
	CHECK(rom_scans == scans + 1);
	CHECK(amiberry_host_settings_set("rom_path", "/roms") == AMIBERRY_SETTINGS_OK);
	CHECK(rom_scans == scans + 1);
	CHECK(amiberry_settings::rescan_roms() == romlist_count() && rom_scans == scans + 2);
	CHECK(loaded_key == get_rom_path() + "rom.key"); // Amiga Forever's ROMs decode
	const int saves = host_saves;
	CHECK(amiberry_host_settings_save());
	CHECK(host_saves == saves + 1);
	host_save_result = false;
	CHECK(!amiberry_host_settings_save());
	host_save_result = true;
	// A path the conf loader would split or trim is refused.
	CHECK(amiberry_host_settings_set("rom_path", "/roms ") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("rom_path", " roms") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("rom_path", std::string(PATH_MAX, 'r').c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_get("rom_path", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "/roms"));
	CHECK(rom_scans == scans + 2); // refused values change nothing
	const amiberry_setting_descriptor* delay = amiberry_host_settings_find("default_whd_configdelay");
	CHECK(delay && delay->max == INT_MAX);
	amiberry_options = {};
}

// Host settings that pick what the host has list it as the editor does, and
// keep a value the list cannot show now.
void check_host_choices()
{
	const amiberry_setting_choice* list = nullptr;
	auto listed = [&list](size_t count, const char* value) {
		for (size_t i = 0; i < count; i++) {
			if (!std::strcmp(list[i].value, value))
				return true;
		}
		return false;
	};
	char value[AMIBERRY_SETTINGS_VALUE_MAX];

	// A default device: the fallback first, then the joyport devices. The
	// factory default (joy1) may name a joystick this host does not have,
	// which would come first; the fallback is stored for this check.
	const amiberry_setting_descriptor* controller = amiberry_host_settings_find("default_controller1");
	CHECK(controller && controller->type == AMIBERRY_SETTING_CHOICE);
	CHECK(amiberry_host_settings_set("default_controller1", "") == AMIBERRY_SETTINGS_OK);
	size_t count = amiberry_host_settings_choices("default_controller1", &list);
	CHECK(count > 2 && list && !std::strcmp(list[0].value, "") && !std::strcmp(list[0].label, "Automatic"));
	CHECK(listed(count, "none") && listed(count, "mouse") && listed(count, "joy0"));
	// A joystick that is not connected now is kept; what no device is, not.
	CHECK(!listed(count, "joy7"));
	// A value the host does not offer now comes first, named by the layer.
	auto first_is = [&list](const char* id, const char* value, const char* label) {
		const size_t n = amiberry_host_settings_choices(id, &list);
		size_t seen = 0;
		for (size_t i = 0; i < n; i++)
			seen += !std::strcmp(list[i].value, value);
		return n > 0 && seen == 1 && !std::strcmp(list[0].value, value) && !std::strcmp(list[0].label, label);
	};
	CHECK(amiberry_host_settings_set("default_controller1", "joy7") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_host_settings_get("default_controller1", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "joy7"));
	CHECK(first_is("default_controller1", "joy7", "Joystick 8 (not connected)"));
	CHECK(amiberry_host_settings_set("default_mouse2", "mouse3") == AMIBERRY_SETTINGS_OK);
	CHECK(first_is("default_mouse2", "mouse3", "Mouse 4 (not connected)"));
	// A listed value is not repeated.
	CHECK(amiberry_host_settings_set("default_controller1", "joy0") == AMIBERRY_SETTINGS_OK);
	count = amiberry_host_settings_choices("default_controller1", &list);
	CHECK(!std::strcmp(list[0].value, "") && listed(count, "joy0"));
	CHECK(amiberry_host_settings_set("default_controller1", "joystick") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("default_mouse1", "mouse0") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// A number no device can have is refused, not wrapped into one.
	CHECK(amiberry_host_settings_set("default_controller1", "joy99999999999") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("default_controller1", "joy4294967296") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// So are the core parser's own tokens, as the file always took them,
	// under readable names.
	for (const char* token : {"custom", "custom1", "joydefault", "mousedefault"})
		CHECK(amiberry_host_settings_set("default_controller2", token) == AMIBERRY_SETTINGS_OK);
	CHECK(first_is("default_controller2", "mousedefault", "Default mouse"));
	CHECK(amiberry_host_settings_set("default_controller2", "custom1") == AMIBERRY_SETTINGS_OK);
	CHECK(first_is("default_controller2", "custom1", "Custom input mapping 2"));
	CHECK(amiberry_host_settings_set("default_controller2", "customx") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// A parallel port's empty default is no device, as is the joyports'
	// "none": one None entry stands for both, the stored spelling.
	CHECK(amiberry_host_settings_set("default_controller3", "") == AMIBERRY_SETTINGS_OK);
	count = amiberry_host_settings_choices("default_controller3", &list);
	CHECK(count > 2 && !std::strcmp(list[0].value, "") && !std::strcmp(list[0].label, "None"));
	CHECK(!listed(count, "none") && listed(count, "mouse"));
	CHECK(amiberry_host_settings_set("default_controller3", "none") == AMIBERRY_SETTINGS_OK);
	count = amiberry_host_settings_choices("default_controller3", &list);
	CHECK(count > 2 && !std::strcmp(list[0].value, "none") && !std::strcmp(list[0].label, "None"));
	CHECK(!listed(count, "") && listed(count, "mouse"));

	// The audio device: the system default, then the host's other devices.
	count = amiberry_host_settings_choices("default_soundcard", &list);
	CHECK(count >= 1 && list && !std::strcmp(list[0].value, "0") && !std::strcmp(list[0].label, "System default"));
	for (size_t i = 1; i < count; i++)
		CHECK(std::to_string(i) == list[i].value);
	// An index whose device is unplugged now is kept.
	CHECK(amiberry_host_settings_set("default_soundcard", "9") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_options.default_soundcard == 9);
	CHECK(first_is("default_soundcard", "9", "Audio device 9 (not connected)"));
	CHECK(amiberry_host_settings_set("default_soundcard", "256") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("default_soundcard", "speaker") == AMIBERRY_SETTINGS_INVALID_VALUE);

	// Static lists come through the same call; rates read in Hz.
	count = amiberry_host_settings_choices("default_sound_frequency", &list);
	CHECK(count == 5);
	for (size_t i = 0; i < count; i++) {
		const std::string label = list[i].label;
		CHECK(label.size() > 3 && label.compare(label.size() - 3, 3, " Hz") == 0);
	}
	CHECK(listed(count, "44100"));
	CHECK(amiberry_host_settings_find("default_sound_frequency")->choices[3].label == std::string("44,100 Hz"));

	// Shaders and bezels: "none" once and first, then the catalog.
	count = amiberry_host_settings_choices("shader_rtg", &list);
	CHECK(count == 4 && !std::strcmp(list[0].value, "none") && listed(count, "crt/crt-geom.glslp"));
	count = amiberry_host_settings_choices("custom_bezel", &list);
	CHECK(count == 2 && !std::strcmp(list[0].value, "none") && !std::strcmp(list[1].value, "frames/arcade.png"));
	CHECK(amiberry_host_settings_set("shader", "gone.glslp") == AMIBERRY_SETTINGS_OK);
	CHECK(first_is("shader", "gone.glslp", "gone.glslp (not found)"));
	// Picking a bezel shows it instead of the built-in one; none shows none.
	amiberry_options.use_bezel = true;
	CHECK(amiberry_host_settings_set("custom_bezel", "frames/arcade.png") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_options.use_custom_bezel && !amiberry_options.use_bezel);
	CHECK(amiberry_host_settings_set("custom_bezel", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(!amiberry_options.use_custom_bezel && !amiberry_options.use_bezel);
	// Empty names no image: refused, with nothing switched on.
	CHECK(amiberry_host_settings_set("custom_bezel", "") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_host_settings_set("shader", "") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(!amiberry_options.use_custom_bezel);

	// Hotkeys are their own type and list nothing.
	for (const char* id : {"default_open_gui_key", "default_quit_key", "default_ar_key",
		"default_fullscreen_toggle_key", "default_vkbd_toggle"}) {
		const amiberry_setting_descriptor* hotkey = amiberry_host_settings_find(id);
		CHECK(hotkey && hotkey->type == AMIBERRY_SETTING_HOTKEY);
		CHECK(amiberry_host_settings_choices(id, &list) == 0 && !list);
	}
	CHECK(amiberry_host_settings_choices("nope", &list) == 0 && !list);
	amiberry_host_settings_refresh_choices();
	CHECK(amiberry_host_settings_choices("default_mouse2", &list) > 2);
	amiberry_options = {};
}

// The configuration's description and category: one line each (the writer
// does not escape them), the category without surrounding spaces.
void check_config_text()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(set(p, "config_description", "My A1200 setup") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "config_description") == "My A1200 setup");
	CHECK(set(p, "config_description", "two\nlines") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "config_description", "two\rlines") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(value_of(p, "config_description") == "My A1200 setup");
	// Up to the field's 255 characters.
	const std::string longest(255, 'x');
	CHECK(set(p, "config_description", longest.c_str()) == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "config_description") == longest);
	CHECK(set(p, "config_description", (longest + "x").c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "config_category", "  Games \t") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "config_category") == "Games");
	CHECK(set(p, "config_category", "   ") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "config_category").empty());
	CHECK(set(p, "config_category", "a\nb") == AMIBERRY_SETTINGS_INVALID_VALUE);
	reset_prefs(p);
}

// A hotkey value read the way get_hotkey_from_config() (amiberry.cpp) reads
// it: modifier tokens before the last '+', then a key name for SDL, a
// modifier token alone naming that modifier key.
struct read_hotkey {
	unsigned int modifiers;
	SDL_Scancode scancode;
};

read_hotkey read_like_core(const std::string& value)
{
	static const struct {
		const char* token;
		unsigned int bit;
		const char* key;
	} modifiers[] = {
		{"LCtrl", AMIBERRY_HOTKEY_LCTRL, "Left Ctrl"}, {"RCtrl", AMIBERRY_HOTKEY_RCTRL, "Right Ctrl"},
		{"LShift", AMIBERRY_HOTKEY_LSHIFT, "Left Shift"}, {"RShift", AMIBERRY_HOTKEY_RSHIFT, "Right Shift"},
		{"LAlt", AMIBERRY_HOTKEY_LALT, "Left Alt"}, {"RAlt", AMIBERRY_HOTKEY_RALT, "Right Alt"},
		{"LGUI", AMIBERRY_HOTKEY_LGUI, "Left GUI"}, {"RGUI", AMIBERRY_HOTKEY_RGUI, "Right GUI"},
	};
	read_hotkey out{0, SDL_SCANCODE_UNKNOWN};
	size_t start = 0;
	for (size_t plus; (plus = value.find('+', start)) != std::string::npos; start = plus + 1) {
		for (const auto& m : modifiers) {
			if (value.compare(start, plus - start, m.token) == 0)
				out.modifiers |= m.bit;
		}
	}
	std::string key = value.substr(start);
	for (const auto& m : modifiers) {
		if (key == m.token)
			key = m.key;
	}
	out.scancode = SDL_GetScancodeFromName(key.c_str());
	return out;
}

// Volumes the configuration stores as attenuation read as volumes: they are
// reversed (shown as min + max - value) and nothing else is, so a front end
// turns around exactly these.
void check_reversed_volumes()
{
	const char* volumes[] = {"sound_volume", "sound_volume_paula", "sound_volume_cd", "sound_volume_ahi",
		"sound_volume_midi", "floppy0soundvolume_disk", "floppy3soundvolume_disk",
		"floppy0soundvolume_empty", "floppy3soundvolume_empty"};
	for (const char* id : volumes) {
		const amiberry_setting_descriptor* d = amiberry_settings_find(id);
		CHECK(d && d->reversed && d->type == AMIBERRY_SETTING_INT && d->min == 0 && d->max == 100);
	}
	// The stored value keeps the .uae meaning: 0 is full volume.
	CHECK(!std::strcmp(amiberry_settings_find("sound_volume")->default_value, "0"));
	size_t reversed = 0;
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		if (d->reversed) {
			reversed++;
			CHECK(d->type == AMIBERRY_SETTING_INT);
		}
	}
	// Master, Paula, CD, AHI, MIDI, and a disk and an empty volume per drive.
	CHECK(reversed == 13);
	for (size_t i = 0; i < amiberry_host_settings_count(); i++)
		CHECK(!amiberry_host_settings_at(i)->reversed);
}

// Units, scales and levels: only numbers carry them, a level has a finite
// range a slider can span, and a label does not repeat its unit.
void check_number_metadata()
{
	auto check = [](const amiberry_setting_descriptor* d) {
		const bool number = d->type == AMIBERRY_SETTING_INT || d->type == AMIBERRY_SETTING_REAL;
		if (!number) {
			CHECK(!d->unit && (d->scale == 0 || d->scale == 1) && !d->level);
			return;
		}
		if (d->level)
			CHECK(std::isfinite(d->min) && std::isfinite(d->max) && d->min < d->max);
		CHECK(d->scale >= 0);
		const std::string label = d->label;
		for (const char* suffix : {"(%)", "(KB)", "(ms)", "(s)", "(seconds)", "(px)", "(scanlines)"}) {
			const size_t n = std::strlen(suffix);
			CHECK(label.size() < n || label.compare(label.size() - n, n, suffix) != 0);
		}
	};
	for (size_t i = 0; i < amiberry_settings_count(); i++)
		check(amiberry_settings_at(i));
	for (size_t i = 0; i < amiberry_host_settings_count(); i++)
		check(amiberry_host_settings_at(i));

	auto find = [](const char* id) {
		const amiberry_setting_descriptor* d = amiberry_settings_find(id);
		CHECK(d != nullptr);
		return d;
	};
	// Every volume is a level in percent.
	size_t volumes = 0;
	for (size_t i = 0; i < amiberry_settings_count(); i++) {
		const amiberry_setting_descriptor* d = amiberry_settings_at(i);
		if (d->reversed) {
			volumes++;
			CHECK(d->level && d->unit && !std::strcmp(d->unit, "%"));
		}
	}
	CHECK(volumes == 13);
	// The picture attributes are stored in tenths.
	for (const char* id : {"gfx_luminance", "gfx_contrast", "gfx_gamma", "gfx_gamma_r", "gfx_gamma_g",
		"gfx_gamma_b"}) {
		const amiberry_setting_descriptor* d = find(id);
		CHECK(d->scale == 10 && d->level && !d->unit);
		CHECK(!std::strstr(d->help, "tenths"));
	}
	// The CPU speed adjustment is stored in tenths of a percent.
	const amiberry_setting_descriptor* throttle = find("cpu_throttle");
	CHECK(throttle->type == AMIBERRY_SETTING_REAL && throttle->scale == 10 && throttle->level);
	CHECK(!std::strcmp(throttle->unit, "%") && !std::strcmp(throttle->label, "CPU speed adjustment"));
	for (const char* id : {"amiberry.cpu_idle", "genlock_mix", "input.joystick_deadzone",
		"sound_stereo_mixing_delay"})
		CHECK(find(id)->level);
	// A number that means "the default" at 0, or one people type, is no
	// level.
	for (const char* id : {"amiberry.vkbd_transparency", "input.mouse_speed", "cachesize"})
		CHECK(!find(id)->level);
	const struct { const char* id; const char* unit; } units[] = {
		{"input.joystick_deadzone", "%"}, {"input.mouse_speed", "%"}, {"amiberry.vkbd_transparency", "%"},
		{"cachesize", "KB"}, {"fastmem_size_k", "KB"}, {"fastmem4_size_k", "KB"},
		{"gfx_monitorblankdelay", "ms"}, {"chipset_rtc_adjust", "s"}, {"gfx_width_windowed", "px"},
		{"gfx_height_windowed", "px"}, {"amiberry.gfx_manual_crop_width", "px"},
		{"amiberry.gfx_vertical_offset", "px"}, {"input.autofire_speed", "scanlines"},
	};
	for (const auto& u : units) {
		const amiberry_setting_descriptor* d = find(u.id);
		CHECK(d->unit && !std::strcmp(d->unit, u.unit));
	}
	CHECK(!std::strcmp(find("input.joystick_deadzone")->label, "Joystick dead zone"));

	const struct { const char* id; const char* unit; bool level; } host_units[] = {
		{"default_joystick_deadzone", "%", true}, {"input_default_mouse_speed", "%", false},
		{"default_vkbd_transparency", "%", false}, {"default_whd_configdelay", "s", false},
		{"default_width", "px", false}, {"default_height", "px", false},
	};
	for (const auto& u : host_units) {
		const amiberry_setting_descriptor* d = amiberry_host_settings_find(u.id);
		CHECK(d && d->unit && !std::strcmp(d->unit, u.unit) && d->level == u.level);
	}
	// The host default and the option it seeds read the same, and so do
	// the choices they share.
	CHECK(!std::strcmp(amiberry_host_settings_find("default_vkbd_transparency")->label,
		find("amiberry.vkbd_transparency")->label));
	const struct { const char* host; const char* option; } seeded[] = {
		{"default_vkbd_language", "amiberry.vkbd_language"},
		{"default_sound_frequency", "sound_frequency"},
		{"default_sound_buffer", "sound_max_buff"},
		{"default_stereo_separation", "sound_stereo_separation"},
		{"default_scaling_method", "amiberry.scaling_method"},
		{"default_gfx_autoresolution", "gfx_autoresolution"},
	};
	for (const auto& s : seeded) {
		const amiberry_setting_descriptor* host = amiberry_host_settings_find(s.host);
		const amiberry_setting_descriptor* option = find(s.option);
		CHECK(host && host->choice_count && option->choice_count);
		size_t shared = 0;
		for (size_t h = 0; host && h < host->choice_count; h++) {
			for (size_t o = 0; o < option->choice_count; o++) {
				if (std::strcmp(host->choices[h].value, option->choices[o].value))
					continue;
				shared++;
				CHECK(!std::strcmp(host->choices[h].label, option->choices[o].label));
			}
		}
		CHECK(shared > 0);
	}

	// The disk swapper's entries are counted from 1, the drives from 0.
	CHECK(!std::strcmp(find("diskimage0")->label, "Disk 1"));
	CHECK(!std::strcmp(find("diskimage19")->label, "Disk 20"));
	CHECK(!std::strcmp(find("floppy0")->label, "DF0: image"));
}

// Keyboard shortcuts: a key press the layer spells reads back as that key
// with those modifiers, and the shortcut options are typed as hotkeys.
void check_hotkeys()
{
	char value[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(amiberry_settings_hotkey_format(0x45, AMIBERRY_HOTKEY_LCTRL, value, sizeof value));
	CHECK(!std::strcmp(value, "LCtrl+F12"));
	CHECK(amiberry_settings_hotkey_format(0, AMIBERRY_HOTKEY_RCTRL, value, sizeof value));
	CHECK(!std::strcmp(value, "RCtrl"));
	// Ctrl, Shift, Alt, GUI, as the GUI's capture writes them, and the left
	// key when both of one modifier are held.
	CHECK(amiberry_settings_hotkey_format(SDL_SCANCODE_A,
		AMIBERRY_HOTKEY_RGUI | AMIBERRY_HOTKEY_RALT | AMIBERRY_HOTKEY_LSHIFT | AMIBERRY_HOTKEY_RSHIFT,
		value, sizeof value));
	CHECK(!std::strcmp(value, "LShift+RAlt+RGUI+A"));
	std::strcpy(value, "kept");
	CHECK(!amiberry_settings_hotkey_format(SDL_SCANCODE_LCTRL, 0, value, sizeof value)); // a modifier key
	CHECK(!amiberry_settings_hotkey_format(SDL_SCANCODE_RGUI, AMIBERRY_HOTKEY_LCTRL, value, sizeof value));
	CHECK(!amiberry_settings_hotkey_format(0, 0, value, sizeof value));
	CHECK(!amiberry_settings_hotkey_format(0, AMIBERRY_HOTKEY_LCTRL | AMIBERRY_HOTKEY_LALT, value, sizeof value));
	CHECK(!amiberry_settings_hotkey_format(SDL_SCANCODE_KP_PLUS, 0, value, sizeof value)); // "Keypad +"
	CHECK(!amiberry_settings_hotkey_format(0xa6, 0, value, sizeof value)); // reserved, no name
	CHECK(!amiberry_settings_hotkey_format(SDL_SCANCODE_A, 0x100, value, sizeof value));
	CHECK(!amiberry_settings_hotkey_format(SDL_SCANCODE_F12, AMIBERRY_HOTKEY_LCTRL, value, 9)); // "LCtrl+F12"
	CHECK(!std::strcmp(value, "kept"));

	// Every key it spells reads back as the key (or one SDL names alike,
	// Return2 as Return) with the modifiers held.
	const unsigned int held[] = {0, AMIBERRY_HOTKEY_LCTRL | AMIBERRY_HOTKEY_RALT,
		AMIBERRY_HOTKEY_RSHIFT | AMIBERRY_HOTKEY_LGUI};
	int spelled = 0;
	for (unsigned int usage = SDL_SCANCODE_A; usage < SDL_SCANCODE_LCTRL; usage++) {
		for (const unsigned int modifiers : held) {
			if (!amiberry_settings_hotkey_format(usage, modifiers, value, sizeof value))
				continue;
			spelled++;
			const read_hotkey read = read_like_core(value);
			CHECK(read.modifiers == modifiers);
			CHECK(read.scancode == static_cast<SDL_Scancode>(usage) || !std::strcmp(SDL_GetScancodeName(read.scancode),
				SDL_GetScancodeName(static_cast<SDL_Scancode>(usage))));
		}
	}
	CHECK(spelled > 3 * 100);
	// A modifier alone binds that modifier key.
	const struct {
		unsigned int bit;
		SDL_Scancode key;
	} alone[] = {
		{AMIBERRY_HOTKEY_LCTRL, SDL_SCANCODE_LCTRL}, {AMIBERRY_HOTKEY_RCTRL, SDL_SCANCODE_RCTRL},
		{AMIBERRY_HOTKEY_LSHIFT, SDL_SCANCODE_LSHIFT}, {AMIBERRY_HOTKEY_RSHIFT, SDL_SCANCODE_RSHIFT},
		{AMIBERRY_HOTKEY_LALT, SDL_SCANCODE_LALT}, {AMIBERRY_HOTKEY_RALT, SDL_SCANCODE_RALT},
		{AMIBERRY_HOTKEY_LGUI, SDL_SCANCODE_LGUI}, {AMIBERRY_HOTKEY_RGUI, SDL_SCANCODE_RGUI},
	};
	for (const auto& m : alone) {
		CHECK(amiberry_settings_hotkey_format(0, m.bit, value, sizeof value));
		const read_hotkey read = read_like_core(value);
		CHECK(read.modifiers == 0 && read.scancode == m.key);
	}

	// The controller buttons are SDL's, which the emulator reads back.
	const amiberry_setting_choice* buttons = nullptr;
	const size_t button_count = amiberry_settings_hotkey_buttons(&buttons);
	CHECK(button_count == SDL_GAMEPAD_BUTTON_COUNT && buttons);
	bool guide = false;
	for (size_t i = 0; buttons && i < button_count; i++) {
		CHECK(buttons[i].label && *buttons[i].label);
		CHECK(SDL_GetGamepadButtonFromString(buttons[i].value) == static_cast<SDL_GamepadButton>(i));
		guide = guide || (!std::strcmp(buttons[i].value, "guide") && !std::strcmp(buttons[i].label, "Guide"));
	}
	CHECK(guide);

	const char* const shortcuts[] = {"amiberry.open_gui", "amiberry.quit_amiberry", "amiberry.action_replay",
		"amiberry.fullscreen_toggle", "amiberry.minimize", "amiberry.left_amiga", "amiberry.right_amiga",
		"amiberry.screenshot", "amiberry.debugger_trigger", "amiberry.vkbd_toggle"};
	for (const char* id : shortcuts) {
		const amiberry_setting_descriptor* d = amiberry_settings_find(id);
		CHECK(d && d->type == AMIBERRY_SETTING_HOTKEY);
	}
	// The Amiga keys map a key only; the others answer a controller button.
	CHECK(amiberry_settings_hotkey_takes_buttons("amiberry.open_gui"));
	CHECK(amiberry_settings_hotkey_takes_buttons("amiberry.vkbd_toggle"));
	CHECK(!amiberry_settings_hotkey_takes_buttons("amiberry.left_amiga"));
	CHECK(!amiberry_settings_hotkey_takes_buttons("amiberry.right_amiga"));
	CHECK(!amiberry_settings_hotkey_takes_buttons("cpu_model"));
	CHECK(!amiberry_settings_hotkey_takes_buttons(nullptr));

	// A recorded value is stored as it is spelled.
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(set(p, "amiberry.right_amiga", "RCtrl") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "amiberry.right_amiga") == "RCtrl");
	CHECK(set(p, "amiberry.open_gui", "guide") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "amiberry.open_gui") == "guide");
	reset_prefs(p);
}

// Custom controls: a session stages each port's mapping for the device the
// port has, and commit hands it to the input code live.
void check_custom_controls()
{
	// The controls (buttons, then axes) and the events they can trigger.
	CHECK(amiberry_settings_custom_control_count() == SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT);
	const amiberry_settings_custom_control* south = amiberry_settings_custom_control_at(0);
	CHECK(south && !std::strcmp(south->id, "a") && !south->axis);
	const amiberry_settings_custom_control* leftx = amiberry_settings_custom_control_at(SDL_GAMEPAD_BUTTON_COUNT);
	CHECK(leftx && !std::strcmp(leftx->id, "leftx") && leftx->axis);
	CHECK(!amiberry_settings_custom_control_at(amiberry_settings_custom_control_count()));
	CHECK(amiberry_settings_input_event_count() == 3);
	CHECK(!std::strcmp(amiberry_settings_input_event_at(1)->value, "Space"));
	CHECK(!amiberry_settings_input_event_at(3));

	// Port 1 has joystick 0, which has every control but Misc 1, and its
	// D-pad on a hat. Back is its hotkey button, Start its quit button.
	reset_prefs(changed_prefs);
	changed_prefs.jports[1].id = JSEM_JOYS;
	std::strcpy(changed_prefs.jports[1].idc.configname, "pad-a");
	std::strcpy(changed_prefs.jports[1].idc.name, "Pad A");
	didata& did = di_joystick[0];
	for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
		did.mapping.button[i] = i;
	for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
		did.mapping.axis[i] = i;
	did.mapping.button[SDL_GAMEPAD_BUTTON_MISC1] = -1;
	did.mapping.button[SDL_GAMEPAD_BUTTON_DPAD_UP] = -1;
	did.mapping.number_of_hats = 1;
	did.mapping.hotkey_button = SDL_GAMEPAD_BUTTON_BACK;
	did.mapping.quit_button = SDL_GAMEPAD_BUTTON_START;
	did.mapping.menu_button = did.mapping.reset_button = did.mapping.vkbd_button = SDL_GAMEPAD_BUTTON_INVALID;
	changed_prefs.use_retroarch_quit = true;
	test_port_mappings[1] = controller_mapping{};
	test_port_mapping_devices[1] = "pad-a";

	amiberry_settings_session* session = amiberry_settings_session_open();
	char value[AMIBERRY_SETTINGS_VALUE_MAX];
	// A port without a joystick has no custom controls.
	CHECK(amiberry_settings_custom_control_get(session, 0, false, "a", value, sizeof value) == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(!amiberry_settings_custom_control_available(session, 0, false, "a"));
	CHECK(amiberry_settings_custom_control_set(session, 0, false, "a", "Space") == AMIBERRY_SETTINGS_UNAVAILABLE);
	// Nor does one naming a joystick that is not connected.
	joystick_total = 0;
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "a", value, sizeof value) == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(!amiberry_settings_custom_control_available(session, 1, false, "a"));
	joystick_total = 1;
	CHECK(amiberry_settings_custom_control_get(session, 4, false, "a", value, sizeof value) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "nope", value, sizeof value) == AMIBERRY_SETTINGS_UNKNOWN_ID);

	// Unset controls are reported as such: what they do depends on the mode.
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "a", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, ""));
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "a", "Space") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "b", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_custom_control_set(session, 1, true, "lefttrigger", "Joy2 Up") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "a", "Fire") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "a", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "Space"));
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "b", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "none"));
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "a", value, 5) == AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
	// The edits stay in the session: the input code has none of them yet.
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_SOUTH] == 0);

	// A control the device lacks; a D-pad on a hat has its directions.
	CHECK(!amiberry_settings_custom_control_available(session, 1, false, "misc1"));
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "misc1", "Space") == AMIBERRY_SETTINGS_UNAVAILABLE);
	// No function holds it: it is no button (the unset on-screen keyboard
	// button is no button either).
	CHECK(amiberry_settings_custom_control_available(session, 1, false, "dpup"));
	CHECK(!amiberry_settings_custom_control_use(session, 1, false, "dpup"));
	// The hotkey button is held in both modes; the quit button only with
	// the hotkey, while quitting by button is on.
	CHECK(!std::strcmp(amiberry_settings_custom_control_use(session, 1, false, "back"), "hotkey"));
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "back", "Space") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(!amiberry_settings_custom_control_use(session, 1, false, "start"));
	CHECK(!std::strcmp(amiberry_settings_custom_control_use(session, 1, true, "start"), "quit"));
	CHECK(amiberry_settings_custom_control_set(session, 1, true, "start", "Space") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(amiberry_settings_set(session, "amiberry.use_retroarch_quit", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(!amiberry_settings_custom_control_use(session, 1, true, "start"));
	CHECK(amiberry_settings_set(session, "amiberry.use_retroarch_quit", "true") == AMIBERRY_SETTINGS_OK);

	// Commit hands the mapping to the input code live and rebuilds the
	// input configuration.
	const int updates = input_config_updates;
	bool changed = false;
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed);
	CHECK(input_config_updates == updates + 1);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_SOUTH] == INPUTEVENT_KEY_SPACE);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_EAST] == -1);
	CHECK(test_port_mappings[1].amiberry_custom_axis_hotkey[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] == INPUTEVENT_JOY2_UP);
	// Nothing staged: nothing to apply.
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(!changed);
	CHECK(input_config_updates == updates + 1);
	// An empty event resets the control.
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "a", "") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_SOUTH] == 0);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_EAST] == -1);

	// Commit applies only the controls the session set: one changed
	// elsewhere meanwhile (a configuration loaded, the GUI) is kept.
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "a", "Space") == AMIBERRY_SETTINGS_OK);
	test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_WEST] = INPUTEVENT_JOY2_UP;
	CHECK(amiberry_settings_custom_control_get(session, 1, false, "x", value, sizeof value) == AMIBERRY_SETTINGS_OK);
	CHECK(!std::strcmp(value, "Joy2 Up"));
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(changed);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_SOUTH] == INPUTEVENT_KEY_SPACE);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_WEST] == INPUTEVENT_JOY2_UP);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_EAST] == -1);

	// Edits for a device the port no longer has when the session commits
	// are dropped, not applied to the new one: another slot, or another
	// pad in the same slot.
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "y", "Space") == AMIBERRY_SETTINGS_OK);
	std::strcpy(changed_prefs.jports[1].idc.configname, "pad-b");
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_NORTH] == 0);
	CHECK(test_port_mapping_devices[1] == "pad-a");
	std::strcpy(changed_prefs.jports[1].idc.configname, "pad-a");
	CHECK(amiberry_settings_custom_control_set(session, 1, false, "y", "Space") == AMIBERRY_SETTINGS_OK);
	std::strcpy(changed_prefs.jports[1].idc.name, "Pad B");
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_LIVE);
	CHECK(test_port_mappings[1].amiberry_custom_none[SDL_GAMEPAD_BUTTON_NORTH] == 0);
	amiberry_settings_session_close(session);

	// Configurations spell "none" (a control set to do nothing) apart from
	// an unset control (empty), and read both back.
	CHECK(!std::strcmp(custom_event_name(-1), "none"));
	CHECK(!std::strcmp(custom_event_name(0), ""));
	CHECK(!std::strcmp(custom_event_name(INPUTEVENT_KEY_SPACE), "Space"));
	CHECK(!std::strcmp(custom_event_name(INPUTEVENT_SPC_QUIT), "")); // not one a control can trigger
	CHECK(custom_event_of("none") == -1);
	CHECK(custom_event_of("None") == -1);
	CHECK(custom_event_of("") == 0);
	CHECK(custom_event_of("Space") == INPUTEVENT_KEY_SPACE);
	CHECK(custom_event_of("Fire") == 0);
	did = didata{};
	reset_prefs(changed_prefs);
}

// IO ports: the parallel port has a printer (or prints to a folder) or a
// sampler; serial and MIDI options follow the port and device picked.
void check_io_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(value_of(p, "amiberry.parallel_port") == "none" && value_of(p, "amiberry.parallel_port:folder") == "");
	CHECK(set(p, "amiberry.parallel_port:folder", "/home/amiga/prints") == AMIBERRY_SETTINGS_OK);
	CHECK(!strcmp(p.prtname, "FILE:/home/amiga/prints") && value_of(p, "amiberry.parallel_port") == "FILE:/home/amiga/prints");
	CHECK(value_of(p, "amiberry.parallel_port:folder") == "/home/amiga/prints");
	// A sampler takes the port from the printer, and the other way round.
	CHECK(!available(p, "sampler_stereo"));
	CHECK(set(p, "amiberry.samplersoundcard", "0") == AMIBERRY_SETTINGS_OK && p.prtname[0] == 0);
	CHECK(available(p, "sampler_stereo"));
	CHECK(set(p, "amiberry.parallel_port", "FILE:/tmp") == AMIBERRY_SETTINGS_OK && p.samplersoundcard == -1);
	CHECK(set(p, "amiberry.parallel_port", "none") == AMIBERRY_SETTINGS_OK && p.prtname[0] == 0);
	CHECK(set(p, "amiberry.parallel_port", std::string(MAX_DPATH, 'x').c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.samplersoundcard", "2") == AMIBERRY_SETTINGS_INVALID_VALUE); // two inputs

	// Serial: the port turns serial on; its options need one.
	CHECK(!available(p, "serial_on_demand") && value_of(p, "serial_port") == "none");
	CHECK(set(p, "serial_port", "TCP://0.0.0.0:1234") == AMIBERRY_SETTINGS_OK && p.use_serial);
	CHECK(set(p, "serial_translate", "crlf_cr") == AMIBERRY_SETTINGS_OK && p.serial_crlf == 1);
	CHECK(set(p, "serial_hardware_ctsrts", "false") == AMIBERRY_SETTINGS_OK && !p.serial_hwctsrts);
	CHECK(set(p, "serial_port", "/dev/nonexistent") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "serial_port", "none") == AMIBERRY_SETTINGS_OK && !p.use_serial && p.sername[0] == 0);

	// MIDI: in needs out (and goes with it), the router needs in.
	CHECK(!available(p, "amiberry.midiin_device_name") && !available(p, "amiberry.midirouter"));
	CHECK(set(p, "amiberry.midiout_device_name", "Munt MT-32") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.midiin_device_name", "Keyboard In") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.midirouter", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.midiout_device_name", "none") == AMIBERRY_SETTINGS_OK && p.midiindev[0] == 0);

	// The dongle, by the configuration's names; one beyond them reads as
	// its number, as the writer spells it.
	CHECK(set(p, "dongle", "leaderboard") == AMIBERRY_SETTINGS_OK && p.dongle == 2);
	p.dongle = 40;
	CHECK(value_of(p, "dongle") == "40");

	// A host refresh lists the ports and MIDI devices again.
	const int listed = io_enumerations;
	settings::refresh_dynamic_choices();
	CHECK(io_enumerations == listed + 2);
}

// Display filters: one cropping at a time, the window within the frame at
// the render resolution, the shaders and bezels the folders hold, and the
// built-in bezel or a custom one.
void check_filter_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(value_of(p, "amiberry.gfx_manual_crop_width") == "754" && value_of(p, "amiberry.gfx_manual_crop_height") == "576");
	CHECK(!available(p, "amiberry.gfx_manual_crop_width") && !available(p, "amiberry.gfx_horizontal_offset"));
	CHECK(set(p, "amiberry.gfx_auto_crop", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.gfx_manual_crop", "true") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "amiberry.gfx_auto_crop", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.gfx_manual_crop", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "amiberry.gfx_auto_crop"));
	// The window: up to the frame at hires/double (754 x 576), offsets as far
	// either way; a superhires frame is twice as wide.
	CHECK(set(p, "amiberry.gfx_manual_crop_width", "700") == AMIBERRY_SETTINGS_OK && p.gfx_manual_crop_width == 700);
	CHECK(set(p, "amiberry.gfx_manual_crop_width", "800") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.gfx_manual_crop_height", "577") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.gfx_horizontal_offset", "-754") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.gfx_vertical_offset", "577") == AMIBERRY_SETTINGS_INVALID_VALUE);
	p.gfx_resolution = 2;
	CHECK(set(p, "amiberry.gfx_manual_crop_width", "1400") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.gfx_manual_crop_width", "1600") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// Back at hires the window and offsets follow the narrower frame (and
	// single lines the lower one).
	p.gfx_horizontal_offset = 1400;
	CHECK(set(p, "gfx_resolution", "hires") == AMIBERRY_SETTINGS_OK);
	CHECK(p.gfx_manual_crop_width == 754 && p.gfx_horizontal_offset == 754);
	CHECK(set(p, "gfx_linemode", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(p.gfx_manual_crop_height == 288);
	// Auto crop clears the centering it would fight (target_fixup_options).
	reset_prefs(p);
	p.gfx_xcenter = p.gfx_ycenter = 2;
	CHECK(set(p, "amiberry.gfx_auto_crop", "true") == AMIBERRY_SETTINGS_OK && !p.gfx_xcenter && !p.gfx_ycenter);

	// Shaders from the folder's list; a configured one it lacks reads back.
	CHECK(set(p, "amiberry.shader", "crt/crt-geom.glslp") == AMIBERRY_SETTINGS_OK && !strcmp(p.shader, "crt/crt-geom.glslp"));
	CHECK(set(p, "amiberry.shader_rtg", "tv") == AMIBERRY_SETTINGS_OK && !strcmp(p.shader_rtg, "tv"));
	CHECK(set(p, "amiberry.shader", "missing.glslp") == AMIBERRY_SETTINGS_INVALID_VALUE);
	strcpy(p.shader, "gone.glslp");
	CHECK(value_of(p, "amiberry.shader") == "gone.glslp");

	// A custom bezel image shows instead of the built-in bezel; "none" shows
	// none. The built-in one turns the custom one off.
	reset_prefs(p);
	CHECK(!available(p, "amiberry.use_custom_bezel"));
	CHECK(set(p, "amiberry.custom_bezel", "frames/arcade.png") == AMIBERRY_SETTINGS_OK);
	CHECK(p.use_custom_bezel && !p.use_bezel && !available(p, "amiberry.use_bezel"));
	CHECK(set(p, "amiberry.use_custom_bezel", "false") == AMIBERRY_SETTINGS_OK && available(p, "amiberry.use_bezel"));
	CHECK(set(p, "amiberry.use_bezel", "true") == AMIBERRY_SETTINGS_OK && !p.use_custom_bezel);
	CHECK(!available(p, "amiberry.custom_bezel"));
	CHECK(set(p, "amiberry.use_bezel", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.custom_bezel", "none") == AMIBERRY_SETTINGS_OK && !p.use_custom_bezel);

	// A host refresh rescans both folders.
	const int rescans = display_catalog_rescans;
	settings::refresh_dynamic_choices();
	CHECK(display_catalog_rescans == rescans + 2);
}

// RTG: the board's options follow its type and memory, and the packed
// entries (gfxcard_options, rtg_modes) agree with their parts.
void check_rtg_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	CHECK(!available(p, "rtg_modes:16bit"));
	CHECK(value_of(p, "gfxcard_options") == "");

	// 24-bit addressing (the 68000) leaves no room for a Zorro III board:
	// turning the board on gives the UAE Zorro II one, at most 8 MB.
	p.rtg_zerocopy = false;
	CHECK(choice_available(p, "gfxcard_size", "8") && !choice_available(p, "gfxcard_size", "16"));
	CHECK(set(p, "gfxcard_size", "4") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_type") == "ZorroII" && p.rtg_zerocopy);
	CHECK(!choice_available(p, "gfxcard_type", "ZorroIII") && !choice_available(p, "gfxcard_type", "ZZ9000_Z3"));
	CHECK(choice_available(p, "gfxcard_type", "PicassoII"));
	// More than 2 MB chip RAM leaves no room for a Zorro II board.
	p.chipmem.size = 0x400000;
	CHECK(!choice_available(p, "gfxcard_type", "PicassoII"));
	p.chipmem.size = 0x80000;

	// Picking a board turns it on with the memory the GUI gave it.
	reset_prefs(p);
	p.rtg_zerocopy = false;
	CHECK(available(p, "gfxcard_type"));
	CHECK(set(p, "gfxcard_type", "ZorroIII") == AMIBERRY_SETTINGS_UNAVAILABLE); // 24-bit
	CHECK(set(p, "gfxcard_type", "ZorroII") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_size") == "4" && p.rtg_zerocopy);
	reset_prefs(p);
	CHECK(set(p, "gfxcard_type", "PicassoII") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_size") == "1"); // its least
	// Turning the board off resets what the configuration then omits.
	CHECK(set(p, "gfxcard_options:initial", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "gfxcard_size", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(!p.rtgboards[0].initial_active && p.rtgboards[0].autoswitch && p.rtgboards[0].monitor_id == 0);

	// A UAE Zorro II board takes 1-8 MB.
	reset_prefs(p);
	p.cpu_model = 68020;
	p.address_space_24 = false;
	CHECK(set(p, "gfxcard_size", "16") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_type") == "ZorroIII");
	CHECK(set(p, "gfxcard_type", "ZorroII") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_size") == "8"); // clamped to the board
	CHECK(choice_available(p, "gfxcard_size", "8") && !choice_available(p, "gfxcard_size", "16"));
	CHECK(available(p, "rtg_modes:32bit") && available(p, "gfxcard_zerocopy"));
	CHECK(!available(p, "gfxcard_options:autoswitch")); // the UAE board always switches

	// A hardware board: its own memory range, no UAE helpers.
	p.rtg_hardwaresprite = true;
	CHECK(set(p, "gfxcard_type", "PicassoII") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_size") == "2");
	CHECK(choice_available(p, "gfxcard_size", "1") && !choice_available(p, "gfxcard_size", "4"));
	CHECK(!p.rtg_hardwaresprite);
	CHECK(!available(p, "rtg_modes:16bit") && !available(p, "gfxcard_multithread"));
	CHECK(!available(p, "gfxcard_options:autoswitch")); // its own switcher
	CHECK(!available(p, "zz9000_net"));
	CHECK(set(p, "gfxcard_type", "GD5446_PCI") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "gfxcard_options:autoswitch"));
	CHECK(set(p, "gfxcard_type", "ZZ9000_Z3") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_size") == "128" && available(p, "zz9000_net"));
	CHECK(set(p, "gfxcard_type", "NoSuchBoard") == AMIBERRY_SETTINGS_INVALID_VALUE);

	// gfxcard_options and its parts.
	CHECK(set(p, "gfxcard_options:monitor", "2") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "gfxcard_options:autoswitch", "false") == AMIBERRY_SETTINGS_UNAVAILABLE); // ZZ9000 switches
	p.rtgboards[0].autoswitch = false;
	p.rtgboards[1].initial_active = true;
	CHECK(set(p, "gfxcard_options:initial", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(!p.rtgboards[1].initial_active); // one board starts on its screen
	CHECK(value_of(p, "gfxcard_options") == "monitor=2,noautoswitch,initial");
	CHECK(set(p, "gfxcard_options", "monitor=1") == AMIBERRY_SETTINGS_OK);
	CHECK(p.rtgboards[0].monitor_id == 1 && p.rtgboards[0].autoswitch && !p.rtgboards[0].initial_active);
	CHECK(set(p, "gfxcard_options", "monitor=9") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// The order is part of the entry under custom autoconfig sorting only.
	p.autoconfig_custom_sort = true;
	CHECK(set(p, "gfxcard_options", "order=3,monitor=1") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_options") == "order=3,monitor=1");
	CHECK(set(p, "gfxcard_options", "monitor=1") == AMIBERRY_SETTINGS_OK);
	CHECK(p.rtgboards[0].device_order == 0 && value_of(p, "gfxcard_options") == "monitor=1");
	p.autoconfig_custom_sort = false;
	p.rtgboards[0].device_order = 3;
	CHECK(set(p, "gfxcard_options", "monitor=1") == AMIBERRY_SETTINGS_OK);
	CHECK(p.rtgboards[0].device_order == 3); // not in the entry: kept
	CHECK(set(p, "gfxcard_options", "sparkly") == AMIBERRY_SETTINGS_INVALID_VALUE);

	// rtg_modes and its depths.
	reset_prefs(p);
	p.cpu_model = 68020;
	p.address_space_24 = false;
	CHECK(set(p, "gfxcard_size", "16") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "rtg_modes") == "0x112");
	CHECK(value_of(p, "rtg_modes:8bit") == "CLUT" && value_of(p, "rtg_modes:24bit") == "none");
	CHECK(set(p, "rtg_modes:24bit", "all") == AMIBERRY_SETTINGS_OK);
	CHECK((p.picasso96_modeflags & (RGBFF_R8G8B8 | RGBFF_B8G8R8)) == (RGBFF_R8G8B8 | RGBFF_B8G8R8));
	CHECK(set(p, "rtg_modes:16bit", "B5G5R5PC") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "rtg_modes:16bit") == "B5G5R5PC");
	CHECK((p.picasso96_modeflags & RGBFF_R5G6B5PC) == 0);
	CHECK(set(p, "rtg_modes", "0x2") == AMIBERRY_SETTINGS_OK && value_of(p, "rtg_modes:32bit") == "none");
	CHECK(set(p, "rtg_modes", "purple") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "rtg_modes", "0112") == AMIBERRY_SETTINGS_OK && p.picasso96_modeflags == 112); // decimal, as parsed

	// Turning the board off reads back as a configuration without it.
	CHECK(set(p, "gfxcard_size", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfxcard_type") == "ZorroIII");

	CHECK(!available(p, "gfx_filter_autoscale_rtg")); // no RTG screen
	p.gf[GF_RTG].gfx_filter_autoscale = RTG_MODE_INTEGER_SCALE;
	CHECK(!available(p, "gfx_filter_autoscale_limit_rtg")); // nor its integer scale limit
	p.gf[GF_RTG].gfx_filter_autoscale = RTG_MODE_SCALE;

	// The integer scale limit follows integer scaling.
	CHECK(set(p, "gfxcard_size", "16") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "gfx_filter_autoscale_limit_rtg"));
	CHECK(set(p, "gfx_filter_autoscale_rtg", "integer") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "gfx_filter_autoscale_limit_rtg"));
}

// AE for the machine domains: sessions commit preset-written fields and
// report the strongest apply class.
void check_machine_session()
{
	reset_prefs(changed_prefs);
	changed_prefs.cpuboard_type = 1;

	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(session);
	// A preset through the C API: everything it writes must survive commit,
	// including the fresh-configuration defaults outside every option.
	const int preview_calls = stub_preview_calls;
	CHECK(amiberry_settings_preset_apply(session, "a1200", 1, 1) == AMIBERRY_SETTINGS_OK);
	CHECK(stub_preview_calls > preview_calls); // never the live engine
	CHECK(amiberry_settings_preset_apply(session, "nope", 0, 0) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(amiberry_settings_preset_apply(session, "a1200", 99, 0) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_preset_apply(session, "a1200", 0, 5) == AMIBERRY_SETTINGS_UNAVAILABLE);
	bool changed = false;
	// The stub engine changes no option with a descriptor; a preset still
	// commits as a machine change.
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed);

	// The Quickstart constraints (floppy/CD/joystick) and the target
	// defaults are committed too.
	CHECK(changed_prefs.floppyslots[1].dfxtype == DRV_NONE);
	CHECK(!changed_prefs.cdslots[0].inuse);
	CHECK(changed_prefs.jports[1].mode == 0);
	CHECK(changed_prefs.keyboard_leds[0] == 2);

	// The preset is replayed on the pending configuration: changes made
	// before it survive where it does not write, changes made after it
	// win, and changes made elsewhere meanwhile are kept.
	reset_prefs(changed_prefs);
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", "/roms/before.rom") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_preset_apply(session, "a500", 0, 1) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "chipset", "aga") == AMIBERRY_SETTINGS_OK);
	changed_prefs.kbd_led_scr = 3; // another front end, meanwhile
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET && changed);
	CHECK(std::strcmp(changed_prefs.romfile, "/roms/before.rom") == 0);
	CHECK(value_of(changed_prefs, "chipset") == "aga");
	CHECK(changed_prefs.keyboard_leds[0] == 2 && changed_prefs.kbd_led_scr == 3);
	CHECK(changed_prefs.floppyslots[0].dfxtype == DRV_35_DD); // A500 constraint

	// A machine option through a session: strongest class wins.
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", "/roms/kick31.rom")
		== AMIBERRY_SETTINGS_OK);
	char path[settings::max_value_length] = {};
	CHECK(amiberry_settings_get(session, "kickstart_rom_file", path, sizeof path)
		== AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(path, "/roms/kick31.rom") == 0);
	char tiny[4];
	CHECK(amiberry_settings_get(session, "kickstart_rom_file", tiny, sizeof tiny)
		== AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed && std::strcmp(changed_prefs.romfile, "/roms/kick31.rom") == 0);

	// Two paths that share a 63-character prefix still differ, at the
	// longest length the field holds; one more character is refused
	// (MAX_DPATH follows the host's PATH_MAX), not truncated.
	const std::string long_a(MAX_DPATH - 1, 'a');
	std::string long_b = long_a;
	long_b[100] = 'b';
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", long_a.c_str()) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", long_b.c_str()) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET);
	CHECK(changed_prefs.romfile[100] == 'b' && std::strlen(changed_prefs.romfile) == MAX_DPATH - 1);
	CHECK(amiberry_settings_set(session, "kickstart_rom_file", (long_a + "a").c_str())
		== AMIBERRY_SETTINGS_INVALID_VALUE);

	// A RAM size committed from a session leaves the rest of every board
	// to whoever edited it meanwhile (the RAM panel's board editor).
	CHECK(amiberry_settings_set(session, "fastmem_size", "4") == AMIBERRY_SETTINGS_OK);
	changed_prefs.fastmem[0].manufacturer = 0x1234;
	changed_prefs.fastmem[1].size = 0x100000;
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET && changed);
	CHECK(changed_prefs.fastmem[0].size == 0x400000);
	CHECK(changed_prefs.fastmem[0].manufacturer == 0x1234);
	CHECK(changed_prefs.fastmem[1].size == 0x100000);

	// Chip RAM above 512 KB adds only ECS Agnus to the session's chipset;
	// committing it keeps a chipset chosen elsewhere meanwhile.
	CHECK(amiberry_settings_set(session, "chipset", "ocs") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "chipmem_size", "1") == AMIBERRY_SETTINGS_OK);
	amiberry_settings_commit(session, &changed);
	CHECK(value_of(changed_prefs, "chipset") == "ocs");
	CHECK(amiberry_settings_set(session, "chipmem_size", "4") == AMIBERRY_SETTINGS_OK);
	CHECK(set(changed_prefs, "chipset", "aga") == AMIBERRY_SETTINGS_OK); // meanwhile
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET && changed);
	CHECK(value_of(changed_prefs, "chipset") == "aga");
	CHECK(changed_prefs.chipmem.size == 0x200000);
	// Without the concurrent choice the session's ECS Agnus still lands.
	CHECK(amiberry_settings_set(session, "chipset", "ocs") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "chipmem_size", "1") == AMIBERRY_SETTINGS_OK);
	amiberry_settings_commit(session, &changed);
	CHECK(amiberry_settings_set(session, "chipmem_size", "2") == AMIBERRY_SETTINGS_OK);
	amiberry_settings_commit(session, &changed);
	CHECK(value_of(changed_prefs, "chipset") == "ecs_agnus" && changed_prefs.chipmem.size == 0x100000);
	// A chosen ECS Agnus is a chipset choice, though it differs from OCS by
	// the bit chip RAM adds: it replaces a chipset chosen elsewhere.
	CHECK(amiberry_settings_set(session, "chipmem_size", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_set(session, "chipset", "ocs") == AMIBERRY_SETTINGS_OK);
	amiberry_settings_commit(session, &changed);
	CHECK(amiberry_settings_set(session, "chipset", "ecs_agnus") == AMIBERRY_SETTINGS_OK);
	CHECK(set(changed_prefs, "chipset", "aga") == AMIBERRY_SETTINGS_OK); // meanwhile
	amiberry_settings_commit(session, &changed);
	CHECK(changed);
	CHECK(value_of(changed_prefs, "chipset") == "ecs_agnus");
	amiberry_settings_session_close(session);
}

// Every offered display value survives fixup_prefs_dimensions(), the
// part of fixup_prefs() that owns the mode and sync rules.
void check_display_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);
	auto survives = [&p](const char* id) {
		uae_prefs* copy = static_cast<uae_prefs*>(std::malloc(sizeof(uae_prefs)));
		std::memcpy(static_cast<void*>(copy), static_cast<const void*>(&p), sizeof p);
		fixup_prefs_dimensions(copy);
		const bool same = value_of(*copy, id) == value_of(p, id);
		std::free(copy);
		return same;
	};

	// Windowed sizes are clamped to the fixup's range and step.
	CHECK(available(p, "gfx_width_windowed"));
	CHECK(set(p, "gfx_width_windowed", "722") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "gfx_width_windowed", "100") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "gfx_width_windowed", "1024") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("gfx_width_windowed"));
	CHECK(value_of(p, "gfx_height_windowed") == "568");

	// The host display applies to both screens, like the parser. The
	// range follows the monitors connected now, so the test host borrows
	// one before it can address a second display.
	Displays[0].monitorname = const_cast<TCHAR*>("Test Monitor");
	Displays[1].monitorname = const_cast<TCHAR*>("Test Second Monitor");
	CHECK(set(p, "gfx_display", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(p.gfx_apmode[APMODE_NATIVE].gfx_display == 1 && p.gfx_apmode[APMODE_RTG].gfx_display == 1);
	Displays[0].monitorname = Displays[1].monitorname = nullptr;
	CHECK(!available(p, "gfx_display")); // no second monitor in the test host

	// KMSDRM owns the mode and the sync options.
	CHECK(available(p, "gfx_fullscreen_amiga") && available(p, "gfx_vsync"));
	kmsdrm_detected = true;
	CHECK(!available(p, "gfx_fullscreen_amiga") && !available(p, "gfx_vsync"));
	CHECK(!available(p, "gfx_vrr_monitor") && !available(p, "gfx_width_windowed"));
	kmsdrm_detected = false;

	// The legacy exclusive value is reported but not offered: the parser
	// and the setters turn it into full-window.
	p.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = GFX_FULLSCREEN;
	CHECK(value_of(p, "gfx_fullscreen_amiga") == "true");
	CHECK(!choice_available(p, "gfx_fullscreen_amiga", "true"));
	CHECK(set(p, "gfx_fullscreen_amiga", "true") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "gfx_fullscreen_amiga", "fullwindow") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("gfx_fullscreen_amiga"));

	// Adaptive runs standard vsync under VRR.
	CHECK(set(p, "gfx_vrr_monitor", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfx_vsync") == "true" && p.gfx_apmode[0].gfx_vsyncmode == 0);
	CHECK(set(p, "gfx_vsync", "autoswitch") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfx_vsync") == "autoswitch" && p.gfx_variable_sync == 1);
	CHECK(survives("gfx_vsync"));

	// RTG needs a board; a windowed RTG screen cannot vsync, so the
	// choice is refused (fixup clears it) and switching RTG to a window
	// clears what the core would clear.
	CHECK(!available(p, "gfx_fullscreen_picasso") && !available(p, "gfx_vsync_picasso"));
	p.address_space_24 = false;
	p.rtgboards[0].rtgmem_size = 8;
	CHECK(available(p, "gfx_fullscreen_picasso") && available(p, "gfx_vsync_picasso"));
	CHECK(set(p, "gfx_fullscreen_picasso", "fullwindow") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "gfx_vsync_picasso", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("gfx_vsync_picasso"));
	CHECK(set(p, "gfx_fullscreen_picasso", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfx_vsync_picasso") == "false");
	CHECK(set(p, "gfx_fullscreen_picasso", "fullwindow") == AMIBERRY_SETTINGS_OK);
	CHECK(choice_available(p, "gfx_vsync_picasso", "true"));
	CHECK(set(p, "gfx_vsync_picasso", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(survives("gfx_vsync_picasso"));

	// VGA autoswitching needs hires, line-doubled output; leaving that
	// combination clears it as the panel did every frame.
	CHECK(available(p, "gfx_autoresolution_vga"));
	CHECK(set(p, "gfx_resolution", "lores") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "gfx_autoresolution_vga") && !p.gfx_autoresolution_vga);
	CHECK(set(p, "gfx_resolution", "hires") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "gfx_linemode", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(!p.gfx_autoresolution_vga);
	CHECK(value_of(p, "gfx_linemode") == "none");
	CHECK(set(p, "gfx_linemode", "double2") == AMIBERRY_SETTINGS_OK);
	CHECK(p.gfx_vresolution == 1 && p.gfx_pscanlines == 0 && p.gfx_iscanlines == 1);

	// Resolution autoswitch gates the manual resolution and line modes.
	CHECK(set(p, "gfx_autoresolution", "10") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "gfx_resolution") && !available(p, "gfx_linemode"));
	CHECK(set(p, "gfx_autoresolution", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "gfx_resolution") && available(p, "gfx_linemode"));
	CHECK(value_of(p, "gfx_autoresolution") == "0");

	// The display attributes clamp to the slider's tenths range.
	CHECK(set(p, "gfx_luminance", "2500") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "gfx_luminance", "-2000") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "gfx_atari_palette_fix", "3to4bit") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "gfx_atari_palette_fix") == "3to4bit");

	// Centering follows the overscan border, aspect correction the
	// scaling method.
	CHECK(available(p, "gfx_center_horizontal"));
	CHECK(set(p, "gfx_overscanmode", "extreme") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "gfx_center_horizontal") && !available(p, "gfx_center_vertical"));
	CHECK(set(p, "gfx_overscanmode", "overscan") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "gfx_center_horizontal"));
	CHECK(set(p, "amiberry.scaling_method", "3") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "amiberry.gfx_correct_aspect"));
	CHECK(set(p, "amiberry.scaling_method", "-1") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "amiberry.gfx_correct_aspect"));
	CHECK(value_of(p, "amiberry.scaling_method") == "-1");

	// The FPS adjustment locks and unlocks the PAL/NTSC entry; 0 follows
	// the standard rate. The value is the entry's CSV as the writer emits
	// it. A bare rate keeps the entry's other fields (the PAL binding: an
	// entry without one would drive every video mode), and a full CSV
	// round-trips (rpct included, which the writer used to glue onto the
	// previous flag).
	CHECK(set(p, "displaydata_pal", "59.9375") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "displaydata_pal") == "59.937500,locked,pal");
	CHECK(p.cr[CHIPSET_REFRESH_PAL].locked && p.cr[CHIPSET_REFRESH_PAL].inuse);
	CHECK(p.cr[CHIPSET_REFRESH_PAL].ntsc == 0);
	CHECK(std::strcmp(p.cr[CHIPSET_REFRESH_PAL].label, "PAL") == 0);
	CHECK(set(p, "displaydata_pal", "50.000000,locked,pal,lace,shres,rpct=50,vsync") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "displaydata_pal") == "50.000000,locked,pal,lace,shres,rpct=50,vsync");
	CHECK(set(p, "displaydata_pal", "49.5") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "displaydata_pal") == "49.500000,locked,pal,lace,shres,rpct=50,vsync");
	CHECK(set(p, "displaydata_pal", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(!p.cr[CHIPSET_REFRESH_PAL].locked && p.cr[CHIPSET_REFRESH_PAL].rate == 50.0f);
	CHECK(value_of(p, "displaydata_pal") == "0");
	CHECK(set(p, "displaydata_ntsc", "301") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "displaydata_ntsc", "fast") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "displaydata_ntsc", "0.5") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// A loaded entry can hold rate 0 with other fields: its CSV is parsed,
	// not taken for the bare 0 that resets the entry.
	CHECK(set(p, "displaydata_pal", "0,locked,pal,lace") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cr[CHIPSET_REFRESH_PAL].locked && p.cr[CHIPSET_REFRESH_PAL].lace == 1);
	{
		const std::string zero_rate = value_of(p, "displaydata_pal");
		CHECK(set(p, "displaydata_pal", zero_rate.c_str()) == AMIBERRY_SETTINGS_OK);
		CHECK(value_of(p, "displaydata_pal") == zero_rate);
	}
	// RTG vsync's 50/60 Hz switching is the same as standard there.
	CHECK(!choice_available(p, "gfx_vsync_picasso", "autoswitch"));
	// Commands longer than the entry's field are dropped, not overflowed.
	{
		const std::string overlong = "50,cmd=" + std::string(400, 'a');
		CHECK(set(p, "displaydata_pal", overlong.c_str()) == AMIBERRY_SETTINGS_OK);
		CHECK(std::strlen(p.cr[CHIPSET_REFRESH_PAL].commands) < sizeof p.cr[CHIPSET_REFRESH_PAL].commands);
		CHECK(p.cr[CHIPSET_REFRESH_PAL].filterprofile[0] == 0);
	}

	// An entry with a long command list and filter profile reads back in
	// full and survives being set again (values are not limited to the
	// public adjustment's 64 bytes).
	reset_prefs(p);
	struct chipset_refresh& cr = p.cr[CHIPSET_REFRESH_PAL];
	cr.locked = true;
	std::memset(cr.commands, 'c', 200); // the reader ends each command with a newline
	cr.commands[200] = '\n';
	cr.commands[201] = 0;
	std::memset(cr.filterprofile, 'f', 60);
	cr.filterprofile[60] = 0;
	const std::string long_entry = value_of(p, "displaydata_pal");
	CHECK(long_entry.size() > 260);
	reset_prefs(p);
	CHECK(set(p, "displaydata_pal", long_entry.c_str()) == AMIBERRY_SETTINGS_OK);
	CHECK(std::strlen(p.cr[CHIPSET_REFRESH_PAL].commands) == 201);
	CHECK(std::strlen(p.cr[CHIPSET_REFRESH_PAL].filterprofile) == 60);
	CHECK(value_of(p, "displaydata_pal") == long_entry);

	// target_fixup_options() clears centering under auto-crop.
	reset_prefs(p);
	p.gfx_overscanmode = OVERSCANMODE_OVERSCAN;
	CHECK(available(p, "gfx_center_horizontal"));
	p.gfx_auto_crop = true;
	CHECK(!available(p, "gfx_center_horizontal") && !available(p, "gfx_center_vertical"));
}

void check_sound_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// Sound emulation: exact is the default and reachable; Disabled is
	// refused in memory cycle-exact mode because the core raises it.
	CHECK(value_of(p, "sound_output") == "exact");
	CHECK(set(p, "sound_output", "normal") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "sound_output", "exact") == AMIBERRY_SETTINGS_OK);
	CHECK(!choice_available(p, "sound_output", "none"));
	p.cpu_memory_cycle_exact = false;
	CHECK(choice_available(p, "sound_output", "none"));
	CHECK(set(p, "sound_output", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "sound_auto") && !available(p, "sound_channels"));
	CHECK(!available(p, "sound_stereo_separation"));
	CHECK(set(p, "sound_output", "normal") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "sound_auto") && available(p, "sound_stereo_separation"));
	CHECK(set(p, "sound_channels", "4ch") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "sound_channels") == "4ch");

	// The audio device needs the system default off, sound on and more
	// than one enumerated device.
	CHECK(!available(p, "amiberry.soundcard"));
	settings::choices_of(settings::require("amiberry.soundcard"), p);
	CHECK(!available(p, "amiberry.soundcard")); // still the default device
	CHECK(set(p, "amiberry.soundcard_default", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "amiberry.soundcard"));
	const settings::choice_list devices = settings::choices_of(
		settings::require("amiberry.soundcard"), p);
	CHECK(devices.count == 2);
	CHECK(set(p, "amiberry.soundcard", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "amiberry.soundcard") == "1");
	sound_device_count = 1;
	settings::refresh_dynamic_choices();
	CHECK(!available(p, "amiberry.soundcard"));
	sound_device_count = 2;
	settings::refresh_dynamic_choices();
	CHECK(available(p, "amiberry.soundcard"));

	// The volumes are attenuation: 0 is full volume, 100 is silent.
	CHECK(set(p, "sound_volume", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "sound_volume") == "0");
	CHECK(set(p, "sound_volume", "101") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "sound_volume_cd", "0") == AMIBERRY_SETTINGS_OK);

	// The filter and its curve are two keys; the fixed-only curve
	// round-trips through sound_filter_type.
	CHECK(set(p, "sound_filter", "on") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "sound_filter_type", "fixedonly") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "sound_filter") == "on" && value_of(p, "sound_filter_type") == "fixedonly");

	// The writer saves a delay of -1 as 0, so the range starts at 0; an
	// off-list frequency keeps its value instead of a stale label.
	CHECK(set(p, "sound_stereo_mixing_delay", "-1") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "sound_stereo_mixing_delay", "3") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "sound_stereo_mixing_delay") == "3");
	CHECK(set(p, "sound_frequency", "8000") == AMIBERRY_SETTINGS_INVALID_VALUE);
	p.sound_freq = 8000;
	CHECK(value_of(p, "sound_frequency") == "8000");
	CHECK(set(p, "sound_frequency", "48000") == AMIBERRY_SETTINGS_OK);

	// Drive clicks: external samples (a negative value) stay visible as a
	// named choice, but need a sample set no option picks, so they are
	// not offered.
	CHECK(set(p, "floppy1sound", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "floppy1sound") == "1");
	p.floppyslots[2].dfxclick = -1;
	CHECK(value_of(p, "floppy2sound") == "-1");
	CHECK(set(p, "floppy2sound", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "floppy2sound", "-1") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "floppy2sound", "2") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// A drive's click volumes only apply while its drive sound is on.
	CHECK(value_of(p, "floppy3sound") == "0");
	CHECK(!available(p, "floppy3soundvolume_disk") && !available(p, "floppy3soundvolume_empty"));
	CHECK(set(p, "floppy3soundvolume_empty", "50") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(set(p, "floppy3sound", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "floppy3soundvolume_disk") && available(p, "floppy3soundvolume_empty"));
	CHECK(!available(p, "floppy0soundvolume_disk"));
	CHECK(set(p, "floppy3soundvolume_empty", "50") == AMIBERRY_SETTINGS_OK);
	CHECK(p.dfxclickvolume_empty[3] == 50);
	// And while sound emulation is on.
	CHECK(set(p, "sound_output", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "floppy3soundvolume_disk"));
	CHECK(set(p, "sound_output", "normal") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "floppy3soundvolume_disk"));

	// The buffer size offers the writer's exact sizes.
	CHECK(set(p, "sound_max_buff", "16384") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "sound_max_buff", "5000") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "sound_max_buff", "0") == AMIBERRY_SETTINGS_OK);

	// The lagless vsync mode RTG vsync turns into resets an SDL device to
	// the first one, so only that one is offered then, and switching the
	// mode on makes the change itself.
	reset_prefs(p);
	p.soundcard_default = false;
	CHECK(choice_available(p, "amiberry.soundcard", "1"));
	p.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_FULLWINDOW;
	p.gfx_apmode[APMODE_RTG].gfx_vsync = 1;
	CHECK(!choice_available(p, "amiberry.soundcard", "1"));
	CHECK(choice_available(p, "amiberry.soundcard", "0"));
	p.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_WINDOW; // vsync off in a window
	CHECK(choice_available(p, "amiberry.soundcard", "1"));
	p.soundcard = 1;
	p.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_FULLWINDOW;
	settings::clear_soundcard_for_vsync(p);
	CHECK(p.soundcard == 0);
	// Turning sound on under that vsync mode resets the device as well.
	p.soundcard = 1;
	p.produce_sound = 0;
	CHECK(set(p, "sound_output", "normal") == AMIBERRY_SETTINGS_OK);
	CHECK(p.soundcard == 0);

	// An open session's device is found again by name after a refresh
	// reorders the host's devices.
	reset_prefs(changed_prefs);
	changed_prefs.soundcard_default = false;
	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(amiberry_settings_set(session, "amiberry.soundcard", "1") == AMIBERRY_SETTINGS_OK); // headset
	swap_sound_devices = true;
	amiberry_settings_refresh_choices();
	swap_sound_devices = false;
	char card[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(amiberry_settings_get(session, "amiberry.soundcard", card, sizeof card) == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(card, "0") == 0);
	amiberry_settings_session_close(session);
	std::swap(sound_devices[0], sound_devices[1]);
	std::swap(record_devices[0], record_devices[1]);
	amiberry_settings_refresh_choices();
	// So is its sampler's audio input.
	session = amiberry_settings_session_open();
	CHECK(amiberry_settings_set(session, "amiberry.samplersoundcard", "1") == AMIBERRY_SETTINGS_OK); // line in
	swap_sound_devices = true;
	amiberry_settings_refresh_choices();
	swap_sound_devices = false;
	CHECK(amiberry_settings_get(session, "amiberry.samplersoundcard", card, sizeof card) == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(card, "0") == 0);
	amiberry_settings_session_close(session);
	std::swap(record_devices[0], record_devices[1]);
	std::swap(sound_devices[0], sound_devices[1]);
	amiberry_settings_refresh_choices();

	// Turning RTG vsync off clears the lagless mode the fixup left, so the
	// device check no longer sees it.
	reset_prefs(p);
	p.address_space_24 = false;
	p.rtgboards[0].rtgmem_size = 8;
	p.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_FULLWINDOW;
	p.gfx_apmode[APMODE_RTG].gfx_vsync = 1;
	p.gfx_apmode[APMODE_RTG].gfx_vsyncmode = 1;
	CHECK(set(p, "gfx_vsync_picasso", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.gfx_apmode[APMODE_RTG].gfx_vsyncmode == 0);

	// The filter's "fixedonly" spelling is reported, never offered.
	reset_prefs(p);
	p.sound_filter = 3;
	CHECK(value_of(p, "sound_filter") == "fixedonly");
	CHECK(!choice_available(p, "sound_filter", "fixedonly"));
}

void check_input_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// Port devices decode to the input system's ids and back.
	CHECK(value_of(p, "joyport0") == "mouse");
	CHECK(value_of(p, "joyport1") == "joy0");
	CHECK(set(p, "joyport1", "kbd3") == AMIBERRY_SETTINGS_OK);
	CHECK(p.jports[1].id == JSEM_KBDLAYOUT + 2);
	CHECK(value_of(p, "joyport1") == "kbd3");
	CHECK(set(p, "joyport2", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "joyport3", "mouse") == AMIBERRY_SETTINGS_OK);
	CHECK(p.jports[3].id == JSEM_MICE);
	CHECK(set(p, "joyport1", "joy0") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "joyport1", "joy5") == AMIBERRY_SETTINGS_INVALID_VALUE); // one test joystick
	p.jports[1].id = JSEM_JOYS + 5; // a configuration naming a disconnected device
	CHECK(value_of(p, "joyport1") == "joy5");
	// The port's list starts with that device, once, named for what it is;
	// the other ports and the connected list do not show it.
	auto port_list_starts_with = [&p](const char* id, const char* value, const char* label) {
		const settings::choice_list list = settings::choices_of(settings::require(id), p);
		size_t seen = 0;
		for (size_t i = 0; i < list.count; i++)
			seen += !std::strcmp(list.items[i].value, value);
		return list.count > 1 && seen == 1 && !std::strcmp(list.items[0].value, value)
			&& !std::strcmp(list.items[0].label, label);
	};
	auto lists = [](const settings::choice_list& list, const char* value) {
		for (size_t i = 0; i < list.count; i++) {
			if (!std::strcmp(list.items[i].value, value))
				return true;
		}
		return false;
	};
	CHECK(port_list_starts_with("joyport1", "joy5", "Joystick 6 (not connected)"));
	CHECK(!lists(settings::choices_of(settings::require("joyport0"), p), "joy5"));
	CHECK(!lists(settings::connected_port_devices(), "joy5"));
	// Picking it again keeps it.
	CHECK(set(p, "joyport1", "joy5") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "joyport1") == "joy5");
	p.jports[3].id = JSEM_MICE + 2;
	CHECK(port_list_starts_with("joyport3", "mouse2", "Mouse 3 (not connected)"));
	// A connected device is not repeated at the top.
	p.jports[3].id = JSEM_MICE;
	CHECK(!std::strcmp(settings::choices_of(settings::require("joyport3"), p).items[0].value, "none"));

	// A custom mapping id from a configuration keeps its spelling.
	p.jports[1].id = JSEM_CUSTOM;
	CHECK(value_of(p, "joyport1") == "custom0");
	CHECK(port_list_starts_with("joyport1", "custom0", "Custom input mapping 1"));
	p.jports[1].id = JSEM_JOYS;

	// The mouse map needs a non-default mode; hotkey bits stay visible.
	CHECK(!available(p, "joyport1mousemap"));
	CHECK(set(p, "joyport1mode", "gamepad") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "joyport1mousemap"));
	CHECK(set(p, "joyport1mousemap", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "joyport1mousemap") == "1");
	p.jports[1].mousemap = 3;
	CHECK(value_of(p, "joyport1mousemap") == "3");
	// Only the left stick is routed, so the right stick's bit is no choice.
	CHECK(set(p, "joyport1mousemap", "2") == AMIBERRY_SETTINGS_INVALID_VALUE);

	// Modes include the lightpen entry the old panel indexed past.
	CHECK(set(p, "joyport1mode", "lightpen") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "joyport1mode") == "lightpen");
	CHECK(p.jports[1].mode == 8);

	// Autofire and the shared autofire rate.
	CHECK(set(p, "joyport1autofire", "toggle") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "joyport1autofire") == "toggle");
	CHECK(value_of(p, "input.autofire_speed") == "600");
	CHECK(set(p, "input.autofire_speed", "1248") == AMIBERRY_SETTINGS_OK);

	// The speeds offer the writer's exact values.
	CHECK(set(p, "input.joymouse_speed_digital", "15") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "input.joymouse_speed_digital", "12") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "input.joymouse_speed_analog", "100") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "input.mouse_speed", "1001") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "input.mouse_speed", "0") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "input.mouse_speed", "150") == AMIBERRY_SETTINGS_OK);

	// One untrap bitmask behind two keys.
	CHECK(value_of(p, "amiberry.middle_mouse") == "true");
	CHECK(value_of(p, "magic_mouse") == "false");
	CHECK(set(p, "magic_mouse", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(p.input_mouse_untrap == (MOUSEUNTRAP_MIDDLEBUTTON | MOUSEUNTRAP_MAGIC));
	CHECK(set(p, "amiberry.middle_mouse", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(p.input_mouse_untrap == MOUSEUNTRAP_MAGIC);
	// The two flags commit independently: a session that turned Magic
	// Mouse off keeps a middle-button change made elsewhere meanwhile.
	{
		changed_prefs = p;
		amiberry_settings_session* session = amiberry_settings_session_open();
		CHECK(amiberry_settings_set(session, "magic_mouse", "false") == AMIBERRY_SETTINGS_OK);
		changed_prefs.input_mouse_untrap |= MOUSEUNTRAP_MIDDLEBUTTON; // meanwhile
		bool committed = false;
		amiberry_settings_commit(session, &committed);
		CHECK(committed && changed_prefs.input_mouse_untrap == MOUSEUNTRAP_MIDDLEBUTTON);
		amiberry_settings_session_close(session);
	}

	// Tablet options follow the absolute mouse mode.
	CHECK(!available(p, "magic_mousecursor") && !available(p, "tablet_library"));
	CHECK(set(p, "absolute_mouse", "mousehack") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "magic_mousecursor") && available(p, "tablet_library"));
	CHECK(set(p, "magic_mousecursor", "host") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "magic_mousecursor") == "host");
	CHECK(set(p, "absolute_mouse", "none") == AMIBERRY_SETTINGS_OK);
	CHECK(p.input_tablet == 0);

	// The configuration stores a mode and mouse map for ports 1 and 2 only.
	CHECK(settings::find("joyport1mode") && settings::find("joyport1mousemap"));
	CHECK(!settings::find("joyport2mode") && !settings::find("joyport3mousemap"));

	// A port's settings merge one by one: a session's autofire change keeps
	// a mode changed elsewhere meanwhile.
	{
		reset_prefs(changed_prefs);
		amiberry_settings_session* session = amiberry_settings_session_open();
		CHECK(amiberry_settings_set(session, "joyport1autofire", "normal") == AMIBERRY_SETTINGS_OK);
		changed_prefs.jports[1].mode = JSEM_MODE_GAMEPAD; // meanwhile
		bool committed = false;
		amiberry_settings_commit(session, &committed);
		CHECK(committed && changed_prefs.jports[1].autofire == 1
			&& changed_prefs.jports[1].mode == JSEM_MODE_GAMEPAD);
		amiberry_settings_session_close(session);
	}

	// An open session's joystick follows its device when the host list is
	// reordered.
	{
		reset_prefs(changed_prefs);
		joystick_total = 2;
		amiberry_settings_refresh_choices();
		amiberry_settings_session* session = amiberry_settings_session_open();
		CHECK(amiberry_settings_set(session, "joyport1", "joy1") == AMIBERRY_SETTINGS_OK); // pad-b
		swap_joysticks = true;
		amiberry_settings_refresh_choices();
		char port[AMIBERRY_SETTINGS_VALUE_MAX];
		CHECK(amiberry_settings_get(session, "joyport1", port, sizeof port) == AMIBERRY_SETTINGS_OK);
		CHECK(std::strcmp(port, "joy0") == 0);
		amiberry_settings_session_close(session);
		swap_joysticks = false;
		joystick_total = 1;
		amiberry_settings_refresh_choices();
	}
}

void check_onscreen_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	CHECK(available(p, "amiberry.onscreen_joystick"));
	CHECK(!available(p, "amiberry.vkbd_numpad"));
	CHECK(set(p, "amiberry.vkbd_enabled", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(available(p, "amiberry.vkbd_numpad"));
	CHECK(value_of(p, "amiberry.vkbd_transparency") == "0");
	CHECK(set(p, "amiberry.vkbd_transparency", "40") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "amiberry.vkbd_transparency") == "40");
	CHECK(set(p, "amiberry.vkbd_language", "DE") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "amiberry.vkbd_language") == "DE");
	p.vkbd_language[0] = 'I'; // an unknown layout from a configuration
	p.vkbd_language[1] = 0;
	CHECK(value_of(p, "amiberry.vkbd_language") == "I");

	// The toggle is free-form text: keys, buttons and empty to unbind.
	CHECK(value_of(p, "amiberry.vkbd_toggle") == "guide");
	CHECK(set(p, "amiberry.vkbd_toggle", "leftshoulder") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.vkbd_toggle", "") == AMIBERRY_SETTINGS_OK);
	CHECK(value_of(p, "amiberry.vkbd_toggle") == "");
	// A binding longer than the field is refused, not truncated.
	CHECK(set(p, "amiberry.vkbd_toggle", std::string(sizeof p.vkbd_toggle, 'a').c_str())
		== AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "amiberry.use_retroarch_vkbd", "true") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "amiberry.vkbd_toggle"));
}

void check_floppy_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// A disabled drive has no image to set.
	CHECK(available(p, "floppy0") && !available(p, "floppy2"));
	CHECK(set(p, "floppy0", "/disks/wb.adf") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.floppyslots[0].df, "/disks/wb.adf") == 0);
	CHECK(set(p, "floppy2", "/disks/wb.adf") == AMIBERRY_SETTINGS_UNAVAILABLE);
	// A path that does not fit the field is refused, not truncated.
	const std::string too_long(MAX_DPATH, 'a');
	CHECK(set(p, "floppy0", too_long.c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "diskimage19", too_long.c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(set(p, "diskimage19", "/disks/game2.adf") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.dfxlist[19], "/disks/game2.adf") == 0);

	// Drive types are the raw cfgfile numbers; the PC bridgeboard types
	// are reported but not offered.
	CHECK(set(p, "floppy1type", "7") == AMIBERRY_SETTINGS_OK && p.floppyslots[1].dfxtype == DRV_525_DD);
	CHECK(set(p, "floppy1type", "4") == AMIBERRY_SETTINGS_INVALID_VALUE);
	p.floppyslots[1].dfxtype = DRV_PC_525_ONLY_40;
	CHECK(value_of(p, "floppy1type") == "4");
	CHECK(set(p, "floppy2type", "1") == AMIBERRY_SETTINGS_OK && available(p, "floppy2"));

	// FloppyBridge: switching to it picks the Normal profile, the mode
	// option writes the profile ID, and leaving it clears both.
	CHECK(!available(p, "floppy0subtype"));
	CHECK(set(p, "floppy0type", "8") == AMIBERRY_SETTINGS_OK);
	CHECK(p.floppyslots[0].dfxsubtype == 0 && std::strcmp(p.floppyslots[0].dfxsubtypeid, "1:Normal") == 0);
	CHECK(available(p, "floppy0subtype"));
	CHECK(set(p, "floppy0subtype", "2") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.floppyslots[0].dfxsubtypeid, "3:Turbo") == 0);
	CHECK(set(p, "floppy0type", "8") == AMIBERRY_SETTINGS_OK); // re-selected
	CHECK(p.floppyslots[0].dfxsubtype == 2);
	CHECK(set(p, "floppy0type", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(p.floppyslots[0].dfxsubtype == 0 && p.floppyslots[0].dfxsubtypeid[0] == 0);

	// The drive speed offers the panel's steps and reports others as-is.
	CHECK(set(p, "floppy_speed", "0") == AMIBERRY_SETTINGS_OK && p.floppy_speed == 0);
	CHECK(set(p, "floppy_speed", "300") == AMIBERRY_SETTINGS_INVALID_VALUE);
	p.floppy_speed = 50;
	CHECK(value_of(p, "floppy_speed") == "50");

	// DrawBridge options follow what the selected driver supports.
	reset_prefs(p);
	CHECK(settings::choices_of(settings::require("amiberry.drawbridge_driver"), p).count == 2);
	CHECK(available(p, "amiberry.drawbridge_serial_autodetect"));
	CHECK(!available(p, "amiberry.drawbridge_serial_port")); // detected
	CHECK(set(p, "amiberry.drawbridge_serial_autodetect", "false") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.drawbridge_serial_port", "/dev/ttyUSB0") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.drawbridge_drive_cable", "5") == AMIBERRY_SETTINGS_OK);
	CHECK(set(p, "amiberry.drawbridge_drive_cable", "6") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// Shugart lines need a driver with Shugart mode.
	drawbridge_shugart = false;
	CHECK(!choice_available(p, "amiberry.drawbridge_drive_cable", "2"));
	CHECK(choice_available(p, "amiberry.drawbridge_drive_cable", "1"));
	drawbridge_shugart = true;
	CHECK(set(p, "amiberry.drawbridge_driver", "1") == AMIBERRY_SETTINGS_OK);
	CHECK(!available(p, "amiberry.drawbridge_serial_autodetect"));
	CHECK(!available(p, "amiberry.drawbridge_smartspeed") && !available(p, "amiberry.drawbridge_drive_cable"));
	p.drawbridge_serial_auto = true; // without detection the port is chosen by hand
	CHECK(available(p, "amiberry.drawbridge_serial_port"));
	CHECK(set(p, "amiberry.drawbridge_driver", "2") == AMIBERRY_SETTINGS_INVALID_VALUE);
}

void check_storage_domain()
{
	uae_prefs& p = test_prefs;
	reset_prefs(p);

	// No line means no drive; a drive without a disc reads as "empty", the
	// reader's other spelling for the empty value the writer saves.
	CHECK(value_of(p, "cdimage0") == "");
	CHECK(set(p, "cdimage0", "empty") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cdslots[0].inuse && !p.cdslots[0].name[0]);
	CHECK(value_of(p, "cdimage0") == "empty");
	CHECK(set(p, "cdimage0", "/cd/game.iso") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.cdslots[0].name, "/cd/game.iso") == 0 && p.cdslots[0].type == SCSI_UNIT_DEFAULT);
	CHECK(value_of(p, "cdimage0") == "/cd/game.iso");
	// A host drive and the delay flag use the writer's suffixes.
	CHECK(set(p, "cdimage0", "/dev/disk4,delay:ioctl") == AMIBERRY_SETTINGS_OK);
	CHECK(p.cdslots[0].type == SCSI_UNIT_IOCTL && p.cdslots[0].delayed);
	CHECK(value_of(p, "cdimage0") == "/dev/disk4,delay:ioctl");
	// autodetect replaces the whole slot, the delay included.
	CHECK(set(p, "cdimage0", "autodetect") == AMIBERRY_SETTINGS_OK);
	CHECK(!p.cdslots[0].delayed && value_of(p, "cdimage0") == "empty");
	// A comma in the file name keeps the path whole.
	CHECK(set(p, "cdimage0", "/cd/a,b.iso,") == AMIBERRY_SETTINGS_OK);
	CHECK(std::strcmp(p.cdslots[0].name, "/cd/a,b.iso") == 0);
	CHECK(value_of(p, "cdimage0") == "/cd/a,b.iso,");
	CHECK(set(p, "cdimage0", "") == AMIBERRY_SETTINGS_OK);
	CHECK(!p.cdslots[0].inuse && value_of(p, "cdimage0") == "");
	CHECK(set(p, "cdimage0", std::string(MAX_DPATH, 'a').c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);

	// Automounting host drives needs the SCSI device layer, which the
	// fixup turns on from the final value: the setter leaves it alone.
	p.scsi = 0;
	CHECK(set(p, "amiberry.map_cd_drives", "true") == AMIBERRY_SETTINGS_OK && p.automount_cddrives);
	CHECK(set(p, "amiberry.map_cd_drives", "false") == AMIBERRY_SETTINGS_OK && p.scsi == 0);
	CHECK(set(p, "cd_speed", "0") == AMIBERRY_SETTINGS_OK && p.cd_speed == 0);
	CHECK(set(p, "cd_speed", "50") == AMIBERRY_SETTINGS_INVALID_VALUE);

	// The metadata mode cannot change under mounted volumes.
	const settings::context running{true};
	CHECK(available(p, "filesys_no_fsdb") && !available(p, "filesys_no_fsdb", running));
	CHECK(available(p, "filesys_custom_uaefsdb", running));
}

// The mount list: edits stay in the session, are checked like the hard
// drive panel's, and commit as a whole list that needs a reset.
void check_mount_list()
{
	reset_prefs(changed_prefs);
	changed_prefs.mountitems = 0;
	// A drive the configuration already has, attached to a running unit,
	// with injected icons (a filesystem_extra the spelling does not carry).
	cfgfile_parse_mount_value(&changed_prefs, "dir,rw,DH0:/work", &changed_prefs.mountconfig[0].ci);
	changed_prefs.mountconfig[0].ci.inject_icons = true;
	changed_prefs.mountconfig[0].configoffset = 7;
	changed_prefs.mountconfig[0].unitnum = 3;
	changed_prefs.mountitems = 1;

	amiberry_settings_session* session = amiberry_settings_session_open();
	CHECK(session && amiberry_settings_mount_count(session) == 1);
	char value[64];
	CHECK(amiberry_settings_mount_get(session, 0, value, sizeof value) == AMIBERRY_SETTINGS_OK
		&& !std::strcmp(value, "dir,rw,DH0:/work"));
	CHECK(amiberry_settings_mount_get(session, 0, value, 4) == AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
	CHECK(amiberry_settings_mount_get(session, 1, value, sizeof value) == AMIBERRY_SETTINGS_UNKNOWN_ID);

	CHECK(amiberry_settings_mount_add(session, "dir,rw,DH1:/games") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_add(session, "dir,rw,DH2:/music") == AMIBERRY_SETTINGS_OK);
	// Refused like the panel refuses them: a device name in use, a value
	// that is no entry, an entry too long for a configuration line.
	CHECK(amiberry_settings_mount_add(session, "dir,rw,DH1:/other") == AMIBERRY_SETTINGS_INVALID_VALUE);
	// Device names compare as AmigaDOS compares them: without case, and as
	// the name the entry gets (shortened to 30 characters).
	CHECK(amiberry_settings_mount_add(session, "dir,rw,dh1:/other") == AMIBERRY_SETTINGS_INVALID_VALUE);
	const std::string long_name(30, 'L');
	CHECK(amiberry_settings_mount_add(session, ("dir,rw," + long_name + "A:/a").c_str()) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_add(session, ("dir,rw," + long_name + "B:/b").c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_mount_remove(session, 3) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_add(session, "bogus") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_mount_add(session, nullptr) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_mount_add(session,
		("dir,rw,DH3:/" + std::string(stub_mount_line_max, 'x')).c_str()) == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_mount_count(session) == 3);

	// Replacing keeps the entry's place, its running unit and what the
	// spelling does not carry; its own device name stays usable, another
	// entry's does not.
	CHECK(amiberry_settings_mount_replace(session, 0, "dir,rw,DH0:/work2") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_replace(session, 0, "dir,rw,DH2:/clash") == AMIBERRY_SETTINGS_INVALID_VALUE);
	CHECK(amiberry_settings_mount_replace(session, 3, "dir,rw,DH9:/x") == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(amiberry_settings_mount_get(session, 0, value, sizeof value) == AMIBERRY_SETTINGS_OK
		&& !std::strcmp(value, "dir,rw,DH0:/work2"));

	// Removing moves the later entries up.
	CHECK(amiberry_settings_mount_remove(session, 1) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_remove(session, 2) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(amiberry_settings_mount_count(session) == 2);
	CHECK(amiberry_settings_mount_get(session, 1, value, sizeof value) == AMIBERRY_SETTINGS_OK
		&& !std::strcmp(value, "dir,rw,DH2:/music"));

	// Nothing reaches the pending configuration before commit, which stores
	// the whole list and asks for a reset.
	CHECK(changed_prefs.mountitems == 1 && !std::strcmp(changed_prefs.mountconfig[0].ci.rootdir, "/work"));
	bool changed = false;
	CHECK(amiberry_settings_commit(session, &changed) == AMIBERRY_APPLY_RESET && changed);
	CHECK(changed_prefs.mountitems == 2);
	CHECK(!std::strcmp(changed_prefs.mountconfig[0].ci.rootdir, "/work2"));
	CHECK(changed_prefs.mountconfig[0].ci.inject_icons);
	CHECK(changed_prefs.mountconfig[0].configoffset == 7 && changed_prefs.mountconfig[0].unitnum == 3);
	CHECK(!std::strcmp(changed_prefs.mountconfig[1].ci.devname, "DH2"));

	// A session that leaves the list alone keeps what the panel changed
	// since, and its other changes do not ask for a reset.
	changed_prefs.mountitems = 1;
	CHECK(amiberry_settings_set(session, "cd_speed", "0") == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_commit(session, &changed) != AMIBERRY_APPLY_RESET && changed);
	CHECK(changed_prefs.mountitems == 1);
	CHECK(amiberry_settings_mount_count(session) == 1);

	// The list holds AMIBERRY_SETTINGS_MOUNT_MAX entries.
	for (int i = 1; i < AMIBERRY_SETTINGS_MOUNT_MAX; i++)
		CHECK(amiberry_settings_mount_add(session, ("dir,rw,X" + std::to_string(i) + ":/x").c_str()) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_count(session) == AMIBERRY_SETTINGS_MOUNT_MAX);
	CHECK(amiberry_settings_mount_add(session, "dir,rw,FULL:/x") == AMIBERRY_SETTINGS_UNAVAILABLE);
	CHECK(amiberry_settings_mount_remove(session, AMIBERRY_SETTINGS_MOUNT_MAX - 1) == AMIBERRY_SETTINGS_OK);
	CHECK(amiberry_settings_mount_count(session) == AMIBERRY_SETTINGS_MOUNT_MAX - 1);
	CHECK(amiberry_settings_mount_count(nullptr) == 0);
	amiberry_settings_session_close(session);
	changed_prefs.mountitems = 0;
}

// get_active reads the configuration the emulation runs with, not the
// pending one a commit stores.
void check_active_values()
{
	reset_prefs(currprefs);
	reset_prefs(changed_prefs);
	currprefs.cpu_model = 68020;
	changed_prefs.cpu_model = 68030;
	char value[AMIBERRY_SETTINGS_VALUE_MAX];
	CHECK(amiberry_settings_get_active("cpu_model", value, sizeof value) == AMIBERRY_SETTINGS_OK
		&& !std::strcmp(value, "68020"));
	CHECK(amiberry_settings_get_active("cpu_model", value, 3) == AMIBERRY_SETTINGS_BUFFER_TOO_SMALL);
	CHECK(amiberry_settings_get_active("not_an_option", value, sizeof value) == AMIBERRY_SETTINGS_UNKNOWN_ID);
	CHECK(amiberry_settings_get_active(nullptr, value, sizeof value) == AMIBERRY_SETTINGS_UNKNOWN_ID);
}

} // namespace

// === Runtime action queue (settings/settings_actions_queue.h) ===
// The queue the emulator drains on its main thread: ordering, cross-thread
// posting, completion bookkeeping and bounded retention.

struct queue_log {
	std::mutex mutex;
	std::vector<int> executed; // the `number` of each executed request
	std::vector<std::string> errors;
};

static bool log_executor(const amiberry_actions::request& r, void* context, std::string* error)
{
	auto* log = static_cast<queue_log*>(context);
	std::lock_guard<std::mutex> lock(log->mutex);
	log->executed.push_back(r.number);
	if (r.flag_a) {
		*error = "requested failure";
		return false;
	}
	return true;
}

static void check_action_queue()
{
	amiberry_actions::action_queue queue;
	queue_log log;

	// Posting assigns increasing non-zero ids; a request stays pending
	// until run_pending() executes it.
	const auto first = queue.post(amiberry_actions::kind::pause, 0, 1, false, false, nullptr);
	const auto second = queue.post(amiberry_actions::kind::resume, 0, 2, false, false, nullptr);
	CHECK(first != 0 && second == first + 1);
	CHECK(queue.poll(first, nullptr) == 0);

	// Actions execute in posting order on the caller of run_pending().
	CHECK(queue.run_pending(log_executor, &log) == 2);
	CHECK(log.executed.size() == 2 && log.executed[0] == 1 && log.executed[1] == 2);

	// Polling consumes the result: done once, unknown afterwards. A failed
	// action reports its error text.
	CHECK(queue.poll(first, nullptr) == 1);
	CHECK(queue.poll(first, nullptr) == 3);
	std::string error;
	const auto failing = queue.post(amiberry_actions::kind::pause, 0, 3, true, false, nullptr);
	queue.run_pending(log_executor, &log);
	CHECK(queue.poll(failing, &error) == 2);
	CHECK(error == "requested failure");
	CHECK(queue.poll(failing, nullptr) == 3);

	// Unknown ids (never assigned, or zero) never report pending.
	CHECK(queue.poll(999999, nullptr) == 3);
	CHECK(queue.poll(0, nullptr) == 3);

	// The queue refuses more than max_pending requests; run_pending drains
	// them all and executes each exactly once.
	size_t accepted = 0;
	for (int i = 0; i < amiberry_actions::action_queue::max_pending + 10; i++) {
		if (queue.post(amiberry_actions::kind::pause, 0, 5000 + i, false, false, nullptr) != 0)
			accepted++;
	}
	CHECK(accepted == amiberry_actions::action_queue::max_pending);
	CHECK(queue.run_pending(log_executor, &log) == accepted);
	CHECK(log.executed.size() == 2 + 1 + accepted);

	// A pass runs only what was queued when it started: an action posted
	// while it runs (a front end that keeps posting) waits for the next
	// pass instead of keeping the emulator thread in this one.
	struct reposting {
		amiberry_actions::action_queue* queue;
		int runs = 0;
	} repost{&queue};
	const auto reposting_executor = [](const amiberry_actions::request&, void* context, std::string*) {
		auto* state = static_cast<reposting*>(context);
		state->runs++;
		state->queue->post(amiberry_actions::kind::pause, 0, 0, false, false, nullptr);
		return true;
	};
	queue.post(amiberry_actions::kind::pause, 0, 0, false, false, nullptr);
	CHECK(queue.run_pending(reposting_executor, &repost) == 1 && repost.runs == 1);
	CHECK(queue.has_pending());
	CHECK(queue.run_pending(reposting_executor, &repost) == 1 && repost.runs == 2);
	const auto ignore = [](const amiberry_actions::request&, void*, std::string*) { return true; };
	CHECK(queue.run_pending(ignore, nullptr) == 1);
	CHECK(!queue.has_pending());

	// Posts from other threads interleave safely and keep their order per
	// the single executor: everything posted runs exactly once.
	std::vector<std::thread> posters;
	std::atomic<int> posted{0};
	for (int t = 0; t < 4; t++) {
		posters.emplace_back([&queue, &posted, t]() {
			for (int i = 0; i < 250; i++) {
				if (queue.post(amiberry_actions::kind::pause, 0, 20000 + t * 1000 + i,
					false, false, nullptr) != 0)
					posted.fetch_add(1);
			}
		});
	}
	for (auto& thread : posters)
		thread.join();
	CHECK(posted.load() == 1000);
	CHECK(queue.run_pending(log_executor, &log) == 1000);
	std::map<int, int> seen;
	{
		std::lock_guard<std::mutex> lock(log.mutex);
		for (int number : log.executed)
			seen[number]++;
	}
	CHECK(seen.size() == 3 + accepted + 1000); // 1,2,3 + queued + threaded
	for (const auto& entry : seen)
		CHECK(entry.second == 1);

	// Results are retained for the most recent max_results completions;
	// older unconsumed results retire to unknown.
	amiberry_actions::action_queue retiring;
	queue_log retiring_log;
	uint64_t oldest = 0;
	for (int round = 0; round < 11; round++) {
		for (int i = 0; i < 100; i++) {
			const auto id = retiring.post(amiberry_actions::kind::pause, 0, i,
				false, false, nullptr);
			if (round == 0 && i == 0)
				oldest = id;
		}
		retiring.run_pending(log_executor, &retiring_log);
	}
	CHECK(retiring_log.executed.size() == 1100);
	CHECK(retiring.poll(oldest, nullptr) == 3);
}

int main(int argc, char** argv)
{
	check_action_queue();

	if (argc > 1 && !std::strcmp(argv[1], "--emit-roundtrip")) {
		emit_roundtrip();
		return failures ? 1 : 0;
	}
	check_descriptors();
	check_availability();
	check_dependent_changes();
	check_rejections();
	check_session();
	check_fixup_consistency();
	check_machine_availability();
	check_machine_dependents();
	check_machine_rejections();
	check_machine_fixup_rules();
	check_presets();
	check_machine_session();
	check_display_domain();
	check_sound_domain();
	check_input_domain();
	check_onscreen_domain();
	check_floppy_domain();
	check_storage_domain();
	check_rtg_domain();
	check_expansions_domain();
	check_boards();
	check_filter_domain();
	check_io_domain();
	check_custom_controls();
	check_hotkeys();
	check_reversed_volumes();
	check_number_metadata();
	check_host_settings();
	check_host_choices();
	check_config_text();
	check_mount_list();
	check_active_values();
	if (failures) {
		std::fprintf(stderr, "settings layer: %d check(s) failed\n", failures);
		return 1;
	}
	std::printf("settings layer checks passed\n");
	return 0;
}
