 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Config file handling
  * This still needs some thought before it's complete...
  *
  * Copyright 1998 Brian King, Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "autoconf.h"
#include "custom.h"
#include "inputdevice.h"
#include "savestate.h"
#include "memory.h"
#include "gui.h"
#include "newcpu.h"
#include "zfile.h"
#include "filesys.h"
#include "fsdb.h"
#include "disk.h"
#include "sd-pandora/sound.h"

static int config_newfilesystem;
static struct strlist *temp_lines;
static struct zfile *default_file;

/* @@@ need to get rid of this... just cut part of the manual and print that
 * as a help text.  */
struct cfg_lines
{
    const char *config_label, *config_help;
};

static const struct cfg_lines opttable[] =
{
    {"help", "Prints this help" },
    {"config_description", "" },
    {"config_info", "" },
    {"use_gui", "Enable the GUI?  If no, then goes straight to emulator" },
    {"use_debugger", "Enable the debugger?" },
    {"cpu_speed", "can be max, real, or a number between 1 and 20" },
    {"cpu_model", "Can be 68000, 68010, 68020, 68030, 68040, 68060" },
    {"fpu_model", "Can be 68881, 68882, 68040, 68060" },
    {"cpu_compatible", "yes enables compatibility-mode" },
    {"cpu_24bit_addressing", "must be set to 'no' in order for Z3mem or P96mem to work" },
    {"autoconfig", "yes = add filesystems and extra ram" },
    {"log_illegal_mem", "print illegal memory access by Amiga software?" },
    {"fastmem_size", "Size in megabytes of fast-memory" },
    {"chipmem_size", "Size in megabytes of chip-memory" },
    {"bogomem_size", "Size in megabytes of bogo-memory at 0xC00000" },
    {"a3000mem_size", "Size in megabytes of A3000 memory" },
    {"gfxcard_size", "Size in megabytes of Picasso96 graphics-card memory" },
    {"z3mem_size", "Size in megabytes of Zorro-III expansion memory" },
    {"gfx_test_speed", "Test graphics speed?" },
    {"gfx_framerate", "Print every nth frame" },
    {"gfx_width", "Screen width" },
    {"gfx_height", "Screen height" },
    {"gfx_refreshrate", "Fullscreen refresh rate" },
    {"gfx_vsync", "Sync screen refresh to refresh rate" },
    {"gfx_lores", "Treat display as lo-res?" },
    {"gfx_linemode", "Can be none, double, or scanlines" },
    {"gfx_fullscreen_amiga", "Amiga screens are fullscreen?" },
    {"gfx_fullscreen_picasso", "Picasso screens are fullscreen?" },
    {"gfx_correct_aspect", "Correct aspect ratio?" },
    {"gfx_center_horizontal", "Center display horizontally?" },
    {"gfx_center_vertical", "Center display vertically?" },
    {"gfx_colour_mode", "" },
    {"32bit_blits", "Enable 32 bit blitter emulation" },
    {"immediate_blits", "Perform blits immediately" },
    {"show_leds", "LED display" },
    {"keyboard_leds", "Keyboard LEDs" },
    {"gfxlib_replacement", "Use graphics.library replacement?" },
    {"sound_output", "" },
    {"sound_frequency", "" },
    {"sound_bits", "" },
    {"sound_channels", "" },
    {"sound_max_buff", "" },
    {"comp_trustbyte", "How to access bytes in compiler (direct/indirect/indirectKS/afterPic" },
    {"comp_trustword", "How to access words in compiler (direct/indirect/indirectKS/afterPic" },
    {"comp_trustlong", "How to access longs in compiler (direct/indirect/indirectKS/afterPic" },
    {"comp_nf", "Whether to optimize away flag generation where possible" },
    {"comp_fpu", "Whether to provide JIT FPU emulation" },
    {"compforcesettings", "Whether to force the JIT compiler settings" },
    {"cachesize", "How many MB to use to buffer translated instructions"},
    {"override_dga_address","Address from which to map the frame buffer (upper 16 bits) (DANGEROUS!)"},
    {"avoid_cmov", "Set to yes on machines that lack the CMOV instruction" },
    {"avoid_dga", "Set to yes if the use of DGA extension creates problems" },
    {"avoid_vid", "Set to yes if the use of the Vidmode extension creates problems" },
    {"parallel_on_demand", "" },
    {"serial_on_demand", "" },
    {"scsi", "scsi.device emulation" },
    {"joyport0", "" },
    {"joyport1", "" },
    {"pci_devices", "List of PCI devices to make visible to the emulated Amiga" },
    {"kickstart_rom_file", "Kickstart ROM image, (C) Copyright Amiga, Inc." },
    {"kickstart_ext_rom_file", "Extended Kickstart ROM image, (C) Copyright Amiga, Inc." },
    {"kickstart_key_file", "Key-file for encrypted ROM images (from Cloanto's Amiga Forever)" },
    {"flash_ram_file", "Flash/battery backed RAM image file." },
    {"cart_file", "Freezer cartridge ROM image file." },
    {"floppy0", "Diskfile for drive 0" },
    {"floppy1", "Diskfile for drive 1" },
    {"floppy2", "Diskfile for drive 2" },
    {"floppy3", "Diskfile for drive 3" },
    {"hardfile", "access,sectors, surfaces, reserved, blocksize, path format" },
    {"filesystem", "access,'Amiga volume-name':'host directory path' - where 'access' can be 'read-only' or 'read-write'" },
    {"catweasel", "Catweasel board io base address" }
};

