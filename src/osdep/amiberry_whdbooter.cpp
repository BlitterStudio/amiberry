/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysdeps.h"
#include "uae.h"
#include "options.h"
#include "rommgr.h"
#include "fsdb.h"
#include "tinyxml2.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "midiemu.h"
#include "registry.h"
#include "zfile.h"
#include "zarchive.h"

extern void set_last_active_config(const char* filename);
extern std::string current_dir;
extern char last_loaded_config[MAX_DPATH];

// Use configs with 8MB Fast RAM, to make it likely
// that WHDLoad preload will cache everything.
enum
{
	A600_CONFIG = 3, // 8MB fast ram
	A1200_CONFIG = 1 // 8MB fast ram
};

struct game_hardware_options
{
	std::string port0 = "nul";
	std::string port1 = "nul";
	std::string control = "nul";
	std::string control2 = "nul";
	std::string cpu = "nul";
	std::string blitter = "nul";
	std::string clock = "nul";
	std::string chipset = "nul";
	std::string jit = "nul";
	std::string cpu_comp = "nul";
	std::string cpu_24bit = "nul";
	std::string cpu_exact = "nul";
	std::string sprites = "nul";
	std::string scr_width = "nul";
	std::string scr_height = "nul";
	std::string scr_autoheight = "nul";
	std::string scr_centerh = "nul";
	std::string scr_centerv = "nul";
	std::string scr_offseth = "nul";
	std::string scr_offsetv = "nul";
	std::string ntsc = "nul";
	std::string chip = "nul";
	std::string fast = "nul";
	std::string z3 = "nul";
};

std::filesystem::path whdbooter_path;
std::filesystem::path boot_path;
std::filesystem::path save_path;
std::filesystem::path conf_path;
std::filesystem::path whd_path;
std::filesystem::path kickstart_path;

std::string uae_config;
std::string whd_config;
std::string whd_startup;

// Returns the temporary directory for WHDBooter boot files
// Uses whdboot/tmp/ which is always user-writable on all platforms
std::filesystem::path get_whdboot_temp_path()
{
	std::filesystem::path temp_path = get_whdbootpath();
	temp_path /= "tmp";
	return temp_path;
}

static TCHAR* parse_text(const TCHAR* s)
{
	if (*s == '"' || *s == '\'')
	{
		const auto c = *s++;
		auto* const d = my_strdup(s);
		for (unsigned int i = 0; i < _tcslen(d); i++)
		{
			if (d[i] == c)
			{
				d[i] = 0;
				break;
			}
		}
		return d;
	}
	return my_strdup(s);
}

static int safe_stoi(const std::string& s, const int default_val = 0)
{
	try { return std::stoi(s); }
	catch (...) { return default_val; }
}

static void create_link_or_copy(const std::filesystem::path& source, const std::filesystem::path& link_path, const char* name)
{
	if (std::filesystem::is_symlink(link_path) || std::filesystem::exists(link_path))
	{
		try
		{
			std::filesystem::remove_all(link_path);
		}
		catch (std::filesystem::filesystem_error& e)
		{
			write_log("WHDBooter - Failed to remove existing %s link/tree %s: %s\n", name, link_path.string().c_str(), e.what());
		}
	}

	if (!std::filesystem::exists(source))
	{
		write_log("WHDBooter - %s source not found: %s\n", name, source.string().c_str());
		return;
	}

	if (std::filesystem::exists(link_path))
		return;

	write_log("WHDBooter - Creating link/copy to %s in %s\n", name, link_path.parent_path().string().c_str());
	try
	{
#if defined(__ANDROID__) || defined(_WIN32)
		std::filesystem::copy(source, link_path, std::filesystem::copy_options::recursive);
#else
		std::filesystem::create_symlink(source, link_path);
#endif
	}
	catch (std::filesystem::filesystem_error& e)
	{
		write_log("WHDBooter - %s link creation failed (%s). Falling back to copy: %s\n", name, link_path.string().c_str(), e.what());
		try
		{
			std::filesystem::copy(source, link_path, std::filesystem::copy_options::recursive);
		}
		catch (std::filesystem::filesystem_error& e2)
		{
			write_log("WHDBooter - %s copy also failed: %s\n", name, e2.what());
		}
	}
}

std::string trim_full_line(std::string full_line)
{
	const std::string tab = "\t";

	auto found = full_line.find(tab);
	while (found != std::string::npos)
	{
		full_line.replace(found, tab.size(), "");
		found = full_line.find(tab);
	}

	return full_line;
}

std::string find_substring(const std::string& search_string, const std::string& whole_string)
{
	const std::string lf = "\n";
	const auto check = string(search_string);
	auto full_line = trim_full_line(string(whole_string));

	auto lf_found = full_line.find(lf);
	while (lf_found != std::string::npos)
	{
		const auto start = full_line.find_first_of(lf, lf_found);
		auto end = full_line.find_first_of(lf, start + 1);
		if (end == std::string::npos) end = full_line.size();

		const auto found = full_line.find(check);
		if (found != std::string::npos)
		{
			const auto found_end = full_line.find_first_of(lf, found);
			auto result = full_line.substr(found + check.size() + 1, found_end - found - check.size() - 1);
			return result;
		}

		if (end < full_line.size())
			lf_found = end;
		else
			lf_found = std::string::npos;
	}

	return "nul";
}

void parse_cfg_line(uae_prefs* prefs, const std::string& line_string)
{
	TCHAR* line = my_strdup(line_string.c_str());
	cfgfile_parse_line(prefs, line, 0);
	xfree(line);
}

void parse_custom_settings(uae_prefs* p, const std::string& settings)
{
	const std::string lf = "\n";
	const std::string check = "amiberry_custom";
	auto full_line = trim_full_line(settings);

	auto lf_found = full_line.find(lf);
	while (lf_found != std::string::npos)
	{
		const auto start = full_line.find_first_of(lf, lf_found);
		auto end = full_line.find_first_of(lf, start + 1);
		if (end == std::string::npos) end = full_line.size();

		std::string line = full_line.substr(start + 1, end - start - 1);
		if (line.find(check) != std::string::npos)
		{
			parse_cfg_line(p, line);
		}

		if (end < full_line.size())
			lf_found = end;
		else
			lf_found = std::string::npos;
	}
}

std::string find_whdload_game_option(const std::string& find_setting, const std::string& whd_options)
{
	return find_substring(find_setting, whd_options);
}

