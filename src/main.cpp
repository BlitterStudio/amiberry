/*
  * UAE - The Un*x Amiga Emulator
  *
  * Main program
  *
  * Copyright 1995 Ed Hanway
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */
#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>
#include "options.h"
#include "td-sdl/thread.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "sd-pandora/sound.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "disk.h"
#include "debug.h"
#include "xwin.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "traps.h"
#include "osemu.h"
#include "picasso96.h"
#include "bsdsocket.h"
#include "drawing.h"
#include "native2amiga.h"
#include "savestate.h"
#include "filesys.h"
#ifdef JIT
#include "compemu.h"
#endif

#ifdef USE_SDL
#include "SDL.h"
#endif
long int version = 256*65536L*UAEMAJOR + 65536L*UAEMINOR + UAESUBREV;

struct uae_prefs currprefs, changed_prefs; 

int no_gui = 0;
int joystickpresent = 0;
int cloanto_rom = 0;

struct gui_info gui_data;

char warning_buffer[256];

char optionsfile[256];

/* If you want to pipe printer output to a file, put something like
 * "cat >>printerfile.tmp" above.
 * The printer support was only tested with the driver "PostScript" on
 * Amiga side, using apsfilter for linux to print ps-data.
 *
 * Under DOS it ought to be -p LPT1: or -p PRN: but you'll need a
 * PostScript printer or ghostscript -=SR=-
 */


void discard_prefs (struct uae_prefs *p, int type)
{
  struct strlist **ps = &p->all_lines;
  while (*ps) {
	  struct strlist *s = *ps;
	  *ps = s->next;
	  free (s->value);
	  free (s->option);
	  free (s);
  }
#ifdef FILESYS
  filesys_cleanup ();
#endif
}

static void fixup_prefs_dim2 (struct wh *wh)
{
    if (wh->width < 320)
	wh->width = 320;
    if (wh->height < 200)
	wh->height = 200;
}

void fixup_prefs_dimensions (struct uae_prefs *prefs)
{
    fixup_prefs_dim2(&prefs->gfx_size_fs);
    fixup_prefs_dim2(&prefs->gfx_size_win);
}

void fixup_cpu(struct uae_prefs *p)
{
    switch(p->cpu_model)
    {
        case 68000:
	p->address_space_24 = 1;
	p->fpu_model = 0;
        break;
	case 68010:
	p->address_space_24 = 1;
	p->fpu_model = 0;
	break;
	case 68020:
	break;
	case 68030:
	p->address_space_24 = 0;
	break;
	case 68040:
	p->address_space_24 = 0;
	if (p->fpu_model)
	    p->fpu_model = 68040;
	break;
	case 68060:
	p->address_space_24 = 0;
	if (p->fpu_model)
	    p->fpu_model = 68060;
	break;
    }
}


