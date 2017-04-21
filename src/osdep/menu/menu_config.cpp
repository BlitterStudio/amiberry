#define _MENU_CONFIG_CPP

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "options.h"
#include "gui.h"
#include "sounddep/sound.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "uae.h"
#include "disk.h"
#include "SDL.h"

static int kickstart;

static void SetDefaultMenuSettings(struct uae_prefs* p)
{
	kickstart = 1;
}

static void replace(char* str, char replace, char toreplace)
{
	while (*str)
	{
		if (*str == toreplace) *str = replace;
		str++;
	}
}

int create_configfilename(char* dest, char* basename, int fromDir)
{
	char* p;
	p = basename + strlen(basename) - 1;
	while (*p != '/')
		p--;
	p++;
	if (fromDir == 0)
	{
		int len = strlen(p) + 1;
		char filename[len];
		strcpy(filename, p);
		char* pch = &filename[MAX_DPATH];
		while (pch != filename && *pch != '.')
			pch--;
		if (pch)
		{
			*pch = '\0';
			snprintf(dest, MAX_DPATH, "%s/conf/%s.uae", start_path_data, filename);
			return 1;
		}
	}
	else
	{
		snprintf(dest, MAX_DPATH, "%s/conf/%s.uae", start_path_data, p);
		return 1;
	}

	return 0;
}

const char* kickstarts_rom_names[] = {"kick12.rom\0", "kick13.rom\0", "kick20.rom\0", "kick31.rom\0", "aros-amiga-m68k-rom.bin\0"};
const char* extended_rom_names[] = {"\0", "\0", "\0", "\0", "aros-amiga-m68k-ext.bin\0"};
const char* kickstarts_names[] = {"KS ROM v1.2\0", "KS ROM v1.3\0", "KS ROM v2.05\0", "KS ROM v3.1\0", "\0"};

static bool CheckKickstart(struct uae_prefs* p)
{
	char kickpath[MAX_DPATH];
	int i;

	// Search via filename
	fetch_rompath(kickpath, MAX_DPATH);
	strncat(kickpath, kickstarts_rom_names[kickstart], 255);
	for (i = 0; i < lstAvailableROMs.size(); ++i)
	{
		if (!strcasecmp(lstAvailableROMs[i]->Path, kickpath))
		{
			// Found it
			strncpy(p->romfile, kickpath, sizeof(p->romfile));
			return true;
		}
	}

	// Search via name
	if (strlen(kickstarts_names[kickstart]) > 0)
	{
		for (i = 0; i < lstAvailableROMs.size(); ++i)
		{
			if (!strncasecmp(lstAvailableROMs[i]->Name, kickstarts_names[kickstart], strlen(kickstarts_names[kickstart])))
			{
				// Found it
				strncpy(p->romfile, lstAvailableROMs[i]->Path, sizeof(p->romfile));
				return true;
			}
		}
	}

	return false;
}