game_hardware_options get_game_hardware_settings(const std::string& hardware)
{
	game_hardware_options output_detail;
	output_detail.port0 = find_whdload_game_option("PORT0", hardware);
	output_detail.port1 = find_whdload_game_option("PORT1", hardware);
	output_detail.control = find_whdload_game_option("PRIMARY_CONTROL", hardware);
	output_detail.control2 = find_whdload_game_option("SECONDARY_CONTROL", hardware);
	output_detail.cpu = find_whdload_game_option("CPU", hardware);
	output_detail.blitter = find_whdload_game_option("BLITTER", hardware);
	output_detail.clock = find_whdload_game_option("CLOCK", hardware);
	output_detail.chipset = find_whdload_game_option("CHIPSET", hardware);
	output_detail.jit = find_whdload_game_option("JIT", hardware);
	output_detail.cpu_24bit = find_whdload_game_option("CPU_24BITADDRESSING", hardware);
	output_detail.cpu_comp = find_whdload_game_option("CPU_COMPATIBLE", hardware);
	output_detail.sprites = find_whdload_game_option("SPRITES", hardware);
	output_detail.scr_height = find_whdload_game_option("SCREEN_HEIGHT", hardware);
	output_detail.scr_width = find_whdload_game_option("SCREEN_WIDTH", hardware);
	output_detail.scr_autoheight = find_whdload_game_option("SCREEN_AUTOHEIGHT", hardware);
	output_detail.scr_centerh = find_whdload_game_option("SCREEN_CENTERH", hardware);
	output_detail.scr_centerv = find_whdload_game_option("SCREEN_CENTERV", hardware);
	output_detail.scr_offseth = find_whdload_game_option("SCREEN_OFFSETH", hardware);
	output_detail.scr_offsetv = find_whdload_game_option("SCREEN_OFFSETV", hardware);
	output_detail.ntsc = find_whdload_game_option("NTSC", hardware);
	output_detail.fast = find_whdload_game_option("FAST_RAM", hardware);
	output_detail.z3 = find_whdload_game_option("Z3_RAM", hardware);
	output_detail.cpu_exact = find_whdload_game_option("CPU_EXACT", hardware);

	return output_detail;
}

void make_rom_symlink(const std::string& kickstart_short_name, const int kickstart_number, struct uae_prefs* prefs)
{
	std::filesystem::path kickstart_long_path = kickstart_path;
	kickstart_long_path /= kickstart_short_name;

	// Remove the symlink if it already exists
	if (std::filesystem::is_symlink(kickstart_long_path)) {
		std::filesystem::remove(kickstart_long_path);
	}

	if (!std::filesystem::exists(kickstart_long_path))
	{
		const int roms[2] = { kickstart_number, -1 };
		// copy the existing prefs->romfile to a backup variable, so we can restore it afterward
		const std::string old_romfile = prefs->romfile;
		if (configure_rom(prefs, roms, 0) == 1)
		{
			try {
#if defined(__ANDROID__) || defined(_WIN32)
				// Android's external storage and Windows often do not support symlinks; copy instead
				std::filesystem::copy(prefs->romfile, kickstart_long_path);
				write_log("Copying Kickstart ROM: %s  [Ok]\n", kickstart_long_path.c_str());
#else
				std::filesystem::create_symlink(prefs->romfile, kickstart_long_path);
				write_log("Making SymLink for Kickstart ROM: %s  [Ok]\n", kickstart_long_path.c_str());
#endif
			}
			catch (std::filesystem::filesystem_error& e) {
#if !defined(__ANDROID__) && !defined(_WIN32)
				// Fallback to copying file if symlink creation fails for any reason
				try {
					std::filesystem::copy(prefs->romfile, kickstart_long_path);
					write_log("SymLink failed, copying Kickstart ROM instead: %s  [Ok]\n", kickstart_long_path.c_str());
				}
				catch (std::filesystem::filesystem_error& e2) {
					write_log("Error copying Kickstart ROM: %s  [Fail]: %s\n", kickstart_long_path.c_str(), e2.what());
				}
#else
				write_log("Error copying Kickstart ROM: %s  [Fail]: %s\n", kickstart_long_path.c_str(), e.what());
#endif
			}
		}
		// restore the original prefs->romfile
		_tcsncpy(prefs->romfile, old_romfile.c_str(), MAX_DPATH - 1);
		prefs->romfile[MAX_DPATH - 1] = '\0';
	}
}

void symlink_roms(struct uae_prefs* prefs)
{
	write_log("SymLink Kickstart ROMs for Booter\n");

	// here we can do some checks for Kickstarts we might need to make symlinks for
	current_dir = home_dir;

	// are we using save-data/ ?
	kickstart_path = get_savedatapath(true);
	kickstart_path /= "Kickstarts";

	if (!std::filesystem::exists(kickstart_path)) {
		// otherwise, use the old route
		whdbooter_path = get_whdbootpath();
		kickstart_path = whdbooter_path / "game-data" / "Devs" / "Kickstarts";
	}
	write_log("WHDBoot - using kickstarts from %s\n", kickstart_path.c_str());

	// These are all the kickstart rom files found in skick346.lha
	//   http://aminet.net/package/util/boot/skick346

	make_rom_symlink("kick33180.A500", 5, prefs);
	make_rom_symlink("kick34005.A500", 6, prefs);
	make_rom_symlink("kick37175.A500", 7, prefs);
	make_rom_symlink("kick39106.A1200", 11, prefs);
	make_rom_symlink("kick40063.A600", 14, prefs);
	make_rom_symlink("kick40068.A1200", 15, prefs);
	make_rom_symlink("kick40068.A4000", 16, prefs);

	// Symlink rom.key also
	// source file
	std::filesystem::path rom_key_source_path = get_rom_path();
	rom_key_source_path /= "rom.key";

	// destination file (symlink)
	std::filesystem::path rom_key_destination_path = kickstart_path;
	rom_key_destination_path /= "rom.key";

	if (std::filesystem::exists(rom_key_source_path) && !std::filesystem::exists(rom_key_destination_path)) {
		try {
#if defined(__ANDROID__) || defined(_WIN32)
			write_log("Copying rom.key\n");
			std::filesystem::copy(rom_key_source_path, rom_key_destination_path);
#else
			write_log("Making SymLink for rom.key\n");
			std::filesystem::create_symlink(rom_key_source_path, rom_key_destination_path);
#endif
		}
		catch (std::filesystem::filesystem_error& e) {
#if !defined(__ANDROID__) && !defined(_WIN32)
			try {
				std::filesystem::copy(rom_key_source_path, rom_key_destination_path);
			}
			catch (std::filesystem::filesystem_error& e2) {
				write_log("Error copying rom.key: %s\n", e2.what());
			}
#else
			write_log("Error copying rom.key: %s\n", e.what());
#endif
		}
	}
}

std::string get_game_filename(const char* filepath)
{
	extract_filename(filepath, last_loaded_config);
	const std::string game_name = extract_filename(filepath);
	return remove_file_extension(game_name);
}

void set_jport_modes(uae_prefs* prefs, const bool is_cd32)
{
	if (is_cd32)
	{
		prefs->jports[0].mode = 7;
		prefs->jports[1].mode = 7;
	}
	else
	{
		// JOY
		prefs->jports[1].mode = 3;
		// MOUSE
		prefs->jports[0].mode = 2;
	}
}
void clear_jports(uae_prefs* prefs)
{
	for (auto& jport : prefs->jports)
	{
		jport.id = JPORT_NONE;
		jport.idc.configname[0] = 0;
		jport.idc.name[0] = 0;
		jport.idc.shortid[0] = 0;
	}
}