void fixup_prefs (struct uae_prefs *p)
{
    int err = 0;

    fixup_cpu(p);

    if ((p->chipmem_size & (p->chipmem_size - 1)) != 0
	|| p->chipmem_size < 0x20000
	|| p->chipmem_size > 0x800000)
    {
	write_log ("Unsupported chipmem size %x!\n", p->chipmem_size);
	p->chipmem_size = 0x200000;
	err = 1;
    }

    if ((p->fastmem_size & (p->fastmem_size - 1)) != 0
	|| (p->fastmem_size != 0 && (p->fastmem_size < 0x100000 || p->fastmem_size > 0x800000)))
    {
	write_log ("Unsupported fastmem size %x!\n", p->fastmem_size);
	err = 1;
    }
    if ((p->gfxmem_size & (p->gfxmem_size - 1)) != 0
	|| (p->gfxmem_size != 0 && (p->gfxmem_size < 0x100000 || p->gfxmem_size > 0x8000000)))
    {
	write_log ("Unsupported graphics card memory size %x!\n", p->gfxmem_size);
	p->gfxmem_size = 0;
	err = 1;
    }
    gfxmem_start = 0x3000000;
    
   if ((p->z3fastmem_size & (p->z3fastmem_size - 1)) != 0
	|| (p->z3fastmem_size != 0 && (p->z3fastmem_size < 0x100000 || p->z3fastmem_size > max_z3fastmem)))
    {
	write_log ("Unsupported Zorro III fastmem size %x (%x)!\n", p->z3fastmem_size, max_z3fastmem);
	if (p->z3fastmem_size > max_z3fastmem)
	    p->z3fastmem_size = max_z3fastmem;
	else
	    p->z3fastmem_size = 0;
	err = 1;
    }
    p->z3fastmem_start &= ~0xffff;
    if (p->z3fastmem_start != 0x1000000)
	p->z3fastmem_start = 0x1000000;
    
    if (p->address_space_24 && (p->gfxmem_size != 0 || p->z3fastmem_size != 0)) {
	p->z3fastmem_size = p->gfxmem_size = 0;
	write_log ("Can't use a graphics card or Zorro III fastmem when using a 24 bit\n"
		 "address space - sorry.\n");
    }
    if (p->bogomem_size != 0 && p->bogomem_size != 0x80000 && p->bogomem_size != 0x100000 && p->bogomem_size != 0x180000 && p->bogomem_size != 0x1c0000) {
	p->bogomem_size = 0;
	write_log ("Unsupported bogomem size!\n");
	err = 1;
    }
    if (p->bogomem_size > 0x100000 && ((p->chipset_mask & CSMASK_AGA) || p->cpu_model >= 68020)) {
	p->bogomem_size = 0x100000;
	write_log ("Possible Gayle bogomem conflict fixed\n");
    }

    if (p->chipmem_size > 0x200000 && p->fastmem_size != 0) {
	write_log ("You can't use fastmem and more than 2MB chip at the same time!\n");
	p->fastmem_size = 0;
	err = 1;
    }

    if (p->produce_sound < 0 || p->produce_sound > 3) {
	write_log ("Bad value for -S parameter: enable value must be within 0..3\n");
	p->produce_sound = 0;
	err = 1;
    }
    if (p->cachesize < 0 || p->cachesize > 16384) {
	write_log ("Bad value for cachesize parameter: value must be within 0..16384\n");
	p->cachesize = 0;
	err = 1;
    }
    if (p->z3fastmem_size > 0 && (p->address_space_24 || p->cpu_model < 68020)) {
	write_log ("Z3 fast memory can't be used with a 68000/68010 emulation. It\n"
		 "requires a 68020 emulation. Turning off Z3 fast memory.\n");
	p->z3fastmem_size = 0;
	err = 1;
    }
    if (p->gfxmem_size > 0 && (p->cpu_model < 68020 || p->address_space_24)) {
	write_log ("Picasso96 can't be used with a 68000/68010 or 68EC020 emulation. It\n"
		 "requires a 68020 emulation. Turning off Picasso96.\n");
	p->gfxmem_size = 0;
	err = 1;
    }

    if (p->nr_floppies < 0 || p->nr_floppies > 4) {
	write_log ("Invalid number of floppies.  Using 4.\n");
	p->nr_floppies = 4;
	p->dfxtype[0] = 0;
	p->dfxtype[1] = 0;
	p->dfxtype[2] = 0;
	p->dfxtype[3] = 0;
	err = 1;
    }

    if (p->floppy_speed > 0 && p->floppy_speed < 10) {
	p->floppy_speed = 100;
    }

    if (p->collision_level < 0 || p->collision_level > 3) {
	write_log ("Invalid collision support level.  Using 1.\n");
	p->collision_level = 1;
	err = 1;
    }
    fixup_prefs_dimensions (p);

    target_fixup_options (p);
}

int quit_program = 0;
static int restart_program;
static char restart_config[256];

void uae_reset (int hardreset)
{
  if (quit_program == 0) {
	  quit_program = -2;
	  if (hardreset)
	    quit_program = -3;
  }
}

void uae_quit (void)
{
    if (quit_program != -1)
	quit_program = -1;
    target_quit ();
}

