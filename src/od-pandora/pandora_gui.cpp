#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>
#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "keybuf.h"
#include "inputdevice.h"
#include "memory.h"
#include "zfile.h"
#include "gui.h"
#include "od-pandora/gui/SelectorEntry.hpp"
#include "gui/gui_handling.h"
#include "custom.h"
#include "memory.h"
#include "xwin.h"
#include "drawing.h"
#include "sd-pandora/sound.h"
#include "audio.h"
#include "keybuf.h"
#include "keyboard.h"
#include "disk.h"
#include "savestate.h"
#include "filesys.h"
#include "autoconf.h"
#include <SDL.h>


int emulating = 0;
extern bool switch_autofire;
static int justMovedUp=0, justMovedDown=0, justMovedLeft=0, justMovedRight=0;
static int justPressedA=0, justPressedB=0, justPressedX=0, justPressedY=0;
static int justPressedL=0, justPressedR=0;
int stylusClickOverride=0;
int stylusAdjustX=0, stylusAdjustY=0;


extern SDL_Surface *prSDLScreen;

int dpadUp=0;
int dpadDown=0;
int dpadLeft=0;
int dpadRight=0;
int buttonA=0;
int buttonB=0;
int buttonX=0;
int buttonY=0;
int triggerL=0;
int triggerR=0;

struct gui_msg {
  int num;
  const char *msg;
};
struct gui_msg gui_msglist[] = {
  { NUMSG_NEEDEXT2,       "The software uses a non-standard floppy disk format. You may need to use a custom floppy disk image file instead of a standard one. This message will not appear again." },
  { NUMSG_NOROM,          "Could not load system ROM, trying system ROM replacement." },
  { NUMSG_NOROMKEY,       "Could not find system ROM key file." },
  { NUMSG_KSROMCRCERROR,  "System ROM checksum incorrect. The system ROM image file may be corrupt." },
  { NUMSG_KSROMREADERROR, "Error while reading system ROM." },
  { NUMSG_NOEXTROM,       "No extended ROM found." },
  { NUMSG_KS68EC020,      "The selected system ROM requires a 68EC020 or later CPU." },
  { NUMSG_KS68020,        "The selected system ROM requires a 68020 or later CPU." },
  { NUMSG_KS68030,        "The selected system ROM requires a 68030 CPU." },
  { NUMSG_STATEHD,        "WARNING: Current configuration is not fully compatible with state saves." },
  { NUMSG_KICKREP,        "You need to have a floppy disk (image file) in DF0: to use the system ROM replacement." },
  { NUMSG_KICKREPNO,      "The floppy disk (image file) in DF0: is not compatible with the system ROM replacement functionality." },
  { -1, "" }
};

std::vector<ConfigFileInfo*> ConfigFilesList;
std::vector<AvailableROM*> lstAvailableROMs;
std::vector<std::string> lstMRUDiskList;


void AddFileToDiskList(const char *file, int moveToTop)
{
  int i;
  
  for(i=0; i<lstMRUDiskList.size(); ++i)
  {
    if(!strcasecmp(lstMRUDiskList[i].c_str(), file))
    {
      if(moveToTop)
      {
        lstMRUDiskList.erase(lstMRUDiskList.begin() + i);
        lstMRUDiskList.insert(lstMRUDiskList.begin(), file);
      }
      break;
    }
  }
  if(i >= lstMRUDiskList.size())
    lstMRUDiskList.insert(lstMRUDiskList.begin(), file);

  while(lstMRUDiskList.size() > MAX_MRU_DISKLIST)
    lstMRUDiskList.pop_back();
}


static void ClearAvailableROMList(void)
{
  while(lstAvailableROMs.size() > 0)
  {
    AvailableROM *tmp = lstAvailableROMs[0];
    lstAvailableROMs.erase(lstAvailableROMs.begin());
    delete tmp;
  }
}

static void addrom(struct romdata *rd, char *path)
{
  AvailableROM *tmp;
  char tmpName[MAX_DPATH];
  tmp = new AvailableROM();
  getromname(rd, tmpName);
  strncpy(tmp->Name, tmpName, MAX_PATH);
  strncpy(tmp->Path, path, MAX_PATH);
  tmp->ROMType = rd->type;
  lstAvailableROMs.push_back(tmp);
}

struct romscandata {
    uae_u8 *keybuf;
    int keysize;
};

static struct romdata *scan_single_rom_2 (struct zfile *f)
{
  uae_u8 buffer[20] = { 0 };
  uae_u8 *rombuf;
  int cl = 0, size;
  struct romdata *rd = 0;

  zfile_fseek (f, 0, SEEK_END);
  size = zfile_ftell (f);
  zfile_fseek (f, 0, SEEK_SET);
  if (size > 524288 * 2) /* don't skip KICK disks or 1M ROMs */
  	return 0;
  zfile_fread (buffer, 1, 11, f);
  if (!memcmp (buffer, "KICK", 4)) {
	  zfile_fseek (f, 512, SEEK_SET);
	  if (size > 262144)
	    size = 262144;
  } else if (!memcmp (buffer, "AMIROMTYPE1", 11)) {
  	cl = 1;
	  size -= 11;
  } else {
	  zfile_fseek (f, 0, SEEK_SET);
  }
  rombuf = (uae_u8 *) xcalloc (size, 1);
  if (!rombuf)
  	return 0;
  zfile_fread (rombuf, 1, size, f);
  if (cl > 0) {
  	decode_cloanto_rom_do (rombuf, size, size);
	  cl = 0;
  }
  if (!cl) {
  	rd = getromdatabydata (rombuf, size);
  	if (!rd && (size & 65535) == 0) {
	    /* check byteswap */
	    int i;
	    for (i = 0; i < size; i+=2) {
    		uae_u8 b = rombuf[i];
    		rombuf[i] = rombuf[i + 1];
    		rombuf[i + 1] = b;
 	    }
 	    rd = getromdatabydata (rombuf, size);
  	}
  }
  free (rombuf);
  return rd;
}

