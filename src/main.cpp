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
#include "akiko.h"
#include "savestate.h"
#include "filesys.h"
#include "blkdev.h"
#include "uaeresource.h"
#ifdef JIT
#include "jit/compemu.h"
#endif

#ifdef USE_SDL
#include "SDL.h"
#endif
long int version = 256*65536L*UAEMAJOR + 65536L*UAEMINOR + UAESUBREV;

struct uae_prefs currprefs, changed_prefs; 

bool no_gui = 0;
bool cloanto_rom = 0;
bool kickstart_rom = 1;

struct gui_info gui_data;

TCHAR optionsfile[256];

void my_trim (TCHAR *s)
{
	int len;
	while (_tcscspn (s, _T("\t \r\n")) == 0)
		memmove (s, s + 1, (_tcslen (s + 1) + 1) * sizeof (TCHAR));
	len = _tcslen (s);
	while (len > 0 && _tcscspn (s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

TCHAR *my_strdup_trim (const TCHAR *s)
{
	TCHAR *out;
	int len;

	while (_tcscspn (s, _T("\t \r\n")) == 0)
		s++;
	len = _tcslen (s);
	while (len > 0 && _tcscspn (s + len - 1, _T("\t \r\n")) == 0)
		len--;
	out = xmalloc (TCHAR, len + 1);
	memcpy (out, s, len * sizeof (TCHAR));
	out[len] = 0;
	return out;
}

void discard_prefs (struct uae_prefs *p, int type)
{
  struct strlist **ps = &p->all_lines;
  while (*ps) {
	  struct strlist *s = *ps;
	  *ps = s->next;
	  xfree (s->value);
	  xfree (s->option);
	  xfree (s);
  }
#ifdef FILESYS
  filesys_cleanup ();
  p->mountitems = 0;
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
    	if (p->cpu_compatible)
    	  p->fpu_model = 0;
      break;
  	case 68010:
    	p->address_space_24 = 1;
    	if (p->cpu_compatible)
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

	if (p->immediate_blits && p->waiting_blits)
		p->waiting_blits = 0;
}


void fixup_prefs (struct uae_prefs *p)
{
  int err = 0;

  fixup_cpu(p);

  if (((p->chipmem_size & (p->chipmem_size - 1)) != 0 && p->chipmem_size != 0x180000)
	|| p->chipmem_size < 0x20000
	|| p->chipmem_size > 0x800000)
  {
		write_log (_T("Unsupported chipmem size %x!\n"), p->chipmem_size);
	  p->chipmem_size = 0x200000;
	  err = 1;
  }

  if ((p->fastmem_size & (p->fastmem_size - 1)) != 0
	|| (p->fastmem_size != 0 && (p->fastmem_size < 0x100000 || p->fastmem_size > 0x800000)))
  {
		write_log (_T("Unsupported fastmem size %x!\n"), p->fastmem_size);
	  err = 1;
  }
  if ((p->rtgmem_size & (p->rtgmem_size - 1)) != 0
	|| (p->rtgmem_size != 0 && (p->rtgmem_size < 0x100000 || p->rtgmem_size > 0x1000000)))
  {
	  write_log (_T("Unsupported graphics card memory size %x!\n"), p->rtgmem_size);
		if (p->rtgmem_size > 0x1000000)
			p->rtgmem_size = 0x1000000;
		else
	    p->rtgmem_size = 0;
	  err = 1;
  }
    
  if ((p->z3fastmem_size & (p->z3fastmem_size - 1)) != 0
	|| (p->z3fastmem_size != 0 && (p->z3fastmem_size < 0x100000 || p->z3fastmem_size > max_z3fastmem)))
  {
		write_log (_T("Unsupported Zorro III fastmem size %x (%x)!\n"), p->z3fastmem_size, max_z3fastmem);
	  if (p->z3fastmem_size > max_z3fastmem)
	    p->z3fastmem_size = max_z3fastmem;
  	else
     p->z3fastmem_size = 0;
	  err = 1;
  }
  p->z3fastmem_start &= ~0xffff;
	if (p->z3fastmem_start < 0x1000000)
		p->z3fastmem_start = 0x1000000;
    
  if (p->address_space_24 && (p->z3fastmem_size != 0)) {
  	p->z3fastmem_size = 0;
  	write_log (_T("Can't use a graphics card or 32-bit memory when using a 24 bit\naddress space.\n"));
  }
  if (p->bogomem_size != 0 && p->bogomem_size != 0x80000 && p->bogomem_size != 0x100000 && p->bogomem_size != 0x180000 && p->bogomem_size != 0x1c0000) {
	  p->bogomem_size = 0;
		write_log (_T("Unsupported bogomem size!\n"));
	  err = 1;
  }
  if (p->bogomem_size > 0x180000 && ((p->chipset_mask & CSMASK_AGA) || p->cpu_model >= 68020)) {
	  p->bogomem_size = 0x180000;
		write_log (_T("Possible Gayle bogomem conflict fixed\n"));
  }

  if (p->chipmem_size > 0x200000 && p->fastmem_size != 0) {
		write_log (_T("You can't use fastmem and more than 2MB chip at the same time!\n"));
	  p->fastmem_size = 0;
	  err = 1;
  }

	if (p->address_space_24 && p->rtgmem_size)
		p->rtgmem_type = 0;
	if (!p->rtgmem_type && (p->chipmem_size > 2 * 1024 * 1024 || getz2size (p) > 8 * 1024 * 1024 || getz2size (p) < 0)) {
		p->rtgmem_size = 0;
		write_log (_T("Too large Z2 RTG memory size\n"));
	}

  if (p->produce_sound < 0 || p->produce_sound > 3) {
		write_log (_T("Bad value for -S parameter: enable value must be within 0..3\n"));
	  p->produce_sound = 0;
	  err = 1;
  }
  if (p->cachesize < 0 || p->cachesize > 16384) {
		write_log (_T("Bad value for cachesize parameter: value must be within 0..16384\n"));
	  p->cachesize = 0;
	  err = 1;
  }
  if (p->z3fastmem_size > 0 && (p->address_space_24 || p->cpu_model < 68020)) {
		write_log (_T("Z3 fast memory can't be used with a 68000/68010 emulation. It\n")
			_T("requires a 68020 emulation. Turning off Z3 fast memory.\n"));
	  p->z3fastmem_size = 0;
	  err = 1;
  }
  if (p->rtgmem_size > 0 && p->rtgmem_type && (p->cpu_model < 68020 || p->address_space_24)) {
		write_log (_T("RTG can't be used with a 68000/68010 or 68EC020 emulation. It\n")
			_T("requires a 68020 emulation. Turning off RTG.\n"));
	  p->rtgmem_size = 0;
	  err = 1;
  }
#if !defined (BSDSOCKET)
	if (p->socket_emu) {
		write_log (_T("Compile-time option of BSDSOCKET_SUPPORTED was not enabled.  You can't use bsd-socket emulation.\n"));
		p->socket_emu = 0;
		err = 1;
	}
#endif

  if (p->nr_floppies < 0 || p->nr_floppies > 4) {
		write_log (_T("Invalid number of floppies.  Using 4.\n"));
	  p->nr_floppies = 4;
	  p->floppyslots[0].dfxtype = 0;
	  p->floppyslots[1].dfxtype = 0;
	  p->floppyslots[2].dfxtype = 0;
	  p->floppyslots[3].dfxtype = 0;
	  err = 1;
  }

  if (p->floppy_speed > 0 && p->floppy_speed < 10) {
  	p->floppy_speed = 100;
  }

  if (p->collision_level < 0 || p->collision_level > 3) {
		write_log (_T("Invalid collision support level.  Using 1.\n"));
	  p->collision_level = 1;
	  err = 1;
  }
  fixup_prefs_dimensions (p);

#if !defined (JIT)
	p->cachesize = 0;
#endif
#ifdef CPU_68000_ONLY
	p->cpu_model = 68000;
	p->fpu_model = 0;
#endif
#ifndef AGA
	p->chipset_mask &= ~CSMASK_AGA;
#endif
#ifndef AUTOCONFIG
	p->z3fastmem_size = 0;
	p->fastmem_size = 0;
	p->rtgmem_size = 0;
#endif
#if !defined (BSDSOCKET)
	p->socket_emu = 0;
#endif

	blkdev_fix_prefs (p);
  target_fixup_options (p);
}

int quit_program = 0;
static int restart_program;
static TCHAR restart_config[MAX_DPATH];
static int default_config;

void uae_reset (int hardreset, int keyboardreset)
{
  if (quit_program == 0) {
		quit_program = -UAE_RESET;
		if (keyboardreset)
			quit_program = -UAE_RESET_KEYBOARD;
	  if (hardreset)
			quit_program = -UAE_RESET_HARD;
  }
}

void uae_quit (void)
{
  if (quit_program != -UAE_QUIT)
  	quit_program = -UAE_QUIT;
  target_quit ();
}

/* 0 = normal, 1 = nogui, -1 = disable nogui */
void uae_restart (int opengui, const TCHAR *cfgfile)
{
  uae_quit ();
  restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
  restart_config[0] = 0;
	default_config = 0;
  if (cfgfile)
  	_tcscpy (restart_config, cfgfile);
	target_restart ();
}

static void parse_cmdline_2 (int argc, TCHAR **argv)
{
  int i;

  cfgfile_addcfgparam (0);
  for (i = 1; i < argc; i++) {
		if (_tcsncmp (argv[i], _T("-cfgparam="), 10) == 0) {
      cfgfile_addcfgparam (argv[i] + 10);
		} else if (_tcscmp (argv[i], _T("-cfgparam")) == 0) {
      if (i + 1 == argc)
				write_log (_T("Missing argument for '-cfgparam' option.\n"));
      else
	      cfgfile_addcfgparam (argv[++i]);
    }
  }
}


static TCHAR *parsetext (const TCHAR *s)
{
  if (*s == '"' || *s == '\'') {
  	TCHAR *d;
  	TCHAR c = *s++;
  	int i;
  	d = my_strdup (s);
  	for (i = 0; i < _tcslen (d); i++) {
	    if (d[i] == c) {
    		d[i] = 0;
    		break;
	    }
  	}
  	return d;
  } else {
  	return my_strdup (s);
  }
}
static TCHAR *parsetextpath (const TCHAR *s)
{
	TCHAR *s2 = parsetext (s);
	TCHAR *s3 = target_expand_environment (s2);
	xfree (s2);
	return s3;
}

void  print_usage()
{
   printf("\nUsage:\n");
   printf(" -f <file>                  Load a configuration file.\n");
   printf(" -config=<file>             Load a configuration file.\n");
   printf(" -statefile=<file>          Load a save state file.\n");
   printf(" -s <config param>=<value>  Set the configuration parameter with value.\n");
   printf("                            Edit a configuration file in order to know valid parameters and settings.\n");
   printf("\nAdditional options:\n");
   printf(" -0 <filename>              Set adf for drive 0.\n");
   printf(" -1 <filename>              Set adf for drive 1.\n");
   printf(" -2 <filename>              Set adf for drive 2.\n");
   printf(" -3 <filename>              Set adf for drive 3.\n");
   printf(" -r <filename>              Set kickstart rom file.\n");
   printf(" -G                         Start directly into emulation.\n");
   printf(" -c <value>                 Size of chip memory (in number of 512 KBytes chunks).\n");
   printf(" -F <value>                 Size of fast memory (in number of 1024 KBytes chunks).\n");
   printf("\nNote:\n");
   printf("Parameters are parsed from the beginning of command line, so in case of ambiguity for parameters, last one will be used.\n");
   printf("File names should be with absolute path.\n");
   printf("\nExample:\n");
   printf("uae4arm -config=conf/A500.uae -statefile=savestates/game.uss -s use_gui=no\n");
   printf("It will load A400.uae configuration with the save state named game.\n");
   printf("It will override use_gui to 'no' so that it enters emulation directly.\n");
   exit(1);
}


static void parse_cmdline (int argc, TCHAR **argv)
{
  int i;

  for (i = 1; i < argc; i++) {
	  if (_tcscmp (argv[i], _T("-cfgparam")) == 0) {
	    if (i + 1 < argc)
		    i++;
		} else if (_tcsncmp (argv[i], _T("-config="), 8) == 0) {
	    TCHAR *txt = parsetextpath (argv[i] + 8);
	    currprefs.mountitems = 0;
	    target_cfgfile_load (&currprefs, txt, -1, 0);
	    xfree (txt);
		} else if (_tcsncmp (argv[i], _T("-statefile="), 11) == 0) {
	    TCHAR *txt = parsetextpath (argv[i] + 11);
	    savestate_state = STATE_DORESTORE;
	    _tcscpy (savestate_fname, txt);
	    xfree (txt);
		} else if (_tcscmp (argv[i], _T("-f")) == 0) {
	  /* Check for new-style "-f xxx" argument, where xxx is config-file */
	    if (i + 1 == argc) {
				write_log (_T("Missing argument for '-f' option.\n"));
	    } else {
				TCHAR *txt = parsetextpath (argv[++i]);
        currprefs.mountitems = 0;
		    target_cfgfile_load (&currprefs, txt, -1, 0);
				xfree (txt);
	    }
		} else if (_tcscmp (argv[i], _T("-s")) == 0) {
	    if (i + 1 == argc)
				write_log (_T("Missing argument for '-s' option.\n"));
	    else
		    cfgfile_parse_line (&currprefs, argv[++i], 0);
	  } else {
	    if (argv[i][0] == '-' && argv[i][1] != '\0' && argv[i][2] == '\0') {
		    int ret;
		    const TCHAR *arg = argv[i] + 2;
		    int extra_arg = *arg == '\0';
		    if (extra_arg)
		      arg = i + 1 < argc ? argv[i + 1] : 0;
		    ret = parse_cmdline_option (&currprefs, argv[i][1], arg);
		    if (ret == -1)
		      print_usage();
		    if (ret && extra_arg)
		      i++;
	    } else
	    {
	      printf("Unknown option %s\n",argv[i]);
	      print_usage();
	    }
	  }
  }
}

static void parse_cmdline_and_init_file (int argc, TCHAR **argv)
{
	_tcscpy (optionsfile, _T(""));

  parse_cmdline_2 (argc, argv);

	_tcscat (optionsfile, restart_config);

  if (! target_cfgfile_load (&currprefs, optionsfile, 0, default_config)) {
  	write_log (_T("failed to load config '%s'\n"), optionsfile);
  }
  fixup_prefs (&currprefs);

  parse_cmdline (argc, argv);
}

void reset_all_systems (void)
{
  init_eventtab ();

#ifdef PICASSO96
  picasso_reset ();
#endif
#ifdef FILESYS
  filesys_prepare_reset ();
  filesys_reset ();
#endif
  memory_reset ();
#if defined (BSDSOCKET)
	bsdlib_reset ();
#endif
#ifdef FILESYS
  filesys_start_threads ();
  hardfile_reset ();
#endif
  native2amiga_reset ();
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
	if (quit_program == -UAE_QUIT)
  	return;
  /* Do a reset on startup. Whether this is elegant is debatable. */
	inputdevice_updateconfig (&changed_prefs, &currprefs);
  if (quit_program >= 0)
	  quit_program = UAE_RESET;
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
	dump_counts ();
#ifdef CD32
	akiko_free ();
#endif
 	gui_exit ();
#ifdef USE_SDL
  SDL_Quit ();
#endif
  hardfile_reset();
#ifdef AUTOCONFIG
  expansion_cleanup ();
#endif
#ifdef FILESYS
  filesys_cleanup ();
#endif
#ifdef BSDSOCKET
  bsdlib_reset ();
#endif
	device_func_reset ();
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

void virtualdevice_init (void)
{
#ifdef AUTOCONFIG
	rtarea_setup ();
#endif
#ifdef FILESYS
	rtarea_init ();
	uaeres_install ();
	hardfile_install ();
#endif
#ifdef AUTOCONFIG
	expansion_init ();
	emulib_install ();
#endif
#ifdef FILESYS
	filesys_install ();
#endif
#if defined (BSDSOCKET)
	bsdlib_install ();
#endif
}

static int real_main2 (int argc, TCHAR **argv)
{
  printf("Uae4arm v0.5 for Raspberry Pi by Chips\n");
#ifdef PANDORA_SPECIFIC
  SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO);
#else 
#ifdef USE_SDL
  SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
#endif
#endif

 
  keyboard_settrans();

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
	  return -1;
  }

  if (! setup_sound ()) {
		write_log (_T("Sound driver unavailable: Sound output disabled\n"));
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
			write_log (_T("Failed to initialize the GUI\n"));
      return -1;
	  } else if (err == -2) {
      return 1;
	  }
  }
  else
  {
  	setCpuSpeed();
    update_display(&currprefs);
  }



	memset (&gui_data, 0, sizeof gui_data);
	gui_data.cd = -1;
	gui_data.hd = -1;

#ifdef PICASSO96
	picasso_reset ();
#endif

  fixup_prefs (&currprefs);
  changed_prefs = currprefs;
	target_run ();
  /* force sound settings change */
  currprefs.produce_sound = 0;

  keybuf_init (); /* Must come after init_joystick */

	memory_hardreset (2);
  memory_reset ();

#ifdef AUTOCONFIG
  native2amiga_install ();
#endif

  custom_init (); /* Must come after memory_init */
  DISK_init ();

  reset_frame_rate_hack ();
  init_m68k(); /* must come after reset_frame_rate_hack (); */

  gui_update ();

  if (graphics_init (true)) {

    if(!init_audio ()) {
  	  if (sound_available && currprefs.produce_sound > 1) {
				write_log (_T("Sound driver unavailable: Sound output disabled\n"));
	    }
		  currprefs.produce_sound = 0;
    }
		start_program ();
	}
  return 0;
}

void real_main (int argc, TCHAR **argv)
{
  restart_program = 1;
  fetch_configurationpath (restart_config, sizeof (restart_config) / sizeof (TCHAR));
  _tcscat (restart_config, OPTIONSFILENAME);
  _tcscat (restart_config, ".uae");
	default_config = 1;

  while (restart_program) {
	  changed_prefs = currprefs;
	  real_main2 (argc, argv);
    leave_program ();
	  quit_program = 0;
  }
  zfile_exit ();
}

#ifndef NO_MAIN_IN_MAIN_C
int main (int argc, TCHAR **argv)
{
    real_main (argc, argv);
    return 0;
}
#endif

#ifdef SINGLEFILE
uae_u8 singlefile_config[50000] = { "_CONFIG_STARTS_HERE" };
uae_u8 singlefile_data[1500000] = { "_DATA_STARTS_HERE" };
#endif