/* 0 = normal, 1 = nogui, -1 = disable nogui */
void uae_restart (int opengui, char *cfgfile)
{
  uae_quit ();
  restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
  restart_config[0] = 0;
  if (cfgfile)
	  strcpy (restart_config, cfgfile);
}

static void parse_cmdline_2 (int argc, char **argv)
{
    int i;

    cfgfile_addcfgparam (0);
    for (i = 1; i < argc; i++) {
	if (strcmp (argv[i], "-cfgparam") == 0) {
	    if (i + 1 == argc)
		write_log ("Missing argument for '-cfgparam' option.\n");
	    else
		cfgfile_addcfgparam (argv[++i]);
	}
    }
}


static void parse_cmdline (int argc, char **argv)
{
  int i;

  for (i = 1; i < argc; i++) {
#ifdef DEBUG_M68K
extern int debug_frame_start;
extern int debug_frame_end;
    if (strcmp (argv[i], "-debug_start") == 0) {
      if(i < argc) {
        ++i;
        debug_frame_start = atoi(argv[i]);
        continue;
      }
    }
    if (strcmp (argv[i], "-debug_end") == 0) {
      if(i < argc) {
        ++i;
        debug_frame_end = atoi(argv[i]);
        continue;
      }
    }
#endif
    if (strcmp (argv[i], "-cfgparam") == 0) {
	    if (i + 1 < argc)
		    i++;
	  } else if (strncmp (argv[i], "-config=", 8) == 0) {
	    currprefs.mountitems = 0;
	    target_cfgfile_load (&currprefs, argv[i] + 8, -1, 1);
	  }
	  /* Check for new-style "-f xxx" argument, where xxx is config-file */
	  else if (strcmp (argv[i], "-f") == 0) {
	    if (i + 1 == argc) {
		    write_log ("Missing argument for '-f' option.\n");
	    } else {
        currprefs.mountitems = 0;
		    target_cfgfile_load (&currprefs, argv[++i], -1, 1);
	    }
	  } else if (strcmp (argv[i], "-s") == 0) {
	    if (i + 1 == argc)
		    write_log ("Missing argument for '-s' option.\n");
	    else
		    cfgfile_parse_line (&currprefs, argv[++i], 0);
	  } else {
	    if (argv[i][0] == '-' && argv[i][1] != '\0') {
		    const char *arg = argv[i] + 2;
		    int extra_arg = *arg == '\0';
		    if (extra_arg)
		      arg = i + 1 < argc ? argv[i + 1] : 0;
		    if (parse_cmdline_option (&currprefs, argv[i][1], (char*)arg) && extra_arg)
		      i++;
	    }
	  }
  }
}

static void parse_cmdline_and_init_file (int argc, char **argv)
{
    parse_cmdline_2 (argc, argv);

    if (! target_cfgfile_load (&currprefs, restart_config, 0, 0)) {
	write_log ("failed to load config '%s'\n", restart_config);
    }
    fixup_prefs (&currprefs);

    parse_cmdline (argc, argv);
}

void reset_all_systems (void)
{
    init_eventtab ();
#ifdef FILESYS
    filesys_prepare_reset ();
    filesys_reset ();
#endif
    memory_reset ();
#ifdef FILESYS
    filesys_start_threads ();
    hardfile_reset ();
#endif
}

/* Okay, this stuff looks strange, but it is here to encourage people who
 * port UAE to re-use as much of this code as possible. Functions that you
 * should be using are do_start_program() and do_leave_program(), as well
 * as real_main(). Some OSes don't call main() (which is braindamaged IMHO,
 * but unfortunately very common), so you need to call real_main() from
 * whatever entry point you have. You may want to write your own versions
 * of start_program() and leave_program() if you need to do anything special.
 * Add #ifdefs around these as appropriate.
 */
void do_start_program (void)
{
  if (quit_program == -1)
  	return;
  /* Do a reset on startup. Whether this is elegant is debatable. */
  inputdevice_updateconfig (&currprefs);
  if (quit_program >= 0)
	  quit_program = 2;
	m68k_go (1);
}

