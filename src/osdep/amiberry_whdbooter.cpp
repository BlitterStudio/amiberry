/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "sysdeps.h"
#include "uae.h"
#include "options.h"
#include "custom.h"
#include "rommgr.h"

extern FILE* debugfile;

#include "fsdb.h"
#include "tinyxml2.h"

extern void SetLastActiveConfig(const char* filename);
extern char current_dir[MAX_DPATH];
extern char last_loaded_config[MAX_DPATH];

#include <fstream>

struct game_options
{
	TCHAR port0[256] = "nul\0";
	TCHAR port1[256] = "nul\0";
	TCHAR control[256] = "nul\0";
	TCHAR control2[256] = "nul\0";
	TCHAR fastcopper[256] = "nul\0";
	TCHAR cpu[256] = "nul\0";
	TCHAR blitter[256] = "nul\0";
	TCHAR clock[256] = "nul\0";
	TCHAR chipset[256] = "nul\0";
	TCHAR jit[256] = "nul\0";
	TCHAR cpu_comp[256] = "nul\0";
	TCHAR cpu_24bit[256] = "nul\0";
	TCHAR sprites[256] = "nul\0";
	TCHAR ntsc[256] = "nul\0";
	TCHAR chip[256] = "nul\0";
	TCHAR fast[256] = "nul\0";
	TCHAR z3[256] = "nul\0";
};

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

static TCHAR* parse_text_path(const TCHAR* s)
{
	auto* s2 = parse_text(s);
	auto* const s3 = target_expand_environment(s2, nullptr, 0);
	xfree(s2);
	return s3;
}

void remove_char(char* array, int len, int index)
{
	for (auto i = index; i < len - 1; ++i)
		array[i] = array[i + 1];
	array[len - 1] = 0;
}

void parse_custom_settings(struct uae_prefs* p, const char* settings)
{
	char temp_options[4096];
	strcpy(temp_options, settings);

	auto* full_line = strtok(temp_options, "\n");

	while (full_line != nullptr)
	{
		std::string line = full_line;
		std::string check = "amiberry_custom";

		if (strstr(line.c_str(), check.c_str()) != nullptr)
		{
			cfgfile_parse_line(p, full_line, 0);
		}
		full_line = strtok(nullptr, "\n");
	}
}

struct membuf final : std::streambuf
{
	membuf(char* begin, char* end)
	{
		this->setg(begin, begin, end);
	}
};

std::string find_whdload_game_option(const TCHAR* find_setting, const char* whd_options)
{
	char temp_options[4096];
	char temp_setting[4096];
	char temp_setting_tab[4096];

	strcpy(temp_options, whd_options);
	const auto* output = "nul";

	auto* full_line = strtok(temp_options, "\n");

	while (full_line != nullptr)
	{
		// Do checks with and without leading tabs  (*** THIS SHOULD BE IMPROVED ***)
		std::string t = full_line;

		strcpy(temp_setting_tab, "\t\t");
		strcat(temp_setting_tab, find_setting);
		strcat(temp_setting_tab, "=");

		strcpy(temp_setting, find_setting);
		strcat(temp_setting, "=");

		// check the beginning of the full line
		if (strncmp(temp_setting, full_line, strlen(temp_setting)) == 0)
		{
			t.erase(t.begin(), t.begin() + strlen(temp_setting));
			return t;
		}
		if (strncmp(temp_setting_tab, full_line, strlen(temp_setting_tab)) == 0)
		{
			t.erase(t.begin(), t.begin() + strlen(temp_setting_tab));
			return t;
		}

		full_line = strtok(nullptr, "\n");
	}

	return output;
}