static const char *guimode1[] = { "no", "yes", "nowait", 0 };
static const char *guimode2[] = { "false", "true", "nowait", 0 };
static const char *guimode3[] = { "0", "1", "nowait", 0 };
static const char *csmode[] = { "ocs", "ecs_agnus", "ecs_denise", "ecs", "aga", 0 };
static const char *linemode1[] = { "none", "double", "scanlines", 0 };
static const char *linemode2[] = { "n", "d", "s", 0 };
static const char *speedmode[] = { "max", "real", 0 };
static const char *colormode1[] = { "8bit", "15bit", "16bit", "8bit_dither", "4bit_dither", "32bit", 0 };
static const char *colormode2[] = { "8", "15", "16", "8d", "4d", "32", 0 };
static const char *soundmode1[] = { "none", "interrupts", "normal", "exact", 0 };
static const char *soundmode2[] = { "none", "interrupts", "good", "best", 0 };
static const char *centermode1[] = { "none", "simple", "smart", 0 };
static const char *centermode2[] = { "false", "true", "smart", 0 };
static const char *stereomode[] = { "mono", "stereo", "clonedstereo", "4ch", "clonedstereo6ch", "6ch", "mixed", 0 };
static const char *interpolmode[] = { "none", "anti", "sinc", "rh", "crux", 0 };
static const char *collmode[] = { "none", "sprites", "playfields", "full", 0 };
static const char *compmode[] = { "direct", "indirect", "indirectKS", "afterPic", 0 };
static const char *flushmode[]   = { "soft", "hard", 0 };
static const char *kbleds[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD", 0 };
static const char *soundfiltermode1[] = { "off", "emulated", "on", 0 };
static const char *soundfiltermode2[] = { "standard", "enhanced", 0 };
static const char *lorestype1[] = { "lores", "hires", "superhires" };
static const char *lorestype2[] = { "true", "false" };
static const char *loresmode[] = { "normal", "filtered", 0 };
#ifdef GFXFILTER
static const char *filtermode1[] = { "no_16", "bilinear_16", "no_32", "bilinear_32", 0 };
static const char *filtermode2[] = { "0x", "1x", "2x", "3x", "4x", 0 };
#endif
static const char *cartsmode[] = { "none", "hrtmon", 0 };
static const char *idemode[] = { "none", "a600/a1200", "a4000", 0 };
static const char *rtctype[] = { "none", "MSM6242B", "RP5C01A", 0 };
static const char *ciaatodmode[] = { "vblank", "50hz", "60hz", 0 };
static const char *ksmirrortype[] = { "none", "e0", "a8+e0", 0 };
static const char *cscompa[] = {
    "-", "Generic", "CDTV", "CD32", "A500", "A500+", "A600",
    "A1000", "A1200", "A2000", "A3000", "A3000T", "A4000", "A4000T", 0
};
/* 3-state boolean! */
static const char *fullmodes[] = { "false", "true", /* "FILE_NOT_FOUND", */ "fullwindow", 0 };
/* bleh for compatibility */
static const char *scsimode[] = { "false", "true", "scsi", 0 };

static const char *obsolete[] = {
    "accuracy", "gfx_opengl", "gfx_32bit_blits", "32bit_blits",
    "gfx_immediate_blits", "gfx_ntsc", "win32", "gfx_filter_bits",
    "sound_pri_cutoff", "sound_pri_time", "sound_min_buff",
    "gfx_test_speed", "gfxlib_replacement", "enforcer", "catweasel_io",
    "kickstart_key_file", "sound_adjust",
    "serial_hardware_dtrdsr",
    0
};

#define UNEXPANDED "$(FILE_PATH)"

static void trimws (char *s)
{
    /* Delete trailing whitespace.  */
    int len = strlen (s);
    while (len > 0 && strcspn (s + len - 1, "\t \r\n") == 0)
        s[--len] = '\0';
}

static int match_string (const char *table[], const char *str)
{
    int i;
    for (i = 0; table[i] != 0; i++)
	if (strcasecmp (table[i], str) == 0)
	    return i;
    return -1;
}

char *cfgfile_subst_path (const char *path, const char *subst, const char *file)
{
    /* @@@ use strcasecmp for some targets.  */
    if (strlen (path) > 0 && strncmp (file, path, strlen (path)) == 0) {
	int l;
	char *p = (char *) xmalloc (strlen (file) + strlen (subst) + 2);
	strcpy (p, subst);
	l = strlen (p);
	while (l > 0 && p[l - 1] == '/')
	    p[--l] = '\0';
	l = strlen (path);
	while (file[l] == '/')
	    l++;
	strcat (p, "/"); strcat (p, file + l);
	return p;
    }
    return my_strdup (file);
}

static int isdefault (const char *s)
{
    char tmp[MAX_DPATH];
    if (!default_file)
	return 0;
    zfile_fseek (default_file, 0, SEEK_SET);
    while (zfile_fgets (tmp, sizeof tmp, default_file)) {
	if (!strcmp (tmp, s))
	    return 1;
    }
    return 0;
}

void cfgfile_write (struct zfile *f, const char *format,...)
{
    va_list parms;
    char tmp[CONFIG_BLEN];

    va_start (parms, format);
    vsprintf (tmp, format, parms);
    zfile_fwrite (tmp, 1, strlen (tmp), f);
    va_end (parms);
}

void cfgfile_dwrite (struct zfile *f, const char *format,...)
{
    va_list parms;
    char tmp[CONFIG_BLEN];

    va_start (parms, format);
    vsprintf (tmp, format, parms);
    if (1 || !isdefault (tmp))
	zfile_fwrite (tmp, 1, strlen (tmp), f);
    va_end (parms);
}

void cfgfile_target_write (struct zfile *f, char *format,...)
{
    va_list parms;
    char tmp[CONFIG_BLEN];

    va_start (parms, format);
    vsprintf (tmp + strlen (TARGET_NAME) + 1, format, parms);
    memcpy (tmp, TARGET_NAME, strlen (TARGET_NAME));
    tmp[strlen (TARGET_NAME)] = '.';
    zfile_fwrite (tmp, 1, strlen (tmp), f);
    va_end (parms);
}

void cfgfile_target_dwrite (struct zfile *f, char *format,...)
{
    va_list parms;
    char tmp[CONFIG_BLEN];

    va_start (parms, format);
    vsprintf (tmp + strlen (TARGET_NAME) + 1, format, parms);
    memcpy (tmp, TARGET_NAME, strlen (TARGET_NAME));
    tmp[strlen (TARGET_NAME)] = '.';
    if (!isdefault (tmp))
	zfile_fwrite (tmp, 1, strlen (tmp), f);
    va_end (parms);
}

static void write_filesys_config (struct uae_prefs *p, const char *unexpanded,
				  const char *default_path, struct zfile *f)
{
    int i;
    char tmp[MAX_DPATH], tmp2[MAX_DPATH];
    const char *hdcontrollers[] = { "uae", 
  "ide0", "ide1", "ide2", "ide3",
	"scsi0", "scsi1", "scsi2", "scsi3", "scsi4", "scsi5", "scsi6",
	"scsram", "scside" }; /* scsram = smart card sram = pcmcia sram card */

    for (i = 0; i < p->mountitems; i++) {
	struct uaedev_config_info *uci = &p->mountconfig[i];
	char *str;
  int bp = uci->bootpri;

	if (!uci->autoboot)
	    bp = -128;
	if (uci->donotmount)
	    bp = -129;
	str = cfgfile_subst_path (default_path, unexpanded, uci->rootdir);
	if (!uci->ishdf) {
	    sprintf (tmp, "%s,%s:%s:%s,%d\n", uci->readonly ? "ro" : "rw",
		uci->devname ? uci->devname : "", uci->volname, str, bp);
	    sprintf (tmp2, "filesystem2=%s", tmp);
	    zfile_fputs (f, tmp2);
#if 0
	    sprintf (tmp2, "filesystem=%s,%s:%s\n", uci->readonly ? "ro" : "rw",
		uci->volname, str);
	    zfile_fputs (f, tmp2);
#endif
	} else {
	    sprintf (tmp, "%s,%s:%s,%d,%d,%d,%d,%d,%s,%s\n",
		     uci->readonly ? "ro" : "rw",
		     uci->devname ? uci->devname : "", str,
		     uci->sectors, uci->surfaces, uci->reserved, uci->blocksize,
		     bp, uci->filesys ? uci->filesys : "", hdcontrollers[uci->controller]);
	    sprintf (tmp2, "hardfile2=%s", tmp);
	    zfile_fputs (f, tmp2);
#if 0
	    sprintf (tmp2, "hardfile=%s,%d,%d,%d,%d,%s\n",
		     uci->readonly ? "ro" : "rw", uci->sectors,
		     uci->surfaces, uci->reserved, uci->blocksize, str);
	    zfile_fputs (f, tmp2);
#endif
	}
	sprintf (tmp2, "uaehf%d=%s,%s", i, uci->ishdf ? "hdf" : "dir", tmp);
	zfile_fputs (f, tmp2);
	xfree (str);
    }
}

static void write_compatibility_cpu(struct zfile *f, struct uae_prefs *p)
{
    char tmp[100];
    int model;

    model = p->cpu_model;
    if (model == 68030)
	model = 68020;
    if (model == 68060)
	model = 68040;
    if (p->address_space_24 && model == 68020)
	strcpy (tmp, "68ec020");
    else
	sprintf(tmp, "%d", model);
    if (model == 68020 && (p->fpu_model == 68881 || p->fpu_model == 68882)) 
	strcat(tmp,"/68881");
    cfgfile_write (f, "cpu_type=%s\n", tmp);
}

void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type)
{
    struct strlist *sl;
    char *str;
    int i;

    cfgfile_write (f, "config_description=%s\n", p->description);
    cfgfile_write (f, "config_hardware=%s\n", (type & CONFIG_TYPE_HARDWARE) ? "true" : "false");
    cfgfile_write (f, "config_host=%s\n", (type & CONFIG_TYPE_HOST) ? "true" : "false");
    if (p->info[0])
	cfgfile_write (f, "config_info=%s\n", p->info);
    cfgfile_write (f, "config_version=%d.%d.%d\n", UAEMAJOR, UAEMINOR, UAESUBREV);
   
    for (sl = p->all_lines; sl; sl = sl->next) {
	if (sl->unknown)
	    cfgfile_write (f, "%s=%s\n", sl->option, sl->value);
    }

    cfgfile_write (f, "%s.rom_path=%s\n", TARGET_NAME, p->path_rom);
    cfgfile_write (f, "%s.floppy_path=%s\n", TARGET_NAME, p->path_floppy);
    cfgfile_write (f, "%s.hardfile_path=%s\n", TARGET_NAME, p->path_hardfile);

    cfgfile_write (f, "; host-specific\n");

    target_save_options (f, p);

    cfgfile_write (f, "; common\n");

    cfgfile_write (f, "use_gui=%s\n", guimode1[p->start_gui]);
    str = cfgfile_subst_path (p->path_rom, UNEXPANDED, p->romfile);
    cfgfile_write (f, "kickstart_rom_file=%s\n", str);
    free (str);
    str = cfgfile_subst_path (p->path_rom, UNEXPANDED, p->romextfile);
    cfgfile_write (f, "kickstart_ext_rom_file=%s\n", str);
    free (str);

    p->nr_floppies = 4;
    for (i = 0; i < 4; i++) {
	str = cfgfile_subst_path (p->path_floppy, UNEXPANDED, p->df[i]);
	cfgfile_write (f, "floppy%d=%s\n", i, str);
	free (str);
	cfgfile_dwrite (f, "floppy%dtype=%d\n", i, p->dfxtype[i]);
	if (p->dfxtype[i] < 0 && p->nr_floppies > i)
	    p->nr_floppies = i;
    }

    cfgfile_write (f, "nr_floppies=%d\n", p->nr_floppies);
    cfgfile_write (f, "floppy_speed=%d\n", p->floppy_speed);

    cfgfile_write (f, "sound_output=%s\n", soundmode1[p->produce_sound]);
    cfgfile_write (f, "sound_channels=%s\n", stereomode[p->sound_stereo]);
    cfgfile_write (f, "sound_stereo_separation=%d\n", p->sound_stereo_separation);
    cfgfile_write (f, "sound_stereo_mixing_delay=%d\n", p->sound_mixed_stereo_delay >= 0 ? p->sound_mixed_stereo_delay : 0);
    cfgfile_write (f, "sound_frequency=%d\n", p->sound_freq);
    cfgfile_write (f, "sound_interpol=%s\n", interpolmode[p->sound_interpol]);
    cfgfile_write (f, "sound_filter=%s\n", soundfiltermode1[p->sound_filter]);
    cfgfile_write (f, "sound_filter_type=%s\n", soundfiltermode2[p->sound_filter_type]);
    cfgfile_write (f, "sound_auto=%s\n", p->sound_auto ? "yes" : "no");

    cfgfile_write (f, "cachesize=%d\n", p->cachesize);

    cfgfile_write (f, "synchronize_clock=%s\n", p->tod_hack ? "yes" : "no");

    cfgfile_dwrite (f, "gfx_framerate=%d\n", p->gfx_framerate);
    cfgfile_dwrite (f, "gfx_width=%d\n", p->gfx_size.width);
    cfgfile_dwrite (f, "gfx_height=%d\n", p->gfx_size.height);
    cfgfile_dwrite (f, "gfx_width_windowed=%d\n", p->gfx_size_win.width);
    cfgfile_dwrite (f, "gfx_height_windowed=%d\n", p->gfx_size_win.height);
    cfgfile_dwrite (f, "gfx_width_fullscreen=%d\n", p->gfx_size_fs.width);
    cfgfile_dwrite (f, "gfx_height_fullscreen=%d\n", p->gfx_size_fs.height);

    cfgfile_write (f, "immediate_blits=%s\n", p->immediate_blits ? "true" : "false");
    cfgfile_write (f, "fast_copper=%s\n", p->fast_copper ? "true" : "false");
    cfgfile_write (f, "ntsc=%s\n", p->ntscmode ? "true" : "false");
    cfgfile_dwrite (f, "show_leds=%s\n", p->leds_on_screen ? "true" : "false");
    if (p->chipset_mask & CSMASK_AGA)
	cfgfile_dwrite (f, "chipset=aga\n");
    else if ((p->chipset_mask & CSMASK_ECS_AGNUS) && (p->chipset_mask & CSMASK_ECS_DENISE))
	cfgfile_dwrite (f, "chipset=ecs\n");
    else if (p->chipset_mask & CSMASK_ECS_AGNUS)
	cfgfile_dwrite (f, "chipset=ecs_agnus\n");
    else if (p->chipset_mask & CSMASK_ECS_DENISE)
	cfgfile_dwrite (f, "chipset=ecs_denise\n");
    else
	cfgfile_dwrite (f, "chipset=ocs\n");
    cfgfile_write (f, "collision_level=%s\n", collmode[p->collision_level]);

    cfgfile_dwrite (f, "a1000ram=%s\n", p->cs_a1000ram ? "true" : "false");

    cfgfile_write (f, "fastmem_size=%d\n", p->fastmem_size / 0x100000);
    cfgfile_write (f, "z3mem_size=%d\n", p->z3fastmem_size / 0x100000);
    cfgfile_write (f, "z3mem_start=0x%x\n", p->z3fastmem_start);
    cfgfile_write (f, "bogomem_size=%d\n", p->bogomem_size / 0x40000);
    cfgfile_write (f, "gfxcard_size=%d\n", p->gfxmem_size / 0x100000);
    cfgfile_write (f, "chipmem_size=%d\n", p->chipmem_size == 0x20000 ? -1 : (p->chipmem_size == 0x40000 ? 0 : p->chipmem_size / 0x80000));

    if (p->m68k_speed > 0)
	cfgfile_write (f, "finegrain_cpu_speed=%d\n", p->m68k_speed);
    else
	cfgfile_write (f, "cpu_speed=%s\n", p->m68k_speed == -1 ? "max" : "real");

    /* do not reorder start */
    write_compatibility_cpu(f, p);
    cfgfile_write (f, "cpu_model=%d\n", p->cpu_model);
    if (p->fpu_model)
	cfgfile_write (f, "fpu_model=%d\n", p->fpu_model);
    cfgfile_write (f, "cpu_compatible=%s\n", p->cpu_compatible ? "true" : "false");
    cfgfile_write (f, "cpu_24bit_addressing=%s\n", p->address_space_24 ? "true" : "false");
    /* do not reorder end */
    cfgfile_write (f, "rtg_nocustom=%s\n", p->picasso96_nocustom ? "true" : "false");

#ifdef FILESYS
    write_filesys_config (p, UNEXPANDED, p->path_hardfile, f);
#endif
    write_inputdevice_config (p, f);

    /* Don't write gfxlib/gfx_test_speed options.  */
}