void build_uae_config_filename(const std::string& game_name)
{
	uae_config = (conf_path / (game_name + ".uae")).string();
}

void cd_auto_prefs(uae_prefs* prefs, char* filepath)
{
	TCHAR tmp[MAX_DPATH];

	write_log("\nCD Autoload: %s  \n\n", filepath);

	conf_path = get_configuration_path();
	whdload_prefs.filename = get_game_filename(filepath);

	// LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE
	//  CONFIG LOAD IF .UAE IS IN CONFIG PATH
	build_uae_config_filename(whdload_prefs.filename);

	if (std::filesystem::exists(uae_config))
	{
		target_cfgfile_load(prefs, uae_config.c_str(), CONFIG_TYPE_DEFAULT, 0);
		config_loaded = true;
		return;
	}

	prefs->start_gui = false;

    const auto is_mt32 = whdload_prefs.filename.find("MT32") != std::string::npos || whdload_prefs.filename.find("mt32") != std::string::npos;
    const auto is_cdtv = whdload_prefs.filename.find("CDTV") != std::string::npos || whdload_prefs.filename.find("cdtv") != std::string::npos;
    const auto is_cd32 = whdload_prefs.filename.find("CD32") != std::string::npos || whdload_prefs.filename.find("cd32") != std::string::npos;

	if (is_cdtv && !is_cd32)
	{
		_tcscpy(prefs->description, _T("AutoBoot Configuration [CDTV]"));
		// SET THE BASE AMIGA (CDTV)
		built_in_prefs(prefs, 9, 0, 0, 0);
	}
	else
	{
		_tcscpy(prefs->description, _T("AutoBoot Configuration [CD32]"));
		// SET THE BASE AMIGA (CD32)
		built_in_prefs(prefs, 8, 3, 0, 0);
	}

	if (is_mt32)
	{
		// Check if we have the MT32 ROMs
		auto mt32_available = midi_emu_available(_T("MT-32"));
		auto cm32_available = midi_emu_available(_T("CM-32L"));
		if (!mt32_available && !cm32_available)
		{
			write_log("MT32/CM32L MIDI Emulation not available (ROMs missing)\n");
		}
		else
		{
			// Enable MIDI output
			_tcscpy(prefs->midioutdev, mt32_available ? "Munt MT-32" : "Munt CM-32L");
		}
	}

	// enable CD
	_sntprintf(tmp, MAX_DPATH, "cd32cd=1");
	TCHAR* parsed = parse_text(tmp);
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	// mount the image
	_sntprintf(tmp, MAX_DPATH, "cdimage0=%s,image", filepath);
	parsed = parse_text(tmp);
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	//APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
	set_jport_modes(prefs, is_cd32);

	// APPLY SPECIAL CONFIG E.G. MOUSE OR ALT. JOYSTICK SETTINGS
	clear_jports(prefs);

	// WHAT IS THE MAIN CONTROL?
	// PORT 0 - MOUSE
	std::string line_string;
	if (is_cd32 && strcmpi(amiberry_options.default_controller2, "") != 0)
	{
		line_string = "joyport0=";
		line_string.append(amiberry_options.default_controller2);
		parse_cfg_line(prefs, line_string);
	}
	else if (strcmpi(amiberry_options.default_mouse1, "") != 0)
	{
		line_string = "joyport0=";
		line_string.append(amiberry_options.default_mouse1);
		parse_cfg_line(prefs, line_string);
	}
	else
	{
		line_string = "joyport0=mouse";
		parse_cfg_line(prefs, line_string);
	}

	// PORT 1 - JOYSTICK
	if (strcmpi(amiberry_options.default_controller1, "") != 0)
	{
		line_string = "joyport1=";
		line_string.append(amiberry_options.default_controller1);
		parse_cfg_line(prefs, line_string);
	}
	else
	{
		line_string = "joyport1=joy1";
		parse_cfg_line(prefs, line_string);
	}
}

void set_input_settings(uae_prefs* prefs, const game_hardware_options& game_detail, const bool is_cd32)
{
	// APPLY SPECIAL CONFIG E.G. MOUSE OR ALT. JOYSTICK SETTINGS
	clear_jports(prefs);

	//  CD32
	if (is_cd32 || strcmpi(game_detail.port0.c_str(), "cd32") == 0)
		prefs->jports[0].mode = 7;

	if (is_cd32	|| strcmpi(game_detail.port1.c_str(), "cd32") == 0)
		prefs->jports[1].mode = 7;

	// JOY
	if (strcmpi(game_detail.port0.c_str(), "joy") == 0)
		prefs->jports[0].mode = 0;
	if (strcmpi(game_detail.port1.c_str(), "joy") == 0)
		prefs->jports[1].mode = 0;

	// MOUSE
	if (strcmpi(game_detail.port0.c_str(), "mouse") == 0)
		prefs->jports[0].mode = 2;
	if (strcmpi(game_detail.port1.c_str(), "mouse") == 0)
		prefs->jports[1].mode = 2;

	// WHAT IS THE MAIN CONTROL?
	// PORT 0 - MOUSE GAMES
	std::string line_string;
	if (strcmpi(game_detail.control.c_str(), "mouse") == 0 && strcmpi(amiberry_options.default_mouse1, "") != 0)
	{
		line_string = "joyport0=";
		line_string.append(amiberry_options.default_mouse1);
		parse_cfg_line(prefs, line_string);
	}
	// PORT 0 - JOYSTICK GAMES
	else if (strcmpi(amiberry_options.default_controller2, "") != 0)
	{
		line_string = "joyport0=";
		line_string.append(amiberry_options.default_controller2);
		parse_cfg_line(prefs, line_string);
	}
	else
	{
		line_string = "joyport0=mouse";
		parse_cfg_line(prefs, line_string);
	}

	// PORT 1 - MOUSE GAMES
	if (strcmpi(game_detail.control.c_str(), "mouse") == 0 && strcmpi(amiberry_options.default_mouse2, "") != 0)
	{
		line_string = "joyport1=";
		line_string.append(amiberry_options.default_mouse2);
		parse_cfg_line(prefs, line_string);
	}
	// PORT 1 - JOYSTICK GAMES
	else if (strcmpi(amiberry_options.default_controller1, "") != 0)
	{
		line_string = "joyport1=";
		line_string.append(amiberry_options.default_controller1);
		parse_cfg_line(prefs, line_string);
	}
	else
	{
		line_string = "joyport1=joy1";
		parse_cfg_line(prefs, line_string);
	}

	// PARALLEL PORT GAMES
	if (strcmpi(amiberry_options.default_controller3, "") != 0)
	{
		line_string = "joyport2=";
		line_string.append(amiberry_options.default_controller3);
		parse_cfg_line(prefs, line_string);
	}
	if (strcmpi(amiberry_options.default_controller4, "") != 0)
	{
		line_string = "joyport3=";
		line_string.append(amiberry_options.default_controller4);
		parse_cfg_line(prefs, line_string);
	}
}