struct game_options get_game_settings(const char* HW)
{
	struct game_options output_detail;
	strcpy(output_detail.port0, find_whdload_game_option("PORT0", HW).c_str());
	strcpy(output_detail.port1, find_whdload_game_option("PORT1", HW).c_str());
	strcpy(output_detail.control, find_whdload_game_option("PRIMARY_CONTROL", HW).c_str());
	strcpy(output_detail.control2, find_whdload_game_option("SECONDARY_CONTROL", HW).c_str());
	strcpy(output_detail.fastcopper, find_whdload_game_option("FAST_COPPER", HW).c_str());
	strcpy(output_detail.cpu, find_whdload_game_option("CPU", HW).c_str());
	strcpy(output_detail.blitter, find_whdload_game_option("BLITTER", HW).c_str());
	strcpy(output_detail.clock, find_whdload_game_option("CLOCK", HW).c_str());
	strcpy(output_detail.chipset, find_whdload_game_option("CHIPSET", HW).c_str());
	strcpy(output_detail.jit, find_whdload_game_option("JIT", HW).c_str());
	strcpy(output_detail.cpu_24bit, find_whdload_game_option("CPU_24BITADDRESSING", HW).c_str());
	strcpy(output_detail.cpu_comp, find_whdload_game_option("CPU_COMPATIBLE", HW).c_str());
	strcpy(output_detail.sprites, find_whdload_game_option("SPRITES", HW).c_str());
	strcpy(output_detail.ntsc, find_whdload_game_option("NTSC", HW).c_str());
	strcpy(output_detail.fast, find_whdload_game_option("FAST_RAM", HW).c_str());
	strcpy(output_detail.z3, find_whdload_game_option("Z3_RAM", HW).c_str());

	return output_detail;
}

void make_rom_symlink(const char* kick_short, char* kick_path, int kick_numb, struct uae_prefs* p)
{
	char kick_long[MAX_DPATH];
	int roms[2];

	// do the checks...
	snprintf(kick_long, MAX_DPATH, "%s/%s", kick_path, kick_short);

	// this should sort any broken links
	my_unlink(kick_long);

	if (!my_existsfile(kick_long))
	{
		roms[0] = kick_numb; // kickstart 1.2 A500
		const auto rom_test = configure_rom(p, roms, 0); // returns 0 or 1 if found or not found
		if (rom_test == 1)
		{
			symlink(p->romfile, kick_long);
			write_log("Making SymLink for Kickstart ROM: %s\n", kick_long);
		}
	}
}

void symlink_roms(struct uae_prefs* prefs)
{
	//      *** KICKSTARTS ***
	//
	char kick_path[MAX_DPATH];
	char tmp[MAX_DPATH];
	char tmp2[MAX_DPATH];

	write_log("SymLink Kickstart ROMs for Booter\n");

	// here we can do some checks for Kickstarts we might need to make symlinks for
	strncpy(current_dir, start_path_data, MAX_DPATH);

	// are we using save-data/ ?
	snprintf(kick_path, MAX_DPATH, "%s/whdboot/save-data/Kickstarts", start_path_data);

	// otherwise, use the old route
	if (!my_existsdir(kick_path))
		snprintf(kick_path, MAX_DPATH, "%s/whdboot/game-data/Devs/Kickstarts", start_path_data);

	// These are all the kickstart rom files found in skick346.lha
	//   http://aminet.net/package/util/boot/skick346

	make_rom_symlink("kick33180.A500", kick_path, 5, prefs);
	make_rom_symlink("kick34005.A500", kick_path, 6, prefs);
	make_rom_symlink("kick37175.A500", kick_path, 7, prefs);
	make_rom_symlink("kick39106.A1200", kick_path, 11, prefs);
	make_rom_symlink("kick40063.A600", kick_path, 14, prefs);
	make_rom_symlink("kick40068.A1200", kick_path, 15, prefs);
	make_rom_symlink("kick40068.A4000", kick_path, 16, prefs);

	// Symlink rom.key also
	// source file
	get_rom_path(tmp2, MAX_DPATH);
	snprintf(tmp, MAX_DPATH, "%s/rom.key", tmp2);

	// destination file (symlink)
	snprintf(tmp2, MAX_DPATH, "%s/rom.key", kick_path);

	if (my_existsfile(tmp))
		symlink(tmp, tmp2);
}