int cfgfile_yesno (const char *option, const char *value, const char *name, int *location)
{
    if (strcmp (option, name) != 0)
	return 0;
    if (strcasecmp (value, "yes") == 0 || strcasecmp (value, "y") == 0
	|| strcasecmp (value, "true") == 0 || strcasecmp (value, "t") == 0)
	*location = 1;
    else if (strcasecmp (value, "no") == 0 || strcasecmp (value, "n") == 0
	|| strcasecmp (value, "false") == 0 || strcasecmp (value, "f") == 0)
	*location = 0;
    else {
	write_log ("Option `%s' requires a value of either `yes' or `no'.\n", option);
	return -1;
    }
    return 1;
}

int cfgfile_intval (char *option, char *value, const char *name, int *location, int scale)
{
    int base = 10;
    char *endptr;
    if (strcmp (option, name) != 0)
	return 0;
    /* I guess octal isn't popular enough to worry about here...  */
    if (value[0] == '0' && value[1] == 'x')
	value += 2, base = 16;
    *location = strtol (value, &endptr, base) * scale;

    if (*endptr != '\0' || *value == '\0') {
	write_log ("Option `%s' requires a numeric argument.\n", option);
	return -1;
    }
    return 1;
}

int cfgfile_strval (const char *option, const char *value, const char *name, int *location, const char *table[], int more)
{
    int val;
    if (strcmp (option, name) != 0)
	return 0;
    val = match_string (table, value);
    if (val == -1) {
	if (more)
	    return 0;

	write_log ("Unknown value for option `%s'.\n", option);
	return -1;
    }
    *location = val;
    return 1;
}