void set_gfx_settings(uae_prefs* prefs, const game_hardware_options& game_detail)
{
	std::string line_string;
	// SCREEN AUTO-HEIGHT
	if (strcmpi(game_detail.scr_autoheight.c_str(), "true") == 0)
	{
		prefs->gfx_auto_crop = true;
	}
	else if (strcmpi(game_detail.scr_autoheight.c_str(), "false") == 0)
	{
		prefs->gfx_auto_crop = false;
	}

	// SCREEN CENTER/HEIGHT/WIDTH
	if (strcmpi(game_detail.scr_centerh.c_str(), "smart") == 0)
	{
		if (prefs->gfx_auto_crop)
		{
			// Disable if using Auto-Crop, otherwise the output won't be correct
			prefs->gfx_xcenter = 0;
		}
		else
		{
			prefs->gfx_xcenter = 2;
		}
	}
	else if (strcmpi(game_detail.scr_centerh.c_str(), "none") == 0)
	{
		prefs->gfx_xcenter = 0;
	}

	if (strcmpi(game_detail.scr_centerv.c_str(), "smart") == 0)
	{
		if (prefs->gfx_auto_crop)
		{
			// Disable if using Auto-Crop, otherwise the output won't be correct
			prefs->gfx_ycenter = 0;
		}
		else
		{
			prefs->gfx_ycenter = 2;
		}
	}
	else if (strcmpi(game_detail.scr_centerv.c_str(), "none") == 0)
	{
		prefs->gfx_ycenter = 0;
	}

	if (strcmpi(game_detail.scr_height.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_manual_crop = true;
			prefs->gfx_manual_crop_height = safe_stoi(game_detail.scr_height);
			prefs->gfx_vertical_offset = ((AMIGA_HEIGHT_MAX << prefs->gfx_vresolution) - prefs->gfx_manual_crop_height) / 2;
		}
	}

	if (strcmpi(game_detail.scr_width.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_manual_crop = true;
			prefs->gfx_manual_crop_width = safe_stoi(game_detail.scr_width);
			prefs->gfx_horizontal_offset = ((AMIGA_WIDTH_MAX << prefs->gfx_resolution) - prefs->gfx_manual_crop_width) / 2;
		}
	}

	if (strcmpi(game_detail.scr_offseth.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_horizontal_offset = safe_stoi(game_detail.scr_offseth);
		}
	}

	if (strcmpi(game_detail.scr_offsetv.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_vertical_offset = safe_stoi(game_detail.scr_offsetv);
		}
	}
}

void set_compatibility_settings(uae_prefs* prefs, const game_hardware_options& game_detail, const bool a600_available, const bool use_aga)
{
	std::string line_string;
	// CPU 68020/040 or no A600 ROM available
	if (strcmpi(game_detail.cpu.c_str(), "68020") == 0 || strcmpi(game_detail.cpu.c_str(), "68040") == 0 || use_aga)
	{
		line_string = "cpu_type=";
		line_string.append(use_aga ? "68020" : game_detail.cpu);
		parse_cfg_line(prefs, line_string);
	}

	// CPU 68000/010 (requires a600 rom)
	else if ((strcmpi(game_detail.cpu.c_str(), "68000") == 0 || strcmpi(game_detail.cpu.c_str(), "68010") == 0) && a600_available)
	{
		line_string = "cpu_type=";
		line_string.append(game_detail.cpu);
		parse_cfg_line(prefs, line_string);

		line_string = "chipmem_size=4";
		parse_cfg_line(prefs, line_string);
	}

	// Invalid or no CPU value specified, but A600 ROM is available? Use 68000
	else if (a600_available)
	{
		write_log("Invalid or no CPU value, A600 ROM available, using CPU: 68000\n");
		line_string = "cpu_type=68000";
		parse_cfg_line(prefs, line_string);
	}
	// Fallback for any invalid values - 68020 CPU
	else
	{
		write_log("Invalid or no CPU value, A600 ROM NOT found, falling back to CPU: 68020\n");
		line_string = "cpu_type=68020";
		parse_cfg_line(prefs, line_string);
	}

	// CPU SPEED
	if (strcmpi(game_detail.clock.c_str(), "7") == 0)
	{
		line_string = "cpu_multiplier=2";
		parse_cfg_line(prefs, line_string);
	}
	else if (strcmpi(game_detail.clock.c_str(), "14") == 0)
	{
		line_string = "cpu_multiplier=4";
		parse_cfg_line(prefs, line_string);
	}
	else if (strcmpi(game_detail.clock.c_str(), "28") == 0 || strcmpi(game_detail.clock.c_str(), "25") == 0)
	{
		line_string = "cpu_multiplier=8";
		parse_cfg_line(prefs, line_string);
	}
	else if (strcmpi(game_detail.clock.c_str(), "max") == 0)
	{
		line_string = "cpu_speed=max";
		parse_cfg_line(prefs, line_string);
	}

	// COMPATIBLE CPU
	if (strcmpi(game_detail.cpu_comp.c_str(), "true") == 0)
	{
		line_string = "cpu_compatible=true";
		parse_cfg_line(prefs, line_string);
	}
	else if (strcmpi(game_detail.cpu_comp.c_str(), "false") == 0)
	{
		line_string = "cpu_compatible=false";
		parse_cfg_line(prefs, line_string);
	}

	// COMPATIBLE CPU
	if (strcmpi(game_detail.cpu_24bit.c_str(), "false") == 0 || strcmpi(game_detail.z3.c_str(), "nul") != 0)
	{
		line_string = "cpu_24bit_addressing=false";
		parse_cfg_line(prefs, line_string);
	}

	// CYCLE-EXACT CPU
	if (strcmpi(game_detail.cpu_exact.c_str(), "true") == 0)
	{
		line_string = "cpu_cycle_exact=true";
		parse_cfg_line(prefs, line_string);
	}

	// FAST / Z3 MEMORY REQUIREMENTS
	if (strcmpi(game_detail.fast.c_str(), "nul") != 0)
	{
		line_string = "fastmem_size=";
		line_string.append(game_detail.fast);
		parse_cfg_line(prefs, line_string);
	}
	if (strcmpi(game_detail.z3.c_str(), "nul") != 0)
	{
		line_string = "z3mem_size=";
		line_string.append(game_detail.z3);
		parse_cfg_line(prefs, line_string);
	}

	// BLITTER=IMMEDIATE/WAIT/NORMAL
	if (strcmpi(game_detail.blitter.c_str(), "immediate") == 0)
	{
		line_string = "immediate_blits=true";
		parse_cfg_line(prefs, line_string);
	}
	else if (strcmpi(game_detail.blitter.c_str(), "normal") == 0)
	{
		line_string = "waiting_blits=automatic";
		parse_cfg_line(prefs, line_string);
	}

	// JIT
	if (strcmpi(game_detail.jit.c_str(), "true") == 0)
	{
		line_string = "cachesize=16384";
		parse_cfg_line(prefs, line_string);

		line_string = "cpu_compatible=false";
		parse_cfg_line(prefs, line_string);

		line_string = "cpu_cycle_exact=false";
		parse_cfg_line(prefs, line_string);

		line_string = "cpu_memory_cycle_exact=false";
		parse_cfg_line(prefs, line_string);

		line_string = "address_space_24=false";
		parse_cfg_line(prefs, line_string);
	}

	// NTSC
	if (strcmpi(game_detail.ntsc.c_str(), "true") == 0)
	{
		line_string = "ntsc=true";
		parse_cfg_line(prefs, line_string);
	}

	// SPRITE COLLISION
	if (strcmpi(game_detail.sprites.c_str(), "nul") != 0)
	{
		line_string = "collision_level=";
		line_string.append(game_detail.sprites);
		parse_cfg_line(prefs, line_string);
	}

	// Screen settings, only if allowed to override the defaults from amiberry.conf
	if (amiberry_options.allow_display_settings_from_xml)
	{
		set_gfx_settings(prefs, game_detail);
	}
}