// In this procedure, we use changed_prefs
int loadconfig_old(struct uae_prefs* p, const char* orgpath)
{
	char path[MAX_PATH];
	int cpu_level;

	strcpy(path, orgpath);
	char* ptr = strstr(path, ".uae");
	if (ptr > nullptr)
	{
		*(ptr + 1) = '\0';
		strcat(path, "conf");
	}

	FILE* f = fopen(path, "rt");
	if (!f)
	{
		write_log("No config file %s!\n", path);
		return 0;
	}
	// Set everthing to default and clear HD settings
	default_prefs(p, 0);
	SetDefaultMenuSettings(p);

	char filebuffer[MAX_PATH];
	int dummy;

	fscanf(f, "kickstart=%d\n", &kickstart);
#if defined(AMIBERRY) || defined(ANDROIDSDL)
	fscanf(f, "scaling=%d\n", &dummy);
#else
		fscanf(f, "scaling=%d\n", &mainMenu_enableHWscaling);
#endif
	fscanf(f, "showstatus=%d\n", &p->leds_on_screen);
	fscanf(f, "mousemultiplier=%d\n", &p->input_joymouse_multiplier);
	p->input_joymouse_multiplier *= 10;
#if defined(AMIBERRY) || defined(ANDROIDSDL)
	fscanf(f, "systemclock=%d\n", &dummy); // mainMenu_throttle never changes -> removed
	fscanf(f, "syncthreshold=%d\n", &dummy); // timeslice_mode never changes -> removed
#else
		fscanf(f, "systemclock=%d\n", &mainMenu_throttle);
		fscanf(f, "syncthreshold=%d\n", &timeslice_mode);
#endif
	fscanf(f, "frameskip=%d\n", &p->gfx_framerate);
	fscanf(f, "sound=%d\n", &p->produce_sound);
	if (p->produce_sound >= 10)
	{
		p->sound_stereo = 1;
		p->produce_sound -= 10;
		if (p->produce_sound > 0)
			p->produce_sound += 1;
	}
	else
		p->sound_stereo = 0;
	fscanf(f, "soundrate=%d\n", &p->sound_freq);
	fscanf(f, "autosave=%d\n", &dummy);
	int joybuffer = 0;
	fscanf(f, "joyconf=%d\n", &joybuffer);
	fscanf(f, "autofireRate=%d\n", &p->input_autofire_linecnt);
	p->input_autofire_linecnt = p->input_autofire_linecnt * 312;
	fscanf(f, "autofire=%d\n", &dummy);
	fscanf(f, "stylusOffset=%d\n", &dummy);
	fscanf(f, "scanlines=%d\n", &dummy);
#if defined(AMIBERRY) || defined(ANDROIDSDL)
	fscanf(f, "ham=%d\n", &dummy);
#else
		fscanf(f, "ham=%d\n", &mainMenu_ham);
#endif
	fscanf(f, "enableScreenshots=%d\n", &dummy);
	fscanf(f, "floppyspeed=%d\n", &p->floppy_speed);
	fscanf(f, "drives=%d\n", &p->nr_floppies);
	fscanf(f, "videomode=%d\n", &p->ntscmode);
	if (p->ntscmode)
		p->chipset_refreshrate = 60;
	else
		p->chipset_refreshrate = 50;
	fscanf(f, "displayedLines=%d\n", &p->gfx_size.height);
	fscanf(f, "screenWidth=%d\n", &p->gfx_size_fs.width);
	fscanf(f, "amiberry.custom_controls=%d\n", &p->customControls);
	fscanf(f, "amiberry.custom_up=%d\n", &p->custom_up);
	fscanf(f, "amiberry.custom_down=%d\n", &p->custom_down);
	fscanf(f, "amiberry.custom_left=%d\n", &p->custom_left);
	fscanf(f, "amiberry.custom_right=%d\n", &p->custom_right);
	fscanf(f, "amiberry.custom_a=%d\n", &p->custom_a);
	fscanf(f, "amiberry.custom_b=%d\n", &p->custom_b);
	fscanf(f, "amiberry.custom_x=%d\n", &p->custom_x);
	fscanf(f, "amiberry.custom_y=%d\n", &p->custom_y);
	fscanf(f, "amiberry.custom_l=%d\n", &p->custom_l);
	fscanf(f, "amiberry.custom_r=%d\n", &p->custom_r);
	fscanf(f, "amiberry.custom_play=%d\n", &p->custom_play);
	fscanf(f, "cpu=%d\n", &cpu_level);
	if (cpu_level > 0) // M68000
	// Was old format
		cpu_level = 2; // M68020
	fscanf(f, "chipset=%d\n", &p->chipset_mask);
	p->immediate_blits = (p->chipset_mask & 0x100) == 0x100;
	switch (p->chipset_mask & 0xff)
	{
	case 1:
		p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
		break;
	case 2:
		p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
		break;
	default:
		p->chipset_mask = CSMASK_ECS_AGNUS;
		break;
	}
	fscanf(f, "cpu=%d\n", &p->m68k_speed);
	if (p->m68k_speed < 0)
	{
		// New version of this option
		p->m68k_speed = -p->m68k_speed;
	}
	else
	{
		// Old version (500 5T 1200 12T 12T2)
		if (p->m68k_speed >= 2)
		{
			// 1200: set to 68020 with 14 MHz
			cpu_level = 2; // M68020
			p->m68k_speed--;
			if (p->m68k_speed > 2)
				p->m68k_speed = 2;
		}
	}
	if (p->m68k_speed == 1)
		p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;
	if (p->m68k_speed == 2)
		p->m68k_speed = M68K_SPEED_25MHZ_CYCLES;
	p->cachesize = 0;
	p->cpu_compatible = false;
	switch (cpu_level)
	{
	case 0:
		p->cpu_model = 68000;
		p->fpu_model = 0;
		break;
	case 1:
		p->cpu_model = 68010;
		p->fpu_model = 0;
		break;
	case 2:
		p->cpu_model = 68020;
		p->fpu_model = 0;
		break;
	case 3:
		p->cpu_model = 68020;
		p->fpu_model = 68881;
		break;
	case 4:
		p->cpu_model = 68040;
		p->fpu_model = 68881;
		break;
	}

	disk_eject(0);
	disk_eject(1);
	disk_eject(2);
	disk_eject(3);
	fscanf(f, "df0=%s\n", &filebuffer);
	replace(filebuffer, ' ', '|');
	if (DISK_validate_filename(p, filebuffer, 0, nullptr, nullptr, nullptr))
		strcpy(p->floppyslots[0].df, filebuffer);
	else
		p->floppyslots[0].df[0] = 0;
	disk_insert(0, filebuffer);
	if (p->nr_floppies > 1)
	{
		memset(filebuffer, 0, 256);
		fscanf(f, "df1=%s\n", &filebuffer);
		replace(filebuffer, ' ', '|');
		if (DISK_validate_filename(p, filebuffer, 0, nullptr, nullptr, nullptr))
			strcpy(p->floppyslots[1].df, filebuffer);
		else
			p->floppyslots[1].df[0] = 0;
		disk_insert(1, filebuffer);
	}
	if (p->nr_floppies > 2)
	{
		memset(filebuffer, 0, 256);
		fscanf(f, "df2=%s\n", &filebuffer);
		replace(filebuffer, ' ', '|');
		if (DISK_validate_filename(p, filebuffer, 0, nullptr, nullptr, nullptr))
			strcpy(p->floppyslots[2].df, filebuffer);
		else
			p->floppyslots[2].df[0] = 0;
		disk_insert(2, filebuffer);
	}
	if (p->nr_floppies > 3)
	{
		memset(filebuffer, 0, sizeof filebuffer);
		fscanf(f, "df3=%s\n", &filebuffer);
		replace(filebuffer, ' ', '|');
		if (DISK_validate_filename(p, filebuffer, 0, nullptr, nullptr, nullptr))
			strcpy(p->floppyslots[3].df, filebuffer);
		else
			p->floppyslots[3].df[0] = 0;
		disk_insert(3, filebuffer);
	}

	for (int i = 0; i < 4; ++i)
	{
		if (i < p->nr_floppies)
			p->floppyslots[i].dfxtype = DRV_35_DD;
		else
			p->floppyslots[i].dfxtype = DRV_NONE;
	}

	fscanf(f, "chipmemory=%d\n", &p->chipmem_size);
	if (p->chipmem_size < 10)
	// Was saved in old format
		p->chipmem_size = 0x80000 << p->chipmem_size;
	fscanf(f, "slowmemory=%d\n", &p->bogomem_size);
	if (p->bogomem_size > 0 && p->bogomem_size < 10)
	// Was saved in old format
		p->bogomem_size =
			(p->bogomem_size <= 2) ? 0x080000 << p->bogomem_size :
				(p->bogomem_size == 3) ? 0x180000 : 0x1c0000;
	fscanf(f, "fastmemory=%d\n", &p->fastmem_size);
	if (p->fastmem_size > 0 && p->fastmem_size < 10)
	// Was saved in old format
		p->fastmem_size = 0x080000 << p->fastmem_size;

	fclose(f);

	CheckKickstart(p);

	return 1;
}