int cfgfile_string (const char *option, const char *value, const char *name, char *location, int maxsz)
{
    if (strcmp (option, name) != 0)
	return 0;
    strncpy (location, value, maxsz - 1);
    location[maxsz - 1] = '\0';
    return 1;
}

static int getintval (char **p, int *result, int delim)
{
    char *value = *p;
    int base = 10;
    char *endptr;
    char *p2 = strchr (*p, delim);

    if (p2 == 0)
	return 0;

    *p2++ = '\0';

    if (value[0] == '0' && toupper (value[1]) == 'X')
	value += 2, base = 16;
    *result = strtol (value, &endptr, base);
    *p = p2;

    if (*endptr != '\0' || *value == '\0')
	return 0;

    return 1;
}

static int getintval2 (char **p, int *result, int delim)
{
    char *value = *p;
    int base = 10;
    char *endptr;
    char *p2 = strchr (*p, delim);

    if (p2 == 0) {
	p2 = strchr (*p, 0);
	if (p2 == 0) {
	    *p = 0;
	    return 0;
	}
    }
    if (*p2 != 0)
	*p2++ = '\0';

    if (value[0] == '0' && toupper (value[1]) == 'X')
	value += 2, base = 16;
    *result = strtol (value, &endptr, base);
    *p = p2;

    if (*endptr != '\0' || *value == '\0') {
	*p = 0;
	return 0;
    }

    return 1;
}

static void set_chipset_mask (struct uae_prefs *p, int val)
{
    p->chipset_mask = (val == 0 ? 0
		       : val == 1 ? CSMASK_ECS_AGNUS
		       : val == 2 ? CSMASK_ECS_DENISE
		       : val == 3 ? CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS
		       : CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS);
}

static int cfgfile_parse_host (struct uae_prefs *p, char *option, char *value)
{
    int i;
    char *section = 0;
    char *tmpp;
    char tmpbuf[CONFIG_BLEN];

    if (memcmp (option, "input.", 6) == 0) {
	read_inputdevice_config (p, option, value);
	return 1;
    }

    for (tmpp = option; *tmpp != '\0'; tmpp++)
	if (isupper (*tmpp))
	    *tmpp = tolower (*tmpp);
    tmpp = strchr (option, '.');
    if (tmpp) {
	section = option;
	option = tmpp + 1;
	*tmpp = '\0';
	if (strcmp (section, TARGET_NAME) == 0) {
	    /* We special case the various path options here.  */
	    if (cfgfile_string (option, value, "rom_path", p->path_rom, sizeof p->path_rom)
  		|| cfgfile_string (option, value, "floppy_path", p->path_floppy, sizeof p->path_floppy)
  		|| cfgfile_string (option, value, "hardfile_path", p->path_hardfile, sizeof p->path_hardfile))
	    	return 1;
  	  return target_parse_option (p, option, value);
	}
	return 0;
    }

    if (cfgfile_intval (option, value, "sound_frequency", &p->sound_freq, 1)
	|| cfgfile_intval (option, value, "sound_stereo_separation", &p->sound_stereo_separation, 1)
	|| cfgfile_intval (option, value, "sound_stereo_mixing_delay", &p->sound_mixed_stereo_delay, 1)

	|| cfgfile_intval (option, value, "gfx_framerate", &p->gfx_framerate, 1)
	|| cfgfile_intval (option, value, "gfx_width_windowed", &p->gfx_size_win.width, 1)
	|| cfgfile_intval (option, value, "gfx_height_windowed", &p->gfx_size_win.height, 1)
	|| cfgfile_intval (option, value, "gfx_width_fullscreen", &p->gfx_size_fs.width, 1)
	|| cfgfile_intval (option, value, "gfx_height_fullscreen", &p->gfx_size_fs.height, 1))

	    return 1;

	if (cfgfile_string (option, value, "config_info", p->info, sizeof p->info)
	|| cfgfile_string (option, value, "config_description", p->description, sizeof p->description))
	    return 1;

	if (cfgfile_yesno (option, value, "sound_auto", &p->sound_auto)
	|| cfgfile_yesno (option, value, "show_leds", &p->leds_on_screen)
	|| cfgfile_yesno (option, value, "synchronize_clock", &p->tod_hack))
	    return 1;

    if (cfgfile_strval (option, value, "sound_output", &p->produce_sound, soundmode1, 1)
	|| cfgfile_strval (option, value, "sound_output", &p->produce_sound, soundmode2, 0)
	|| cfgfile_strval (option, value, "sound_interpol", &p->sound_interpol, interpolmode, 0)
	|| cfgfile_strval (option, value, "sound_filter", &p->sound_filter, soundfiltermode1, 0)
	|| cfgfile_strval (option, value, "sound_filter_type", &p->sound_filter_type, soundfiltermode2, 0)
	|| cfgfile_strval (option, value, "use_gui", &p->start_gui, guimode1, 1)
	|| cfgfile_strval (option, value, "use_gui", &p->start_gui, guimode2, 1)
	|| cfgfile_strval (option, value, "use_gui", &p->start_gui, guimode3, 0))
	    return 1;
	

    if (strcmp (option, "gfx_width") == 0 || strcmp (option, "gfx_height") == 0) {
	cfgfile_intval (option, value, "gfx_width", &p->gfx_size.width, 1);
	cfgfile_intval (option, value, "gfx_height", &p->gfx_size.height, 1);
	return 1;
    }

    if (cfgfile_string (option, value, "statefile", tmpbuf, sizeof (tmpbuf))) {
	savestate_state = STATE_DORESTORE;
	strcpy (savestate_fname, tmpbuf);
	return 1;
    }

    if (cfgfile_strval (option, value, "sound_channels", &p->sound_stereo, stereomode, 1)) {
	if (p->sound_stereo == SND_NONE) { /* "mixed stereo" compatibility hack */
	    p->sound_stereo = SND_STEREO;
	    p->sound_mixed_stereo_delay = 5;
	    p->sound_stereo_separation = 7;
	}
	return 1;
    }

    if (cfgfile_string (option, value, "config_version", tmpbuf, sizeof (tmpbuf))) {
	char *tmpp2;
	tmpp = strchr (value, '.');
	if (tmpp) {
	    *tmpp++ = 0;
	    tmpp2 = tmpp;
	    p->config_version = atol (tmpbuf) << 16;
	    tmpp = strchr (tmpp, '.');
	    if (tmpp) {
		*tmpp++ = 0;
		p->config_version |= atol (tmpp2) << 8;
		p->config_version |= atol (tmpp);
	    }
	}
	return 1;
    }

    return 0;
}

static struct uaedev_config_info *getuci(struct uae_prefs *p)
{
    if (p->mountitems < MOUNT_CONFIG_SIZE)
	return &p->mountconfig[p->mountitems++];
    return NULL;
}

struct uaedev_config_info *add_filesys_config (struct uae_prefs *p, int index,
			char *devname, char *volname, char *rootdir, int readonly,
			int secspertrack, int surfaces, int reserved,
			int blocksize, int bootpri, 
      char *filesysdir, int hdc, int flags) {
    struct uaedev_config_info *uci;
    int i;
    char *s;

    if (index < 0) {
	uci = getuci(p);
	uci->configoffset = -1;
    } else {
	uci = &p->mountconfig[index];
    }
    if (!uci)
	return 0;
    uci->ishdf = volname == NULL ? 1 : 0;
    strcpy (uci->devname, devname ? devname : "");
    strcpy (uci->volname, volname ? volname : "");
    strcpy (uci->rootdir, rootdir ? rootdir : "");
    uci->readonly = readonly;
    uci->sectors = secspertrack;
    uci->surfaces = surfaces;
    uci->reserved = reserved;
    uci->blocksize = blocksize;
    uci->bootpri = bootpri;
    uci->donotmount = 0;
    uci->autoboot = 0;
    if (bootpri < -128)
	uci->donotmount = 1;
    else if (bootpri >= -127)
	uci->autoboot = 1;
    uci->controller = hdc;
    strcpy (uci->filesys, filesysdir ? filesysdir : "");
    if (!uci->devname[0]) {
	char base[32];
	char base2[32];
	int num = 0;
	if (uci->rootdir[0] == 0 && !uci->ishdf)
	    strcpy (base, "RDH");
	else
	    strcpy (base, "DH");
	strcpy (base2, base);
	for (i = 0; i < p->mountitems; i++) {
	    sprintf(base2, "%s%d", base, num);
	    if (!strcmp(base2, p->mountconfig[i].devname)) {
		num++;
		i = -1;
		continue;
	    }
	}
	strcpy (uci->devname, base2);
    }
    s = filesys_createvolname (volname, rootdir, "Harddrive");
    strcpy (uci->volname, s);
    xfree (s);
    return uci;
}

