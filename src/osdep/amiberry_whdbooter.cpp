/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */
#include <algorithm>
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
#include <fstream>

extern void SetLastActiveConfig(const char* filename);
extern std::string current_dir;
extern char last_loaded_config[MAX_DPATH];

// Use configs with 8MB Fast RAM, to make it likely
// that WHDLoad preload will cache everything.
enum
{
	A600_CONFIG = 3, // 8MB fast ram
	A1200_CONFIG = 2 // 8MB fast ram
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

std::string whdbooter_path;
TCHAR boot_path[MAX_DPATH];
TCHAR save_path[MAX_DPATH];
TCHAR conf_path[MAX_DPATH];
TCHAR whd_path[MAX_DPATH];
TCHAR kick_path[MAX_DPATH];

TCHAR uae_config[255];
TCHAR whd_config[255];
TCHAR whd_startup[255];

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

void make_rom_symlink(const char* kick_short, int kick_numb, struct uae_prefs* p)
{
	char kick_long[MAX_DPATH];
	int roms[2] = { -1,-1 };

	// do the checks...
	_sntprintf(kick_long, MAX_DPATH, "%s/%s", kick_path, kick_short);

	// this should sort any broken links (only remove if a link, not a file. See vfat handling of link below)
	// Only remove file IF it is a symlink.
	// On VFAT USB stick, the roms are copied not symlinked, which takes a long time, also the user
	// may have provided their own rom files, so we do not want to remove and replace those.
	//
	if (my_existslink(kick_long)) {
		my_unlink(kick_long);
	}

	if (!my_existsfile2(kick_long))
	{
		roms[0] = kick_numb;
		const auto rom_test = configure_rom(p, roms, 0);
		if (rom_test == 1)
		{
			int r = symlink(p->romfile, kick_long);
			// VFAT filesystems do not support creation of symlinks.
			// Fallback to copying file if filesystem does not support the generation of symlinks
			if (r < 0 && errno == EPERM)
				r = copyfile(kick_long, p->romfile, true);
			write_log("Making SymLink for Kickstart ROM: %s  [%s]\n", kick_long, r < 0 ? "Fail" : "Ok");
		}
	}
}

void symlink_roms(struct uae_prefs* prefs)
{
	TCHAR tmp[MAX_DPATH];
	TCHAR tmp2[MAX_DPATH];

	write_log("SymLink Kickstart ROMs for Booter\n");

	// here we can do some checks for Kickstarts we might need to make symlinks for
	current_dir = start_path_data;

	// are we using save-data/ ?
	get_savedatapath(tmp, MAX_DPATH, 1);
	_sntprintf(kick_path, MAX_DPATH, _T("%s/Kickstarts"), tmp);

	if (!my_existsdir(kick_path)) {
		// otherwise, use the old route
		whdbooter_path = get_whdbootpath();
		_sntprintf(kick_path, MAX_DPATH, _T("%sgame-data/Devs/Kickstarts"), whdbooter_path.c_str());
	}
	write_log("WHDBoot - using kickstarts from %s\n", kick_path);

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
	get_rom_path(tmp2, MAX_DPATH);
	_sntprintf(tmp, MAX_DPATH, _T("%s/rom.key"), tmp2);

	// destination file (symlink)
	_sntprintf(tmp2, MAX_DPATH, _T("%s/rom.key"), kick_path);

	if (my_existsfile2(tmp)) {
		const int r = symlink(tmp, tmp2);
		if (r < 0 && errno == EPERM)
			copyfile(tmp2, tmp, true);
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
	_tcscpy(uae_config, conf_path);
	_tcscat(uae_config, game_name.c_str());
	_tcscat(uae_config, ".uae");
}

void cd_auto_prefs(uae_prefs* prefs, char* filepath)
{
	TCHAR tmp[MAX_DPATH];

	write_log("\nCD Autoload: %s  \n\n", filepath);

	get_configuration_path(conf_path, MAX_DPATH);
	whdload_prefs.filename = get_game_filename(filepath);

	// LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE
	//  CONFIG LOAD IF .UAE IS IN CONFIG PATH
	build_uae_config_filename(whdload_prefs.filename);

	if (my_existsfile2(uae_config))
	{
		target_cfgfile_load(prefs, uae_config, CONFIG_TYPE_ALL, 0);
		return;
	}

	prefs->start_gui = false;

	const auto is_cdtv = _tcsstr(filepath, _T("CDTV")) != nullptr || _tcsstr(filepath, _T("cdtv")) != nullptr;
	const auto is_cd32 = _tcsstr(filepath, _T("CD32")) != nullptr || _tcsstr(filepath, _T("cd32")) != nullptr;

	// CD32
	if (is_cd32)
	{
		_tcscpy(prefs->description, _T("AutoBoot Configuration [CD32]"));
		// SET THE BASE AMIGA (CD32)
		built_in_prefs(prefs, 8, 0, 0, 0);
	}
	else if (is_cdtv)
	{
		_tcscpy(prefs->description, _T("AutoBoot Configuration [CDTV]"));
		// SET THE BASE AMIGA (CDTV)
		built_in_prefs(prefs, 9, 0, 0, 0);
	}
	else
	{
		_tcscpy(prefs->description, _T("AutoBoot Configuration [A1200CD]"));
		// SET THE BASE AMIGA (Expanded A1200)
		built_in_prefs(prefs, 4, A1200_CONFIG, 0, 0);
	}

	// enable CD
	_sntprintf(tmp, MAX_DPATH, "cd32cd=1");
	cfgfile_parse_line(prefs, parse_text(tmp), 0);

	// mount the image
	_sntprintf(tmp, MAX_DPATH, "cdimage0=%s,image", filepath);
	cfgfile_parse_line(prefs, parse_text(tmp), 0);

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
			prefs->gfx_manual_crop_height = std::stoi(game_detail.scr_height);
			prefs->gfx_vertical_offset = (576 - prefs->gfx_manual_crop_height) / 2;
		}
	}

