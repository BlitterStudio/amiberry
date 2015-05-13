#define _MENU_CONFIG_CPP

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "options.h"
#include "gui.h"
#include "sounddep/sound.h"
#include "custom.h"
#include "osdep/gp2x.h"
#include "cfgfile.h"
#include "uae.h"
#include "disk.h"
#include "../inputmode.h"

static int kickstart;

static int presetModeId = 2;

int saveMenu_n_savestate = 0;


static void SetPresetMode(int mode, struct uae_prefs *p)
{
	presetModeId = mode;
	
	switch(mode)
	{
		case 0:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 768;
			break;
			
		case 1:
			p->gfx_size.height = 216;
			p->gfx_size_fs.width = 716;
			break;
			
		case 2:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 640;
			break;

		case 3:
			p->gfx_size.height = 256;
			p->gfx_size_fs.width = 600;
			break;
						
		case 4:
			p->gfx_size.height = 262;
			p->gfx_size_fs.width = 588;
			break;
						
		case 5:
			p->gfx_size.height = 270;
			p->gfx_size_fs.width = 570;
			break;
						
		case 6:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 640;
			break;
						
		case 7:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
						
		case 10:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 768;
			break;
			
		case 11:
			p->gfx_size.height = 216;
			p->gfx_size_fs.width = 716;
			break;
			
		case 12:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 640;
			break;

		case 13:
			p->gfx_size.height = 256;
			p->gfx_size_fs.width = 600;
			break;
						
		case 14:
			p->gfx_size.height = 262;
			p->gfx_size_fs.width = 588;
			break;
						
		case 15:
			p->gfx_size.height = 270;
			p->gfx_size_fs.width = 570;
			break;
						
		case 16:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 640;
			break;
						
		case 17:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;

		case 20:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
			
		case 21:
			p->gfx_size.height = 216;
			p->gfx_size_fs.width = 784;
			break;
			
		case 22:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 704;
			break;

		case 23:
			p->gfx_size.height = 256;
			p->gfx_size_fs.width = 660;
			break;
						
		case 24:
			p->gfx_size.height = 262;
			p->gfx_size_fs.width = 640;
			break;
						
		case 25:
			p->gfx_size.height = 270;
			p->gfx_size_fs.width = 624;
			break;
						
		case 26:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 704;
			break;
						
		case 27:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
						
		case 30:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
			
		case 31:
			p->gfx_size.height = 216;
			p->gfx_size_fs.width = 784;
			break;
			
		case 32:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 704;
			break;

		case 33:
			p->gfx_size.height = 256;
			p->gfx_size_fs.width = 660;
			break;
						
		case 34:
			p->gfx_size.height = 262;
			p->gfx_size_fs.width = 640;
			break;
						
		case 35:
			p->gfx_size.height = 270;
			p->gfx_size_fs.width = 624;
			break;
						
		case 36:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 704;
			break;
						
		case 37:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
						
		case 40:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
			
		case 41:
			p->gfx_size.height = 216;
			p->gfx_size_fs.width = 800;
			break;
			
		case 42:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 768;
			break;

		case 43:
			p->gfx_size.height = 256;
			p->gfx_size_fs.width = 720;
			break;
						
		case 44:
			p->gfx_size.height = 262;
			p->gfx_size_fs.width = 704;
			break;
						
		case 45:
			p->gfx_size.height = 270;
			p->gfx_size_fs.width = 684;
			break;
						
		case 46:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
						
		case 47:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;

		case 50:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
			
		case 51:
			p->gfx_size.height = 216;
			p->gfx_size_fs.width = 800;
			break;
			
		case 52:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 768;
			break;

		case 53:
			p->gfx_size.height = 256;
			p->gfx_size_fs.width = 720;
			break;
						
		case 54:
			p->gfx_size.height = 262;
			p->gfx_size_fs.width = 704;
			break;
						
		case 55:
			p->gfx_size.height = 270;
			p->gfx_size_fs.width = 684;
			break;
						
		case 56:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
						
		case 57:
			p->gfx_size.height = 200;
			p->gfx_size_fs.width = 800;
			break;
						
  	default:
			p->gfx_size.height = 240;
			p->gfx_size_fs.width = 640;
			presetModeId = 2;
			break;
	}
	
	switch(presetModeId / 10)
	{
		case 0:
			p->gfx_size.width = 320;
			break;

		case 1:
			p->gfx_size.width = 640;
			break;

		case 2:
			p->gfx_size.width = 352;
			break;

		case 3:
			p->gfx_size.width = 704;
			break;

		case 4:
			p->gfx_size.width = 384;
			break;

		case 5:
			p->gfx_size.width = 768;
			break;
	}
}