void cd_auto_prefs(struct uae_prefs* prefs, char* filepath)
{
	TCHAR game_name[MAX_DPATH];
	TCHAR* txt2;
	TCHAR tmp[MAX_DPATH];
	char config_path[MAX_DPATH];
	char whd_config[255];

	get_configuration_path(config_path, MAX_DPATH);

	//      *** GAME DETECTION ***
	write_log("\nCD Autoload: %s  \n\n", filepath);

	extract_filename(filepath, last_loaded_config);
	extract_filename(filepath, game_name);
	remove_file_extension(game_name);

	// LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE
	//  CONFIG LOAD IF .UAE IS IN CONFIG PATH
	strcpy(whd_config, config_path);
	strcat(whd_config, game_name);
	strcat(whd_config, ".uae");

	if (my_existsfile(whd_config))
	{
		target_cfgfile_load(prefs, whd_config, CONFIG_TYPE_ALL, 0);
		return;
	}

	// LOAD HOST OPTIONS
	char whd_path[MAX_DPATH];
	snprintf(whd_path, MAX_DPATH, "%s/whdboot/", start_path_data);

	prefs->start_gui = false;

	const auto is_cdtv = strstr(filepath, "CDTV") != nullptr || strstr(filepath, "cdtv") != nullptr;
	const auto is_cd32 = strstr(filepath, "CD32") != nullptr || strstr(filepath, "cd32") != nullptr;

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
		built_in_prefs(prefs, 4, 1, 0, 0);
	}

	// enable CD
	_stprintf(tmp, "cd32cd=1");
	txt2 = parse_text_path(_T(tmp));
	cfgfile_parse_line(prefs, txt2, 0);

	// mount the image
	_stprintf(tmp, "cdimage0=%s,image", filepath);
	txt2 = parse_text_path(_T(tmp));
	cfgfile_parse_line(prefs, txt2, 0);

	//APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
	// CD32
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

	// APPLY SPECIAL CONFIG E.G. MOUSE OR ALT. JOYSTICK SETTINGS
	for (auto& jport : prefs->jports)
	{
		jport.id = JPORT_NONE;
		jport.idc.configname[0] = 0;
		jport.idc.name[0] = 0;
		jport.idc.shortid[0] = 0;
	}

	// WHAT IS THE MAIN CONTROL?
	// PORT 0 - MOUSE
	if (is_cd32 && strcmpi(amiberry_options.default_controller2, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport0"), _T(amiberry_options.default_controller2));
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else if (strcmpi(amiberry_options.default_mouse1, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport0"), _T(amiberry_options.default_mouse1));
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else
	{
		_stprintf(txt2, "%s=mouse", _T("joyport0"));
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// PORT 1 - JOYSTICK
	if (strcmpi(amiberry_options.default_controller1, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport1"), _T(amiberry_options.default_controller1));
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else
	{
		_stprintf(txt2, "%s=joy1", _T("joyport1"));
		cfgfile_parse_line(prefs, txt2, 0);
	}
}

void whdload_auto_prefs(struct uae_prefs* prefs, char* filepath)
{
	TCHAR game_name[MAX_DPATH];
	TCHAR* txt2 = nullptr;
	TCHAR tmp[MAX_DPATH];
	char boot_path[MAX_DPATH];
	char save_path[MAX_DPATH];
	char config_path[MAX_DPATH];
	char whd_path[MAX_DPATH];
	char kick_path[MAX_DPATH];

	char uae_config[255];
	char whd_config[255];
	char whd_startup[255];

	char selected_slave[4096];
	// note!! this should be global later on, and only collected from the XML if set to 'nothing'
	char subpath[4096];

	auto use_slave_libs = false;

	write_log("WHDBooter Launched\n");
	strcpy(selected_slave, "");

	get_configuration_path(config_path, MAX_DPATH);

	//      *** KICKSTARTS ***

	symlink_roms(prefs);

	// this allows A600HD to be used to slow games down
	int roms[2];
	roms[0] = 15; // kickstart 2.05 A600HD  ..  10
	const auto rom_test = configure_rom(prefs, roms, 0); // returns 0 or 1 if found or not found
	const auto a600_available = rom_test;

	if (a600_available == true)
	{
		write_log("WHDBooter - Host: A600 ROM Available \n");
	}

	//      *** GAME DETECTION ***

	// REMOVE THE FILE PATH AND EXTENSION
	const auto* const filename = my_getfilepart(filepath);
	// SOMEWHERE HERE WE NEED TO SET THE GAME 'NAME' FOR SAVESTATE ETC PURPOSES
	extract_filename(filepath, last_loaded_config);
	extract_filename(filepath, game_name);
	remove_file_extension(game_name);

	// LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE
	//  CONFIG LOAD IF .UAE IS IN CONFIG PATH
	strcpy(uae_config, config_path);
	strcat(uae_config, game_name);
	strcat(uae_config, ".uae");

	// setups for tmp folder.
	my_mkdir("/tmp/s");
	my_mkdir("/tmp/c");
	my_mkdir("/tmp/devs");
	strcpy(whd_startup, "/tmp/s/startup-sequence");
	remove(whd_startup);


	// LOAD HOST OPTIONS
	snprintf(whd_path, MAX_DPATH, "%s/whdboot/WHDLoad", start_path_data);

	// are we using save-data/ ?
	snprintf(kick_path, MAX_DPATH, "%s/whdboot/save-data/Kickstarts", start_path_data);

	// if we have a config file, we will use it
	// we will need it for the WHDLoad options too.
	if (my_existsfile(uae_config))
	{
		write_log("WHDBooter -  %s found. Loading Config for WHDload options.\n", uae_config);
		target_cfgfile_load(&currprefs, uae_config, CONFIG_TYPE_ALL, 0);
	}

	//  this should be made into it's own routine!! 1 (see repeat, above)
	snprintf(whd_path, MAX_DPATH, "%s/whdboot/", start_path_data);

	// LOAD GAME SPECIFICS - USE SHA1 IF AVAILABLE
	snprintf(whd_path, MAX_DPATH, "%s/whdboot/game-data/", start_path_data);
	struct game_options game_detail;

	// EDIT THE FILE NAME TO USE HERE

	strcpy(whd_config, whd_path);
	strcat(whd_config, "whdload_db.xml");

	if (my_existsfile(whd_config)) // use XML database
	{
		tinyxml2::XMLDocument doc;
		auto error = false;
		write_log("WHDBooter - Loading whdload_db.xml\n");
		write_log("WHDBooter - Searching whdload_db.xml for %s\n", game_name);

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
			auto* game_node = doc.FirstChildElement("whdbooter");
			game_node = game_node->FirstChildElement("game");
			while (game_node != nullptr)
			{
				if (game_node->Attribute("filename", game_name))
				{
					// now get the <hardware> and <custom_controls> items
					// get hardware
					auto* temp_node = game_node->FirstChildElement("hardware");
					if (temp_node)
					{
						const auto* hardware = temp_node->GetText();
						if (hardware)
						{
							game_detail = get_game_settings(hardware);
							write_log("WHDBooter - Game H/W Settings: \n%s\n", hardware);
						}
					}
					
					// get custom controls
					temp_node = game_node->FirstChildElement("custom_controls");
					if (temp_node)
					{
						const auto* custom_settings = temp_node->GetText();
						if (custom_settings)
						{
							parse_custom_settings(prefs, custom_settings);
						}
					}

					if (strlen(selected_slave) == 0)
					{
						temp_node = game_node->FirstChildElement("slave_default");

						// use a selected slave if we have one
						if (strlen(currprefs.whdbootprefs.slave) != 0)
						{
							strcpy(selected_slave, currprefs.whdbootprefs.slave);
							write_log("WHDBooter - Config Selected Slave: %s \n", selected_slave);
						}
						// otherwise use the XML default
						else if (temp_node->GetText() != nullptr)
						{
							_stprintf(selected_slave, "%s", temp_node->GetText());
							write_log("WHDBooter - Default Slave: %s\n", selected_slave);
						}

						temp_node = game_node->FirstChildElement("subpath");

						if (temp_node->GetText() != nullptr)
						{
							_stprintf(subpath, "%s",	temp_node->GetText());
							write_log("WHDBooter - SubPath:  %s\n", subpath);
						}
					}

					// get slave_libraries
					temp_node = game_node->FirstChildElement("slave_libraries");
					if (temp_node->GetText() != nullptr)
					{
						if (strcmpi(temp_node->GetText(), "true") == 0)
							use_slave_libs = true;

						write_log("WHDBooter - Libraries:  %s\n", subpath);
					}
					break;
				}
				game_node = game_node->NextSiblingElement();
			}
		}
	}
	else
		write_log("WHDBooter -  Could not load whdload_db.xml - does not exist?\n");

	// currently, we have selected a slave, so we create a startup-sequence
	if (strlen(selected_slave) != 0)
	{
		std::ostringstream whd_bootscript;
		whd_bootscript << '\n';

		if (use_slave_libs)
		{
			whd_bootscript << "DH3:C/Assign LIBS: DH3:LIBS/ ADD\n";
		}

		whd_bootscript << "IF NOT EXISTS WHDLoad\n";
		whd_bootscript << "DH3:C/Assign C: DH3:C/ ADD\n";
		whd_bootscript << "ENDIF\n";

		whd_bootscript << "CD \"Games:" << subpath << "\"\n";
		whd_bootscript << "WHDLoad SLAVE=\"Games:" << subpath << "/" << selected_slave << "\"";
		whd_bootscript << " PRELOAD NOWRITECACHE NOREQ";

		// CUSTOM options
		if (currprefs.whdbootprefs.custom1 > 0)
		{
			whd_bootscript << " CUSTOM1=" << currprefs.whdbootprefs.custom1;
		}
		if (currprefs.whdbootprefs.custom2 > 0)
		{
			whd_bootscript << " CUSTOM2=" << currprefs.whdbootprefs.custom2;
		}
		if (currprefs.whdbootprefs.custom3 > 0)
		{
			whd_bootscript << " CUSTOM3=" << currprefs.whdbootprefs.custom3;
		}
		if (currprefs.whdbootprefs.custom4 > 0)
		{
			whd_bootscript << " CUSTOM4=" << currprefs.whdbootprefs.custom4;
		}
		if (currprefs.whdbootprefs.custom5 > 0)
		{
			whd_bootscript << " CUSTOM5=" << currprefs.whdbootprefs.custom5;
		}
		if (strlen(currprefs.whdbootprefs.custom) != 0)
		{
			whd_bootscript << " CUSTOM=\"" << currprefs.whdbootprefs.custom << "\"";
		}

		// BUTTONWAIT
		if (currprefs.whdbootprefs.buttonwait == true)
		{
			whd_bootscript << " BUTTONWAIT";
		}

		// SPLASH
		if (currprefs.whdbootprefs.showsplash != true)
		{
			whd_bootscript << " SPLASHDELAY=0";
		}

		// CONFIGDELAY
		if (currprefs.whdbootprefs.configdelay != 0)
		{
			whd_bootscript << " CONFIGDELAY=" << currprefs.whdbootprefs.configdelay;
		}

		// SPECIAL SAVE PATH
		whd_bootscript << " SAVEPATH=Saves:Savegames/ SAVEDIR=\"" << subpath << "\"";
		whd_bootscript << '\n';

		write_log("WHDBooter - Created Startup-Sequence  \n\n%s\n", whd_bootscript.str().c_str());
		write_log("WHDBooter - Saved Auto-Startup to %s\n", whd_startup);

		ofstream myfile(whd_startup);
		if (myfile.is_open())
		{
			myfile << whd_bootscript.str();
			myfile.close();
		}
	}

	// now we should have a startup-file (if we don't, we are going to use the original booter)
	if (my_existsfile(whd_startup))
	{
		// create a symlink to WHDLoad in /tmp/
		snprintf(whd_path, MAX_DPATH, "%s/whdboot/WHDLoad", start_path_data);
		symlink(whd_path, "/tmp/c/WHDLoad");

		// create a symlink for DEVS in /tmp/
		symlink(kick_path, "/tmp/devs/Kickstarts");
	}
#if DEBUG
	// debugging code!
	write_log("WHDBooter - Game: Port 0     : %s  \n", game_detail.port0);
	write_log("WHDBooter - Game: Port 1     : %s  \n", game_detail.port1);
	write_log("WHDBooter - Game: Control    : %s  \n", game_detail.control);
	write_log("WHDBooter - Game: Fast Copper: %s  \n", game_detail.fastcopper);
	write_log("WHDBooter - Game: CPU        : %s  \n", game_detail.cpu);
	write_log("WHDBooter - Game: Blitter    : %s  \n", game_detail.blitter);
	write_log("WHDBooter - Game: CPU Clock  : %s  \n", game_detail.clock);
	write_log("WHDBooter - Game: Chipset    : %s  \n", game_detail.chipset);
	write_log("WHDBooter - Game: JIT        : %s  \n", game_detail.jit);
	write_log("WHDBooter - Game: CPU Compat : %s  \n", game_detail.cpu_comp);
	write_log("WHDBooter - Game: Sprite Col : %s  \n", game_detail.sprites);
	write_log("WHDBooter - Game: NTSC       : %s  \n", game_detail.ntsc);
	write_log("WHDBooter - Game: Fast Ram   : %s  \n", game_detail.fast);
	write_log("WHDBooter - Game: Z3 Ram     : %s  \n", game_detail.z3);

	// debugging code!
	write_log("WHDBooter - Host: Controller 1   : %s  \n", amiberry_options.default_controller1);
	write_log("WHDBooter - Host: Controller 2   : %s  \n", amiberry_options.default_controller2);
	write_log("WHDBooter - Host: Controller 3   : %s  \n", amiberry_options.default_controller3);
	write_log("WHDBooter - Host: Controller 4   : %s  \n", amiberry_options.default_controller4);
	write_log("WHDBooter - Host: Mouse 1        : %s  \n", amiberry_options.default_mouse1);
	write_log("WHDBooter - Host: Mouse 2        : %s  \n", amiberry_options.default_mouse2);
#endif

	// so remember, we already loaded a .uae config, so we don't need to do the below manual setup for hardware
	if (my_existsfile(uae_config))
	{
		write_log("WHDBooter - %s found; ignoring WHD Quickstart setup.\n", uae_config);
		return;
	}

	//    *** EMULATED HARDWARE ***
	//    SET UNIVERSAL DEFAULTS
	prefs->start_gui = false;

	if ((strcmpi(game_detail.cpu, "68000") == 0 || strcmpi(game_detail.cpu, "68010") == 0) && a600_available != 0)
		// SET THE BASE AMIGA (Expanded A600)
		built_in_prefs(prefs, 2, 2, 0, 0);
	else
		// SET THE BASE AMIGA (Expanded A1200)
	{
		built_in_prefs(prefs, 4, 1, 0, 0);
		if (strcmpi(game_detail.fast, "nul") != 0 && (strcmpi(game_detail.cpu, "nul") == 0))
			strcpy(game_detail.cpu, _T("68020"));
	}

	// DO CHECKS FOR AGA / CD32
	const int is_aga = strstr(filename, "_AGA") != nullptr || strcmpi(game_detail.chipset, "AGA") == 0;
	const int is_cd32 = strstr(filename, "_CD32") != nullptr || strcmpi(game_detail.chipset, "CD32") == 0;

	// A1200 no AGA
	if (!static_cast<bool>(is_aga) && !static_cast<bool>(is_cd32))
	{
		_tcscpy(prefs->description, _T("AutoBoot Configuration [WHDLoad]"));

		prefs->cs_compatible = CP_A600;
		built_in_chipset_prefs(prefs);
		prefs->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	}
		// A1200
	else
		_tcscpy(prefs->description, _T("AutoBoot Configuration [WHDLoad] [AGA]"));

	//SET THE WHD BOOTER AND GAME DATA
	if (strlen(selected_slave) != 0) // new booter solution
	{
		snprintf(boot_path, MAX_DPATH, "/tmp/");

		_stprintf(tmp, _T("filesystem2=rw,DH0:DH0:%s,10"), boot_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);

		_stprintf(tmp, _T("uaehf0=dir,rw,DH0:DH0::%s,10"), boot_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);

		snprintf(boot_path, MAX_DPATH, "%s/whdboot/boot-data.zip", start_path_data);
		if (!my_existsfile(boot_path))
			snprintf(boot_path, MAX_DPATH, "%s/whdboot/boot-data/", start_path_data);

		_stprintf(tmp, _T("filesystem2=rw,DH3:DH3:%s,-10"), boot_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);

		_stprintf(tmp, _T("uaehf0=dir,rw,DH3:DH3::%s,-10"), boot_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);
	}

	else // revert to original booter is no slave was set
	{
		snprintf(boot_path, MAX_DPATH, "%s/whdboot/boot-data.zip", start_path_data);
		if (!my_existsfile(boot_path))
			snprintf(boot_path, MAX_DPATH, "%s/whdboot/boot-data/", start_path_data);

		_stprintf(tmp, _T("filesystem2=rw,DH0:DH0:%s,10"), boot_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);

		_stprintf(tmp, _T("uaehf0=dir,rw,DH0:DH0::%s,10"), boot_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);
	}

	//set the Second (game data) drive
	_stprintf(tmp, "filesystem2=rw,DH1:Games:%s,0", filepath);
	txt2 = parse_text_path(_T(tmp));
	cfgfile_parse_line(prefs, txt2, 0);

	_stprintf(tmp, "uaehf1=dir,rw,DH1:Games:%s,0", filepath);
	txt2 = parse_text_path(_T(tmp));
	cfgfile_parse_line(prefs, txt2, 0);

	//set the third (save data) drive
	snprintf(save_path, MAX_DPATH, "%s/whdboot/save-data/", start_path_data);

	if (my_existsdir(save_path))
	{
		_stprintf(tmp, "filesystem2=rw,DH2:Saves:%s,0", save_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);

		_stprintf(tmp, "uaehf2=dir,rw,DH2:Saves:%s,0", save_path);
		txt2 = parse_text_path(_T(tmp));
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
	//  CD32
	if (static_cast<bool>(is_cd32)
		&& (strcmpi(game_detail.port0, "nul") == 0 || strcmpi(game_detail.port0, "cd32") == 0))
		prefs->jports[0].mode = 7;

	if (static_cast<bool>(is_cd32)
		&& (strcmpi(game_detail.port1, "nul") == 0 || strcmpi(game_detail.port1, "cd32") == 0))
		prefs->jports[1].mode = 7;

	// JOY
	if (strcmpi(game_detail.port0, "joy") == 0)
		prefs->jports[0].mode = 0;
	if (strcmpi(game_detail.port1, "joy") == 0)
		prefs->jports[1].mode = 0;

	// MOUSE
	if (strcmpi(game_detail.port0, "mouse") == 0)
		prefs->jports[0].mode = 2;
	if (strcmpi(game_detail.port1, "mouse") == 0)
		prefs->jports[1].mode = 2;

	// APPLY SPECIAL CONFIG E.G. MOUSE OR ALT. JOYSTICK SETTINGS
	for (auto& jport : prefs->jports)
	{
		jport.id = JPORT_NONE;
		jport.idc.configname[0] = 0;
		jport.idc.name[0] = 0;
		jport.idc.shortid[0] = 0;
	}

	// WHAT IS THE MAIN CONTROL?
	// PORT 0 - MOUSE GAMES
	if (strcmpi(game_detail.control, "mouse") == 0 && strcmpi(amiberry_options.default_mouse1, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport0"), _T(amiberry_options.default_mouse1));
		cfgfile_parse_line(prefs, txt2, 0);
		write_log("WHDBooter Option (Mouse Control): %s\n", txt2);
	}

	// PORT 0 - JOYSTICK GAMES
	else if (strcmpi(amiberry_options.default_controller2, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport0"), _T(amiberry_options.default_controller2));
		cfgfile_parse_line(prefs, txt2, 0);
		write_log("WHDBooter Option (Joystick Control): %s\n", txt2);
	}
	else
	{
		_stprintf(txt2, "%s=mouse", _T("joyport0"));
		cfgfile_parse_line(prefs, txt2, 0);
		write_log("WHDBooter Option (Default Mouse): %s\n", txt2);
	}

	// PORT 1 - MOUSE GAMES
	if (strcmpi(game_detail.control, "mouse") == 0 && strcmpi(amiberry_options.default_mouse2, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport1"), _T(amiberry_options.default_mouse2));
		cfgfile_parse_line(prefs, txt2, 0);
		write_log("WHDBooter Option (Mouse Control): %s\n", txt2);
	}
	// PORT 1 - JOYSTICK GAMES
	else if (strcmpi(amiberry_options.default_controller1, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport1"), _T(amiberry_options.default_controller1));
		cfgfile_parse_line(prefs, txt2, 0);
		write_log("WHDBooter Option (Joystick Control): %s\n", txt2);
	}
	else
	{
		_stprintf(txt2, "%s=joy1", _T("joyport1"));
		cfgfile_parse_line(prefs, txt2, 0);
		write_log("WHDBooter Option (Default Joystick): %s\n", txt2);
	}

	// PARALLEL PORT GAMES
	if (strcmpi(amiberry_options.default_controller3, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport2"), _T(amiberry_options.default_controller3));
		cfgfile_parse_line(prefs, txt2, 0);
	}
	if (strcmpi(amiberry_options.default_controller4, "") != 0)
	{
		_stprintf(txt2, "%s=%s", _T("joyport3"), _T(amiberry_options.default_controller4));
		cfgfile_parse_line(prefs, txt2, 0);
	}

	//      *** GAME-SPECIFICS ***
	//  SET THE GAME COMPATIBILITY SETTINGS
	//
	// BLITTER, SPRITES, MEMORY, JIT, BIG CPU ETC

	// CPU 68020/040
	if (strcmpi(game_detail.cpu, "68020") == 0 || strcmpi(game_detail.cpu, "68040") == 0)
	{
		_stprintf(txt2, "cpu_type=%s", game_detail.cpu);
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// CPU 68000/010 [requires a600 rom)]
	if ((strcmpi(game_detail.cpu, "68000") == 0 || strcmpi(game_detail.cpu, "68010") == 0) && a600_available != 0)
	{
		_stprintf(txt2, "cpu_type=%s", game_detail.cpu);
		cfgfile_parse_line(prefs, txt2, 0);

		_stprintf(txt2, "chipmem_size=4");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// CPU SPEED
	if (strcmpi(game_detail.clock, "7") == 0)
	{
		_stprintf(txt2, "cpu_speed=real");
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else if (strcmpi(game_detail.clock, "14") == 0)
	{
		_stprintf(txt2, "finegrain_cpu_speed=1024");
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else if (strcmpi(game_detail.clock, "28") == 0)
	{
		_stprintf(txt2, "finegrain_cpu_speed=128");
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else if (strcmpi(game_detail.clock, "max") == 0)
	{
		_stprintf(txt2, "cpu_speed=max");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// COMPATIBLE CPU
	if (strcmpi(game_detail.cpu_comp, "true") == 0)
	{
		_stprintf(txt2, "cpu_compatible=true");
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else if (strcmpi(game_detail.cpu_comp, "false") == 0)
	{
		_stprintf(txt2, "cpu_compatible=false");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// COMPATIBLE CPU
	if (strcmpi(game_detail.cpu_24bit, "false") == 0 || strcmpi(game_detail.z3, "nul") != 0)
	{
		_stprintf(txt2, "cpu_24bit_addressing=false");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// FAST / Z3 MEMORY REQUIREMENTS
	int temp_ram;
	if (strcmpi(game_detail.fast, "nul") != 0)
	{
		temp_ram = atol(game_detail.fast);
		_stprintf(txt2, "fastmem_size=%d", temp_ram);
		cfgfile_parse_line(prefs, txt2, 0);
	}
	if (strcmpi(game_detail.z3, "nul") != 0)
	{
		temp_ram = atol(game_detail.z3);
		_stprintf(txt2, "z3mem_size=%d", temp_ram);
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// FAST COPPER
	if (strcmpi(game_detail.fastcopper, "true") == 0)
	{
		_stprintf(txt2, "fast_copper=true");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// BLITTER=IMMEDIATE/WAIT/NORMAL
	if (strcmpi(game_detail.blitter, "immediate") == 0)
	{
		_stprintf(txt2, "immediate_blits=true");
		cfgfile_parse_line(prefs, txt2, 0);
	}
	else if (strcmpi(game_detail.blitter, "normal") == 0)
	{
		_stprintf(txt2, "waiting_blits=disabled");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// CHIPSET OVERWRITE
	if (strcmpi(game_detail.chipset, "ocs") == 0)
	{
		prefs->cs_compatible = CP_A600;
		built_in_chipset_prefs(prefs);
		prefs->chipset_mask = 0;
	}
	else if (strcmpi(game_detail.chipset, "ecs") == 0)
	{
		prefs->cs_compatible = CP_A600;
		built_in_chipset_prefs(prefs);
		prefs->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	}
	else if (strcmpi(game_detail.chipset, "aga") == 0)
	{
		prefs->cs_compatible = CP_A1200;
		built_in_chipset_prefs(prefs);
		prefs->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	}

	// JIT
	if (strcmpi(game_detail.jit, "true") == 0)
	{
		_stprintf(txt2, "cachesize=16384");
		cfgfile_parse_line(prefs, txt2, 0);
		_stprintf(txt2, "cpu_compatible=false");
		cfgfile_parse_line(prefs, txt2, 0);
		_stprintf(txt2, "cpu_cycle_exact=false");
		cfgfile_parse_line(prefs, txt2, 0);
		_stprintf(txt2, "cpu_memory_cycle_exact=false");
		cfgfile_parse_line(prefs, txt2, 0);
		_stprintf(txt2, "address_space_24=false");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// NTSC
	if (strcmpi(game_detail.ntsc, "true") == 0)
	{
		_stprintf(txt2, "ntsc=true");
		cfgfile_parse_line(prefs, txt2, 0);
	}

	// SPRITE COLLISION
	if (strcmpi(game_detail.sprites, "nul") != 0)
	{
		_stprintf(txt2, "collision_level=%s", game_detail.sprites);
		cfgfile_parse_line(prefs, txt2, 0);
	}
}