	if (strcmpi(game_detail.scr_width.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_manual_crop = true;
			prefs->gfx_manual_crop_width = std::stoi(game_detail.scr_width);
			prefs->gfx_horizontal_offset = (754 - prefs->gfx_manual_crop_width) / 2;
		}
	}

	if (strcmpi(game_detail.scr_offseth.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_horizontal_offset = std::stoi(game_detail.scr_offseth);
		}
	}

	if (strcmpi(game_detail.scr_offsetv.c_str(), "nul") != 0)
	{
		if (!prefs->gfx_auto_crop)
		{
			prefs->gfx_vertical_offset = std::stoi(game_detail.scr_offsetv);
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

	// CPU 68000/010 [requires a600 rom)]
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

			// Process seglist as needed
			if (seglist[0] == "C1")
			{
				if (seglist[1] == "B")
				{
					slave.custom1.type = bool_type;
					slave.custom1.caption = seglist[2];
					slave.custom1.value = 0;
				}
				else if (seglist[1] == "X")
				{
					slave.custom1.type = bit_type;
					slave.custom1.label_bit_pairs.insert(slave.custom1.label_bit_pairs.end(), { seglist[2], stoi(seglist[3]) });
				}
				else if (seglist[1] == "L")
				{
					slave.custom1.type = list_type;
					slave.custom1.caption = seglist[2];
					slave.custom1.value = 0;
					std::string token;
					std::istringstream token_stream(seglist[3]);
					while (std::getline(token_stream, token, ',')) {
						slave.custom1.labels.push_back(token);
					}
				}
			}
			else if (seglist[0] == "C2")
			{
				if (seglist[1] == "B")
				{
					slave.custom2.type = bool_type;
					slave.custom2.caption = seglist[2];
					slave.custom2.value = 0;
				}
				else if (seglist[1] == "X")
				{
					slave.custom2.type = bit_type;
					slave.custom2.label_bit_pairs.insert(slave.custom2.label_bit_pairs.end(), { seglist[2], stoi(seglist[3]) });
				}
				else if (seglist[1] == "L")
				{
					slave.custom2.type = list_type;
					slave.custom2.caption = seglist[2];
					slave.custom2.value = 0;
					std::string token;
					std::istringstream token_stream(seglist[3]);
					while (std::getline(token_stream, token, ',')) {
						slave.custom2.labels.push_back(token);
					}
				}
			}
			else if (seglist[0] == "C3")
			{
				if (seglist[1] == "B")
				{
					slave.custom3.type = bool_type;
					slave.custom3.caption = seglist[2];
					slave.custom3.value = 0;
				}
				else if (seglist[1] == "X")
				{
					slave.custom3.type = bit_type;
					slave.custom3.label_bit_pairs.insert(slave.custom3.label_bit_pairs.end(), { seglist[2], stoi(seglist[3]) });
				}
				else if (seglist[1] == "L")
				{
					slave.custom3.type = list_type;
					slave.custom3.caption = seglist[2];
					slave.custom3.value = 0;
					std::string token;
					std::istringstream token_stream(seglist[3]);
					while (std::getline(token_stream, token, ',')) {
						slave.custom3.labels.push_back(token);
					}
				}
			}
			else if (seglist[0] == "C4")
			{
				if (seglist[1] == "B")
				{
					slave.custom4.type = bool_type;
					slave.custom4.caption = seglist[2];
					slave.custom4.value = 0;
				}
				else if (seglist[1] == "X")
				{
					slave.custom4.type = bit_type;
					slave.custom4.label_bit_pairs.insert(slave.custom4.label_bit_pairs.end(), { seglist[2], stoi(seglist[3]) });
				}
				else if (seglist[1] == "L")
				{
					slave.custom4.type = list_type;
					slave.custom4.caption = seglist[2];
					slave.custom4.value = 0;
					std::string token;
					std::istringstream token_stream(seglist[3]);
					while (std::getline(token_stream, token, ',')) {
						slave.custom4.labels.push_back(token);
					}
				}
			}
			else if (seglist[0] == "C5")
			{
				if (seglist[1] == "B")
				{
					slave.custom5.type = bool_type;
					slave.custom5.caption = seglist[2];
					slave.custom5.value = 0;
				}
				else if (seglist[1] == "X")
				{
					slave.custom5.type = bit_type;
					slave.custom5.label_bit_pairs.insert(slave.custom5.label_bit_pairs.end(), { seglist[2], stoi(seglist[3]) });
				}
				else if (seglist[1] == "L")
				{
					slave.custom5.type = list_type;
					slave.custom5.caption = seglist[2];
					slave.custom5.value = 0;
					std::string token;
					std::istringstream token_stream(seglist[3]);
					while (std::getline(token_stream, token, ',')) {
						slave.custom5.labels.push_back(token);
					}
				}
			}
		}
	}
}