static void SetDefaultMenuSettings(int general)
{
	if(general < 2)
	{
    while(nr_units(currprefs.mountinfo) > 0)
			kill_filesys_unit(currprefs.mountinfo, 0);
	}

	kickstart = 1;
	
	SetPresetMode(2, &changed_prefs);
}


static void replace(char * str,char replace, char toreplace)
{
	while(*str)
	{	
		if (*str==toreplace) *str=replace;
		str++;
	}
}


int create_configfilename(char *dest, char *basename, int fromDir)
{
	char *p; 
	p = basename + strlen(basename) - 1;
	while (*p != '/') 
		p--;
	p++;
	if(fromDir == 0)
	{
		int len = strlen(p) + 1;
		char filename[len];
		strcpy(filename, p);
		char *pch = &filename[len];
		while (pch != filename && *pch != '.')
			pch--;
		if (pch)
		{
			*pch='\0';
			snprintf(dest, 300, "%s/conf/%s.uae", start_path_data, filename);
			return 1;
		}
	}
	else
	{
		snprintf(dest, 300, "%s/conf/%s.uae", start_path_data, p);
		return 1;
	}
			
	return 0;
}


const char *kickstarts_rom_names[] = { "kick12.rom\0", "kick13.rom\0", "kick20.rom\0", "kick31.rom\0", "aros-amiga-m68k-rom.bin\0" };
const char *extended_rom_names[] = { "\0", "\0", "\0", "\0", "aros-amiga-m68k-ext.bin\0" };
const char *kickstarts_names[] = { "KS ROM v1.2\0", "KS ROM v1.3\0", "KS ROM v2.05\0", "KS ROM v3.1\0", "\0" };
#ifdef ANDROIDSDL
const char *af_kickstarts_rom_names[] = { "amiga-os-120.rom\0", "amiga-os-130.rom\0", "amiga-os-204.rom\0", "amiga-os-310-a1200.rom\0" };
#endif

static bool CheckKickstart(void)
{
  char kickpath[MAX_DPATH];
  int i;
  
  // Search via filename
  fetch_rompath(kickpath, MAX_DPATH);
  strncat(kickpath, kickstarts_rom_names[kickstart], MAX_DPATH);
  for(i=0; i<lstAvailableROMs.size(); ++i)
  {
    if(!strcasecmp(lstAvailableROMs[i]->Path, kickpath))
    {
      // Found it
      strncpy(changed_prefs.romfile, kickpath, sizeof(changed_prefs.romfile));
      return true;
    }
  }

  // Search via name
  if(strlen(kickstarts_names[kickstart]) > 0)
  {
    for(i=0; i<lstAvailableROMs.size(); ++i)
    {
      if(!strncasecmp(lstAvailableROMs[i]->Name, kickstarts_names[kickstart], strlen(kickstarts_names[kickstart])))
      {
        // Found it
        strncpy(changed_prefs.romfile, lstAvailableROMs[i]->Path, sizeof(changed_prefs.romfile));
        return true;
      }
    }
  }

  return false;
}