static struct romdata *scan_single_rom (char *path)
{
    struct zfile *z;
    char tmp[MAX_DPATH];
    struct romdata *rd;

    strcpy (tmp, path);
    rd = getromdatabypath(path);
    if (rd && rd->crc32 == 0xffffffff)
	return rd;
    z = zfile_fopen (path, "rb");
    if (!z)
	return 0;
    return scan_single_rom_2 (z);
}

static int isromext(char *path)
{
  char *ext;
  int i;

  if (!path)
	  return 0;
  ext = strrchr (path, '.');
  if (!ext)
  	return 0;
  ext++;

  if (!stricmp (ext, "rom") ||  !stricmp (ext, "adf") || !stricmp (ext, "key")
	|| !stricmp (ext, "a500") || !stricmp (ext, "a1200") || !stricmp (ext, "a4000"))
    return 1;
  for (i = 0; uae_archive_extensions[i]; i++) {
	  if (!stricmp (ext, uae_archive_extensions[i]))
	    return 1;
  }
  return 0;
}

static int scan_rom_2 (struct zfile *f, void *dummy)
{
  char *path = zfile_getname(f);
  struct romdata *rd;

  if (!isromext(path))
	  return 0;
  rd = scan_single_rom_2(f);
  if (rd)
    addrom (rd, path);
  return 0;
}

static void scan_rom(char *path)
{
  struct romdata *rd;

    if (!isromext(path)) {
	//write_log("ROMSCAN: skipping file '%s', unknown extension\n", path);
	return;
    }
  rd = getarcadiarombyname(path);
  if (rd) 
    addrom(rd, path);
  else
    zfile_zopen (path, scan_rom_2, 0);
}


void RescanROMs(void)
{
  std::vector<std::string> files;
  char path[MAX_DPATH];
  
  ClearAvailableROMList();
  fetch_rompath(path, MAX_DPATH);
  
  load_keyring(&changed_prefs, path);
  ReadDirectory(path, NULL, &files);
  for(int i=0; i<files.size(); ++i)
  {
    char tmppath[MAX_PATH];
    strncpy(tmppath, path, MAX_PATH);
    strncat(tmppath, files[i].c_str(), MAX_PATH);
    scan_rom (tmppath);
  }
}

static void ClearConfigFileList(void)
{
  while(ConfigFilesList.size() > 0)
  {
    ConfigFileInfo *tmp = ConfigFilesList[0];
    ConfigFilesList.erase(ConfigFilesList.begin());
    delete tmp;
  }
}


void ReadConfigFileList(void)
{
  char path[MAX_PATH];
  std::vector<std::string> files;
  const char *filter_uae[] = { ".uae", "\0" };
  const char *filter_conf[] = { ".conf", "\0" };
    
  ClearConfigFileList();
  
  fetch_configurationpath(path, MAX_PATH);
  ReadDirectory(path, NULL, &files);
  FilterFiles(&files, filter_uae);
  for (int i=0; i<files.size(); ++i)
  {
    ConfigFileInfo *tmp = new ConfigFileInfo();
    strncpy(tmp->FullPath, path, MAX_DPATH);
    strcat(tmp->FullPath, files[i].c_str());
    strncpy(tmp->Name, files[i].c_str(), MAX_DPATH);
    removeFileExtension(tmp->Name);
    cfgfile_get_description(tmp->FullPath, tmp->Description);
    ConfigFilesList.push_back(tmp);
  }

  // Read also old style configs
  ReadDirectory(path, NULL, &files);
  FilterFiles(&files, filter_conf);
  for (int i=0; i<files.size(); ++i)
  {
    if(strcmp(files[i].c_str(), "adfdir.conf"))
    { 
      ConfigFileInfo *tmp = new ConfigFileInfo();
      strncpy(tmp->FullPath, path, MAX_DPATH);
      strcat(tmp->FullPath, files[i].c_str());
      strncpy(tmp->Name, files[i].c_str(), MAX_DPATH);
      removeFileExtension(tmp->Name);
      strcpy(tmp->Description, "Old style configuration file");
      for(int j=0; j<ConfigFilesList.size(); ++j)
      {
        if(!strcmp(ConfigFilesList[j]->Name, tmp->Name))
        {
          // Config in new style already in list
          delete tmp;
          tmp = NULL;
          break;
        }
      }
      if(tmp != NULL)
        ConfigFilesList.push_back(tmp);
    }
  }
}

ConfigFileInfo* SearchConfigInList(const char *name)
{
  for(int i=0; i<ConfigFilesList.size(); ++i)
  {
    if(!strncasecmp(ConfigFilesList[i]->Name, name, MAX_DPATH))
      return ConfigFilesList[i];
  }
  return NULL;
}