static int cfgfile_parse_hardware (struct uae_prefs *p, char *option, char *value)
{
    int tmpval, dummy, i;
    char *section = 0;
    char tmpbuf[CONFIG_BLEN];

    if (cfgfile_yesno (option, value, "immediate_blits", &p->immediate_blits)
	|| cfgfile_yesno (option, value, "a1000ram", &p->cs_a1000ram)
	|| cfgfile_yesno (option, value, "fast_copper", &p->fast_copper)
	|| cfgfile_yesno (option, value, "ntsc", &p->ntscmode)
	|| cfgfile_yesno (option, value, "cpu_compatible", &p->cpu_compatible)
	|| cfgfile_yesno (option, value, "cpu_24bit_addressing", &p->address_space_24)
	|| cfgfile_yesno (option, value, "rtg_nocustom", &p->picasso96_nocustom))
	return 1;
    if (cfgfile_intval (option, value, "cachesize", &p->cachesize, 1)
	|| cfgfile_intval (option, value, "chipset_refreshrate", &p->chipset_refreshrate, 1)
	|| cfgfile_intval (option, value, "fastmem_size", (int *) &p->fastmem_size, 0x100000)
	|| cfgfile_intval (option, value, "z3mem_size", (int *) &p->z3fastmem_size, 0x100000)
	|| cfgfile_intval (option, value, "z3mem_start", (int *) &p->z3fastmem_start, 1)
	|| cfgfile_intval (option, value, "bogomem_size", (int *) &p->bogomem_size, 0x40000)
	|| cfgfile_intval (option, value, "gfxcard_size", (int *) &p->gfxmem_size, 0x100000)
	|| cfgfile_intval (option, value, "floppy_speed", &p->floppy_speed, 1)
	|| cfgfile_intval (option, value, "nr_floppies", &p->nr_floppies, 1)
	|| cfgfile_intval (option, value, "floppy0type", &p->dfxtype[0], 1)
	|| cfgfile_intval (option, value, "floppy1type", &p->dfxtype[1], 1)
	|| cfgfile_intval (option, value, "floppy2type", &p->dfxtype[2], 1)
	|| cfgfile_intval (option, value, "floppy3type", &p->dfxtype[3], 1))
	return 1;
    if (cfgfile_strval (option, value, "collision_level", &p->collision_level, collmode, 0))
	return 1;
    if (cfgfile_string (option, value, "kickstart_rom_file", p->romfile, sizeof p->romfile)
	|| cfgfile_string (option, value, "kickstart_ext_rom_file", p->romextfile, sizeof p->romextfile))
	return 1;

    for (i = 0; i < 4; i++) {
	sprintf (tmpbuf, "floppy%d", i);
	if (cfgfile_string (option, value, tmpbuf, p->df[i], sizeof p->df[i]))
	    return 1;
    }

    if (cfgfile_intval (option, value, "chipmem_size", &dummy, 1)) {
	if (dummy < 0)
	    p->chipmem_size = 0x20000; /* 128k, prototype support */
	else if (dummy == 0)
	    p->chipmem_size = 0x40000; /* 256k */
	else
	    p->chipmem_size = dummy * 0x80000;
	return 1;
    }

    if (cfgfile_strval (option, value, "chipset", &tmpval, csmode, 0)) {
	set_chipset_mask (p, tmpval);
	return 1;
    }

    if (cfgfile_string (option, value, "fpu_model", tmpbuf, sizeof tmpbuf)) {
	p->fpu_model = atol(tmpbuf);
	return 1;
    }

    if (cfgfile_string (option, value, "cpu_model", tmpbuf, sizeof tmpbuf)) {
	p->cpu_model = atol(tmpbuf);
	p->fpu_model = 0;
	return 1;
    }

    /* old-style CPU configuration */
    if (cfgfile_string (option, value, "cpu_type", tmpbuf, sizeof tmpbuf)) {
	p->fpu_model = 0;
	p->address_space_24 = 0;
	p->cpu_model = 680000;
	if (!strcmp(tmpbuf, "68000")) {
	    p->cpu_model = 68000;
	} else if (!strcmp(tmpbuf, "68010")) {
	    p->cpu_model = 68010;
	} else if (!strcmp(tmpbuf, "68ec020")) {
	    p->cpu_model = 68020;
	    p->address_space_24 = 1;
	} else if (!strcmp(tmpbuf, "68020")) {
	    p->cpu_model = 68020;
	} else if (!strcmp(tmpbuf, "68ec020/68881")) {
	    p->cpu_model = 68020;
	    p->fpu_model = 68881;
	    p->address_space_24 = 1;
	} else if (!strcmp(tmpbuf, "68020/68881")) {
	    p->cpu_model = 68020;
	    p->fpu_model = 68881;
	} else if (!strcmp(tmpbuf, "68040")) {
	    p->cpu_model = 68040;
	    p->fpu_model = 68040;
	} else if (!strcmp(tmpbuf, "68060")) {
	    p->cpu_model = 68060;
	    p->fpu_model = 68060;
	}
	return 1;
    }
    
    if (p->config_version < (21 << 16)) {
	if (cfgfile_strval (option, value, "cpu_speed", &p->m68k_speed, speedmode, 1)
	    /* Broken earlier versions used to write this out as a string.  */
	    || cfgfile_strval (option, value, "finegraincpu_speed", &p->m68k_speed, speedmode, 1))
	{
	    p->m68k_speed--;
	    return 1;
	}
    }

    if (cfgfile_intval (option, value, "cpu_speed", &p->m68k_speed, 1)) {
        p->m68k_speed *= CYCLE_UNIT;
	return 1;
    }

    if (cfgfile_intval (option, value, "finegrain_cpu_speed", &p->m68k_speed, 1)) {
	if (OFFICIAL_CYCLE_UNIT > CYCLE_UNIT) {
	    int factor = OFFICIAL_CYCLE_UNIT / CYCLE_UNIT;
	    p->m68k_speed = (p->m68k_speed + factor - 1) / factor;
	}
        if (strcasecmp (value, "max") == 0)
	    p->m68k_speed = -1;
	return 1;
    }

    for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
	char tmp[100];
	sprintf (tmp, "uaehf%d", i);
	if (strcmp (option, tmp) == 0)
	    return 1;
    }

    if (strcmp (option, "filesystem") == 0
	|| strcmp (option, "hardfile") == 0)
    {
	int secs, heads, reserved, bs, ro;
	char *aname, *root;
	char *tmpp = strchr (value, ',');
	char *str;

	if (config_newfilesystem)
	    return 1;

	if (tmpp == 0)
	    goto invalid_fs_1;

	*tmpp++ = '\0';
	if (strcmp (value, "1") == 0 || strcasecmp (value, "ro") == 0
	    || strcasecmp (value, "readonly") == 0
	    || strcasecmp (value, "read-only") == 0)
	    ro = 1;
	else if (strcmp (value, "0") == 0 || strcasecmp (value, "rw") == 0
		 || strcasecmp (value, "readwrite") == 0
		 || strcasecmp (value, "read-write") == 0)
	    ro = 0;
	else
	    goto invalid_fs_1;
	secs = 0; heads = 0; reserved = 0; bs = 0;

	value = tmpp;
	if (strcmp (option, "filesystem") == 0) {
	    tmpp = strchr (value, ':');
	    if (tmpp == 0)
		goto invalid_fs_1;
	    *tmpp++ = '\0';
	    aname = value;
	    root = tmpp;
	} else {
	    if (! getintval (&value, &secs, ',')
		|| ! getintval (&value, &heads, ',')
		|| ! getintval (&value, &reserved, ',')
		|| ! getintval (&value, &bs, ','))
		goto invalid_fs_1;
	    root = value;
	    aname = 0;
	}
	str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, root);