void parse_slave_custom_fields(whdload_slave& slave, const std::string& custom)
{
	std::istringstream stream(custom);
	std::string line;

	while (std::getline(stream, line)) {
		if (line.find("C1") != std::string::npos || line.find("C2") != std::string::npos ||
			line.find("C3") != std::string::npos || line.find("C4") != std::string::npos ||
			line.find("C5") != std::string::npos) {

			std::istringstream lineStream(line);
			std::string segment;
			std::vector<std::string> seglist;

			while (std::getline(lineStream, segment, ':')) {
				segment.erase(std::remove(segment.begin(), segment.end(), '\t'), segment.end());
				seglist.push_back(segment);
			}

			if (seglist.size() < 2)
				continue;

			for (int ci = 1; ci <= 5; ++ci)
			{
				if (seglist[0] == "C" + std::to_string(ci))
				{
					auto& custom_field = slave.get_custom(ci);
					if (seglist[1] == "B")
					{
						if (seglist.size() < 3)
							break;
						custom_field.type = bool_type;
						custom_field.caption = seglist[2];
						custom_field.value = 0;
					}
					else if (seglist[1] == "X")
					{
						if (seglist.size() < 4)
							break;
						custom_field.type = bit_type;
						custom_field.value = 0;
						custom_field.label_bit_pairs.insert(custom_field.label_bit_pairs.end(), { seglist[2], safe_stoi(seglist[3]) });
					}
					else if (seglist[1] == "L")
					{
						if (seglist.size() < 4)
							break;
						custom_field.type = list_type;
						custom_field.caption = seglist[2];
						custom_field.value = 0;
						std::string token;
						std::istringstream token_stream(seglist[3]);
						while (std::getline(token_stream, token, ',')) {
							custom_field.labels.push_back(token);
						}
					}
					break;
				}
			}
		}
	}
}

