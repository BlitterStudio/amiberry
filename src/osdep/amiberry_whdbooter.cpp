/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdarg>
#include <asm/sigcontext.h>
#include <csignal>
#include <dlfcn.h>
#ifndef ANDROID
#include <execinfo.h>
#endif
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "custom.h"
#include "inputdevice.h"
#include "disk.h"
#include "savestate.h"
#include "rommgr.h"
#include "zfile.h"
#include <SDL.h>
#include "amiberry_rp9.h"
#include "machdep/rpt.h"

#include "threaddep/thread.h"
#include "include/memory.h"
#include "keyboard.h"
#include "rtgmodes.h"
#include "gfxboard.h"
#include "amiberry_gfx.h"
#ifdef USE_SDL2
#include <map>
#endif

#ifdef WITH_LOGGING
extern FILE *debugfile;
#endif


#include "crc32.h"
#include "fsdb.h"


extern void SetLastActiveConfig(const char* filename);

//char start_path_data[MAX_DPATH];
//char currentDir[MAX_DPATH];
//static char config_path[MAX_DPATH];
//char last_loaded_config[MAX_DPATH] = {'\0'};

extern char start_path_data[MAX_DPATH];
extern char currentDir[MAX_DPATH];
extern char last_loaded_config[MAX_DPATH];

#include "zfile.h"  /// Horace added
#include <fstream>  /// Horace added 
#include <string> /// Horace added (to remove)
struct game_options {
       TCHAR port0[256]     = "nul\0";
       TCHAR port1[256]     = "nul\0";
       TCHAR control[256]     = "nul\0";
       TCHAR fastcopper[256] = "nul\0";
       TCHAR cpu[256]       = "nul\0";
       TCHAR blitter[256]   = "nul\0";
       TCHAR clock[256]     = "nul\0";
       TCHAR chipset[256]   = "nul\0";
       TCHAR jit[256]       = "nul\0";
       TCHAR cpu_comp[256]  = "nul\0";
       TCHAR sprites[256]   = "nul\0";
       TCHAR scr_height[256] = "nul\0";
       TCHAR y_offset[256]  = "nul\0";
       TCHAR ntsc[256]      = "nul\0";
       TCHAR chip[256]      = "nul\0";
       TCHAR fast[256]      = "nul\0";
       TCHAR z3[256]       = "nul\0";
};



static TCHAR *parsetext(const TCHAR *s)
{
	if (*s == '"' || *s == '\'') {
		TCHAR *d;
		TCHAR c = *s++;
		int i;
		d = my_strdup(s);
		for (i = 0; i < _tcslen(d); i++) {
			if (d[i] == c) {
				d[i] = 0;
				break;
			}
		}
		return d;
	}
	else {
		return my_strdup(s);
	}
}

static TCHAR *parsetextpath(const TCHAR *s)
{
	TCHAR *s2 = parsetext(s);
	TCHAR *s3 = target_expand_environment(s2, NULL, 0);
	xfree(s2);
	return s3;
}


long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void RemoveChar(char* array, int len, int index)
{
  for(int i = index; i < len-1; ++i)
    array[i] = array[i+1];
  array[len-1] = 0;
}



const TCHAR* find_whdload_game_option(const TCHAR* find_setting, char* whd_game_file)
{
	// opening file and parsing
	ifstream readFile(whd_game_file);
	string line;
	string delimiter = "=";

	auto output = "nul";

	// read each line in
	while (std::getline(readFile, line))
	{
		const auto option = line.substr(0, line.find(delimiter));

		if (option != line) // exit if we got no result from splitting the string
		{
			// using the " = " to work out which is the option, and which is the parameter.
			auto param = line.substr(line.find(delimiter) + delimiter.length(), line.length());

			if (!param.empty())
			{
				// remove leading "
				if (param.at(0) == '"')
					param.erase(0, 1);

				// remove trailing "
				if (param.at(param.length() - 1) == '"')
					param.erase(param.length() - 1, 1);

				output = &param[0u];

				// ok, this is the 'normal' storing of values
				if (option == find_setting)
					break;

				output = "nul";
			}
		}
	}

	return output;
}