#ifdef FILESYS
	add_filesys_config (p, -1, NULL, aname, str, ro, secs, heads, reserved, bs, 0, NULL, 0, 0);
#endif
	free (str);
	return 1;

invalid_fs_1:
	write_log ("Invalid filesystem/hardfile specification.\n");
	return 1;
    }

    if (strcmp (option, "filesystem2") == 0
	|| strcmp (option, "hardfile2") == 0)
    {
	int secs, heads, reserved, bs, ro, bp, hdcv;
	char *dname = NULL, *aname = "", *root = NULL, *fs = NULL, *hdc;
	char *tmpp = strchr (value, ',');
	char *str = NULL;

	config_newfilesystem = 1;
	if (tmpp == 0)
	    goto invalid_fs;

	*tmpp++ = '\0';
	if (strcasecmp (value, "ro") == 0)
	    ro = 1;
	else if (strcasecmp (value, "rw") == 0)
	    ro = 0;
	else
	    goto invalid_fs;
	secs = 0; heads = 0; reserved = 0; bs = 0; bp = 0;
	fs = 0; hdc = 0; hdcv = 0;

	value = tmpp;
	if (strcmp (option, "filesystem2") == 0) {
	    tmpp = strchr (value, ':');
	    if (tmpp == 0)
		goto empty_fs;
	    *tmpp++ = 0;
	    dname = value;
	    aname = tmpp;
	    tmpp = strchr (tmpp, ':');
	    if (tmpp == 0)
		goto empty_fs;
	    *tmpp++ = 0;
	    root = tmpp;
	    tmpp = strchr (tmpp, ',');
	    if (tmpp == 0)
		goto empty_fs;
	    *tmpp++ = 0;
	    if (! getintval (&tmpp, &bp, 0))
		goto empty_fs;
	} else {
	    tmpp = strchr (value, ':');
	    if (tmpp == 0)
		goto invalid_fs;
	    *tmpp++ = '\0';
	    dname = value;
	    root = tmpp;
	    tmpp = strchr (tmpp, ',');
	    if (tmpp == 0)
		goto invalid_fs;
	    *tmpp++ = 0;
	    aname = 0;
	    if (! getintval (&tmpp, &secs, ',')
		|| ! getintval (&tmpp, &heads, ',')
		|| ! getintval (&tmpp, &reserved, ',')
		|| ! getintval (&tmpp, &bs, ','))
		goto invalid_fs;
	    if (getintval2 (&tmpp, &bp, ',')) {
		fs = tmpp;
	    tmpp = strchr (tmpp, ',');
		  if (tmpp != 0) {
		    *tmpp++ = 0;
		    hdc = tmpp;
		    if(strlen(hdc) >= 4 && !memcmp(hdc, "ide", 3)) {
			hdcv = hdc[3] - '0' + HD_CONTROLLER_IDE0;
			if (hdcv < HD_CONTROLLER_IDE0 || hdcv > HD_CONTROLLER_IDE3)
			    hdcv = 0;
		    }
		    if(strlen(hdc) >= 5 && !memcmp(hdc, "scsi", 4)) {
			hdcv = hdc[4] - '0' + HD_CONTROLLER_SCSI0;
			if (hdcv < HD_CONTROLLER_SCSI0 || hdcv > HD_CONTROLLER_SCSI6)
			    hdcv = 0;
		    }
		    if (strlen (hdc) >= 6 && !memcmp (hdc, "scsram", 6))
			hdcv = HD_CONTROLLER_PCMCIA_SRAM;
		}
	    }
	}
empty_fs:
	if (root)
	  str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, root);
#ifdef FILESYS
	add_filesys_config (p, -1, dname, aname, str, ro, secs, heads, reserved, bs, bp, fs, hdcv, 0);
#endif
	free (str);
	return 1;

      invalid_fs:
	write_log ("Invalid filesystem/hardfile specification.\n");
	return 1;
    }

    return 0;
}

int cfgfile_parse_option (struct uae_prefs *p, char *option, char *value, int type)
{
    if (!strcmp (option, "config_hardware"))
	return 1;
    if (!strcmp (option, "config_host"))
	return 1;
    if (type == 0 || (type & CONFIG_TYPE_HARDWARE)) {
	if (cfgfile_parse_hardware (p, option, value))
	    return 1;
    }
    if (type == 0 || (type & CONFIG_TYPE_HOST)) {
	if (cfgfile_parse_host (p, option, value))
	    return 1;
    }
    if (type > 0)
	return 1;
    return 0;
}

static int cfgfile_separate_line (char *line, char *line1b, char *line2b)
{
    char *line1, *line2;
    int i;

    line1 = line;
    line2 = strchr (line, '=');
    if (! line2) {
	write_log ("CFGFILE: line was incomplete with only %s\n", line1);
	return 0;
    }
    *line2++ = '\0';
    strcpy (line1b, line1);
    strcpy (line2b, line2);

    /* Get rid of whitespace.  */
    i = strlen (line2);
    while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		     || line2[i - 1] == '\r' || line2[i - 1] == '\n'))
	line2[--i] = '\0';
    line2 += strspn (line2, "\t \r\n");
    strcpy (line2b, line2);
    i = strlen (line);
    while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
		     || line[i - 1] == '\r' || line[i - 1] == '\n'))
	line[--i] = '\0';
    line += strspn (line, "\t \r\n");
    strcpy (line1b, line);
    return 1;
}

static int isobsolete (char *s)
{
    int i = 0;
    while (obsolete[i]) {
	if (!strcasecmp (s, obsolete[i])) {
	    write_log ("obsolete config entry '%s'\n", s);
	    return 1;
	}
	i++;
    }
    if (strlen (s) >= 10 && !memcmp (s, "gfx_opengl", 10)) {
	write_log ("obsolete config entry '%s\n", s);
	return 1;
    }
    if (strlen (s) >= 6 && !memcmp (s, "gfx_3d", 6)) {
	write_log ("obsolete config entry '%s\n", s);
	return 1;
    }
    return 0;
}

static void cfgfile_parse_separated_line (struct uae_prefs *p, char *line1b, char *line2b, int type)
{
    char line3b[CONFIG_BLEN], line4b[CONFIG_BLEN];
    struct strlist *sl;
    int ret;

    strcpy (line3b, line1b);
    strcpy (line4b, line2b);
    ret = cfgfile_parse_option (p, line1b, line2b, type);
    if (!isobsolete (line3b)) {
	for (sl = p->all_lines; sl; sl = sl->next) {
	    if (sl->option && !strcasecmp (line1b, sl->option)) break;
	}
	if (!sl) {
	    struct strlist *u = (struct strlist *) xcalloc (sizeof (struct strlist), 1);
	    u->option = my_strdup(line3b);
	    u->value = my_strdup(line4b);
	    u->next = p->all_lines;
	    p->all_lines = u;
	    if (!ret) {
		u->unknown = 1;
		write_log ("unknown config entry: '%s=%s'\n", u->option, u->value);
	    }
	}
    }
}

void cfgfile_parse_line (struct uae_prefs *p, char *line, int type)
{
    char line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

    if (!cfgfile_separate_line (line, line1b, line2b))
	return;
    cfgfile_parse_separated_line (p, line1b, line2b, type);
}

static void subst (char *p, char *f, int n)
{
    char *str = cfgfile_subst_path (UNEXPANDED, p, f);
    strncpy (f, str, n - 1);
    f[n - 1] = '\0';
    free (str);
}

