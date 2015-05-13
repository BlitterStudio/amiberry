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
#include "threaddep/thread.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "sounddep/sound.h"
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
  free_mountinfo (p->mountinfo);
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

void fixup_prefs (struct uae_prefs *p)
{
    int err = 0;

    if ((p->chipmem_size & (p->chipmem_size - 1)) != 0
	|| p->chipmem_size < 0x40000
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

    if (p->bogomem_size != 0 && p->bogomem_size != 0x80000 && p->bogomem_size != 0x100000 && p->bogomem_size != 0x180000 && p->bogomem_size != 0x1c0000) {
	p->bogomem_size = 0;
	write_log ("Unsupported bogomem size!\n");
	err = 1;
    }
    if (p->bogomem_size > 0x100000 && ((p->chipset_mask & CSMASK_AGA) || p->cpu_level >= 2)) {
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
    fixup_prefs_dimensions (&currprefs);

    fixup_prefs_dimensions (p);

    if (err)
	write_log ("Please use \"uae -h\" to get usage information.\n");
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

static void parse_cmdline_and_init_file (int argc, char **argv)
{
}

void reset_all_systems (void)
{
    init_eventtab ();
    memory_reset ();
#ifdef FILESYS
    filesys_reset ();
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

extern void dump_blitter_stat(void);

void do_leave_program (void)
{
#ifdef JIT
    compiler_exit();
#endif
//    dump_blitter_stat();
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
  memory_cleanup ();
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
#ifdef PANDORA
  SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO);
#else 
#ifdef USE_SDL
  SDL_Init (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
#endif
#endif

  if (restart_config[0]) {
#ifdef FILESYS
	  free_mountinfo (currprefs.mountinfo);
#endif
	  default_prefs (&currprefs, 0);
	  changed_prefs = currprefs;
    my_cfgfile_load(&changed_prefs, restart_config, 0);
    currprefs = changed_prefs;
	  fixup_prefs (&currprefs);
  }

  if (! graphics_setup ()) {
	exit (1);
  }

#ifdef FILESYS
  rtarea_init ();
  hardfile_install();
#endif

  	currprefs = changed_prefs;

  machdep_init ();

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
    update_display(&changed_prefs);
  }

  fixup_prefs (&currprefs);
  changed_prefs = currprefs;

#ifdef AUTOCONFIG
  /* Install resident module to get 8MB chipmem, if requested */
  rtarea_setup ();
#endif

  keybuf_init (); /* Must come after init_joystick */

#ifdef AUTOCONFIG
  expansion_init ();
#endif
  memory_init ();
  memory_reset ();

#ifdef FILESYS
  filesys_install (); 
#endif
#ifdef AUTOCONFIG
  native2amiga_install ();
#endif

  custom_init (); /* Must come after memory_init */
  DISK_init ();

  reset_frame_rate_hack ();
  init_m68k(); /* must come after reset_frame_rate_hack (); */

  gui_update ();

  if (graphics_init ()) {
#ifdef FILESYS
  	filesys_init (); /* New function, to do 'add_filesys_unit()' calls at start-up */
#endif
  	if (sound_available && currprefs.produce_sound > 1 && ! init_audio ()) {
		  write_log ("Sound driver unavailable: Sound output disabled\n");
		  currprefs.produce_sound = 0;
    }

		start_program ();
	}
}

void real_main (int argc, char **argv)
{
  free_mountinfo (&options_mountinfo);
  currprefs.mountinfo = changed_prefs.mountinfo = &options_mountinfo;
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