void whdload_auto_prefs (struct uae_prefs* p, char* filepath)

{
 // setup variables etc
    TCHAR game_name[MAX_DPATH];;
    TCHAR *txt2;
    TCHAR tmp[MAX_DPATH];
    char BootPath[MAX_DPATH];
    char KickPath[MAX_DPATH];
    char ConfigPath[MAX_DPATH];
    char GameTypePath[MAX_DPATH];
    char WHDConfig[255];
    int rom_test;    
    int roms[2];   
    int *type;
    auto config_type = CONFIG_TYPE_ALL;
     
       fetch_configurationpath(ConfigPath,MAX_DPATH);
            
 //     
//      *** KICKSTARTS ***
 //            
  // here we can do some checks for Kickstarts we might need to make symlinks for
 	strncpy(currentDir, start_path_data, MAX_DPATH);
        snprintf(KickPath, MAX_DPATH, "%s/whdboot/boot-data/Devs/Kickstarts/kick33180.A500", start_path_data);

        if (zfile_exists(KickPath))
        {   roms[0] = 6;   // kickstart 1.2 A500
            roms[1] = -1;
            rom_test = configure_rom(p, roms, 0);  // returns 0 or 1 if found or not found
        // if (rom_test == 1)
        // create symlink <<< ERRR... ?
        }
        
        snprintf(KickPath, MAX_DPATH, "%s/whdboot/boot-data/Devs/Kickstarts/kick34005.A500", start_path_data);
        if (zfile_exists(KickPath))
        {   roms[0] = 5;   // kickstart 1.3 A500
            rom_test = configure_rom(p, roms, 0);  // returns 0 or 1 if found or not found
        // if (rom_test == 1)
        // create symlink <<< ERRR... ?
        }
        
        snprintf(KickPath, MAX_DPATH, "%s/whdboot/boot-data/Devs/Kickstarts/kick40068.A1200", start_path_data);
        if (zfile_exists(KickPath))
        {   roms[0] = 15;  // kickstart 3.1 A1200
            rom_test = configure_rom(p, roms, 0);  // returns 0 or 1 if found or not found
        // if (rom_test == 1)
        // create symlink <<< ERRR... ?
        }
        
    // this allows A600HD to be used to slow games down
        roms[0] = 9;   // kickstart 2.05 A600HD  ..  10
        rom_test = configure_rom(p, roms, 0);  // returns 0 or 1 if found or not found
        const int a600_available = rom_test;

        printf ("a600: %d\n",a600_available);
//     
//      *** GAME DETECTION ***
   
   // REMOVE THE FILE PATH AND EXTENSION
        const TCHAR* filename = my_getfilepart(filepath);
   // SOMEWHERE HERE WE NEED TO SET THE GAME 'NAME' FOR SAVESTATE ETC PURPOSES
        extractFileName(filepath, last_loaded_config);
        extractFileName(filepath, game_name);
        removeFileExtension(game_name);
                 
    // find the SHA1 - this currently does not return the correct result!! 
        long filesize;   
        filesize = GetFileSize(filepath);
     // const TCHAR* filesha = get_sha1_txt (input, filesize); <<< ??! FIX ME

        
        
     // LOAD GAME SPECIFICS FOR EXISTING .UAE - USE SHA1 IF AVAILABLE   
        //  CONFIG LOAD IF .UAE IS IN CONFIG PATH  
            strcpy(WHDConfig, ConfigPath);
            strcat(WHDConfig, game_name);
            strcat(WHDConfig,".uae");  
            
            
       printf("cgf: %s  \n",WHDConfig);
            
        if (zfile_exists(WHDConfig))
        {        target_cfgfile_load(&currprefs, WHDConfig,  CONFIG_TYPE_ALL, 0);
                 return;}

        
        
     // LOAD GAME SPECIFICS - USE SHA1 IF AVAILABLE
      	char WHDPath[MAX_DPATH];
	snprintf(WHDPath, MAX_DPATH, "%s/whdboot/game-data/", start_path_data);          
          
       // EDIT THE FILE NAME TO USE HERE
 	strcpy(WHDConfig, WHDPath);
        strcat(WHDConfig,game_name);
        strcat(WHDConfig,".whd");     
        
        printf("game-db: %s  \n",WHDConfig);

        struct game_options game_detail;

       if (zfile_exists(WHDConfig))
        {
             strcpy(game_detail.port0,       find_whdload_game_option("PORT0",WHDConfig));
             strcpy(game_detail.port1,       find_whdload_game_option("PORT1",WHDConfig));
             strcpy(game_detail.control,     find_whdload_game_option("PRIMARY_CONTROL",WHDConfig));            
             strcpy(game_detail.fastcopper,  find_whdload_game_option("FAST_COPPER",WHDConfig));
             strcpy(game_detail.cpu,         find_whdload_game_option("CPU",WHDConfig));
             strcpy(game_detail.blitter,     find_whdload_game_option("BLITTER",WHDConfig));             
             strcpy(game_detail.clock,       find_whdload_game_option("CLOCK",WHDConfig));
             strcpy(game_detail.chipset,     find_whdload_game_option("CHIPSET",WHDConfig));
             strcpy(game_detail.jit,         find_whdload_game_option("JIT",WHDConfig));
             strcpy(game_detail.cpu_comp,    find_whdload_game_option("CPU_COMPATIBLE",WHDConfig));
             strcpy(game_detail.sprites,     find_whdload_game_option("SPRITES",WHDConfig));            
             strcpy(game_detail.scr_height,  find_whdload_game_option("SCREEN_HEIGHT",WHDConfig));             
             strcpy(game_detail.y_offset,    find_whdload_game_option("SCREEN_Y_OFFSET",WHDConfig));
             strcpy(game_detail.ntsc,        find_whdload_game_option("NTSC",WHDConfig));
             strcpy(game_detail.chip,        find_whdload_game_option("CHIP_RAM",WHDConfig));
             strcpy(game_detail.fast,        find_whdload_game_option("FAST_RAM",WHDConfig));
             strcpy(game_detail.z3,          find_whdload_game_option("Z3_RAM",WHDConfig));            
        }
        
        
        printf("port 0: %s  \n",game_detail.port0); 
        printf("port 1: %s  \n",game_detail.port1); 
        

  //     
//      *** EMULATED HARDWARE ***
 //            

   // SET UNIVERSAL DEFAULTS
        p->start_gui = false;
    
        
   // APPLY SPECIAL CONFIG E.G. MOUSE OR ALT. JOYSTICK SETTINGS   
    // if mouse game        
        if (strcmpi(game_detail.control,"mouse") != 0)
            snprintf(GameTypePath, MAX_DPATH, "%s/whdboot/default_joystick.uae", start_path_data);    
    // if joystick game
        else
            snprintf(GameTypePath, MAX_DPATH, "%s/whdboot/default_mouse.uae", start_path_data);
        
            
                
        if (zfile_exists(GameTypePath))
        {
                for (auto i = 0; i < MAX_JPORTS; i++)
		{
                    p->jports[i].id = JPORT_NONE;
                    p->jports[i].idc.configname[0] = 0;
                    p->jports[i].idc.name[0] = 0;
                    p->jports[i].idc.shortid[0] = 0;
                }
                cfgfile_load(p, GameTypePath, type, 0, 1);
        }           
           

    if ((strcmpi(game_detail.cpu,"68000") == 0 || strcmpi(game_detail.cpu,"68010") == 0) && a600_available != 0)
    // SET THE BASE AMIGA (Expanded A600)
    {   built_in_prefs(&currprefs, 2, 2, 0, 0);
        _stprintf(txt2,"chipmem_size=4");
              cfgfile_parse_line(p, txt2, 0); 
    }
    else
    // SET THE BASE AMIGA (Expanded A1200)
    {    built_in_prefs(&currprefs, 3, 1, 0, 0);
        if  ((strcmpi(game_detail.fast,"nul") != 0) && (strcmpi(game_detail.cpu,"nul") == 0))
                strcpy(game_detail.cpu,_T("68020"));
    }
        
   // DO CHECKS FOR AGA / CD32
        const int is_aga = (strstr(filename,"_AGA") != NULL || strcmpi(game_detail.chipset,"AGA") == 0);
        const int is_cd32 = (strstr(filename,"_CD32") != NULL || strcmpi(game_detail.chipset,"CD32") == 0);
 
   // A1200 no AGA
       if (is_aga == false && is_cd32 == false)   
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
        snprintf(BootPath, MAX_DPATH, "%s/whdboot/boot-data/", start_path_data);     
    
      // set the first (whdboot) Drive
         _stprintf(tmp,_T("filesystem2=rw,DH0:DH0:%s,10"),BootPath);   
         txt2 = parsetextpath(_T(tmp));
         cfgfile_parse_line(p, txt2, 0);

         _stprintf(tmp,_T("uaehf0=dir,rw,DH0:DH0::%s,10") , BootPath);
         txt2 = parsetextpath(_T(tmp));
         cfgfile_parse_line(p, txt2, 0);

       //set the Second (game data) drive
         _stprintf(tmp,"filesystem2=rw,DH1:games:%s,0" , filepath);
         txt2 = parsetextpath(_T(tmp));
         cfgfile_parse_line(p, txt2, 0);

         _stprintf(tmp,"uaehf1=dir,rw,DH1:games:%s,0" , filepath);
         txt2 = parsetextpath(_T(tmp));
         cfgfile_parse_line(p, txt2, 0);
    
         
   //APPLY THE SETTINGS FOR MOUSE/JOYSTICK ETC
          
        if (is_cd32 == true)
        {  p->jports[0].mode = 7;
           p->jports[1].mode = 7;   }
        
         if (game_detail.port0 == "CD32")
             p->jports[0].mode = 7;
         if (game_detail.port1 == "CD32")
             p->jports[1].mode = 7;
         
   //      *** GAME-SPECIFICS ***
   //  SET THE GAME COMPATIBILITY SETTINGS   
   // 
   // SCREEN HEIGHT, BLITTER, SPRITES, MEMORY, JIT, BIG CPU ETC  
        

    // CPU 68020/040
        if (strcmpi(game_detail.cpu,"68020") == 0 || strcmpi(game_detail.cpu,"68040") == 0)
            {  _stprintf(txt2,"cpu_type=%s",game_detail.cpu);
                cfgfile_parse_line(p, txt2, 0);    }

    // CPU 68000/010 [requires a600 rom)]
         if ((strcmpi(game_detail.cpu,"68000") == 0 || strcmpi(game_detail.cpu,"68010") == 0) && a600_available != 0)
              {  _stprintf(txt2,"cpu_type=%s",game_detail.cpu);
                cfgfile_parse_line(p, txt2, 0);    }       
         

    // CPU SPEED
         if  (strcmpi(game_detail.clock,"7") == 0) 
             {  _stprintf(txt2,"cpu_speed=real");
                cfgfile_parse_line(p, txt2, 0);    }                
         else if  (strcmpi(game_detail.clock,"14") == 0) 
             {  _stprintf(txt2,"finegrain_cpu_speed=1024");
                cfgfile_parse_line(p, txt2, 0);    }                    
         else if  (strcmpi(game_detail.clock,"28") == 0) 
             {  _stprintf(txt2,"finegrain_cpu_speed=128");
                cfgfile_parse_line(p, txt2, 0);    }                    
         else if  (strcmpi(game_detail.clock,"max") == 0) 
             {  _stprintf(txt2,"cpu_speed=max");
                cfgfile_parse_line(p, txt2, 0);    }                    
         else if  (strcmpi(game_detail.clock,"turbo") == 0) 
             {  _stprintf(txt2,"cpu_speed=turbo");
                cfgfile_parse_line(p, txt2, 0);    }                    
         
    //FAST / Z3 MEMORY REQUIREMENTS
         
        int temp_ram; 
        if  (strcmpi(game_detail.fast,"nul") != 0)
          {     
              temp_ram = atol(game_detail.fast) ;
             _stprintf(txt2,"fastmem_size=%d",temp_ram);
              cfgfile_parse_line(p, txt2, 0);  
          }
        if  (strcmpi(game_detail.fast,"nul") != 0)
          {     
              temp_ram = atol(game_detail.z3) ; 
             _stprintf(txt2,"z3mem_size=%d",temp_ram);
              cfgfile_parse_line(p, txt2, 0);  
          }
                        
         
    // FAST COPPER  
        if (strcmpi(game_detail.fastcopper,"true") == 0)
            {  _stprintf(txt2,"fast_copper=true");
                cfgfile_parse_line(p, txt2, 0);    }

    // BLITTER=IMMEDIATE/WAIT/NORMAL
        if (strcmpi(game_detail.blitter,"immediate") == 0)
            {  _stprintf(txt2,"immediate_blits=true");
                cfgfile_parse_line(p, txt2, 0); 
            }
        else if (strcmpi(game_detail.blitter,"normal") == 0)
            {  _stprintf(txt2,"waiting_blits=disabled");
                cfgfile_parse_line(p, txt2, 0);    }
         
    // CHIPSET OVERWRITE
        if (strcmpi(game_detail.chipset,"ocs") == 0)
        {    p->cs_compatible = CP_A600;
            built_in_chipset_prefs(p);
            p->chipset_mask = 0; }
         
        else if (strcmpi(game_detail.chipset,"ecs") == 0)
        {    p->cs_compatible = CP_A600;
            built_in_chipset_prefs(p);
            p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE; }

        else if (strcmpi(game_detail.chipset,"aga") == 0)
        {    p->cs_compatible = CP_A1200;
            built_in_chipset_prefs(p);
            p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA; }
        
     // JIT
        if (strcmpi(game_detail.jit,"true") == 0)
            {  _stprintf(txt2,"cachesize=8192");
                cfgfile_parse_line(p, txt2, 0);    }  
         
     // COMPATIBLE CPU
        if (strcmpi(game_detail.cpu_comp,"true") == 0)
            {  _stprintf(txt2,"cpu_compatible=true");
                cfgfile_parse_line(p, txt2, 0);    }        
          
     // NTSC 
        if (strcmpi(game_detail.ntsc,"true") == 0)
            {  _stprintf(txt2,"ntsc=true");
                cfgfile_parse_line(p, txt2, 0);    }    
         
     // NTSC 
        if (strcmpi(game_detail.ntsc,"true") == 0)
            {  _stprintf(txt2,"ntsc=true");
                cfgfile_parse_line(p, txt2, 0);    }    
         
      // SCREEN HEIGHT 
         if (strcmpi(game_detail.scr_height,"nul") != 0 ) 
         {  _stprintf(txt2,"gfx_height=%s",game_detail.scr_height);
             cfgfile_parse_line(p, txt2, 0);    
            _stprintf(txt2,"gfx_height_windowed=%s",game_detail.scr_height);
             cfgfile_parse_line(p, txt2, 0); 
            _stprintf(txt2,"gfx_height_fullscreen=%s",game_detail.scr_height);
             cfgfile_parse_line(p, txt2, 0);    }  
        
      // Y OFFSET
         if (strcmpi(game_detail.y_offset,"nul") != 0 ) 
         {  _stprintf(txt2,"amiberry.vertical_offset=%s",game_detail.y_offset);
             cfgfile_parse_line(p, txt2, 0);      
         }
            
      // SPRITE COLLISION
          if (strcmpi(game_detail.scr_height,"nul") != 0 ) 
         {  _stprintf(txt2,"collision_level=%s",game_detail.sprites);
             cfgfile_parse_line(p, txt2, 0);      }       
         

             
         
         
             
    // CUSTOM CONTROLS FILES
        strcpy(WHDConfig, WHDPath);
        strcat(WHDConfig,game_name);
        strcat(WHDConfig,".controls");  
        if (zfile_exists(WHDConfig))
           cfgfile_load(&currprefs, WHDConfig, &config_type, 0, 1);       
    
        
    //  CLEAN UP SETTINGS 
     //   fixup_prefs(&currprefs, true);    
     //   cfgfile_configuration_change(1); 
        
}