static char *cfg_fgets (char *line, int max, struct zfile *fh)
{
#ifdef SINGLEFILE
    extern char singlefile_config[];
    static char *sfile_ptr;
    char *p;
#endif

    if (fh)
	return zfile_fgets (line, max, fh);
#ifdef SINGLEFILE
    if (sfile_ptr == 0) {
	sfile_ptr = singlefile_config;
	if (*sfile_ptr) {
	    write_log ("singlefile config found\n");
	    while (*sfile_ptr++);
	}
    }
    if (*sfile_ptr == 0) {
	sfile_ptr = singlefile_config;
	return 0;
    }
    p = sfile_ptr;
    while (*p != 13 && *p != 10 && *p != 0) p++;
    memset (line, 0, max);
    memcpy (line, sfile_ptr, p - sfile_ptr);
    sfile_ptr = p + 1;
    if (*sfile_ptr == 13)
	sfile_ptr++;
    if (*sfile_ptr == 10)
	sfile_ptr++;
    return line;
#endif
    return 0;
}

static int cfgfile_load_2 (struct uae_prefs *p, const char *filename, int real, int *type)
{
    int i;
    struct zfile *fh;
    char line[CONFIG_BLEN], line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];
    struct strlist *sl;
    int type1 = 0, type2 = 0, askedtype = 0;

    if (type) {
	askedtype = *type;
	*type = 0;
    }
    if (real) {
	p->config_version = 0;
	config_newfilesystem = 0;
	reset_inputdevice_config (p);
    }

    fh = zfile_fopen (filename, "r");
#ifndef SINGLEFILE
    if (! fh)
	return 0;
#endif

    while (cfg_fgets (line, sizeof (line), fh) != 0) {
	trimws (line);
	if (strlen (line) > 0) {
	    if (line[0] == '#' || line[0] == ';')
		continue;
	    if (!cfgfile_separate_line (line, line1b, line2b))
		continue;
	    type1 = type2 = 0;
	    if (cfgfile_yesno (line1b, line2b, "config_hardware", &type1) ||
	        cfgfile_yesno (line1b, line2b, "config_host", &type2)) {
	    	if (type1 && type)
	    	    *type |= CONFIG_TYPE_HARDWARE;
	    	if (type2 && type)
		    *type |= CONFIG_TYPE_HOST;
		continue;
	    }
	    if (real) {
		cfgfile_parse_separated_line (p, line1b, line2b, askedtype);
	    } else {
		cfgfile_string (line1b, line2b, "config_description", p->description, 128);
	    }
	}
    }

    if (type && *type == 0)
	*type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
    zfile_fclose (fh);

    if (!real)
	return 1;

    for (sl = temp_lines; sl; sl = sl->next) {
	sprintf (line, "%s=%s", sl->option, sl->value);
	cfgfile_parse_line (p, line, 0);
    }

    for (i = 0; i < 4; i++) {
    	subst (p->path_floppy, p->df[i], sizeof p->df[i]);
    	if(i >= p->nr_floppies)
    	  p->dfxtype[i] = DRV_NONE;
    }
    subst (p->path_rom, p->romfile, sizeof p->romfile);
    subst (p->path_rom, p->romextfile, sizeof p->romextfile);

    return 1;
}

int cfgfile_load (struct uae_prefs *p, const char *filename, int *type, int ignorelink)
{
    int v;
    char tmp[MAX_DPATH];
    int type2;
    static int recursive;

    if (recursive > 1)
	return 0;
    recursive++;
    write_log ("load config '%s':%d\n", filename, type ? *type : -1);
    v = cfgfile_load_2 (p, filename, 1, type);
    if (!v) {
	write_log ("load failed\n");
	goto end;
    }
end:
    recursive--;
    fixup_prefs (p);
    return v;
}

void cfgfile_backup (const char *path)
{
    char dpath[MAX_DPATH];

    fetch_configurationpath (dpath, sizeof (dpath));
    strcat (dpath, "configuration.backup");
    my_unlink (dpath);
    my_rename (path, dpath);
}

int cfgfile_save (struct uae_prefs *p, const char *filename, int type)
{
    struct zfile *fh;

    cfgfile_backup (filename);
    fh = zfile_fopen (filename, "w");
    write_log ("save config '%s'\n", filename);
    if (! fh)
	return 0;

    if (!type)
	type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
    cfgfile_save_options (fh, p, type);
    zfile_fclose (fh);
    return 1;
}


int cfgfile_get_description (const char *filename, char *description)
{
  int result = 0;
  struct uae_prefs *p = (struct uae_prefs *) xmalloc (sizeof (struct uae_prefs));
  p->description[0] = 0;
  if (cfgfile_load_2 (p, filename, 0, 0)) {
  	result = 1;
	  if (description)
	    strcpy (description, p->description);
  }
  free (p);
  return result;
}


int cfgfile_configuration_change(int v)
{
    static int mode;
    if (v >= 0)
	mode = v;
    return mode;
}

static void parse_sound_spec (struct uae_prefs *p, char *spec)
{
    char *x0 = my_strdup (spec);
    char *x1, *x2 = NULL, *x3 = NULL, *x4 = NULL, *x5 = NULL;

    x1 = strchr (x0, ':');
    if (x1 != NULL) {
	*x1++ = '\0';
	x2 = strchr (x1 + 1, ':');
	if (x2 != NULL) {
	    *x2++ = '\0';
	    x3 = strchr (x2 + 1, ':');
	    if (x3 != NULL) {
		*x3++ = '\0';
		x4 = strchr (x3 + 1, ':');
		if (x4 != NULL) {
		    *x4++ = '\0';
		    x5 = strchr (x4 + 1, ':');
		}
	    }
	}
    }
    p->produce_sound = atoi (x0);
    if (x1) {
	p->sound_stereo_separation = 0;
	if (*x1 == 'S') {
	    p->sound_stereo = SND_STEREO;
	    p->sound_stereo_separation = 7;
	} else if (*x1 == 's')
	    p->sound_stereo = SND_STEREO;
	else
	    p->sound_stereo = SND_MONO;
    }
    if (x3)
	p->sound_freq = atoi (x3);
    free (x0);
}