game_hardware_options parse_settings_from_xml(uae_prefs* prefs, const char* filepath)
{
	tinyxml2::XMLDocument doc;
	write_log(_T("WHDBooter - Searching whdload_db.xml for %s\n"), whdload_prefs.filename.c_str());

	FILE* f = fopen(whd_config.c_str(), _T("rb"));
	if (!f)
	{
		write_log(_T("Failed to open '%s'\n"), whd_config.c_str());
		return {};
	}

	tinyxml2::XMLError err = doc.LoadFile(f);
	fclose(f);
	if (err != tinyxml2::XML_SUCCESS)
	{
		write_log(_T("Failed to parse '%s':  %d\n"), whd_config.c_str(), err);
		return {};
	}

	game_hardware_options game_detail{};
	auto sha1 = my_get_sha1_of_file(filepath);
	std::transform(sha1.begin(), sha1.end(), sha1.begin(),
		[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

	auto* root_element = doc.FirstChildElement("whdbooter");
	if (!root_element)
	{
		write_log(_T("WHDBooter - whdload_db.xml has no <whdbooter> root element\n"));
		return {};
	}
	tinyxml2::XMLElement* game_node = root_element->FirstChildElement("game");
	while (game_node != nullptr)
	{
		// Ideally we'd just match by sha1, but filename has worked up until now, so try that first
		// then fall back to sha1 if a user has renamed the file!
		//
		if (game_node->Attribute("filename", whdload_prefs.filename.c_str()) || 
			game_node->Attribute("sha1", sha1.c_str()))
		{
			auto xml_element = game_node->FirstChildElement("name");
			if (xml_element && xml_element->GetText())
			{
				whdload_prefs.game_name.assign(xml_element->GetText());
			}

			xml_element = game_node->FirstChildElement("subpath");
			if (xml_element && xml_element->GetText())
			{
				whdload_prefs.sub_path.assign(xml_element->GetText());
			}

			xml_element = game_node->FirstChildElement("variant_uuid");
			if (xml_element && xml_element->GetText())
			{
				whdload_prefs.variant_uuid.assign(xml_element->GetText());
			}

			xml_element = game_node->FirstChildElement("slave_count");
			if (xml_element)
			{
				whdload_prefs.slave_count = xml_element->IntText(0);
			}

			xml_element = game_node->FirstChildElement("slave_default");
			if (xml_element && xml_element->GetText())
			{
				whdload_prefs.slave_default.assign(xml_element->GetText());
				write_log("WHDBooter - Selected Slave: %s \n", whdload_prefs.slave_default.c_str());
			}

			xml_element = game_node->FirstChildElement("slave_libraries");
			if (xml_element && xml_element->GetText() != nullptr)
			{
				if (strcmpi(xml_element->GetText(), "true") == 0)
					whdload_prefs.slave_libraries = true;
			}

			xml_element = game_node->FirstChildElement("slave");
			whdload_prefs.slaves.clear();

			for (int i = 0; i < whdload_prefs.slave_count && xml_element; ++i)
			{
				whdload_slave slave;
				const char* slave_text = nullptr;

				auto* filename_elem = xml_element->FirstChildElement("filename");
				if (filename_elem && (slave_text = filename_elem->GetText()))
					slave.filename.assign(slave_text);

				auto* datapath_elem = xml_element->FirstChildElement("datapath");
				if (datapath_elem && (slave_text = datapath_elem->GetText()))
					slave.data_path.assign(slave_text);

				auto customElement = xml_element->FirstChildElement("custom");
				if (customElement && ((slave_text = customElement->GetText())))
				{
					auto custom = std::string(slave_text);
					parse_slave_custom_fields(slave, custom);
				}

				whdload_prefs.slaves.emplace_back(slave);

				// Set the default slave as the selected one
				if (slave.filename == whdload_prefs.slave_default)
					whdload_prefs.selected_slave = slave;

				xml_element = xml_element->NextSiblingElement("slave");
			}

			xml_element = game_node->FirstChildElement("hardware");
			if (xml_element && xml_element->GetText())
			{
				std::string hardware(xml_element->GetText());
				if (!hardware.empty())
				{
					game_detail = get_game_hardware_settings(hardware);
					write_log("WHDBooter - Game H/W Settings: \n%s\n", hardware.c_str());
				}
			}

			xml_element = game_node->FirstChildElement("custom_controls");
			if (xml_element && xml_element->GetText())
			{
				std::string custom_settings(xml_element->GetText());
				if (!custom_settings.empty())
				{
					parse_custom_settings(prefs, custom_settings);
					write_log("WHDBooter - Game Custom Settings: \n%s\n", custom_settings.c_str());
				}
			}

			break;
		}
		game_node = game_node->NextSiblingElement();
	}

	return game_detail;
}

static void detect_slave_hardware_from_header(const char* filepath,
	const std::string& subpath, const std::string& slave_filename,
	game_hardware_options& game_detail)
{
	std::string slave_path(filepath);
	slave_path += "/";
	if (!subpath.empty())
		slave_path += subpath + "/";
	slave_path += slave_filename;

	struct zfile* sf = zfile_fopen(slave_path.c_str(), _T("rb"), ZFD_ARCHIVE);
	if (!sf)
		return;

	uae_u8 buf[256];
	auto bytes_read = zfile_fread(buf, 1, sizeof(buf), sf);
	zfile_fclose(sf);

	if (bytes_read < 14)
		return;

	static const uae_u8 magic[] = { 'W','H','D','L','O','A','D','S' };
	for (size_t i = 0; i + 12 <= bytes_read; i++)
	{
		if (memcmp(buf + i, magic, 8) != 0)
			continue;

		uae_u16 ws_version = (buf[i + 8] << 8) | buf[i + 9];
		uae_u16 ws_flags = (buf[i + 10] << 8) | buf[i + 11];

		write_log("WHDBooter - Slave header: version=%d, flags=0x%04x\n",
			ws_version, ws_flags);

		constexpr uae_u16 WHDLF_Req68020 = (1 << 4);
		constexpr uae_u16 WHDLF_ReqAGA   = (1 << 5);

		if (ws_flags & WHDLF_ReqAGA)
			game_detail.chipset = "AGA";
		if (ws_flags & WHDLF_Req68020)
			game_detail.cpu = "68020";

		return;
	}
}

static bool auto_detect_slave_from_archive(const char* filepath,
	game_hardware_options& game_detail)
{
	struct zvolume* zv = zfile_fopen_archive(filepath);
	if (!zv)
	{
		write_log("WHDBooter - Auto-detect: failed to open archive %s\n", filepath);
		return false;
	}

	struct slave_entry {
		std::string filename;
		std::string subpath;
	};
	std::vector<slave_entry> found_slaves;

	std::function<void(struct znode*, const std::string&)> scan_nodes;
	scan_nodes = [&](struct znode* node, const std::string& current_path) {
		while (node)
		{
			std::string name(node->name ? node->name : "");

			if (node->type == ZNODE_FILE && name.length() > 6)
			{
				std::string lower_name = name;
				std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
					[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
				if (lower_name.length() >= 6 &&
					lower_name.compare(lower_name.length() - 6, 6, ".slave") == 0)
				{
					found_slaves.push_back({name, current_path});
				}
			}

			if (node->child)
			{
				std::string child_path = current_path.empty() ? name : current_path + "/" + name;
				scan_nodes(node->child, child_path);
			}

			node = node->sibling;
		}
	};

	scan_nodes(zv->root.child, "");
	zfile_fclose_archive(zv);

	if (found_slaves.empty())
	{
		write_log("WHDBooter - Auto-detect: no .slave files found in archive\n");
		return false;
	}

	auto to_lower = [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); };

	std::string filename_lower = whdload_prefs.filename;
	std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), to_lower);

	const slave_entry* best = &found_slaves[0];
	for (const auto& s : found_slaves)
	{
		std::string slave_lower = s.filename;
		std::transform(slave_lower.begin(), slave_lower.end(), slave_lower.begin(), to_lower);
		std::string slave_base = slave_lower.substr(0, slave_lower.length() - 6);
		if (slave_base == filename_lower)
		{
			best = &s;
			break;
		}
	}

	whdload_prefs.sub_path = best->subpath;
	whdload_prefs.slave_default = best->filename;
	whdload_prefs.selected_slave = whdload_slave{};
	whdload_prefs.selected_slave.filename = best->filename;
	whdload_prefs.slave_count = static_cast<int>(found_slaves.size());

	whdload_prefs.slaves.clear();
	for (const auto& s : found_slaves)
	{
		whdload_slave slave;
		slave.filename = s.filename;
		whdload_prefs.slaves.emplace_back(slave);
	}

	detect_slave_hardware_from_header(filepath, best->subpath, best->filename, game_detail);

	write_log("WHDBooter - Auto-detected slave: %s (subpath: %s, total slaves found: %d)\n",
		best->filename.c_str(), best->subpath.c_str(), static_cast<int>(found_slaves.size()));

	return true;
}

void create_startup_sequence()
{
	std::ostringstream whd_bootscript;
	whd_bootscript << "FAILAT 999\n";

	if (whdload_prefs.slave_libraries)
	{
		whd_bootscript << "DH3:C/Assign LIBS: DH3:LIBS/ ADD\n";
	}
	if (amiberry_options.use_jst_instead_of_whd)
		whd_bootscript << "IF NOT EXISTS JST\n";
	else
		whd_bootscript << "IF NOT EXISTS WHDLoad\n";

	whd_bootscript << "DH3:C/Assign C: DH3:C/ ADD\n";
	whd_bootscript << "ENDIF\n";

	whd_bootscript << "CD \"Games:" << whdload_prefs.sub_path << "\"\n";
	const std::string slave_path = whdload_prefs.sub_path.empty()
		? whdload_prefs.selected_slave.filename
		: whdload_prefs.sub_path + "/" + whdload_prefs.selected_slave.filename;
	if (amiberry_options.use_jst_instead_of_whd)
		whd_bootscript << "JST SLAVE=\"Games:" << slave_path << "\"";
	else
		whd_bootscript << "WHDLoad SLAVE=\"Games:" << slave_path << "\"";

	// Write Cache
	if (amiberry_options.use_jst_instead_of_whd)
		whd_bootscript << " PRELOAD ";
	else
		whd_bootscript << " PRELOAD NOREQ";
	if (!whdload_prefs.write_cache)
	{
		if (amiberry_options.use_jst_instead_of_whd)
			whd_bootscript << " NOCACHE";
		else
			whd_bootscript << " NOWRITECACHE";
	}

	// CUSTOM options
	for (int i = 1; i <= 5; ++i) {
		auto& custom = whdload_prefs.selected_slave.get_custom(i);
		if (custom.value != 0) {
			whd_bootscript << " CUSTOM" << i << "=" << custom.value;
		}
	}
	if (!whdload_prefs.custom.empty())
	{
		whd_bootscript << " CUSTOM=\"" << whdload_prefs.custom << "\"";
	}

	// BUTTONWAIT
	if (whdload_prefs.button_wait)
	{
		whd_bootscript << " BUTTONWAIT";
	}

	// SPLASH
	if (!whdload_prefs.show_splash)
	{
		whd_bootscript << " SPLASHDELAY=0";
	}

	// CONFIGDELAY
	if (whdload_prefs.config_delay != 0)
	{
		whd_bootscript << " CONFIGDELAY=" << whdload_prefs.config_delay;
	}

	// SPECIAL SAVE PATH
	whd_bootscript << " SAVEPATH=Saves:Savegames/ SAVEDIR=\"" << whdload_prefs.sub_path << "\"";

	// DATA PATH
	if (!whdload_prefs.selected_slave.data_path.empty())
		whd_bootscript << " DATA=\"" << whdload_prefs.selected_slave.data_path << "\"";

	whd_bootscript << '\n';

	// Launches utility program to quit the emulator (via a UAE trap in RTAREA)
	if (whdload_prefs.quit_on_exit)
	{
		whd_bootscript << "DH0:C/AmiQuit\n";
	}

	write_log("WHDBooter - Created Startup-Sequence  \n\n%s\n", whd_bootscript.str().c_str());
	write_log("WHDBooter - Saved Auto-Startup to %s\n", whd_startup.c_str());

	std::ofstream myfile(whd_startup, std::ios::binary);
	if (myfile.is_open())
	{
		myfile << whd_bootscript.str();
		myfile.close();
	}
}