game_hardware_options parse_settings_from_xml(uae_prefs* prefs, const char* filepath)
{
	game_hardware_options game_detail{};
	tinyxml2::XMLDocument doc;
	auto error = false;
	write_log(_T("WHDBooter - Searching whdload_db.xml for %s\n"), whdload_prefs.filename.c_str());

	auto* f = fopen(whd_config, _T("rb"));
	if (f)
	{
		auto err = doc.LoadFile(f);
		if (err != tinyxml2::XML_SUCCESS)
		{
			write_log(_T("Failed to parse '%s':  %d\n"), whd_config, err);
			error = true;
		}
		fclose(f);
	}
	else
	{
		error = true;
	}

	if (!error)
	{
		auto sha1 = my_get_sha1_of_file(filepath);
		std::transform(sha1.begin(), sha1.end(), sha1.begin(), ::tolower);
		auto* game_node = doc.FirstChildElement("whdbooter");
		game_node = game_node->FirstChildElement("game");
		while (game_node != nullptr)
		{
			// Ideally we'd just match by sha1, but filename has worked up until now, so try that first
			// then fall back to sha1 if a user has renamed the file!
			//
			int found = 0;
			if (game_node->Attribute("filename", whdload_prefs.filename.c_str()) || 
				game_node->Attribute("sha1", sha1.c_str()))
			{
				found = 1;
			}

			if (found)
			{
				// Name
				auto xml_element = game_node->FirstChildElement("name");
				if (xml_element)
				{
					whdload_prefs.game_name.assign(xml_element->GetText());
				}

				// Sub Path
				xml_element = game_node->FirstChildElement("subpath");
				if (xml_element)
				{
					whdload_prefs.sub_path.assign(xml_element->GetText());
				}

				// Variant UUID
				xml_element = game_node->FirstChildElement("variant_uuid");
				if (xml_element)
				{
					whdload_prefs.variant_uuid.assign(xml_element->GetText());
				}

				// Slave count
				xml_element = game_node->FirstChildElement("slave_count");
				if (xml_element)
				{
					whdload_prefs.slave_count = xml_element->IntText(0);
				}

				// Default slave
				xml_element = game_node->FirstChildElement("slave_default");
				if (xml_element)
				{
					whdload_prefs.slave_default.assign(xml_element->GetText());
					write_log("WHDBooter - Selected Slave: %s \n", whdload_prefs.slave_default.c_str());
				}

				// Slave_libraries
				xml_element = game_node->FirstChildElement("slave_libraries");
				if (xml_element->GetText() != nullptr)
				{
					if (strcmpi(xml_element->GetText(), "true") == 0)
						whdload_prefs.slave_libraries = true;
				}

				// Get slaves and settings
				xml_element = game_node->FirstChildElement("slave");
				whdload_prefs.slaves.clear();

				for (int i = 0; i < whdload_prefs.slave_count && xml_element; ++i)
				{
					whdload_slave slave;
					const char* slave_text = nullptr;

					slave_text = xml_element->FirstChildElement("filename")->GetText();
					if (slave_text)
						slave.filename.assign(slave_text);

					slave_text = xml_element->FirstChildElement("datapath")->GetText();
					if (slave_text)
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

					xml_element = game_node->NextSiblingElement("slave");
				}

				// get hardware
				xml_element = game_node->FirstChildElement("hardware");
				if (xml_element)
				{
					std::string hardware;
					hardware.assign(xml_element->GetText());
					if (!hardware.empty())
					{
						game_detail = get_game_hardware_settings(hardware);
						write_log("WHDBooter - Game H/W Settings: \n%s\n", hardware.c_str());
					}
				}

				// get custom controls
				xml_element = game_node->FirstChildElement("custom_controls");
				if (xml_element)
				{
					std::string custom_settings;
					custom_settings.assign(xml_element->GetText());
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
	}
	return game_detail;
}

void create_startup_sequence()
{
	std::ostringstream whd_bootscript;
	whd_bootscript << "FAILAT 999\n";

	if (whdload_prefs.slave_libraries)
	{
		whd_bootscript << "DH3:C/Assign LIBS: DH3:LIBS/ ADD\n";
	}

	whd_bootscript << "IF NOT EXISTS WHDLoad\n";
	whd_bootscript << "DH3:C/Assign C: DH3:C/ ADD\n";
	whd_bootscript << "ENDIF\n";

	whd_bootscript << "CD \"Games:" << whdload_prefs.sub_path << "\"\n";
	whd_bootscript << "WHDLoad SLAVE=\"Games:" << whdload_prefs.sub_path << "/" << whdload_prefs.selected_slave.filename << "\"";

	// Write Cache
	if (whdload_prefs.write_cache)
	{
		whd_bootscript << " PRELOAD NOREQ";
	}
	else
	{
		whd_bootscript << " PRELOAD NOREQ NOWRITECACHE";
	}

	// CUSTOM options
	if (whdload_prefs.selected_slave.custom1.value > 0)
	{
		whd_bootscript << " CUSTOM1=" << whdload_prefs.selected_slave.custom1.value;
	}
	if (whdload_prefs.selected_slave.custom2.value > 0)
	{
		whd_bootscript << " CUSTOM2=" << whdload_prefs.selected_slave.custom2.value;
	}
	if (whdload_prefs.selected_slave.custom3.value > 0)
	{
		whd_bootscript << " CUSTOM3=" << whdload_prefs.selected_slave.custom3.value;
	}
	if (whdload_prefs.selected_slave.custom4.value > 0)
	{
		whd_bootscript << " CUSTOM4=" << whdload_prefs.selected_slave.custom4.value;
	}
	if (whdload_prefs.selected_slave.custom5.value > 0)
	{
		whd_bootscript << " CUSTOM5=" << whdload_prefs.selected_slave.custom5.value;
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
		whd_bootscript << "DATA=\"" << whdload_prefs.selected_slave.data_path << "\"";

	whd_bootscript << '\n';

	// Launches utility program to quit the emulator (via a UAE trap in RTAREA)
	if (whdload_prefs.quit_on_exit)
	{
		whd_bootscript << "DH0:C/AmiQuit\n";
	}

	write_log("WHDBooter - Created Startup-Sequence  \n\n%s\n", whd_bootscript.str().c_str());
	write_log("WHDBooter - Saved Auto-Startup to %s\n", whd_startup);

	std::ofstream myfile(whd_startup);
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
	TCHAR tmp[MAX_DPATH];

	if (!whdload_prefs.selected_slave.filename.empty()) // new booter solution
	{
		_sntprintf(boot_path, MAX_DPATH, "/tmp/amiberry/");

		_sntprintf(tmp, MAX_DPATH, _T("filesystem2=rw,DH0:DH0:%s,10"), boot_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);

		_sntprintf(tmp, MAX_DPATH, _T("uaehf0=dir,rw,DH0:DH0::%s,10"), boot_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);

		_sntprintf(boot_path, MAX_DPATH, "%sboot-data.zip", whdbooter_path.c_str());
		if (!my_existsfile2(boot_path))
			_sntprintf(boot_path, MAX_DPATH, "%sboot-data/", whdbooter_path.c_str());

		_sntprintf(tmp, MAX_DPATH, _T("filesystem2=rw,DH3:DH3:%s,-10"), boot_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);

		_sntprintf(tmp, MAX_DPATH, _T("uaehf0=dir,rw,DH3:DH3::%s,-10"), boot_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);
	}
	else // revert to original booter is no slave was set
	{
		_sntprintf(boot_path, MAX_DPATH, "%sboot-data.zip", whdbooter_path.c_str());
		if (!my_existsfile2(boot_path))
			_sntprintf(boot_path, MAX_DPATH, "%sboot-data/", whdbooter_path.c_str());

		_sntprintf(tmp, MAX_DPATH, _T("filesystem2=rw,DH0:DH0:%s,10"), boot_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);

		_sntprintf(tmp, MAX_DPATH, _T("uaehf0=dir,rw,DH0:DH0::%s,10"), boot_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);
	}

	//set the Second (game data) drive
	_sntprintf(tmp, MAX_DPATH, "filesystem2=rw,DH1:Games:\"%s\",0", filepath);
	cfgfile_parse_line(prefs, parse_text(tmp), 0);

	_sntprintf(tmp, MAX_DPATH, "uaehf1=dir,rw,DH1:Games:\"%s\",0", filepath);
	cfgfile_parse_line(prefs, parse_text(tmp), 0);

	//set the third (save data) drive
	_sntprintf(whd_path, MAX_DPATH, "%s/", save_path);

	if (my_existsdir(save_path))
	{
		_sntprintf(tmp, MAX_DPATH, "filesystem2=rw,DH2:Saves:%s,0", save_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);

		_sntprintf(tmp, MAX_DPATH, "uaehf2=dir,rw,DH2:Saves:%s,0", save_path);
		cfgfile_parse_line(prefs, parse_text(tmp), 0);
	}
}

void whdload_auto_prefs(uae_prefs* prefs, const char* filepath)
{
	write_log("WHDBooter Launched\n");

	get_configuration_path(conf_path, MAX_DPATH);
	whdbooter_path = get_whdbootpath();
	get_savedatapath(save_path, MAX_DPATH, 0 );

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

	// LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE
	//  CONFIG LOAD IF .UAE IS IN CONFIG PATH
	build_uae_config_filename(whdload_prefs.filename);

	// If we have a config file, we will use it.
	// We will need it for the WHDLoad options too.
	if (my_existsfile2(uae_config))
	{
		write_log("WHDBooter -  %s found. Loading Config for WHDLoad options.\n", uae_config);
		target_cfgfile_load(&currprefs, uae_config, CONFIG_TYPE_ALL, 0);
	}

	// setups for tmp folder.
	my_mkdir("/tmp/amiberry");
	my_mkdir("/tmp/amiberry/s");
	my_mkdir("/tmp/amiberry/c");
	my_mkdir("/tmp/amiberry/devs");
	_tcscpy(whd_startup, "/tmp/amiberry/s/startup-sequence");
	remove(whd_startup);

	// LOAD HOST OPTIONS
	_sntprintf(whd_path, MAX_DPATH, "%sWHDLoad", whdbooter_path.c_str());

	// are we using save-data/ ?
	_sntprintf(kick_path, MAX_DPATH, "%s/Kickstarts", save_path);

	// LOAD GAME SPECIFICS
	_sntprintf(whd_path, MAX_DPATH, "%sgame-data/", whdbooter_path.c_str());
	game_hardware_options game_detail;

	_tcscpy(whd_config, whd_path);
	_tcscat(whd_config, "whdload_db.xml");

	if (my_existsfile2(whd_config))
	{
		game_detail = parse_settings_from_xml(prefs, filepath);
	}
	else
	{
		write_log("WHDBooter -  Could not load whdload_db.xml - does not exist?\n");
	}

	// If we have a slave, create a startup-sequence
	if (!whdload_prefs.selected_slave.filename.empty())
	{
		create_startup_sequence();
	}

	// now we should have a startup-sequence file (if we don't, we are going to use the original booter)
	if (my_existsfile2(whd_startup))
	{
		// create a symlink to WHDLoad in /tmp/amiberry/
		_sntprintf(whd_path, MAX_DPATH, "%sWHDLoad", whdbooter_path.c_str());
		symlink(whd_path, "/tmp/amiberry/c/WHDLoad");

		// Create a symlink to AmiQuit in /tmp/amiberry/
		_sntprintf(whd_path, MAX_DPATH, "%sAmiQuit", whdbooter_path.c_str());
		symlink(whd_path, "/tmp/amiberry/c/AmiQuit");

		// create a symlink for DEVS in /tmp/amiberry/
		symlink(kick_path, "/tmp/amiberry/devs/Kickstarts");
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
	if (my_existsfile2(uae_config))
	{
		write_log("WHDBooter - %s found; ignoring WHD Quickstart setup.\n", uae_config);
		return;
	}

	//    *** EMULATED HARDWARE ***
	//    SET UNIVERSAL DEFAULTS
	prefs->start_gui = false;

	// DO CHECKS FOR AGA / CD32
	const auto is_aga = _tcsstr(filename, "AGA") != nullptr || strcmpi(game_detail.chipset.c_str(), "AGA") == 0;
	const auto is_cd32 = _tcsstr(filename, "CD32") != nullptr || strcmpi(game_detail.chipset.c_str(), "CD32") == 0;

	if (is_aga || is_cd32 || !a600_available)
	{
		// SET THE BASE AMIGA (Expanded A1200)
		built_in_prefs(prefs, 4, A1200_CONFIG, 0, 0);
		_tcscpy(prefs->description, _T("AutoBoot Configuration [WHDLoad] [AGA]"));
	}
	else
	{
		// SET THE BASE AMIGA (Expanded A600)
		built_in_prefs(prefs, 2, A600_CONFIG, 0, 0);
		_tcscpy(prefs->description, _T("AutoBoot Configuration [WHDLoad]"));
	}

	// SET THE WHD BOOTER AND GAME DATA
	set_booter_drives(prefs, filepath);

	// APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
	set_input_settings(prefs, game_detail, is_cd32);

	//  SET THE GAME COMPATIBILITY SETTINGS
	// BLITTER, SPRITES, MEMORY, JIT, BIG CPU ETC
	set_compatibility_settings(prefs, game_detail, a600_available, is_aga || is_cd32);
}
