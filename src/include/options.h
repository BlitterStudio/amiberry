 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Stuff
  *
  * Copyright 1995, 1996 Ed Hanway
  * Copyright 1995-2001 Bernd Schmidt
  */

#ifndef OPTIONS_H
#define OPTIONS_H

#define UAEMAJOR 2
#define UAEMINOR 8
#define UAESUBREV 1

extern long int version;

struct strlist {
  struct strlist *next;
  TCHAR *option, *value;
  int unknown;
};

#define MAX_TOTAL_SCSI_DEVICES 8

/* maximum number native input devices supported (single type) */
#define MAX_INPUT_DEVICES 8
/* maximum number of native input device's buttons and axles supported */
#define MAX_INPUT_DEVICE_EVENTS 256
/* 4 different customization settings */
#define MAX_INPUT_SETTINGS 4
#define GAMEPORT_INPUT_SETTINGS 3 // last slot is for gameport panel mappings

#define MAX_INPUT_SUB_EVENT 8
#define MAX_INPUT_SUB_EVENT_ALL 9
#define SPARE_SUB_EVENT 8

struct uae_input_device {
	TCHAR *name;
	TCHAR *configname;
	uae_s16 eventid[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	TCHAR *custom[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	uae_u64 flags[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	uae_s8 port[MAX_INPUT_DEVICE_EVENTS][MAX_INPUT_SUB_EVENT_ALL];
	uae_s16 extra[MAX_INPUT_DEVICE_EVENTS];
	uae_s8 enabled;
};

#define MAX_JPORTS 4
#define NORMAL_JPORTS 2
#define MAX_JPORTNAME 128
struct jport {
	int id;
	int mode; // 0=def,1=mouse,2=joy,3=anajoy,4=lightpen
	int autofire;
	TCHAR name[MAX_JPORTNAME];
	TCHAR configname[MAX_JPORTNAME];
	bool nokeyboardoverride;
};
#define JPORT_NONE -1
#define JPORT_CUSTOM -2
#define JPORT_AF_NORMAL 1
#define JPORT_AF_TOGGLE 2
#define JPORT_AF_ALWAYS 3

#define CONFIG_TYPE_HARDWARE 1
#define CONFIG_TYPE_HOST 2
#define CONFIG_BLEN 2560

#define TABLET_OFF 0
#define TABLET_MOUSEHACK 1
#define TABLET_REAL 2

struct cdslot
{
	TCHAR name[MAX_DPATH];
	bool inuse;
	bool delayed;
	int type;
};
struct floppyslot
{
	TCHAR df[MAX_DPATH];
	int dfxtype;
	bool forcedwriteprotect;
};

struct wh {
  int x, y;
  int width, height;
};

#define MOUNT_CONFIG_SIZE 30
#define UAEDEV_DIR 0
#define UAEDEV_HDF 1
#define UAEDEV_CD 2
#define UAEDEV_TAPE 3

#define BOOTPRI_NOAUTOBOOT -128
#define BOOTPRI_NOAUTOMOUNT -129
#define ISAUTOBOOT(ci) ((ci)->bootpri > BOOTPRI_NOAUTOBOOT)
#define ISAUTOMOUNT(ci) ((ci)->bootpri > BOOTPRI_NOAUTOMOUNT)
struct uaedev_config_info {
	int type;
  TCHAR devname[MAX_DPATH];
  TCHAR volname[MAX_DPATH];
  TCHAR rootdir[MAX_DPATH];
  bool readonly;
  int bootpri;
  TCHAR filesys[MAX_DPATH];
	int lowcyl;
	int highcyl; // zero if detected from size
	int cyls; // calculated/corrected highcyl
  int surfaces;
  int sectors;
  int reserved;
  int blocksize;
  int controller;
	// zero if default
	int pcyls, pheads, psecs;
	int flags;
	int buffers;
	int bufmemtype;
	int stacksize;
	int priority;
	uae_u32 mask;
	int maxtransfer;
	uae_u32 dostype;
	int unit;
	int interleave;
	int sectorsperblock;
	int forceload;
	int device_emu_unit;
};

struct uaedev_config_data
{
	struct uaedev_config_info ci;
	int configoffset; // HD config entry index
	int unitnum; // scsi unit number (if tape currently)
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
  int sound_volume_cd;

  int cachesize;
  int optcount[10];

#ifdef RASPBERRY
    int gfx_correct_aspect;
    int gfx_fullscreen_ratio;
    int kbd_led_num;
    int kbd_led_scr;
    int kbd_led_cap;
	int key_for_menu;
#endif

  int gfx_framerate;
  struct wh gfx_size_win;
  struct wh gfx_size_fs;
  struct wh gfx_size;
  int gfx_resolution;
 
  bool immediate_blits;
  int waiting_blits;
  unsigned int chipset_mask;
  bool ntscmode;
  int chipset_refreshrate;
  int collision_level;
  int leds_on_screen;
  int fast_copper;
  int floppy_speed;
  int floppy_write_length;
  bool tod_hack;
  int filesys_limit;
  int filesys_max_name;

  bool cs_cd32cd;
  bool cs_cd32c2p;
  bool cs_cd32nvram;

  TCHAR romfile[MAX_DPATH];
  TCHAR romextfile[MAX_DPATH];
  TCHAR flashfile[MAX_DPATH];
  struct cdslot cdslots[MAX_TOTAL_SCSI_DEVICES];

  TCHAR path_floppy[256];
  TCHAR path_hardfile[256];
  TCHAR path_rom[256];
  TCHAR path_cd[256];

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
  struct uaedev_config_data mountconfig[MOUNT_CONFIG_SIZE];

  int nr_floppies;
  struct floppyslot floppyslots[4];

  /* Target specific options */
  int pandora_horizontal_offset;
  int pandora_vertical_offset;
  int pandora_cpu_speed;
  int pandora_hide_idle_led;
  
  int pandora_tapDelay;
  int pandora_customControls;
    
  /* input */

	struct jport jports[MAX_JPORTS];
	int input_selected_setting;
  int input_joymouse_multiplier;
	int input_joymouse_deadzone;
	int input_joystick_deadzone;
	int input_joymouse_speed;
	int input_analog_joystick_mult;
	int input_analog_joystick_offset;
	int input_autofire_linecnt;
	int input_mouse_speed;
  int input_tablet;
	int input_keyboard_type;
	struct uae_input_device joystick_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device mouse_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device keyboard_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	TCHAR input_config_name[GAMEPORT_INPUT_SETTINGS][256];
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

extern struct uaedev_config_data *add_filesys_config (struct uae_prefs *p, int index, struct uaedev_config_info*);
extern bool get_hd_geometry (struct uaedev_config_info *);
extern void uci_set_defaults (struct uaedev_config_info *uci, bool rdb);

extern void error_log (const TCHAR*, ...);
extern TCHAR *get_error_log (void);
extern bool is_error_log (void);

extern void default_prefs (struct uae_prefs *, int);
extern void discard_prefs (struct uae_prefs *, int);
extern int bip_a500 (struct uae_prefs *p, int rom);
extern int bip_a500plus (struct uae_prefs *p, int rom);
extern int bip_a1200 (struct uae_prefs *p, int rom);
extern int bip_a2000 (struct uae_prefs *p, int rom);
extern int bip_a4000 (struct uae_prefs *p, int rom);
extern int bip_cd32 (struct uae_prefs *p, int rom);

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
extern uae_u32 cfgfile_uaelib (int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen);
extern uae_u32 cfgfile_uaelib_modify (uae_u32 mode, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize);
extern uae_u32 cfgfile_modify (uae_u32 index, TCHAR *parms, uae_u32 size, TCHAR *out, uae_u32 outsize);
extern void cfgfile_addcfgparam (TCHAR *);
extern int cfgfile_configuration_change(int);
extern void fixup_prefs_dimensions (struct uae_prefs *prefs);
extern void fixup_prefs (struct uae_prefs *prefs);
extern void fixup_cpu (struct uae_prefs *prefs);

extern void check_prefs_changed_custom (void);
extern void check_prefs_changed_cpu (void);
extern void check_prefs_changed_audio (void);
extern void check_prefs_changed_cd (void);
extern int check_prefs_changed_gfx (void);

extern struct uae_prefs currprefs, changed_prefs;

extern int machdep_init (void);
extern void machdep_free (void);

#endif /* OPTIONS_H */
