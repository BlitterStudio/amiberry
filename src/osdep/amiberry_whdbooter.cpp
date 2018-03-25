/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdarg>
#include <asm/sigcontext.h>
#include <csignal>
#ifndef ANDROID
#endif
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae.h"
#include "options.h"
#include "custom.h"
#include "rommgr.h"
#include "zfile.h"

#ifdef USE_SDL2
#include <map>
#endif


#ifdef WITH_LOGGING
extern FILE *debugfile;
#endif


#include "fsdb.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <cstring>

extern void SetLastActiveConfig(const char* filename);

//char start_path_data[MAX_DPATH];
//char currentDir[MAX_DPATH];
//static char config_path[MAX_DPATH];
//char last_loaded_config[MAX_DPATH] = {'\0'};

extern char currentDir[MAX_DPATH];
extern char last_loaded_config[MAX_DPATH];

#include <fstream>  /// Horace added 
#include <string> /// Horace added (to remove)

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
	TCHAR scr_height[256] = "nul\0";
	TCHAR y_offset[256] = "nul\0";
	TCHAR ntsc[256] = "nul\0";
	TCHAR chip[256] = "nul\0";
	TCHAR fast[256] = "nul\0";
	TCHAR z3[256] = "nul\0";
};

struct host_options
{
	TCHAR controller1[256] = "nul\0";
	TCHAR controller2[256] = "nul\0";
	TCHAR controller3[256] = "nul\0";
	TCHAR controller4[256] = "nul\0";
	TCHAR mouse1[256] = "nul\0";
	TCHAR mouse2[256] = "nul\0";
	TCHAR ra_quit[256] = "nul\0";
	TCHAR ra_menu[256] = "nul\0";
	TCHAR ra_reset[256] = "nul\0";
	TCHAR key_quit[256] = "nul\0";
	TCHAR key_gui[256] = "nul\0";
	TCHAR deadzone[256] = "nul\0";
	TCHAR stereo_split[256] = "nul\0";
	TCHAR sound_on[256] = "nul\0";
	TCHAR sound_mode[256] = "nul\0";
	TCHAR frameskip[256] = "nul\0";
	TCHAR aspect_ratio[256] = "nul\0";
};

static xmlNode* get_node(xmlNode* node, const char* name)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), name) == 0)
			return curr_node->children;
	}
	return nullptr;
}


static bool get_value(xmlNode* node, const char* key, char* value, int max_size)
{
	auto result = false;

	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), key) == 0)
		{
			const auto content = xmlNodeGetContent(curr_node);
			if (content != nullptr)
			{
				strncpy(value, reinterpret_cast<char *>(content), max_size);
				xmlFree(content);
				result = true;
			}
			break;
		}
	}

	return result;
}