static void parse_filesys_spec (struct uae_prefs *p, int readonly, char *spec)
{
    char buf[256];
    char *s2;

    strncpy (buf, spec, 255); buf[255] = 0;
    s2 = strchr (buf, ':');
    if (s2) {
	*s2++ = '\0';
#ifdef __DOS__
	{
	    char *tmp;
 
	    while ((tmp = strchr (s2, '\\')))
		*tmp = '/';
	}
#endif
#ifdef FILESYS
	add_filesys_config (p, -1, NULL, buf, s2, readonly, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
    } else {
	write_log ("Usage: [-m | -M] VOLNAME:mount_point\n");
    }
}

static void parse_hardfile_spec (struct uae_prefs *p, char *spec)
{
    char *x0 = my_strdup (spec);
    char *x1, *x2, *x3, *x4;

    x1 = strchr (x0, ':');
    if (x1 == NULL)
	goto argh;
    *x1++ = '\0';
    x2 = strchr (x1 + 1, ':');
    if (x2 == NULL)
	goto argh;
    *x2++ = '\0';
    x3 = strchr (x2 + 1, ':');
    if (x3 == NULL)
	goto argh;
    *x3++ = '\0';
    x4 = strchr (x3 + 1, ':');
    if (x4 == NULL)
	goto argh;
    *x4++ = '\0';
#ifdef FILESYS
    add_filesys_config (p, -1, NULL, NULL, x4, 0, atoi (x0), atoi (x1), atoi (x2), atoi (x3), 0, 0, 0, 0);
#endif

    free (x0);
    return;

 argh:
    free (x0);
    write_log ("Bad hardfile parameter specified - type \"uae -h\" for help.\n");
    return;
}

static void parse_cpu_specs (struct uae_prefs *p, char *spec)
{
    if (*spec < '0' || *spec > '4') {
	write_log ("CPU parameter string must begin with '0', '1', '2', '3' or '4'.\n");
	return;
    }

    p->cpu_model = (*spec++) * 10 + 68000;
    p->address_space_24 = p->cpu_model < 68020;
    p->cpu_compatible = 0;
    while (*spec != '\0') {
	switch (*spec) {
	 case 'a':
	    if (p->cpu_model < 68020)
		write_log ("In 68000/68010 emulation, the address space is always 24 bit.\n");
	    else if (p->cpu_model >= 68040)
		write_log ("In 68040/060 emulation, the address space is always 32 bit.\n");
	    else
		p->address_space_24 = 1;
	    break;
	 case 'c':
	    if (p->cpu_model != 68000)
		write_log  ("The more compatible CPU emulation is only available for 68000\n"
			 "emulation, not for 68010 upwards.\n");
	    else
		p->cpu_compatible = 1;
	    break;
	 default:
	    write_log  ("Bad CPU parameter specified - type \"uae -h\" for help.\n");
	    break;
	}
	spec++;
    }
}

/* Returns the number of args used up (0 or 1).  */
int parse_cmdline_option (struct uae_prefs *p, char c, char *arg)
{
    struct strlist *u = (struct strlist *) xcalloc (sizeof (struct strlist), 1);
    const char arg_required[] = "0123rKpImWSAJwNCZUFcblOdHRv";

    if (strchr (arg_required, c) && ! arg) {
	write_log ("Missing argument for option `-%c'!\n", c);
	return 0;
    }

    u->option = (char *)malloc (2);
    u->option[0] = c;
    u->option[1] = 0;
    u->value = my_strdup(arg);
    u->next = p->all_lines;
    p->all_lines = u;

    switch (c) {
    case '0': strncpy (p->df[0], arg, 255); p->df[0][255] = 0; break;
    case '1': strncpy (p->df[1], arg, 255); p->df[1][255] = 0; break;
    case '2': strncpy (p->df[2], arg, 255); p->df[2][255] = 0; break;
    case '3': strncpy (p->df[3], arg, 255); p->df[3][255] = 0; break;
    case 'r': strncpy (p->romfile, arg, 255); p->romfile[255] = 0; break;
    case 'K': strncpy (p->romextfile, arg, 255); p->romextfile[255] = 0; break;
	/*     case 'I': strncpy (p->sername, arg, 255); p->sername[255] = 0; currprefs.use_serial = 1; break; */
    case 'm': case 'M': parse_filesys_spec (p, c == 'M', arg); break;
    case 'W': parse_hardfile_spec (p, arg); break;
    case 'S': parse_sound_spec (p, arg); break;
    case 'R': p->gfx_framerate = atoi (arg); break;

    case 'w': p->m68k_speed = atoi (arg); break;

	/* case 'g': p->use_gfxlib = 1; break; */
    case 'G': p->start_gui = 0; break;

    case 'n':
	if (strchr (arg, 'i') != 0)
	    p->immediate_blits = 1;
	break;

    case 'v':
	set_chipset_mask (p, atoi (arg));
	break;

    case 'C':
	parse_cpu_specs (p, arg);
	break;

    case 'Z':
	p->z3fastmem_size = atoi (arg) * 0x100000;
	break;

    case 'U':
	p->gfxmem_size = atoi (arg) * 0x100000;
	break;

    case 'F':
	p->fastmem_size = atoi (arg) * 0x100000;
	break;

    case 'b':
	p->bogomem_size = atoi (arg) * 0x40000;
	break;

    case 'c':
	p->chipmem_size = atoi (arg) * 0x80000;
	break;

    default:
	write_log ("Unknown option `-%c'!\n", c);
	break;
    }
    return !! strchr (arg_required, c);
}

void cfgfile_addcfgparam (char *line)
{
    struct strlist *u;
    char line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

    if (!line) {
	struct strlist **ps = &temp_lines;
	while (*ps) {
	    struct strlist *s = *ps;
	    *ps = s->next;
	    free (s->value);
	    free (s->option);
	    free (s);
	}
	temp_lines = 0;
	return;
    }
    if (!cfgfile_separate_line (line, line1b, line2b))
	return;
    u = (struct strlist *) xcalloc (sizeof (struct strlist), 1);
    u->option = my_strdup(line1b);
    u->value = my_strdup(line2b);
    u->next = temp_lines;
    temp_lines = u;
}

void default_prefs (struct uae_prefs *p, int type)
{
  int i;
  uae_u8 zero = 0;
  struct zfile *f;

  memset (p, 0, sizeof (struct uae_prefs));
  strcpy (p->description, "UAE default configuration");

  p->start_gui = 1;

  p->all_lines = 0;
  p->produce_sound = 3;
  p->sound_stereo = SND_STEREO;
  p->sound_stereo_separation = 7;
  p->sound_mixed_stereo_delay = 0;
  p->sound_freq = DEFAULT_SOUND_FREQ;
  p->sound_interpol = 0;
  p->sound_filter = FILTER_SOUND_OFF;
  p->sound_filter_type = 0;
  p->sound_auto = 1;

  p->cachesize = DEFAULT_JIT_CACHE_SIZE;

  for (i = 0;i < 10; i++)
	  p->optcount[i] = -1;
  p->optcount[0] = 4;	/* How often a block has to be executed before it is translated */
  p->optcount[1] = 0;	/* How often to use the naive translation */
  p->optcount[2] = 0;
  p->optcount[3] = 0;
  p->optcount[4] = 0;
  p->optcount[5] = 0;

  p->gfx_framerate = 0;
#ifdef RASPBERRY
  p->gfx_size.width = 640;
  p->gfx_size.height = 256;
#else
  p->gfx_size.width = 320;
  p->gfx_size.height = 240;
#endif
  p->gfx_size_win.width = 320;
  p->gfx_size_win.height = 240;
  p->gfx_size_fs.width = 640;
  p->gfx_size_fs.height = 480;
#ifdef RASPBERRY
  p->gfx_correct_aspect = 1;
#endif
  
  p->immediate_blits = 0;
  p->chipset_refreshrate = 50;
  p->collision_level = 2;
  p->leds_on_screen = 0;
  p->fast_copper = 1;
  p->cpu_idle = 0;
  p->floppy_speed = 100;
  p->tod_hack = 1;
  p->picasso96_nocustom = 1;

  p->cs_df0idhw = 0;

  strcpy (p->df[0], "");
  strcpy (p->df[1], "");
  strcpy (p->df[2], "");
  strcpy (p->df[3], "");

  #ifdef RASPBERRY
  // Choose automatically first rom.
  if (lstAvailableROMs.size() >= 1)
    strncpy(currprefs.romfile,lstAvailableROMs[0]->Path,255);
  else
    strcpy (p->romfile, "kick.rom");
  #else
  strcpy (p->romfile, "kick.rom");
  #endif
  strcpy (p->romextfile, "");

  sprintf (p->path_rom, "%s/kickstarts/", start_path_data);
  sprintf (p->path_floppy, "%s/disks/", start_path_data);
  sprintf (p->path_hardfile, "%s/", start_path_data);

  p->fpu_model = 0;
  p->cpu_model = 68000;
  p->m68k_speed = 0;
  p->cpu_compatible = 0;
  p->address_space_24 = 1;
  p->chipset_mask = CSMASK_ECS_AGNUS;
  p->ntscmode = 0;

  p->fastmem_size = 0x00000000;
  p->z3fastmem_size = 0x00000000;
  p->z3fastmem_start = 0x01000000;
  p->chipmem_size = 0x00100000;
  p->bogomem_size = 0x00000000;
  p->gfxmem_size = 0x00000000;

  p->cs_a1000ram = 0;

  p->nr_floppies = 2;
  p->dfxtype[0] = DRV_35_DD;
  p->dfxtype[1] = DRV_35_DD;
  p->dfxtype[2] = DRV_NONE;
  p->dfxtype[3] = DRV_NONE;
  p->floppy_speed = 100;
  
  target_default_options (p, type);

  inputdevice_default_prefs (p);
}