// In this procedure, we use changed_prefs
int loadconfig_old(const char *orgpath, int general)
{
  char path[MAX_PATH];

  strcpy(path, orgpath);
	char *ptr = strstr(path, ".uae");
	if(ptr > 0)
  {
    *(ptr + 1) = '\0';
    strcat(path, "conf");
  }
		
	FILE *f=fopen(path,"rt");
	if (!f){
		write_log ("No config file %s!\n",path);
		return 0;
	}
	else
	{
		// Set everthing to default and clear HD settings
		SetDefaultMenuSettings(general);
	
		char filebuffer[256];
		int dummy;

		fscanf(f,"kickstart=%d\n",&kickstart);
#if defined(PANDORA) || defined(ANDROIDSDL)
		fscanf(f,"scaling=%d\n",&dummy);
#else
		fscanf(f,"scaling=%d\n",&mainMenu_enableHWscaling);
#endif
		fscanf(f,"showstatus=%d\n", &changed_prefs.leds_on_screen);
		fscanf(f,"mousemultiplier=%d\n", &changed_prefs.input_joymouse_multiplier);
		changed_prefs.input_joymouse_multiplier *= 10;
#if defined(PANDORA) || defined(ANDROIDSDL)
		fscanf(f,"systemclock=%d\n",&dummy);    // mainMenu_throttle never changes -> removed
		fscanf(f,"syncthreshold=%d\n", &dummy); // timeslice_mode never changes -> removed
#else
		fscanf(f,"systemclock=%d\n",&mainMenu_throttle);
		fscanf(f,"syncthreshold=%d\n", &timeslice_mode);
#endif
		fscanf(f,"frameskip=%d\n", &changed_prefs.gfx_framerate);
		fscanf(f,"sound=%d\n",&changed_prefs.produce_sound );
		if(changed_prefs.produce_sound >= 10)
	  {
	    changed_prefs.sound_stereo = 1;
	    changed_prefs.produce_sound -= 10;
	    if(changed_prefs.produce_sound > 0)
	      changed_prefs.produce_sound += 1;
	  }
	  else
	    changed_prefs.sound_stereo = 0;
		fscanf(f,"soundrate=%d\n",&changed_prefs.sound_freq);
		fscanf(f,"autosave=%d\n", &dummy);
		fscanf(f,"gp2xclock=%d\n", &dummy);
		int joybuffer = 0;
		fscanf(f,"joyconf=%d\n",&joybuffer);
		changed_prefs.pandora_joyConf = (joybuffer & 0x0f);
		changed_prefs.pandora_joyPort = ((joybuffer >> 4) & 0x0f);
		fscanf(f,"autofireRate=%d\n",&changed_prefs.input_autofire_framecnt);
		fscanf(f,"autofire=%d\n", &dummy);
		fscanf(f,"stylusOffset=%d\n",&changed_prefs.pandora_stylusOffset);
		fscanf(f,"tapDelay=%d\n",&changed_prefs.pandora_tapDelay);
		fscanf(f,"scanlines=%d\n", &dummy);
#if defined(PANDORA) || defined(ANDROIDSDL)
		fscanf(f,"ham=%d\n",&dummy);
#else
		fscanf(f,"ham=%d\n",&mainMenu_ham);
#endif
		fscanf(f,"enableScreenshots=%d\n", &dummy);
		fscanf(f,"floppyspeed=%d\n",&changed_prefs.floppy_speed);
		fscanf(f,"drives=%d\n", &changed_prefs.nr_floppies);
		fscanf(f,"videomode=%d\n", &changed_prefs.ntscmode);
		if(changed_prefs.ntscmode)
		  changed_prefs.chipset_refreshrate = 60;
		else
		  changed_prefs.chipset_refreshrate = 50;
		fscanf(f,"mainMenu_cpuSpeed=%d\n",&changed_prefs.pandora_cpu_speed);
		fscanf(f,"presetModeId=%d\n",&presetModeId);
		fscanf(f,"moveX=%d\n", &changed_prefs.pandora_horizontal_offset);
		fscanf(f,"moveY=%d\n", &changed_prefs.pandora_vertical_offset);
		fscanf(f,"displayedLines=%d\n",&changed_prefs.gfx_size.height);
		fscanf(f,"screenWidth=%d\n", &changed_prefs.gfx_size_fs.width);
		fscanf(f,"cutLeft=%d\n", &dummy);
		fscanf(f,"cutRight=%d\n", &dummy);
		fscanf(f,"customControls=%d\n",&changed_prefs.pandora_customControls);
		fscanf(f,"custom_dpad=%d\n",&changed_prefs.pandora_custom_dpad);
		fscanf(f,"custom_up=%d\n",&changed_prefs.pandora_custom_up);
		fscanf(f,"custom_down=%d\n",&changed_prefs.pandora_custom_down);
		fscanf(f,"custom_left=%d\n",&changed_prefs.pandora_custom_left);
		fscanf(f,"custom_right=%d\n",&changed_prefs.pandora_custom_right);
		fscanf(f,"custom_A=%d\n",&changed_prefs.pandora_custom_A);
		fscanf(f,"custom_B=%d\n",&changed_prefs.pandora_custom_B);
		fscanf(f,"custom_X=%d\n",&changed_prefs.pandora_custom_X);
		fscanf(f,"custom_Y=%d\n",&changed_prefs.pandora_custom_Y);
		fscanf(f,"custom_L=%d\n",&changed_prefs.pandora_custom_L);
		fscanf(f,"custom_R=%d\n",&changed_prefs.pandora_custom_R);
		fscanf(f,"cpu=%d\n", &changed_prefs.cpu_level);
		if(changed_prefs.cpu_level > M68000)
		  // Was old format
		  changed_prefs.cpu_level = M68020;
		fscanf(f,"chipset=%d\n", &changed_prefs.chipset_mask);
		changed_prefs.immediate_blits = (changed_prefs.chipset_mask & 0x100) == 0x100;
		changed_prefs.pandora_partial_blits = (changed_prefs.chipset_mask & 0x200) == 0x200;
  	switch (changed_prefs.chipset_mask & 0xff) 
  	{
  		case 1: changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE; break;
  		case 2: changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA; break;
  		default: changed_prefs.chipset_mask = CSMASK_ECS_AGNUS; break;
  	}
		fscanf(f,"cpu=%d\n", &changed_prefs.m68k_speed);
	  if(changed_prefs.m68k_speed < 0)
    {
      // New version of this option
      changed_prefs.m68k_speed = -changed_prefs.m68k_speed;
    }
    else
    {
      // Old version (500 5T 1200 12T 12T2)
	    if(changed_prefs.m68k_speed >= 2)
      {
        // 1200: set to 68020 with 14 MHz
        changed_prefs.cpu_level = M68020;
        changed_prefs.m68k_speed--;
        if(changed_prefs.m68k_speed > 2)
          changed_prefs.m68k_speed = 2;
      }
	  }
	  if(changed_prefs.m68k_speed == 1)
	    changed_prefs.m68k_speed = M68K_SPEED_14MHZ_CYCLES;
	  if(changed_prefs.m68k_speed == 2)
	    changed_prefs.m68k_speed = M68K_SPEED_25MHZ_CYCLES;
    changed_prefs.cachesize = 0;
    changed_prefs.cpu_compatible = 0;
      
		if(general == 0)
		{
		  disk_eject(0);
		  disk_eject(1);
		  disk_eject(2);
		  disk_eject(3);
			fscanf(f,"df0=%s\n",&filebuffer);
			replace(filebuffer,' ','|');
			strcpy(changed_prefs.df[0], filebuffer);	
			disk_insert(0, filebuffer);
			if(changed_prefs.nr_floppies > 1)
			{
				memset(filebuffer, 0, 256);
				fscanf(f,"df1=%s\n",&filebuffer);
				replace(filebuffer,' ','|');
				strcpy(changed_prefs.df[1], filebuffer);
  			disk_insert(1, filebuffer);
			}
			if(changed_prefs.nr_floppies > 2)
			{
				memset(filebuffer, 0, 256);
				fscanf(f,"df2=%s\n",&filebuffer);
				replace(filebuffer,' ','|');
				strcpy(changed_prefs.df[2], filebuffer);
  			disk_insert(2, filebuffer);
			}
			if(changed_prefs.nr_floppies > 3)
			{
				memset(filebuffer, 0, 256);
				fscanf(f,"df3=%s\n",&filebuffer);
				replace(filebuffer,' ','|');
				strcpy(changed_prefs.df[3], filebuffer);
  			disk_insert(3, filebuffer);
			}
		}
		else
		{
			fscanf(f,"script=%d\n", &dummy);
			fscanf(f,"screenshot=%d\n", &dummy);
      fread(filebuffer, 1, 4, f);
      fseek(f, -4, SEEK_CUR);
      filebuffer[4] = '\0';
      if(strcmp(filebuffer, "skip") == 0)
        fscanf(f,"skipintro=%d\n", &dummy);
			fscanf(f,"boot_hd=%d\n", &dummy);
			fscanf(f,"hard_disk_dir=", filebuffer);
			filebuffer[0] = '\0';
			{
				char c[2] = {0, 0};
				*c = fgetc(f);
				while (*c && (*c != '\n')) {
					strcat(filebuffer, c);
					*c = fgetc(f);
				}
			}
			if (filebuffer[0] == '*')
				filebuffer[0] = '\0';
			if(filebuffer[0] != '\0')
        parse_filesys_spec(0, filebuffer);
			
			fscanf(f,"hard_disk_file=", filebuffer);
			filebuffer[0] = '\0';
			{
				char c[2] = {0, 0};
				*c = fgetc(f);
				while (*c && (*c != '\n')) {
					strcat(filebuffer, c);
					*c = fgetc(f);
				}
			}
			if (filebuffer[0] == '*')
				filebuffer[0] = '\0';
			if(filebuffer[0] != '\0')
        parse_hardfile_spec(filebuffer);
		}
		currprefs.nr_floppies = changed_prefs.nr_floppies;
		for(int i=0; i<4; ++i)
		{
		  if(i < changed_prefs.nr_floppies)
		    changed_prefs.dfxtype[i] = DRV_35_DD;
		  else
		    changed_prefs.dfxtype[i] = DRV_NONE;
		}
	
		fscanf(f,"chipmemory=%d\n",&changed_prefs.chipmem_size);
		if(changed_prefs.chipmem_size < 10)
		  // Was saved in old format
		  changed_prefs.chipmem_size = 0x80000 << changed_prefs.chipmem_size;
		fscanf(f,"slowmemory=%d\n",&changed_prefs.bogomem_size);
		if(changed_prefs.bogomem_size > 0 && changed_prefs.bogomem_size < 10)
		  // Was saved in old format
		  changed_prefs.bogomem_size = 
		    (changed_prefs.bogomem_size <= 2) ? 0x080000 << changed_prefs.bogomem_size :
		    (changed_prefs.bogomem_size == 3) ? 0x180000 : 0x1c0000;
		fscanf(f,"fastmemory=%d\n",&changed_prefs.fastmem_size);
		if(changed_prefs.fastmem_size > 0 && changed_prefs.fastmem_size < 10)
		  // Was saved in old format
		  changed_prefs.fastmem_size = 0x080000 << changed_prefs.fastmem_size;
#ifdef ANDROIDSDL		
		fscanf(f,"onscreen=%d\n",&mainMenu_onScreen);
		fscanf(f,"onScreen_textinput=%d\n",&mainMenu_onScreen_textinput);
		fscanf(f,"onScreen_dpad=%d\n",&mainMenu_onScreen_dpad);
		fscanf(f,"onScreen_button1=%d\n",&mainMenu_onScreen_button1);
		fscanf(f,"onScreen_button2=%d\n",&mainMenu_onScreen_button2);
		fscanf(f,"onScreen_button3=%d\n",&mainMenu_onScreen_button3);
		fscanf(f,"onScreen_button4=%d\n",&mainMenu_onScreen_button4);
		fscanf(f,"onScreen_button5=%d\n",&mainMenu_onScreen_button5);
		fscanf(f,"onScreen_button6=%d\n",&mainMenu_onScreen_button6);
		fscanf(f,"custom_position=%d\n",&mainMenu_custom_position);
		fscanf(f,"pos_x_textinput=%d\n",&mainMenu_pos_x_textinput);
		fscanf(f,"pos_y_textinput=%d\n",&mainMenu_pos_y_textinput);
		fscanf(f,"pos_x_dpad=%d\n",&mainMenu_pos_x_dpad);
		fscanf(f,"pos_y_dpad=%d\n",&mainMenu_pos_y_dpad);
		fscanf(f,"pos_x_button1=%d\n",&mainMenu_pos_x_button1);
		fscanf(f,"pos_y_button1=%d\n",&mainMenu_pos_y_button1);
		fscanf(f,"pos_x_button2=%d\n",&mainMenu_pos_x_button2);
		fscanf(f,"pos_y_button2=%d\n",&mainMenu_pos_y_button2);
		fscanf(f,"pos_x_button3=%d\n",&mainMenu_pos_x_button3);
		fscanf(f,"pos_y_button3=%d\n",&mainMenu_pos_y_button3);
		fscanf(f,"pos_x_button4=%d\n",&mainMenu_pos_x_button4);
		fscanf(f,"pos_y_button4=%d\n",&mainMenu_pos_y_button4);
		fscanf(f,"pos_x_button5=%d\n",&mainMenu_pos_x_button5);
		fscanf(f,"pos_y_button5=%d\n",&mainMenu_pos_y_button5);
		fscanf(f,"pos_x_button6=%d\n",&mainMenu_pos_x_button6);
		fscanf(f,"pos_y_button6=%d\n",&mainMenu_pos_y_button6);
		fscanf(f,"quick_switch=%d\n",&mainMenu_quickSwitch);
		fscanf(f,"FloatingJoystick=%d\n",&mainMenu_FloatingJoystick);
#endif
	
		fclose(f);
	}

	SetPresetMode(presetModeId, &changed_prefs);

  CheckKickstart();
	set_joyConf();

	return 1;
}