static void prefs_to_gui()
{
  /* filesys hack */
  changed_prefs.mountitems = currprefs.mountitems;
  memcpy(&changed_prefs.mountconfig, &currprefs.mountconfig, MOUNT_CONFIG_SIZE * sizeof (struct uaedev_config_info));
}


static void gui_to_prefs (void)
{
  /* filesys hack */
  currprefs.mountitems = changed_prefs.mountitems;
  memcpy(&currprefs.mountconfig, &changed_prefs.mountconfig, MOUNT_CONFIG_SIZE * sizeof (struct uaedev_config_info));
}


int gui_init (void)
{
  int ret = 0;
  
#ifndef ANDROIDSDL
  SDL_JoystickEventState(SDL_ENABLE);
  SDL_JoystickOpen(0);
#endif

	emulating=0;

  if(lstAvailableROMs.size() == 0)
    RescanROMs();

  graphics_subshutdown();
  prefs_to_gui();
  run_gui();
  gui_to_prefs();
  if(quit_program < 0)
    quit_program = -quit_program;
  if(quit_program == 1)
    ret = -2; // Quit without start of emulator

	setCpuSpeed();
  update_display(&changed_prefs);

	inputmode_init();
  inputdevice_copyconfig (&changed_prefs, &currprefs);
  inputdevice_config_change_test();
	emulating=1;
  return ret;
}

void gui_exit(void)
{
	resetCpuSpeed();
	sync();
	pandora_stop_sound();
	saveAdfDir();
	ClearConfigFileList();
	ClearAvailableROMList();
}


void gui_purge_events(void)
{
	int counter = 0;

	SDL_Event event;
	SDL_Delay(150);
	// Strangely PS3 controller always send events, so we need a maximum number of event to purge.
	while(SDL_PollEvent(&event) && counter < 50)
	{
		counter++;
		SDL_Delay(10);
	}
	keybuf_init();
}


int gui_update (void)
{
  char tmp[MAX_PATH];

  fetch_savestatepath(savestate_fname, MAX_DPATH);
  fetch_screenshotpath(screenshot_filename, MAX_DPATH);
  
  if(strlen(currprefs.df[0]) > 0)
    extractFileName(currprefs.df[0], tmp);
  else
    strncpy(tmp, last_loaded_config, MAX_PATH);

  strncat(savestate_fname, tmp, MAX_DPATH);
  strncat(screenshot_filename, tmp, MAX_DPATH);
  removeFileExtension(savestate_fname);
  removeFileExtension(screenshot_filename);

  switch(currentStateNum)
  {
    case 1:
  		strcat(savestate_fname,"-1.uss");
	    strcat(screenshot_filename,"-1.png");
	    break;
    case 2:
  		strcat(savestate_fname,"-2.uss");
  		strcat(screenshot_filename,"-2.png");
  		break;
    case 3:
  		strcat(savestate_fname,"-3.uss");
  		strcat(screenshot_filename,"-3.png");
  		break;
    default: 
  	   	strcat(savestate_fname,".uss");
    		strcat(screenshot_filename,".png");
  }
    return 0;
}


static void goMenu(void)
{
	if (quit_program != 0)
		return;
	emulating=1;
	pause_sound();

  if(lstAvailableROMs.size() == 0)
    RescanROMs();
  graphics_subshutdown();
  prefs_to_gui();
  run_gui();
  gui_to_prefs();
	setCpuSpeed();
  update_display(&changed_prefs);

	/* Clear menu garbage at the bottom of the screen */
	black_screen_now();
	notice_screen_contents_lost();
	resume_sound();

  inputdevice_copyconfig (&changed_prefs, &currprefs);
  inputdevice_config_change_test ();

  gui_update ();

  gui_purge_events();
  fpscounter_reset();
  notice_screen_contents_lost();
}


static int customKey[256] = 
 { 0,             AK_UP,    AK_DN,      AK_LF,    AK_RT,        AK_NP0,       AK_NP1,         AK_NP2,       /*   0 -   7 */
   AK_NP3,        AK_NP4,   AK_NP5,     AK_NP6,   AK_NP7,       AK_NP8,       AK_NP9,         AK_ENT,       /*   8 -  15 */
   AK_NPDIV,      AK_NPMUL, AK_NPSUB,   AK_NPADD, AK_NPDEL,     AK_NPLPAREN,  AK_NPRPAREN,    AK_SPC,       /*  16 -  23 */
   AK_BS,         AK_TAB,   AK_RET,     AK_ESC,   AK_DEL,       AK_LSH,       AK_RSH,         AK_CAPSLOCK,  /*  24 -  31 */
   AK_CTRL,       AK_LALT,  AK_RALT,    AK_LAMI,  AK_RAMI,      AK_HELP,      AK_LBRACKET,    AK_RBRACKET,  /*  32 -  39 */
   AK_SEMICOLON,  AK_COMMA, AK_PERIOD,  AK_SLASH, AK_BACKSLASH, AK_QUOTE,     AK_NUMBERSIGN,  AK_LTGT,      /*  40 -  47 */
   AK_BACKQUOTE,  AK_MINUS, AK_EQUAL,   AK_A,     AK_B,         AK_C,         AK_D,           AK_E,         /*  48 -  55 */
   AK_F,          AK_G,     AK_H,       AK_I,     AK_J,         AK_K,         AK_L,           AK_M,         /*  56 -  63 */
   AK_N,          AK_O,     AK_P,       AK_Q,     AK_R,         AK_S,         AK_T,           AK_U,         /*  64 -  71 */
   AK_V,          AK_W,     AK_X,       AK_Y,     AK_Z,         AK_1,         AK_2,           AK_3,         /*  72 -  79 */
   AK_4,          AK_5,     AK_6,       AK_7,     AK_8,         AK_9,         AK_0,           AK_F1,        /*  80 -  87 */
   AK_F2,         AK_F3,    AK_F4,      AK_F5,    AK_F6,        AK_F7,        AK_F8,          AK_F9,        /*  88 -  95 */
   AK_F10,        0 }; /*  96 -  97 */
  