bool is_a600_available(uae_prefs* prefs)
{
	int roms[3] = { 10, 9, -1 }; // kickstart 2.05 A600HD
	const auto rom_test = configure_rom(prefs, roms, 0);
	return rom_test == 1;
}

void set_booter_drives(uae_prefs* prefs, const char* filepath)
{
	std::string tmp;

	boot_path = get_whdboot_temp_path();

	tmp = "filesystem2=rw,DH0:DH0:" + boot_path.string() + ",10";
	TCHAR* parsed = parse_text(tmp.c_str());
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	tmp = "uaehf0=dir,rw,DH0:DH0::" + boot_path.string() + ",10";
	parsed = parse_text(tmp.c_str());
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	boot_path = whdbooter_path / "boot-data.zip";
	if (!std::filesystem::exists(boot_path))
		boot_path = whdbooter_path / "boot-data";

	tmp = "filesystem2=rw,DH3:DH3:" + boot_path.string() + ",-10";
	parsed = parse_text(tmp.c_str());
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	tmp = "uaehf3=dir,rw,DH3:DH3::" + boot_path.string() + ",-10";
	parsed = parse_text(tmp.c_str());
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	tmp = "filesystem2=rw,DH1:Games:\"" + std::string(filepath) + "\",0";
	parsed = parse_text(tmp.c_str());
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	tmp = "uaehf1=dir,rw,DH1:Games:\"" + std::string(filepath) + "\",0";
	parsed = parse_text(tmp.c_str());
	cfgfile_parse_line(prefs, parsed, 0);
	xfree(parsed);

	if (std::filesystem::exists(save_path))
	{
		tmp = "filesystem2=rw,DH2:Saves:" + save_path.string() + ",0";
		parsed = parse_text(tmp.c_str());
		cfgfile_parse_line(prefs, parsed, 0);
		xfree(parsed);

		tmp = "uaehf2=dir,rw,DH2:Saves:" + save_path.string() + ",0";
		parsed = parse_text(tmp.c_str());
		cfgfile_parse_line(prefs, parsed, 0);
		xfree(parsed);
	}
}

