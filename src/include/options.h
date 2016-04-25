 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Stuff
  *
  * Copyright 1995, 1996 Ed Hanway
  * Copyright 1995-2001 Bernd Schmidt
  */

#define UAEMAJOR 2
#define UAEMINOR 4
#define UAESUBREV 1

extern long int version;

struct strlist {
  struct strlist *next;
  TCHAR *option, *value;
  int unknown;
};

#define DEFAULT_JIT_CACHE_SIZE 8192

#define CONFIG_TYPE_HARDWARE 1
#define CONFIG_TYPE_HOST 2
#define CONFIG_BLEN 2560

#define TABLET_OFF 0
#define TABLET_MOUSEHACK 1
#define TABLET_REAL 2

struct floppyslot
{
	TCHAR df[MAX_DPATH];
	int dfxtype;
};

struct wh {
  int x, y;
  int width, height;
};

#define MOUNT_CONFIG_SIZE 30
struct uaedev_config_info {
  TCHAR devname[MAX_DPATH];
  TCHAR volname[MAX_DPATH];
  TCHAR rootdir[MAX_DPATH];
  bool ishdf;
  bool readonly;
  int bootpri;
  bool autoboot;
  bool donotmount;
  TCHAR filesys[MAX_DPATH];
  int surfaces;
  int sectors;
  int reserved;
  int blocksize;
  int configoffset;
  int controller;
};

struct uae_prefs {
  struct strlist *all_lines;

  TCHAR description[256];
  TCHAR info[256];
  int config_version;

	bool socket_emu;

  bool start_gui;

  int produce_sound;
  int sound_stereo;
  int sound_stereo_separation;
  int sound_mixed_stereo_delay;
  int sound_freq;
  int sound_interpol;
  int sound_filter;
  int sound_filter_type;

  int cachesize;
  int optcount[10];

  int gfx_framerate;
  struct wh gfx_size_win;
  struct wh gfx_size_fs;
  struct wh gfx_size;
  int gfx_resolution;

#ifdef RASPBERRY
    int gfx_correct_aspect;
#endif 

  bool immediate_blits;
  unsigned int chipset_mask;
  bool ntscmode;
  int chipset_refreshrate;
  int collision_level;
  int leds_on_screen;
  int fast_copper;
  int floppy_speed;
  int floppy_write_length;
  bool tod_hack;

  TCHAR romfile[MAX_DPATH];
  TCHAR romextfile[MAX_DPATH];

  TCHAR path_floppy[256];
  TCHAR path_hardfile[256];
  TCHAR path_rom[256];

  int m68k_speed;
  int cpu_model;
  int fpu_model;
  bool cpu_compatible;
  bool address_space_24;
  int picasso96_modeflags;

  uae_u32 z3fastmem_size;
  uae_u32 z3fastmem_start;
  uae_u32 fastmem_size;
  uae_u32 chipmem_size;
  uae_u32 bogomem_size;
  uae_u32 rtgmem_size;
  int rtgmem_type;

  int mountitems;
  struct uaedev_config_info mountconfig[MOUNT_CONFIG_SIZE];

  int nr_floppies;
  struct floppyslot floppyslots[4];

  /* Target specific options */
  int pandora_horizontal_offset;
  int pandora_vertical_offset;
  int pandora_cpu_speed;
  
  int pandora_joyConf;
  int pandora_joyPort;
  int pandora_tapDelay;
  
  int pandora_customControls;
  int pandora_custom_dpad;    // 0-joystick, 1-mouse, 2-custom
  int pandora_custom_A;
  int pandora_custom_B;
  int pandora_custom_X;
  int pandora_custom_Y;
  int pandora_custom_L;
  int pandora_custom_R;
  int pandora_custom_up;
  int pandora_custom_down;
  int pandora_custom_left;
  int pandora_custom_right;
    
  int pandora_button1;
  int pandora_button2;
  int pandora_autofireButton1;
  int pandora_jump;

  /* input */

  int input_joymouse_multiplier;
  int input_autofire_framecnt;
  int input_tablet;
};

/* Contains the filename of .uaerc */
extern void cfgfile_write (struct zfile *, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_dwrite (struct zfile *, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_target_write (struct zfile *, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_target_dwrite (struct zfile *, const TCHAR *option, const TCHAR *format,...);

extern void cfgfile_write_bool (struct zfile *f, const TCHAR *option, bool b);
extern void cfgfile_dwrite_bool (struct zfile *f,const  TCHAR *option, bool b);
extern void cfgfile_target_write_bool (struct zfile *f, const TCHAR *option, bool b);
extern void cfgfile_target_dwrite_bool (struct zfile *f, const TCHAR *option, bool b);

extern void cfgfile_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value);

extern struct uaedev_config_info *add_filesys_config (struct uae_prefs *p, int index,
			TCHAR *devname, TCHAR *volname, TCHAR *rootdir, bool readonly,
			int secspertrack, int surfaces, int reserved,
			int blocksize, int bootpri, TCHAR *filesysdir, int hdc, int flags);

extern void default_prefs (struct uae_prefs *, int);
extern void discard_prefs (struct uae_prefs *, int);
extern int bip_a500 (struct uae_prefs *p, int rom);
extern int bip_a500plus (struct uae_prefs *p, int rom);
extern int bip_a1200 (struct uae_prefs *p, int rom);
extern int bip_a2000 (struct uae_prefs *p, int rom);
extern int bip_a4000 (struct uae_prefs *p, int rom);

int parse_cmdline_option (struct uae_prefs *, TCHAR, const TCHAR *);

extern int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location);
extern int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale);
extern int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more);
extern int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz);
extern TCHAR *cfgfile_subst_path (const TCHAR *path, const TCHAR *subst, const TCHAR *file);

extern TCHAR *target_expand_environment (const TCHAR *path);
extern int target_parse_option (struct uae_prefs *, const TCHAR *option, const TCHAR *value);
extern void target_save_options (struct zfile*, struct uae_prefs *);
extern void target_default_options (struct uae_prefs *, int type);
extern void target_fixup_options (struct uae_prefs *);
extern int target_cfgfile_load (struct uae_prefs *, const TCHAR *filename, int type, int isdefault);
extern void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type);

extern int cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int *type, int ignorelink, int userconfig);
extern int cfgfile_save (struct uae_prefs *p, const TCHAR *filename, int);
extern void cfgfile_parse_line (struct uae_prefs *p, TCHAR *, int);
extern int cfgfile_parse_option (struct uae_prefs *p, TCHAR *option, TCHAR *value, int);
extern int cfgfile_get_description (const TCHAR *filename, TCHAR *description);
extern void cfgfile_addcfgparam (TCHAR *);
extern int cfgfile_configuration_change(int);
extern void fixup_prefs_dimensions (struct uae_prefs *prefs);
extern void fixup_prefs (struct uae_prefs *prefs);
extern void fixup_cpu (struct uae_prefs *prefs);

extern void check_prefs_changed_custom (void);
extern void check_prefs_changed_cpu (void);
extern void check_prefs_changed_audio (void);
extern int check_prefs_changed_gfx (void);

extern struct uae_prefs currprefs, changed_prefs;

extern int machdep_init (void);
extern void machdep_free (void);