static int getMapping(int customId)
{
  if(customId > 96)
    return 0;
  else
    return customKey[customId];
}

void moveVertical(int value)
{
	changed_prefs.pandora_vertical_offset += value;
	if(changed_prefs.pandora_vertical_offset < -16)
		changed_prefs.pandora_vertical_offset = -16;
	else if(changed_prefs.pandora_vertical_offset > 16)
		changed_prefs.pandora_vertical_offset = 16;
}

void gui_handle_events (void)
{
  int i;
  int key = 0;

	Uint8 *keystate = SDL_GetKeyState(NULL);
	dpadUp = keystate[SDLK_UP];
	dpadDown = keystate[SDLK_DOWN];
	dpadLeft = keystate[SDLK_LEFT];
	dpadRight = keystate[SDLK_RIGHT];
	buttonA = keystate[SDLK_HOME];
	buttonB = keystate[SDLK_END];
	buttonX = keystate[SDLK_PAGEDOWN];
	buttonY = keystate[SDLK_PAGEUP];
	triggerL = keystate[SDLK_RSHIFT];
	triggerR = keystate[SDLK_RCTRL];

	if(keystate[SDLK_LCTRL])
		goMenu();

	//L + R
	if(triggerL && triggerR)
	{
		//up
		if(dpadUp)
		{
			moveVertical(1);
			stylusAdjustY += 2;
		}
		//down
		else if(dpadDown)
		{
			moveVertical(-1);
			stylusAdjustY -= 2;
		}
		//1
		else if(keystate[SDLK_1])
		{
			changed_prefs.gfx_size.height = amigaheight_values[0];
			update_display(&changed_prefs);
		}
		//2
		else if(keystate[SDLK_2])
		{
			changed_prefs.gfx_size.height = amigaheight_values[1];
			update_display(&changed_prefs);
		}
		//3
		else if(keystate[SDLK_3])
		{
			changed_prefs.gfx_size.height = amigaheight_values[2];
			update_display(&changed_prefs);
		}
		//4
		else if(keystate[SDLK_4])
		{
			changed_prefs.gfx_size.height = amigaheight_values[3];
			update_display(&changed_prefs);
		}
		//5
		else if(keystate[SDLK_5])
		{
			changed_prefs.gfx_size.height = amigaheight_values[4];
			update_display(&changed_prefs);
		}
		//6
		else if(keystate[SDLK_6])
		{
			changed_prefs.gfx_size.height = amigaheight_values[5];
			update_display(&changed_prefs);
		}
		//9
		else if(keystate[SDLK_9])
		{
			for(i=0; i<AMIGAHEIGHT_COUNT; ++i)
			{
			  if(currprefs.gfx_size.height == amigaheight_values[i])
		    {
		      if(i > 0)
		        changed_prefs.gfx_size.height = amigaheight_values[i - 1];
		      else
		        changed_prefs.gfx_size.height = amigaheight_values[AMIGAHEIGHT_COUNT - 1];
		      break;
		    }
			}
			update_display(&changed_prefs);
		}
		//0
		else if(keystate[SDLK_0])
		{
			for(i=0; i<AMIGAHEIGHT_COUNT; ++i)
			{
			  if(currprefs.gfx_size.height == amigaheight_values[i])
		    {
		      if(i < AMIGAHEIGHT_COUNT - 1)
		        changed_prefs.gfx_size.height = amigaheight_values[i + 1];
		      else
		        changed_prefs.gfx_size.height = amigaheight_values[0];
		      break;
		    }
			}
			update_display(&changed_prefs);
		}
		else if(keystate[SDLK_w])
		{
			// Change width
			for(i=0; i<AMIGAWIDTH_COUNT; ++i)
			{
			  if(currprefs.gfx_size.width == amigawidth_values[i])
		    {
		      if(i < AMIGAWIDTH_COUNT - 1)
		        changed_prefs.gfx_size.width = amigawidth_values[i + 1];
		      else
		        changed_prefs.gfx_size.width = amigawidth_values[0];
		      break;
		    }
			}
			update_display(&changed_prefs);
		}
	}

	else if(triggerL)
	{
		if(keystate[SDLK_c])
		{
			keystate[SDLK_c]=0;
		  currprefs.pandora_customControls = !currprefs.pandora_customControls;
		}
		else if(keystate[SDLK_d])
		{
			keystate[SDLK_d]=0;
			changed_prefs.leds_on_screen = !currprefs.leds_on_screen;
		}
		else if(keystate[SDLK_f])
		{
			keystate[SDLK_f]=0;
			changed_prefs.gfx_framerate ? changed_prefs.gfx_framerate = 0 : changed_prefs.gfx_framerate = 1;
		}
  	else if(keystate[SDLK_s])
  	{
  		keystate[SDLK_s]=0;
  		savestate_state = STATE_DOSAVE;
  	}
	  else if(keystate[SDLK_l])
  	{
  		FILE *f=fopen(savestate_fname, "rb");
  		keystate[SDLK_l]=0;
  		if(f)
  		{
  			fclose(f);
  			savestate_state = STATE_DORESTORE;
  		}
  	}
	}

  if(currprefs.pandora_customControls)
  {
    if(currprefs.pandora_custom_dpad == 2) // dPad is custom
    {
			//UP
			if(dpadUp)
			{
				if(!justMovedUp)
				{
					if(currprefs.pandora_custom_up == -1) buttonstate[0]=1;
					else if(currprefs.pandora_custom_up == -2) buttonstate[2]=1;
					else if(currprefs.pandora_custom_up > 0)
					{
						key = getMapping(currprefs.pandora_custom_up);
						uae4all_keystate[key] = 1;
						record_key(key << 1);
					}
					justMovedUp=1;
				}
			}
			else if(justMovedUp)
			{
				if(currprefs.pandora_custom_up == -1) buttonstate[0]=0;
				else if(currprefs.pandora_custom_up == -2) buttonstate[2]=0;
				else if(currprefs.pandora_custom_up > 0)
				{		
					key = getMapping(currprefs.pandora_custom_up);
					uae4all_keystate[key] = 0;
					record_key((key << 1) | 1);
				}
				justMovedUp=0;
			}

			//DOWN
			if(dpadDown)
			{
				if(!justMovedDown)
				{
					if(currprefs.pandora_custom_down == -1) buttonstate[0]=1;
					else if(currprefs.pandora_custom_down == -2) buttonstate[2]=1;
					else if(currprefs.pandora_custom_down > 0)
					{
						key = getMapping(currprefs.pandora_custom_down);
						uae4all_keystate[key] = 1;
						record_key(key << 1);
					}
					justMovedDown=1;
				}
			}
			else if(justMovedDown)
			{
				if(currprefs.pandora_custom_down == -1) buttonstate[0]=0;
				else if(currprefs.pandora_custom_down == -2) buttonstate[2]=0;
				else if(currprefs.pandora_custom_down > 0)
				{		
					key = getMapping(currprefs.pandora_custom_down);
					uae4all_keystate[key] = 0;
					record_key((key << 1) | 1);
				}
				justMovedDown=0;
			}

			//LEFT
			if(dpadLeft)
			{
				if(!justMovedLeft)
				{
					if(currprefs.pandora_custom_left == -1) buttonstate[0]=1;
					else if(currprefs.pandora_custom_left == -2) buttonstate[2]=1;
					else if(currprefs.pandora_custom_left > 0)
					{
						key = getMapping(currprefs.pandora_custom_left);
						uae4all_keystate[key] = 1;
						record_key(key << 1);
					}
					justMovedLeft=1;
				}
			}
			else if(justMovedLeft)
			{
				if(currprefs.pandora_custom_left == -1) buttonstate[0]=0;
				else if(currprefs.pandora_custom_left == -2) buttonstate[2]=0;
				else if(currprefs.pandora_custom_left > 0)
				{		
					key = getMapping(currprefs.pandora_custom_left);
					uae4all_keystate[key] = 0;
					record_key((key << 1) | 1);
				}
				justMovedLeft=0;
			}

 			//RIGHT
			if(dpadRight)
			{
				if(!justMovedRight)
				{
					if(currprefs.pandora_custom_right == -1) buttonstate[0]=1;
					else if(currprefs.pandora_custom_right == -2) buttonstate[2]=1;
					else if(currprefs.pandora_custom_right > 0)
					{
						key = getMapping(currprefs.pandora_custom_right);
						uae4all_keystate[key] = 1;
						record_key(key << 1);
					}
					justMovedRight=1;
				}
			}
			else if(justMovedRight)
			{
				if(currprefs.pandora_custom_right == -1) buttonstate[0]=0;
				else if(currprefs.pandora_custom_right == -2) buttonstate[2]=0;
				else if(currprefs.pandora_custom_right > 0)
				{		
					key = getMapping(currprefs.pandora_custom_right);
					uae4all_keystate[key] = 0;
					record_key((key << 1) | 1);
				}
				justMovedRight=0;
			}
    }

		//(A)
		if(buttonA)
		{
			if(!justPressedA)
			{
				if(currprefs.pandora_custom_A == -1) buttonstate[0]=1;
				else if(currprefs.pandora_custom_A == -2) buttonstate[2]=1;
				else if(currprefs.pandora_custom_A > 0)
				{
					key = getMapping(currprefs.pandora_custom_A);
					uae4all_keystate[key] = 1;
					record_key(key << 1);
				}
				justPressedA=1;
			}
		}
		else if(justPressedA)
		{
			if(currprefs.pandora_custom_A == -1) buttonstate[0]=0;
			else if(currprefs.pandora_custom_A == -2) buttonstate[2]=0;
			else if(currprefs.pandora_custom_A > 0)
			{
				key = getMapping(currprefs.pandora_custom_A);
				uae4all_keystate[key] = 0;
				record_key((key << 1) | 1);
			}
			justPressedA=0;
		}

		//(B)
		if(buttonB)
		{
			if(!justPressedB)
			{
				if(currprefs.pandora_custom_B == -1) buttonstate[0]=1;
				else if(currprefs.pandora_custom_B == -2) buttonstate[2]=1;
				else if(currprefs.pandora_custom_B > 0)
				{
					key = getMapping(currprefs.pandora_custom_B);
					uae4all_keystate[key] = 1;
					record_key(key << 1);
				}
				justPressedB=1;
			}
		}
		else if(justPressedB)
		{
			if(currprefs.pandora_custom_B == -1) buttonstate[0]=0;
			else if(currprefs.pandora_custom_B == -2) buttonstate[2]=0;
			else if(currprefs.pandora_custom_B > 0)
			{
				key = getMapping(currprefs.pandora_custom_B);
				uae4all_keystate[key] = 0;
				record_key((key << 1) | 1);
			}
			justPressedB=0;
		}

		//(X)
		if(buttonX)
		{
			if(!justPressedX)
			{
				if(currprefs.pandora_custom_X == -1) buttonstate[0]=1;
				else if(currprefs.pandora_custom_X == -2) buttonstate[2]=1;
				else if(currprefs.pandora_custom_X > 0)
				{
					key = getMapping(currprefs.pandora_custom_X);
					uae4all_keystate[key] = 1;
					record_key(key << 1);
				}
				justPressedX=1;
			}
		}
		else if(justPressedX)
		{
			if(currprefs.pandora_custom_X == -1) buttonstate[0]=0;
			else if(currprefs.pandora_custom_X == -2) buttonstate[2]=0;
			else if(currprefs.pandora_custom_X > 0)
			{		
				key = getMapping(currprefs.pandora_custom_X);
				uae4all_keystate[key] = 0;
				record_key((key << 1) | 1);
			}
			justPressedX=0;
		}

		//(Y)
		if(buttonY)
		{
			if(!justPressedY)
			{
				if(currprefs.pandora_custom_Y == -1) buttonstate[0]=1;
				else if(currprefs.pandora_custom_Y == -2) buttonstate[2]=1;
				else if(currprefs.pandora_custom_Y > 0)
				{
					key = getMapping(currprefs.pandora_custom_Y);
					uae4all_keystate[key] = 1;
					record_key(key << 1);
				}
				justPressedY=1;
			}
		}
		else if(justPressedY)
		{
			if(currprefs.pandora_custom_Y == -1) buttonstate[0]=0;
			else if(currprefs.pandora_custom_Y == -2) buttonstate[2]=0;
			else if(currprefs.pandora_custom_Y > 0)
			{		
				key = getMapping(currprefs.pandora_custom_Y);
				uae4all_keystate[key] = 0;
				record_key((key << 1) | 1);
			}
			justPressedY=0;
		}

		//(L)
		if(triggerL)
		{
			if(!justPressedL)
			{
				if(currprefs.pandora_custom_L == -1) buttonstate[0]=1;
				else if(currprefs.pandora_custom_L == -2) buttonstate[2]=1;
				else if(currprefs.pandora_custom_L > 0)
				{
					key = getMapping(currprefs.pandora_custom_L);
					uae4all_keystate[key] = 1;
					record_key(key << 1);
				}
				justPressedL=1;
			}
		}
		else if(justPressedL)
		{
			if(currprefs.pandora_custom_L == -1) buttonstate[0]=0;
			else if(currprefs.pandora_custom_L == -2) buttonstate[2]=0;
			else if(currprefs.pandora_custom_L > 0)
			{		
				key = getMapping(currprefs.pandora_custom_L);
				uae4all_keystate[key] = 0;
				record_key((key << 1) | 1);
			}
			justPressedL=0;
		}

		//(R)
		if(triggerR)
		{
			if(!justPressedR)
			{
				if(currprefs.pandora_custom_R == -1) buttonstate[0]=1;
				else if(currprefs.pandora_custom_R == -2) buttonstate[2]=1;
				else if(currprefs.pandora_custom_R > 0)
				{
					key = getMapping(currprefs.pandora_custom_R);
					uae4all_keystate[key] = 1;
					record_key(key << 1);
				}
				justPressedR=1;
			}
		}
		else if(justPressedR)
		{
			if(currprefs.pandora_custom_R == -1) buttonstate[0]=0;
			else if(currprefs.pandora_custom_R == -2) buttonstate[2]=0;
			else if(currprefs.pandora_custom_R > 0)
			{		
				key = getMapping(currprefs.pandora_custom_R);
				uae4all_keystate[key] = 0;
				record_key((key << 1) | 1);
			}
			justPressedR=0;
		}
  }
  
  else // Custom controls not active
  {
    if(currprefs.pandora_custom_dpad < 2 && triggerR)
    {
  		//R+dpad = arrow keys in joystick and mouse mode
  		//dpad up
  		if(dpadUp)
  		{
  			if(!justMovedUp)
  			{
  				//arrow up
  				uae4all_keystate[AK_UP] = 1;
  				record_key(AK_UP << 1);
  				justMovedUp=1;
  			}
  		}
  		else if(justMovedUp)
  		{
  			//arrow up
  			uae4all_keystate[AK_UP] = 0;
  			record_key((AK_UP << 1) | 1);
  			justMovedUp=0;
  		}
  		//dpad down
  		if(dpadDown)
  		{
  			if(!justMovedDown)
  			{
  				//arrow down
  				uae4all_keystate[AK_DN] = 1;
  				record_key(AK_DN << 1);
  				justMovedDown=1;
  			}
  		}
  		else if(justMovedDown)
  		{
  			//arrow down
  			uae4all_keystate[AK_DN] = 0;
  			record_key((AK_DN << 1) | 1);
  			justMovedDown=0;
  		}
  		//dpad left
  		if(dpadLeft)
  		{
  			if(!justMovedLeft)
  			{
  				//arrow left
  				uae4all_keystate[AK_LF] = 1;
  				record_key(AK_LF << 1);
  				justMovedLeft=1;
  			}
  		}
  		else if(justMovedLeft)
  		{
  			//arrow left
  			uae4all_keystate[AK_LF] = 0;
  			record_key((AK_LF << 1) | 1);
  			justMovedLeft=0;
  		}
  		//dpad right
  		if (dpadRight)
  		{
  			if(!justMovedRight)
  			{
  				//arrow right
  				uae4all_keystate[AK_RT] = 1;
  				record_key(AK_RT << 1);
  				justMovedRight=1;
  			}
  		}
  		else if(justMovedRight)
  		{
  			//arrow right
  			uae4all_keystate[AK_RT] = 0;
  			record_key((AK_RT << 1) | 1);
  			justMovedRight=0;
  		}
    }

    switch(currprefs.pandora_custom_dpad)
    {
      case 0: // dPad is joystick
        // R-trigger emulates some Amiga-Keys
        if(triggerR)
		    {
    			//(A) button
    			if(buttonA)
    			{
    				if(!justPressedA)
    				{
    					//CTRL
    					uae4all_keystate[AK_CTRL] = 1;
    					record_key(AK_CTRL << 1);
    					justPressedA=1;
    				}
    			}
    			else if(justPressedA)
    			{
    				uae4all_keystate[AK_CTRL] = 0;
    				record_key((AK_CTRL << 1) | 1);
    				justPressedA=0;
    			}
    			//(B) button
    			if(buttonB)
    			{
    				if(!justPressedB)
    				{
    					//left ALT
    					uae4all_keystate[AK_LALT] = 1;
    					record_key(AK_LALT << 1);
    					justPressedB=1;
    				}
    			}
    			else if(justPressedB)
    			{
    				uae4all_keystate[AK_LALT] = 0;
    				record_key((AK_LALT << 1) | 1);
    				justPressedB=0;
    			}
    			//(X) button
    			if(buttonX)
    			{
    				if(!justPressedX)
    				{
    					//HELP
    					uae4all_keystate[AK_HELP] = 1;
    					record_key(AK_HELP << 1);
    					justPressedX=1;
    				}
    			}
    			else if(justPressedX)
    			{
    				//HELP
    				uae4all_keystate[AK_HELP] = 0;
    				record_key((AK_HELP << 1) | 1);
    				justPressedX=0;
    			}
		    }
		    // L-trigger emulates left and right mouse button
		    else if(triggerL)
        {
    			//(A) button
    			if(buttonA)
    			{
    				if(!justPressedA)
    				{
    					//left mouse-button down
    					buttonstate[0] = 1;
    					justPressedA=1;
    				}
    			}
    			else if(justPressedA)
    			{
    				//left mouse-button up
    				buttonstate[0] = 0;
    				justPressedA=0;
    			}
    			//(B) button
    			if(buttonB)
    			{
    				if(!justPressedB)
    				{
    					//right mouse-button down
    					buttonstate[2] = 1;
    					justPressedB=1;
    				}
    			}
    			else if(justPressedB)
    			{
    				//right mouse-button up
    				buttonstate[2] = 0;
    				justPressedB=0;
    			}
		    }

    		else if(currprefs.pandora_joyConf < 2)
    		{
    		  // Y-Button mapped to Space
    			if(buttonY)
    			{
    				if(!justPressedY)
    				{
    					//SPACE
    					uae4all_keystate[AK_SPC] = 1;
    					record_key(AK_SPC << 1);
    					justPressedY=1;
    				}
    			}
    			else if(justPressedY)
    			{
    				//SPACE
    				uae4all_keystate[AK_SPC] = 0;
    				record_key((AK_SPC << 1) | 1);
    				justPressedY=0;
    			}
    		}
		    
        break;
      
      case 1: // dPad is mouse
        // A and B emulates mouse buttons
    		if(buttonA)
    		{
    			if(!justPressedA)
    			{
    				//left mouse-button down
    				buttonstate[0] = 1;
    				justPressedA=1;
    			}
    		}
    		else if(justPressedA)
    		{
    			//left mouse-button up
    			buttonstate[0] = 0;
    			justPressedA=0;
    		}
    		//(B) button
    		if(buttonB)
    		{
    			if(!justPressedB)
    			{
    				//right mouse-button down
    				buttonstate[2] = 1;
    				justPressedB=1;
    			}
    		}
    		else if(justPressedB)
    		{
    			//right mouse-button up
    			buttonstate[2] = 0;
    			justPressedB=0;
    		}

        // Y emulates Space
    		if(buttonY)
    		{
    			if(!justPressedY)
    			{
    				//SPACE
    				uae4all_keystate[AK_SPC] = 1;
    				record_key(AK_SPC << 1);
    				justPressedY=1;
    			}
    		}
    		else if(justPressedY)
    		{
    			//SPACE
    			uae4all_keystate[AK_SPC] = 0;
    			record_key((AK_SPC << 1) | 1);
    			justPressedY=0;
    		}
    		
        break;
        
      case 2: // dPad is custom (stylus) -> dPad decides which mouse button is clicked
  			//dpad up
  			if (dpadUp)
  			{
  				if(!justMovedUp)
  				{
  					//left and right mouse-buttons down
  					buttonstate[0] = 1;
  					buttonstate[2] = 1;
  					stylusClickOverride = 1;
  					justMovedUp=1;
  				}
  			}
  			else if(justMovedUp)
  			{
  				//left and right mouse-buttons up
  				buttonstate[0] = 0;
  				buttonstate[2] = 0;
  				stylusClickOverride = 0;
  				justMovedUp=0;
  			}
  			//dpad down
  			if (dpadDown)
  			{
  				if(!justMovedDown)
  				{
  					//no clicks with stylus now
  					stylusClickOverride=1;
  					justMovedDown=1;
  				}
  			}
  			else if(justMovedDown)
  			{
  				//clicks active again
  				stylusClickOverride=0;
  				justMovedDown=0;
  			}
  			//dpad left
  			if (dpadLeft)
  			{
  				if(!justMovedLeft)
  				{
  					//left mouse-button down
  					buttonstate[0] = 1;
  					stylusClickOverride = 1;
  					justMovedLeft=1;
  				}
  			}
  			else if(justMovedLeft)
  			{
  				//left mouse-button up
  				buttonstate[0] = 0;
  				stylusClickOverride = 0;
  				justMovedLeft=0;
  			}
  			//dpad right
  			if (dpadRight)
  			{
  				if(!justMovedRight)
  				{
  					//right mouse-button down
  					buttonstate[2] = 1;
  					stylusClickOverride = 1;
  					justMovedRight=1;
  				}
  			}
  			else if(justMovedRight)
  			{
  				//right mouse-button up
  				buttonstate[2] = 0;
  				stylusClickOverride = 0;
  				justMovedRight=0;
  			}
  			
  			//L + up
  			if(triggerL && dpadUp)
  				stylusAdjustY-=2;
  			//L + down
  			if(triggerL && dpadDown)
  				stylusAdjustY+=2;
  			//L + left
  			if(triggerL && dpadLeft)
  				stylusAdjustX-=2;
  			//L + right
  			if(triggerL && dpadRight)
  				stylusAdjustX+=2;
  				
        break;    
    }
  }
}