static TCHAR* parsetext(const TCHAR* s)
{
	if (*s == '"' || *s == '\'')
	{
		const auto c = *s++;
		const auto d = my_strdup(s);
		for (auto i = 0; i < _tcslen(d); i++)
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

static TCHAR* parsetextpath(const TCHAR* s)
{
	auto s2 = parsetext(s);
	const auto s3 = target_expand_environment(s2, nullptr, 0);
	xfree(s2);
	return s3;
}


long GetFileSize(const std::string& filename)
{
	struct stat stat_buf{};
	const auto rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

void RemoveChar(char* array, int len, int index)
{
	for (auto i = index; i < len - 1; ++i)
		array[i] = array[i + 1];
	array[len - 1] = 0;
}


void parse_custom_settings(struct uae_prefs* p, char* InSettings)
{
	char temp_options[4096];
	strcpy(temp_options, InSettings);

	auto full_line = strtok(temp_options, "\n");

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


struct membuf : std::streambuf
{
	membuf(char* begin, char* end)
	{
		this->setg(begin, begin, end);
	}
};

const TCHAR* find_whdload_game_option(const TCHAR* find_setting, char* whd_options)
{
	char temp_options[4096];
	char temp_setting[4096];

	strcpy(temp_options, whd_options);
	auto output = "nul";

	auto full_line = strtok(temp_options, "\n");

	char* this_option;

	while (full_line != nullptr)
	{
		strcpy(temp_setting, find_setting);
		strcat(temp_setting, "=");

		if (strlen(full_line) >= strlen(temp_setting))
		{
			// check that the beginging of the full line
			if (strncmp(temp_setting, full_line, strlen(find_setting)) == 0)
			{
				std::string t = full_line;
				t.erase(t.begin(), t.begin() + strlen(temp_setting));
				output = &t[0u];
				return output;
			}
		}
		full_line = strtok(nullptr, "\n");
	}

	return output;
}


struct game_options get_game_settings(char* HW)
{
	struct game_options output_detail;
	strcpy(output_detail.port0, find_whdload_game_option("PORT0", HW));
	strcpy(output_detail.port1, find_whdload_game_option("PORT1", HW));
	strcpy(output_detail.control, find_whdload_game_option("PRIMARY_CONTROL", HW));
	strcpy(output_detail.control2, find_whdload_game_option("SECONDARY_CONTROL", HW));
	strcpy(output_detail.fastcopper, find_whdload_game_option("FAST_COPPER", HW));
	strcpy(output_detail.cpu, find_whdload_game_option("CPU", HW));
	strcpy(output_detail.blitter, find_whdload_game_option("BLITTER", HW));
	strcpy(output_detail.clock, find_whdload_game_option("CLOCK", HW));
	strcpy(output_detail.chipset, find_whdload_game_option("CHIPSET", HW));
	strcpy(output_detail.jit, find_whdload_game_option("JIT", HW));
	strcpy(output_detail.cpu_24bit, find_whdload_game_option("CPU_24BITADDRESSING", HW));
	strcpy(output_detail.cpu_comp, find_whdload_game_option("CPU_COMPATIBLE", HW));
	strcpy(output_detail.sprites, find_whdload_game_option("SPRITES", HW));
	strcpy(output_detail.scr_height, find_whdload_game_option("SCREEN_HEIGHT", HW));
	strcpy(output_detail.y_offset, find_whdload_game_option("SCREEN_Y_OFFSET", HW));
	strcpy(output_detail.ntsc, find_whdload_game_option("NTSC", HW));
	strcpy(output_detail.fast, find_whdload_game_option("FAST_RAM", HW));
	strcpy(output_detail.z3, find_whdload_game_option("Z3_RAM", HW));

	return output_detail;
}

struct host_options get_host_settings(char* HW)
{
	struct host_options output_detail;
	strcpy(output_detail.controller1, find_whdload_game_option("CONTROLLER_1", HW));
	strcpy(output_detail.controller2, find_whdload_game_option("CONTROLLER_2", HW));
	strcpy(output_detail.controller3, find_whdload_game_option("CONTROLLER_3", HW));
	strcpy(output_detail.controller4, find_whdload_game_option("CONTROLLER_4", HW));
	strcpy(output_detail.mouse1, find_whdload_game_option("CONTROLLER_MOUSE_1", HW));
	strcpy(output_detail.mouse2, find_whdload_game_option("CONTROLLER_MOUSE_2", HW));
	strcpy(output_detail.ra_quit, find_whdload_game_option("RETROARCH_QUIT", HW));
	strcpy(output_detail.ra_menu, find_whdload_game_option("RETROARCH_MENU", HW));
	strcpy(output_detail.ra_reset, find_whdload_game_option("RETROARCH_RESET", HW));
	strcpy(output_detail.key_quit, find_whdload_game_option("KEY_FOR_QUIT", HW));
	strcpy(output_detail.key_gui, find_whdload_game_option("KEY_FOR_MENU", HW));
	strcpy(output_detail.deadzone, find_whdload_game_option("DEADZONE", HW));
	strcpy(output_detail.stereo_split, find_whdload_game_option("STEREO_SPLIT", HW));
	strcpy(output_detail.sound_on, find_whdload_game_option("SOUND_ON", HW));
	strcpy(output_detail.sound_mode, find_whdload_game_option("SOUND_MODE", HW));
	strcpy(output_detail.aspect_ratio, find_whdload_game_option("ASPECT_RATIO_FIX", HW));
	strcpy(output_detail.frameskip, find_whdload_game_option("FRAMESKIP", HW));

	return output_detail;
}

void symlink_roms(struct uae_prefs* p)

{
	//      *** KICKSTARTS ***
	//     
	char kick_path[MAX_DPATH];
        char kick_long[MAX_DPATH];
	int rom_test;
	int roms[2];

	// here we can do some checks for Kickstarts we might need to make symlinks for
        strncpy(currentDir, start_path_data, MAX_DPATH);
                
                
        // are we using save-data/ ?
        snprintf(kick_path, MAX_DPATH, "%s/whdboot/save-data/Kickstarts", start_path_data);

        // otherwise, use the old route
 	if (!my_existsdir(kick_path))
            snprintf(kick_path, MAX_DPATH, "%s/whdboot/game-data/Devs/Kickstarts", start_path_data);

        
        // do the checks...
	snprintf(kick_long, MAX_DPATH, "%s/kick33180.A500", kick_path);
	if (!zfile_exists(kick_long))
	{
		roms[0] = 5; // kickstart 1.2 A500
		rom_test = configure_rom(p, roms, 0); // returns 0 or 1 if found or not found
		if (rom_test == 1)
			symlink(p->romfile, kick_long);
	}

	snprintf(kick_long, MAX_DPATH, "%s/kick34005.A500", kick_path);
	if (!zfile_exists(kick_long))
	{
		roms[0] = 6; // kickstart 1.3 A500
		rom_test = configure_rom(p, roms, 0); // returns 0 or 1 if found or not found
		//printf(p->romfile);
		//printf("result: %d\n", rom_test);

		if (rom_test == 1)
			symlink(p->romfile, kick_long);
	}

	snprintf(kick_long, MAX_DPATH, "%s/kick40068.A1200", kick_path);
	if (!zfile_exists(kick_long))
	{
		roms[0] = 15; // kickstart 3.1 A1200
		rom_test = configure_rom(p, roms, 0); // returns 0 or 1 if found or not found
		if (rom_test == 1)
			symlink(p->romfile, kick_long);
	}
}


void whdload_auto_prefs(struct uae_prefs* p, char* filepath)

{
	// setup variables etc
	TCHAR game_name[MAX_DPATH];
	TCHAR* txt2;
	TCHAR tmp[MAX_DPATH];
	TCHAR tmp2[MAX_DPATH];

	char boot_path[MAX_DPATH];
        char save_path[MAX_DPATH];
	char config_path[MAX_DPATH];
	// char GameTypePath[MAX_DPATH];
	char whd_config[255];
	int* type;
	auto config_type = CONFIG_TYPE_ALL;

	char hardware_settings[4096];
	char custom_settings[4096];

	fetch_configurationpath(config_path,MAX_DPATH);

	//     
	//      *** KICKSTARTS ***

	symlink_roms(p);


	// this allows A600HD to be used to slow games down
	int roms[2];
	roms[0] = 15; // kickstart 2.05 A600HD  ..  10
	const auto rom_test = configure_rom(p, roms, 0); // returns 0 or 1 if found or not found
	const auto a600_available = rom_test;


	//     
	//      *** GAME DETECTION ***

	// REMOVE THE FILE PATH AND EXTENSION
	const auto filename = my_getfilepart(filepath);
	// SOMEWHERE HERE WE NEED TO SET THE GAME 'NAME' FOR SAVESTATE ETC PURPOSES
	extractFileName(filepath, last_loaded_config);
	extractFileName(filepath, game_name);
	removeFileExtension(game_name);

	auto filesize = GetFileSize(filepath);
	// const TCHAR* filesha = get_sha1_txt (input, filesize); <<< ??! FIX ME


	// LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE   
	//  CONFIG LOAD IF .UAE IS IN CONFIG PATH  
	strcpy(whd_config, config_path);
	strcat(whd_config, game_name);
	strcat(whd_config, ".uae");


	if (zfile_exists(whd_config))
	{
		target_cfgfile_load(&currprefs, whd_config, CONFIG_TYPE_ALL, 0);
		return;
	}

	// LOAD HOST OPTIONS
	char whd_path[MAX_DPATH];
	struct host_options host_detail;
	snprintf(whd_path, MAX_DPATH, "%s/whdboot/", start_path_data);

	strcpy(whd_config, whd_path);
	strcat(whd_config, "hostprefs.conf");

	if (zfile_exists(whd_config)) // use direct .whd file
	{
		ifstream read_file(whd_config);
		std::ifstream in(whd_config);
		std::string contents((std::istreambuf_iterator<char>(in)),
		                     std::istreambuf_iterator<char>());

		_stprintf(hardware_settings, "%s", contents.c_str());

		host_detail = get_host_settings(hardware_settings);
	}


	// LOAD GAME SPECIFICS - USE SHA1 IF AVAILABLE
	snprintf(whd_path, MAX_DPATH, "%s/whdboot/game-data/", start_path_data);
	struct game_options game_detail;


	// EDIT THE FILE NAME TO USE HERE
	strcpy(whd_config, whd_path);
	strcat(whd_config, game_name);
	strcat(whd_config, ".whd");


	if (zfile_exists(whd_config)) // use direct .whd file
	{
		ifstream readFile(whd_config);
		std::ifstream in(whd_config);
		std::string contents((std::istreambuf_iterator<char>(in)),
		                     std::istreambuf_iterator<char>());

		_stprintf(hardware_settings, "%s", contents.c_str());
		game_detail = get_game_settings(hardware_settings);
	}
	else
	{
		strcpy(whd_config, whd_path);
		strcat(whd_config, "whdload_db.xml");

		if (zfile_exists(whd_config)) // use XML database 
		{
                        //printf("XML exists %s\n",game_name); 
                                
                        char buffer[4096];

			const auto doc = xmlParseFile(whd_config);

			const auto root_element = xmlDocGetRootElement(doc);

			auto game_node = get_node(root_element, "whdbooter");

			while (game_node != nullptr)
			{
				const auto attr = xmlGetProp(game_node, reinterpret_cast<const xmlChar *>("filename"));
				if (attr != nullptr)
				{
                                       // printf ("%s\n",attr);
                                    	if (strcmpi(reinterpret_cast<const char*>(attr),game_name) == 0)
					{
						// now get the <hardware> and <custom_controls> items

                                                //printf("found game in XML?\n");
						auto temp_node = game_node->xmlChildrenNode;
						temp_node = get_node(temp_node, "hardware");
						if (xmlNodeGetContent(temp_node) != nullptr)
						{ 
							_stprintf(hardware_settings, "%s", xmlNodeGetContent(temp_node));
                                                        // printf("%s\n",hardware_settings);
							game_detail = get_game_settings(hardware_settings);
                                                        
						}

						temp_node = game_node->xmlChildrenNode;
						temp_node = get_node(temp_node, "custom_controls");
						if (xmlNodeGetContent(temp_node) != nullptr)
						{
							_stprintf(custom_settings, "%p", xmlNodeGetContent(temp_node));
							//  process these later
						}
						break;
					}
				}
				xmlFree(attr);
				game_node = game_node->next;
			}

			xmlCleanupParser();
		}
	}

	// debugging code!
	//        printf("port 0: %s  \n",game_detail.port0); 
	//        printf("port 1: %s  \n",game_detail.port1); 
	//        printf("contrl: %s  \n",game_detail.control);  
	 //       printf("fstcpr: %s  \n",game_detail.fastcopper);  
	 //       printf("cpu   : %s  \n",game_detail.cpu);  
	 //       printf("blitta: %s  \n",game_detail.blitter);  
	 //       printf("clock : %s  \n",game_detail.clock);  
	 //       printf("chipst: %s  \n",game_detail.chipset);  
	//        printf("jit   : %s  \n",game_detail.jit);  
	//        printf("cpcomp: %s  \n",game_detail.cpu_comp);  
	//        printf("scrhei: %s  \n",game_detail.scr_height);   
	//        printf("scr y : %s  \n",game_detail.y_offset);  
	//        printf("ntsc  : %s  \n",game_detail.ntsc);  
	 //       printf("fast  : %s  \n",game_detail.fast);  
	 //       printf("z3    : %s  \n",game_detail.z3);      

	// debugging code!
	//printf("cont 1: %s  \n", host_detail.controller1);
	//printf("cont 2: %s  \n", host_detail.controller2);
	//printf("cont 3: %s  \n", host_detail.controller3);
	//printf("cont 4: %s  \n", host_detail.controller4);
	//printf("mous 1: %s  \n", host_detail.mouse1);
	//printf("mous 2: %s  \n", host_detail.mouse2);
	//printf("ra_qui: %s  \n", host_detail.ra_quit);
	//printf("ra_men: %s  \n", host_detail.ra_menu);
	//printf("ra_rst: %s  \n", host_detail.ra_reset);
	//printf("ky_qut: %s  \n", host_detail.key_quit);
	//printf("ky_gui: %s  \n", host_detail.key_gui);
	//printf("deadzn: %s  \n", host_detail.stereo_split);
	//printf("stereo: %s  \n", host_detail.stereo_split);
	//printf("snd_on: %s  \n", host_detail.sound_on);
	//printf("snd_md: %s  \n", host_detail.sound_mode);
	//printf("aspect: %s  \n", host_detail.aspect_ratio);
	//printf("frames: %s  \n", host_detail.frameskip);


	//     
	//      *** EMULATED HARDWARE ***
	//            

	// SET UNIVERSAL DEFAULTS
	p->start_gui = false;


	if ((strcmpi(game_detail.cpu,"68000") == 0 || strcmpi(game_detail.cpu,"68010") == 0) && a600_available != 0)
		// SET THE BASE AMIGA (Expanded A600)
	{
		built_in_prefs(&currprefs, 2, 2, 0, 0);
		_stprintf(txt2, "chipmem_size=4");
		cfgfile_parse_line(p, txt2, 0);
	}
	else
		// SET THE BASE AMIGA (Expanded A1200)
	{
		built_in_prefs(&currprefs, 3, 1, 0, 0);
		if ((strcmpi(game_detail.fast,"nul") != 0) && (strcmpi(game_detail.cpu,"nul") == 0))
			strcpy(game_detail.cpu,_T("68020"));
	}

	// DO CHECKS FOR AGA / CD32
	const int is_aga = (strstr(filename, "_AGA") != nullptr || strcmpi(game_detail.chipset,"AGA") == 0);
	const int is_cd32 = (strstr(filename, "_CD32") != nullptr || strcmpi(game_detail.chipset,"CD32") == 0);

	// A1200 no AGA
	if (!static_cast<bool>(is_aga) && !static_cast<bool>(is_cd32))
	{
		_tcscpy(p->description, _T("WHDLoad AutoBoot Configuration"));

		p->cs_compatible = CP_A600;
		built_in_chipset_prefs(p);
		p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
		p->m68k_speed = 0;
	}
		// A1200
	else
		_tcscpy(p->description, _T("WHDLoad AutoBoot Configuration [AGA]"));

        
        

	//SET THE WHD BOOTER AND GAME DATA  
	snprintf(boot_path, MAX_DPATH, "%s/whdboot/boot-data.zip", start_path_data);

        if (!zfile_exists(boot_path))
            snprintf(boot_path, MAX_DPATH, "%s/whdboot/boot-data/", start_path_data);               
       
            
        
	// set the first (whdboot) Drive
	_stprintf(tmp,_T("filesystem2=rw,DH0:DH0:%s,10"), boot_path);
	txt2 = parsetextpath(_T(tmp));
	cfgfile_parse_line(p, txt2, 0);

	_stprintf(tmp,_T("uaehf0=dir,rw,DH0:DH0::%s,10"), boot_path);
	txt2 = parsetextpath(_T(tmp));
	cfgfile_parse_line(p, txt2, 0);

	//set the Second (game data) drive
	_stprintf(tmp, "filesystem2=rw,DH1:games:%s,0", filepath);
	txt2 = parsetextpath(_T(tmp));
	cfgfile_parse_line(p, txt2, 0);

	_stprintf(tmp, "uaehf1=dir,rw,DH1:games:%s,0", filepath);
	txt2 = parsetextpath(_T(tmp));
	cfgfile_parse_line(p, txt2, 0);

	//set the third (save data) drive
        snprintf(save_path, MAX_DPATH, "%s/whdboot/save-data/", start_path_data);
                
        if (my_existsdir(save_path))
        {
            _stprintf(tmp, "filesystem2=rw,DH2:saves:%s,0", save_path);
            txt2 = parsetextpath(_T(tmp));
            cfgfile_parse_line(p, txt2, 0);

            _stprintf(tmp, "uaehf2=dir,rw,DH2:saves:%s,0", save_path);
            txt2 = parsetextpath(_T(tmp));
            cfgfile_parse_line(p, txt2, 0);
        }
        
        
        
        
        
        
	//APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
	// CD32
	if ((static_cast<bool>(is_cd32) && strcmpi(game_detail.port0,"nul") == 0)
		|| strcmpi(game_detail.port0,"cd32") == 0)
		p->jports[0].mode = 7;

	if ((static_cast<bool>(is_cd32) && strcmpi(game_detail.port1,"nul") == 0)
		|| strcmpi(game_detail.port1,"cd32") == 0)
		p->jports[1].mode = 7;

	// JOY
	if (strcmpi(game_detail.port0,"joy") == 0)
		p->jports[0].mode = 3;
	if (strcmpi(game_detail.port1,"joy") == 0)
		p->jports[1].mode = 3;

	// MOUSE
	if (strcmpi(game_detail.port0,"mouse") == 0)
		p->jports[0].mode = 2;
	if (strcmpi(game_detail.port1,"mouse") == 0)
		p->jports[1].mode = 2;

	// APPLY SPECIAL CONFIG E.G. MOUSE OR ALT. JOYSTICK SETTINGS   
	for (auto& jport : p->jports)
	{
		jport.id = JPORT_NONE;
		jport.idc.configname[0] = 0;
		jport.idc.name[0] = 0;
		jport.idc.shortid[0] = 0;
	}


	// WHAT IS THE MAIN CONTROL?
	// MOUSE GAMES    
	if (strcmpi(game_detail.control,"mouse") == 0 && strcmpi(host_detail.mouse1,"nul") != 0)
	{
		_stprintf(txt2, "%s=%s",_T("joyport0"),_T(host_detail.mouse1));
		cfgfile_parse_line(p, txt2, 0);
	}

	if (strcmpi(game_detail.control,"mouse") == 0 && strcmpi(host_detail.mouse2,"nul") != 0)
	{
		_stprintf(txt2, "%s=%s",_T("joyport1"),_T(host_detail.mouse2));
		cfgfile_parse_line(p, txt2, 0);
	}

	// JOYSTICK GAMES     
	if (!strcmpi(game_detail.control,"mouse") == 0 && strcmpi(host_detail.controller1,"nul") != 0)
	{
		_stprintf(txt2, "%s=%s",_T("joyport1"),_T(host_detail.controller1));
		cfgfile_parse_line(p, txt2, 0);
	}

	if (!strcmpi(game_detail.control,"mouse") == 0 && strcmpi(host_detail.controller2,"nul") != 0)
	{
		_stprintf(txt2, "%s=%s",_T("joyport0"),_T(host_detail.controller2));
		cfgfile_parse_line(p, txt2, 0);
	}

	// PARALLEL PORT GAMES  
	if (strcmpi(host_detail.controller3,"nul") != 0)
	{
		_stprintf(txt2, "%s=%s",_T("joyport2"),_T(host_detail.controller3));
		cfgfile_parse_line(p, txt2, 0);
	}
	if (strcmpi(host_detail.controller4,"nul") != 0)
	{
		_stprintf(txt2, "%s=%s",_T("joyport3"),_T(host_detail.controller4));
		cfgfile_parse_line(p, txt2, 0);
	}

	// CUSTOM CONTROLS
	if (strlen(custom_settings) > 0)
		parse_custom_settings(p, custom_settings);

	if (!strcmpi(host_detail.deadzone,"nul") == 0)
	{
		_stprintf(txt2, "input.joymouse_deadzone=%s",_T(host_detail.deadzone));
		cfgfile_parse_line(p, txt2, 0);
		_stprintf(txt2, "input.joystick_deadzone=%s",_T(host_detail.deadzone));
		cfgfile_parse_line(p, txt2, 0);
	}


	// RETROARCH CONTROLS
	if (!strcmpi(host_detail.ra_quit,"nul") == 0)
	{
		_stprintf(txt2, "amiberry.use_retroarch_quit=%s",_T(host_detail.ra_quit));
		cfgfile_parse_line(p, txt2, 0);
	}
	if (!strcmpi(host_detail.ra_menu,"nul") == 0)
	{
		_stprintf(txt2, "amiberry.use_retroarch_menu=%s",_T(host_detail.ra_menu));
		cfgfile_parse_line(p, txt2, 0);
	}
	if (!strcmpi(host_detail.ra_reset,"nul") == 0)
	{
		_stprintf(txt2, "amiberry.use_retroarch_reset=%s",_T(host_detail.ra_reset));
		cfgfile_parse_line(p, txt2, 0);
	}
	// KEYBOARD CONTROLS

	if (!strcmpi(host_detail.key_quit,"nul") == 0)
	{
		_stprintf(txt2, "amiberry.quit_amiberry=%s",_T(host_detail.key_quit));
		cfgfile_parse_line(p, txt2, 0);
	}
	if (!strcmpi(host_detail.key_gui,"nul") == 0)
	{
		_stprintf(txt2, "amiberry.open_gui=%s",_T(host_detail.key_gui));
		cfgfile_parse_line(p, txt2, 0);
	}
	// GRAPHICS OPTIONS

	if (!strcmpi(host_detail.aspect_ratio,"nul") == 0)
	{
		_stprintf(txt2, "amiberry.gfx_correct_aspect=%s",_T(host_detail.aspect_ratio));
		cfgfile_parse_line(p, txt2, 0);
	}
	if (!strcmpi(host_detail.frameskip,"nul") == 0)
	{
		_stprintf(txt2, "gfx_framerate=%s",_T(host_detail.frameskip));
		cfgfile_parse_line(p, txt2, 0);
	}
	// SOUND OPTIONS

	if (strcmpi(host_detail.sound_on,"false") == 0 || strcmpi(host_detail.sound_on,"off") == 0 || strcmpi(host_detail.sound_on,"none") == 0)
	{
		_stprintf(txt2, "sound_output=none");
		cfgfile_parse_line(p, txt2, 0);
	}
	if (!strcmpi(host_detail.stereo_split,"nul") == 0)
	{
		_stprintf(txt2, "sound_stereo_separation=%s",_T(host_detail.stereo_split));
		cfgfile_parse_line(p, txt2, 0);
	}

	//      *** GAME-SPECIFICS ***
	//  SET THE GAME COMPATIBILITY SETTINGS   
	// 
	// SCREEN HEIGHT, BLITTER, SPRITES, MEMORY, JIT, BIG CPU ETC  


	// CPU 68020/040
	if (strcmpi(game_detail.cpu,"68020") == 0 || strcmpi(game_detail.cpu,"68040") == 0)
	{
		_stprintf(txt2, "cpu_type=%s", game_detail.cpu);
		cfgfile_parse_line(p, txt2, 0);
	}

	// CPU 68000/010 [requires a600 rom)]
	if ((strcmpi(game_detail.cpu,"68000") == 0 || strcmpi(game_detail.cpu,"68010") == 0) && a600_available != 0)
	{
		_stprintf(txt2, "cpu_type=%s", game_detail.cpu);
		cfgfile_parse_line(p, txt2, 0);
	}


	// CPU SPEED
	if (strcmpi(game_detail.clock,"7") == 0)
	{
		_stprintf(txt2, "cpu_speed=real");
		cfgfile_parse_line(p, txt2, 0);
	}
	else if (strcmpi(game_detail.clock,"14") == 0)
	{
		_stprintf(txt2, "finegrain_cpu_speed=1024");
		cfgfile_parse_line(p, txt2, 0);
	}
	else if (strcmpi(game_detail.clock,"28") == 0)
	{
		_stprintf(txt2, "finegrain_cpu_speed=128");
		cfgfile_parse_line(p, txt2, 0);
	}
	else if (strcmpi(game_detail.clock,"max") == 0)
	{
		_stprintf(txt2, "cpu_speed=max");
		cfgfile_parse_line(p, txt2, 0);
	}
	else if (strcmpi(game_detail.clock,"turbo") == 0)
	{
		_stprintf(txt2, "cpu_speed=turbo");
		cfgfile_parse_line(p, txt2, 0);
	}

	//FAST / Z3 MEMORY REQUIREMENTS

	int temp_ram;
	if (strcmpi(game_detail.fast,"nul") != 0)
	{
		temp_ram = atol(game_detail.fast);
		_stprintf(txt2, "fastmem_size=%d", temp_ram);
		cfgfile_parse_line(p, txt2, 0);
	}
	if (strcmpi(game_detail.z3,"nul") != 0)
	{
		temp_ram = atol(game_detail.z3);
		_stprintf(txt2, "z3mem_size=%d", temp_ram);
		cfgfile_parse_line(p, txt2, 0);
	}


	// FAST COPPER  
	if (strcmpi(game_detail.fastcopper,"true") == 0)
	{
		_stprintf(txt2, "fast_copper=true");
		cfgfile_parse_line(p, txt2, 0);
	}

	// BLITTER=IMMEDIATE/WAIT/NORMAL
	if (strcmpi(game_detail.blitter,"immediate") == 0)
	{
		_stprintf(txt2, "immediate_blits=true");
		cfgfile_parse_line(p, txt2, 0);
	}
	else if (strcmpi(game_detail.blitter,"normal") == 0)
	{
		_stprintf(txt2, "waiting_blits=disabled");
		cfgfile_parse_line(p, txt2, 0);
	}

	// CHIPSET OVERWRITE
	if (strcmpi(game_detail.chipset,"ocs") == 0)
	{
		p->cs_compatible = CP_A600;
		built_in_chipset_prefs(p);
		p->chipset_mask = 0;
	}

	else if (strcmpi(game_detail.chipset,"ecs") == 0)
	{
		p->cs_compatible = CP_A600;
		built_in_chipset_prefs(p);
		p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	}

	else if (strcmpi(game_detail.chipset,"aga") == 0)
	{
		p->cs_compatible = CP_A1200;
		built_in_chipset_prefs(p);
		p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	}

	// JIT
	if (strcmpi(game_detail.jit,"true") == 0)
	{
		_stprintf(txt2, "cachesize=8192");
		cfgfile_parse_line(p, txt2, 0);
	}

	// COMPATIBLE CPU
	if (strcmpi(game_detail.cpu_comp,"true") == 0)
	{
		_stprintf(txt2, "cpu_compatible=true");
		cfgfile_parse_line(p, txt2, 0);
	}

	// COMPATIBLE CPU
	if (strcmpi(game_detail.cpu_comp,"false") == 0)
	{
		_stprintf(txt2, "cpu_24bit_addressing=false");
		cfgfile_parse_line(p, txt2, 0);
	}

        
	// NTSC 
	if (strcmpi(game_detail.ntsc,"true") == 0)
	{
		_stprintf(txt2, "ntsc=true");
		cfgfile_parse_line(p, txt2, 0);
	}

	// NTSC 
	if (strcmpi(game_detail.ntsc,"true") == 0)
	{
		_stprintf(txt2, "ntsc=true");
		cfgfile_parse_line(p, txt2, 0);
	}

	// SCREEN HEIGHT 
	if (strcmpi(game_detail.scr_height,"nul") != 0)
	{
		_stprintf(txt2, "gfx_height=%s", game_detail.scr_height);
		cfgfile_parse_line(p, txt2, 0);
		_stprintf(txt2, "gfx_height_windowed=%s", game_detail.scr_height);
		cfgfile_parse_line(p, txt2, 0);
		_stprintf(txt2, "gfx_height_fullscreen=%s", game_detail.scr_height);
		cfgfile_parse_line(p, txt2, 0);
	}

	// Y OFFSET
	if (strcmpi(game_detail.y_offset,"nul") != 0)
	{
		_stprintf(txt2, "amiberry.vertical_offset=%s", game_detail.y_offset);
		cfgfile_parse_line(p, txt2, 0);
	}

	// SPRITE COLLISION
	if (strcmpi(game_detail.scr_height,"nul") != 0)
	{
		_stprintf(txt2, "collision_level=%s", game_detail.sprites);
		cfgfile_parse_line(p, txt2, 0);
	}


	//  CLEAN UP SETTINGS 
	//   fixup_prefs(&currprefs, true);    
	//   cfgfile_configuration_change(1); 
}