void do_leave_program (void)
{
#ifdef JIT
    compiler_exit();
#endif
  graphics_leave ();
  inputdevice_close ();
  DISK_free ();
  close_sound ();
//    if (! no_gui)
  	gui_exit ();
#ifdef USE_SDL
  SDL_Quit ();
#endif
#ifdef AUTOCONFIG
  expansion_cleanup ();
#endif
#ifdef FILESYS
  filesys_cleanup ();
#endif
#ifdef BSDSOCKET
  bsdlib_reset ();
#endif
  memory_cleanup ();
  cfgfile_addcfgparam (0);
  machdep_free ();
}

void start_program (void)
{
    do_start_program ();
}

void leave_program (void)
{
    do_leave_program ();
}

static void real_main2 (int argc, char **argv)
{
#ifdef RASPBERRY
  printf("Uae4arm v0.4 for Raspberry Pi by Chips\n");
#endif
#ifdef PANDORA
  SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO);
#else 
#ifdef USE_SDL
  SDL_Init (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
#endif
#endif

  if (restart_config[0]) {
	  default_prefs (&currprefs, 0);
	  fixup_prefs (&currprefs);
  }

  if (! graphics_setup ()) {
	abort();
  }

  if (restart_config[0])
	  parse_cmdline_and_init_file (argc, argv);
  else
  	currprefs = changed_prefs;

  if (!machdep_init ()) {
	  restart_program = 0;
	  return;
  }

  if (! setup_sound ()) {
  	write_log ("Sound driver unavailable: Sound output disabled\n");
  	currprefs.produce_sound = 0;
  }
  inputdevice_init();

  changed_prefs = currprefs;
  no_gui = ! currprefs.start_gui;
  if (restart_program == 2)
  	no_gui = 1;
  else if (restart_program == 3)
	  no_gui = 0;
  restart_program = 0;

  if (! no_gui) {
	  int err = gui_init ();
	  currprefs = changed_prefs;
	  if (err == -1) {
      write_log ("Failed to initialize the GUI\n");
	  } else if (err == -2) {
      return;
	  }
  }
  else
  {
  	setCpuSpeed();
    update_display(&currprefs);
  }

  fixup_prefs (&currprefs);
  changed_prefs = currprefs;
  /* force sound settings change */
  currprefs.produce_sound = 0;

#ifdef AUTOCONFIG
  rtarea_setup ();
#endif
#ifdef FILESYS
  rtarea_init ();
  hardfile_install();
#endif
  keybuf_init (); /* Must come after init_joystick */

#ifdef AUTOCONFIG
  expansion_init ();
#endif
#ifdef FILESYS
  filesys_install (); 
#endif
  memory_init ();
  memory_reset ();

#ifdef AUTOCONFIG
  emulib_install ();
  native2amiga_install ();
#endif

  custom_init (); /* Must come after memory_init */
  DISK_init ();

  reset_frame_rate_hack ();
  init_m68k(); /* must come after reset_frame_rate_hack (); */

  gui_update ();

  if (graphics_init ()) {

    if(!init_audio ()) {
  	  if (sound_available && currprefs.produce_sound > 1) {
		    write_log ("Sound driver unavailable: Sound output disabled\n");
	    }
		  currprefs.produce_sound = 0;
    }

		start_program ();
	}
}

void real_main (int argc, char **argv)
{
  restart_program = 1;
  fetch_configurationpath (restart_config, sizeof (restart_config));
  strcat (restart_config, OPTIONSFILENAME);
  strcat (restart_config, ".uae");
  while (restart_program) {
	  changed_prefs = currprefs;
	  real_main2 (argc, argv);
    leave_program ();
	  quit_program = 0;
  }
  zfile_exit ();
}

#ifndef NO_MAIN_IN_MAIN_C
int main (int argc, char *argv[])
{
    real_main (argc, argv);
    return 0;
}
#endif