void gui_disk_image_change (int unitnum, const char *name)
{
}

void gui_hd_led (int led)
{
    static int resetcounter;

    if (led == 0) {
	    resetcounter--;
	    if (resetcounter > 0)
	      return;
    }
    gui_data.hd = led;
    resetcounter = 2;
}

void gui_cd_led (int led)
{
}

void gui_led (int led, int on)
{
}

void gui_filename (int num, const char *name)
{
}

void gui_message (const char *format,...)
{
  char msg[2048];
  va_list parms;

  va_start (parms, format);
  vsprintf( msg, format, parms );
  va_end (parms);

  InGameMessage(msg);
}

void notify_user (int msg)
{
  int i=0;
  while(gui_msglist[i].num >= 0)
  {
    if(gui_msglist[i].num == msg)
    {
      gui_message(gui_msglist[i].msg);
      break;
    }
    ++i;
  }
}


void gui_lock (void)
{
}

void gui_unlock (void)
{
}


void FilterFiles(std::vector<std::string> *files, const char *filter[])
{
  for (int q=0; q<files->size(); q++)
  {
    std::string tmp = (*files)[q];
    
    bool bRemove = true;
    for(int f=0; filter[f] != NULL && strlen(filter[f]) > 0; ++f)
    {
      if(tmp.size() >= strlen(filter[f]))
      {
        if(!strcasecmp(tmp.substr(tmp.size() - strlen(filter[f])).c_str(), filter[f]))
        {
          bRemove = false;
          break;
        }
      }
    }
    
    if(bRemove)
    {
      files->erase(files->begin() + q);
      --q;
    }
  }  
}


bool DevicenameExists(const char *name)
{
  int i;
  struct uaedev_config_info *uci;
    
  for(i=0; i<MAX_HD_DEVICES; ++i)
  {
    uci = &changed_prefs.mountconfig[i];

    if(uci->devname && uci->devname[0])
    {
      if(!strcmp(uci->devname, name))
        return true;
      if(uci->volname != 0 && !strcmp(uci->volname, name))
        return true;
    }
  }
  return false;
}


void CreateDefaultDevicename(char *name)
{
  int freeNum = 0;
  bool foundFree = false;
  
  while(!foundFree && freeNum < 10)
  {
    sprintf(name, "DH%d", freeNum);
    foundFree = !DevicenameExists(name);
    ++freeNum;
  }
}


int tweakbootpri (int bp, int ab, int dnm)
{
    if (dnm)
	return -129;
    if (!ab)
	return -128;
    if (bp < -127)
	bp = -127;
    return bp;
}