void whdload_auto_prefs(uae_prefs* prefs, const char* filepath)
{
	config_loaded = false;

	whdload_prefs.game_name.clear();
	whdload_prefs.sub_path.clear();
	whdload_prefs.variant_uuid.clear();
	whdload_prefs.slave_count = 0;
	whdload_prefs.slave_default.clear();
	whdload_prefs.slave_libraries = false;
	whdload_prefs.slaves.clear();
	whdload_prefs.selected_slave = whdload_slave{};

	write_log("WHDBooter Launched\n");
	if (amiberry_options.use_jst_instead_of_whd)
		write_log("WHDBooter - Using JST instead of WHDLoad\n");

	conf_path = get_configuration_path();
	whdbooter_path = get_whdbootpath();
	save_path = get_savedatapath(false);

	symlink_roms(prefs);

	// this allows A600HD to be used to slow games down
	const auto a600_available = is_a600_available(prefs);
	if (a600_available)
	{
		write_log("WHDBooter - Host: A600 ROM available, will use it for non-AGA titles\n");
	}
	else
	{
		write_log("WHDBooter - Host: A600 ROM not found, falling back to A1200 config for all titles\n");
	}

	// REMOVE THE FILE PATH AND EXTENSION
	const auto* filename = my_getfilepart(filepath);
	const std::string filename_no_extension = get_game_filename(filepath);
	whdload_prefs.filename = filename_no_extension;

	// setup for tmp folder.
	std::filesystem::path temp_base = get_whdboot_temp_path();
	// Clean up any stale files from previous runs to ensure a fresh tmp
	// This helps avoid stale/broken symlinks causing WHDLoad not found
	try {
		if (std::filesystem::exists(temp_base)) {
			write_log("WHDBooter - Cleaning existing tmp directory %s\n", temp_base.string().c_str());
			std::filesystem::remove_all(temp_base);
		}
	}
	catch (std::filesystem::filesystem_error &e) {
		write_log("WHDBooter - Failed to clean tmp directory %s: %s\n", temp_base.string().c_str(), e.what());
	}

	try {
		std::filesystem::create_directories(temp_base / "s");
		std::filesystem::create_directories(temp_base / "c");
		std::filesystem::create_directories(temp_base / "devs");
	}
	catch (std::filesystem::filesystem_error& e) {
		write_log("WHDBooter - Failed to create tmp directories: %s\n", e.what());
	}
	whd_startup = (temp_base / "s" / "startup-sequence").string();
	try {
		std::filesystem::remove(whd_startup);
	}
	catch (std::filesystem::filesystem_error& e) {
		write_log("WHDBooter - Failed to remove old startup-sequence: %s\n", e.what());
	}

	// are we using save-data/ ?
	kickstart_path = std::filesystem::path(get_savedatapath(true)) / "Kickstarts";

	// LOAD GAME SPECIFICS
	whd_path = whdbooter_path / "game-data";
	game_hardware_options game_detail;
	whd_config = (whd_path / "whdload_db.xml").string();

	if (std::filesystem::exists(whd_config))
	{
		game_detail = parse_settings_from_xml(prefs, filepath);
	}
	else
	{
		write_log("WHDBooter - Could not load whdload_db.xml - does not exist?\n");
	}

	if (whdload_prefs.selected_slave.filename.empty())
	{
		write_log("WHDBooter - No XML match found, scanning archive for .slave files\n");
		auto_detect_slave_from_archive(filepath, game_detail);
	}

	build_uae_config_filename(whdload_prefs.filename);
	if (std::filesystem::exists(uae_config))
	{
		write_log("WHDBooter - %s found. Loading Config for WHDLoad options.\n", uae_config.c_str());
		target_cfgfile_load(prefs, uae_config.c_str(), CONFIG_TYPE_DEFAULT, 0);
		config_loaded = true;
	}

	if (!whdload_prefs.selected_slave.filename.empty())
	{
		create_startup_sequence();
	}
	else
	{
		write_log("WHDBooter - WARNING: No slave file found (not in XML database and no .slave in archive)\n");
		write_log("WHDBooter - The game may not boot correctly\n");
	}

	if (std::filesystem::exists(whd_startup))
	{
		if (amiberry_options.use_jst_instead_of_whd)
		{
			create_link_or_copy(whdbooter_path / "JST", temp_base / "c" / "JST", "JST");
		}
		else
		{
			create_link_or_copy(whdbooter_path / "WHDLoad", temp_base / "c" / "WHDLoad", "WHDLoad");
		}

		create_link_or_copy(whdbooter_path / "AmiQuit", temp_base / "c" / "AmiQuit", "AmiQuit");
		create_link_or_copy(kickstart_path, temp_base / "devs" / "Kickstarts", "Kickstarts");
	}
#if DEBUG
	// debugging code!
	write_log("WHDBooter - Game: Port 0     : %s  \n", game_detail.port0.c_str());
	write_log("WHDBooter - Game: Port 1     : %s  \n", game_detail.port1.c_str());
	write_log("WHDBooter - Game: Control    : %s  \n", game_detail.control.c_str());
	write_log("WHDBooter - Game: CPU        : %s  \n", game_detail.cpu.c_str());
	write_log("WHDBooter - Game: Blitter    : %s  \n", game_detail.blitter.c_str());
	write_log("WHDBooter - Game: CPU Clock  : %s  \n", game_detail.clock.c_str());
	write_log("WHDBooter - Game: Chipset    : %s  \n", game_detail.chipset.c_str());
	write_log("WHDBooter - Game: JIT        : %s  \n", game_detail.jit.c_str());
	write_log("WHDBooter - Game: CPU Compat : %s  \n", game_detail.cpu_comp.c_str());
	write_log("WHDBooter - Game: Sprite Col : %s  \n", game_detail.sprites.c_str());
	write_log("WHDBooter - Game: Scr Height : %s  \n", game_detail.scr_height.c_str());
	write_log("WHDBooter - Game: Scr Width  : %s  \n", game_detail.scr_width.c_str());
	write_log("WHDBooter - Game: Scr AutoHgt: %s  \n", game_detail.scr_autoheight.c_str());
	write_log("WHDBooter - Game: Scr CentrH : %s  \n", game_detail.scr_centerh.c_str());
	write_log("WHDBooter - Game: Scr CentrV : %s  \n", game_detail.scr_centerv.c_str());
	write_log("WHDBooter - Game: Scr OffsetH: %s  \n", game_detail.scr_offseth.c_str());
	write_log("WHDBooter - Game: Scr OffsetV: %s  \n", game_detail.scr_offsetv.c_str());
	write_log("WHDBooter - Game: NTSC       : %s  \n", game_detail.ntsc.c_str());
	write_log("WHDBooter - Game: Fast Ram   : %s  \n", game_detail.fast.c_str());
	write_log("WHDBooter - Game: Z3 Ram     : %s  \n", game_detail.z3.c_str());
	write_log("WHDBooter - Game: CPU Exact  : %s  \n", game_detail.cpu_exact.c_str());

	write_log("WHDBooter - Host: Controller 1   : %s  \n", amiberry_options.default_controller1);
	write_log("WHDBooter - Host: Controller 2   : %s  \n", amiberry_options.default_controller2);
	write_log("WHDBooter - Host: Controller 3   : %s  \n", amiberry_options.default_controller3);
	write_log("WHDBooter - Host: Controller 4   : %s  \n", amiberry_options.default_controller4);
	write_log("WHDBooter - Host: Mouse 1        : %s  \n", amiberry_options.default_mouse1);
	write_log("WHDBooter - Host: Mouse 2        : %s  \n", amiberry_options.default_mouse2);
#endif

	// if we already loaded a .uae config, we don't need to do the below manual setup for hardware
	if (config_loaded)
	{
		write_log("WHDBooter - %s found; ignoring WHD Quickstart setup.\n", uae_config.c_str());
		return;
	}

	//    *** EMULATED HARDWARE ***
	//    SET UNIVERSAL DEFAULTS
	prefs->start_gui = false;

	// DO CHECKS FOR AGA / CD32
	const auto is_aga = _tcsstr(filename, "AGA") != nullptr || strcmpi(game_detail.chipset.c_str(), "AGA") == 0;
	const auto is_cd32 = _tcsstr(filename, "CD32") != nullptr || strcmpi(game_detail.chipset.c_str(), "CD32") == 0;
	const auto is_mt32 = _tcsstr(filename, _T("MT32")) != nullptr || _tcsstr(filename, _T("mt32")) != nullptr;

	const auto chipset_unknown = strcmpi(game_detail.chipset.c_str(), "nul") == 0;

	if (is_aga || is_cd32 || !a600_available || chipset_unknown)
	{
		write_log("WHDBooter - Host: A1200 ROM selected%s\n",
			chipset_unknown ? " (no XML hardware info, using A1200 as default)" : "");
		built_in_prefs(prefs, 4, A1200_CONFIG, 0, 0);
		prefs->fastmem[0].size = 0x800000;
		_tcscpy(prefs->description, _T("AutoBoot Configuration [WHDLoad] [AGA]"));
	}
	else
	{
		write_log("WHDBooter - Host: A600 ROM selected\n");
		built_in_prefs(prefs, 2, A600_CONFIG, 0, 0);
		_tcscpy(prefs->description, _T("AutoBoot Configuration [WHDLoad]"));
	}

	if (is_mt32)
	{
		// Check if we have the MT32 ROMs
		auto mt32_available = midi_emu_available(_T("MT-32"));
		auto cm32_available = midi_emu_available(_T("CM-32L"));
		if (!mt32_available && !cm32_available)
		{
			write_log("MT32/CM32L MIDI Emulation not available (ROMs missing)\n");
		}
		else
		{
			// Enable MIDI output
			_tcscpy(prefs->midioutdev, mt32_available ? "Munt MT-32" : "Munt CM-32L");
		}
	}

	// SET THE WHD BOOTER AND GAME DATA
	write_log("WHDBooter - Host: setting up drives\n");
	set_booter_drives(prefs, filepath);

	// APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
	write_log("WHDBooter - Host: setting up controllers\n");
	set_input_settings(prefs, game_detail, is_cd32);

	//  SET THE GAME COMPATIBILITY SETTINGS
	// BLITTER, SPRITES, MEMORY, JIT, BIG CPU ETC
	write_log("WHDBooter - Host: setting up game compatibility settings\n");
	set_compatibility_settings(prefs, game_detail, a600_available, is_aga || is_cd32);

	write_log("WHDBooter - Host: settings applied\n\n");
}
