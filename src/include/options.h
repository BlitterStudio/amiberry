 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Stuff
  *
  * Copyright 1995, 1996 Ed Hanway
  * Copyright 1995-2001 Bernd Schmidt
  */

#define UAEMAJOR 1
#define UAEMINOR 5
#define UAESUBREV 3

extern long int version;

struct strlist {
    struct strlist *next;
    char *option, *value;
    int unknown;
};

#define DEFAULT_JIT_CACHE_SIZE 8192

#define CONFIG_TYPE_HARDWARE 1
#define CONFIG_TYPE_HOST 2
#define CONFIG_BLEN 2560

struct wh {
    int x, y;
    int width, height;
};

#define MOUNT_CONFIG_SIZE 30
struct uaedev_config_info {
    char devname[MAX_DPATH];
    char volname[MAX_DPATH];
    char rootdir[MAX_DPATH];
    int ishdf;
    int readonly;
    int bootpri;
    int autoboot;
    int donotmount;
    char filesys[MAX_DPATH];
    int surfaces;
    int sectors;
    int reserved;
    int blocksize;
    int configoffset;
    int controller;
};

struct uae_prefs {
    struct strlist *all_lines;

    char description[256];
    char info[256];
    int config_version;

    int start_gui;

    int produce_sound;
    int sound_stereo;
    int sound_stereo_separation;
    int sound_mixed_stereo_delay;
    int sound_freq;
    int sound_interpol;
    int sound_filter;
    int sound_filter_type;
    int sound_auto;

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

    int immediate_blits;
    unsigned int chipset_mask;
    int ntscmode;
    int chipset_refreshrate;
    int collision_level;
    int leds_on_screen;
    int fast_copper;
    int cpu_idle;
    int floppy_speed;
    int floppy_write_length;
    int tod_hack;

    int cs_a1000ram;
    int cs_df0idhw;

    char df[4][MAX_DPATH];
    char romfile[MAX_DPATH];
    char romextfile[MAX_DPATH];

    char path_floppy[256];
    char path_hardfile[256];
    char path_rom[256];

    int m68k_speed;
    int cpu_model;
    int fpu_model;
    int cpu_compatible;
    int address_space_24;
    int picasso96_nocustom;
    int picasso96_modeflags;

    uae_u32 z3fastmem_size;
    uae_u32 z3fastmem_start;
    uae_u32 fastmem_size;
    uae_u32 chipmem_size;
    uae_u32 bogomem_size;
    uae_u32 gfxmem_size;

    int mountitems;
    struct uaedev_config_info mountconfig[MOUNT_CONFIG_SIZE];

    int nr_floppies;
    int dfxtype[4];

    /* Target specific options */
    int pandora_horizontal_offset;
    int pandora_vertical_offset;
    int pandora_cpu_speed;
    
    int pandora_joyConf;
    int pandora_joyPort;
    int pandora_tapDelay;
    int pandora_stylusOffset;
    
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
};

/* Contains the filename of .uaerc */
extern void cfgfile_write (struct zfile *, const char *format,...);
extern void cfgfile_dwrite (struct zfile *, const char *format,...);
extern void cfgfile_backup (const char *path);
extern struct uaedev_config_info *add_filesys_config (struct uae_prefs *p, int index,
			char *devname, char *volname, char *rootdir, int readonly,
			int secspertrack, int surfaces, int reserved,
			int blocksize, int bootpri, char *filesysdir, int hdc, int flags);

extern void default_prefs (struct uae_prefs *, int);
extern void discard_prefs (struct uae_prefs *, int);

int parse_cmdline_option (struct uae_prefs *, char, char *);

extern int cfgfile_yesno (const char *option, const char *value, const char *name, int *location);
extern int cfgfile_intval (char *option, char *value, const char *name, int *location, int scale);
extern int cfgfile_strval (const char *option, const char *value, const char *name, int *location, const char *table[], int more);
extern int cfgfile_string (const char *option, const char *value, const char *name, char *location, int maxsz);
extern char *cfgfile_subst_path (const char *path, const char *subst, const char *file);

extern int target_parse_option (struct uae_prefs *, char *option, char *value);
extern void target_save_options (struct zfile*, struct uae_prefs *);
extern void target_default_options (struct uae_prefs *, int type);
extern void target_fixup_options (struct uae_prefs *);
extern int target_cfgfile_load (struct uae_prefs *, char *filename, int type, int isdefault);
extern void target_quit (void);
extern void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type);

extern int cfgfile_load (struct uae_prefs *p, const char *filename, int *type, int ignorelink);
extern int cfgfile_save (struct uae_prefs *p, const char *filename, int);
extern void cfgfile_parse_line (struct uae_prefs *p, char *, int);
extern int cfgfile_parse_option (struct uae_prefs *p, char *option, char *value, int);
extern int cfgfile_get_description (const char *filename, char *description);
extern uae_u32 cfgfile_uaelib (int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen);
extern uae_u32 cfgfile_uaelib_modify (uae_u32 mode, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize);
extern void cfgfile_addcfgparam (char *);
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

#define MAX_COLOR_MODES 5

/* #define NEED_TO_DEBUG_BADLY */

#if !defined(USER_PROGRAMS_BEHAVE)
#define USER_PROGRAMS_BEHAVE 0
#endif
