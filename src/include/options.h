/*
* UAE - The Un*x Amiga Emulator
*
* Stuff
*
* Copyright 1995, 1996 Ed Hanway
* Copyright 1995-2001 Bernd Schmidt
*/

#ifndef UAE_OPTIONS_H
#define UAE_OPTIONS_H

#include <array>
#include <vector>

#include "uae/types.h"

#include "traps.h"
#include "guisan/color.hpp"

#define UAEMAJOR 8
#define UAEMINOR 0
#define UAESUBREV 0

#define MAX_AMIGADISPLAYS 1

typedef enum { KBD_LANG_US, KBD_LANG_DK, KBD_LANG_DE, KBD_LANG_SE, KBD_LANG_FR, KBD_LANG_IT, KBD_LANG_ES } KbdLang;

extern long int version;

#define MAX_PATHS 8

struct multipath {
	TCHAR path[MAX_PATHS][PATH_MAX];
};

#define PATH_NONE -1
#define PATH_FLOPPY 0
#define PATH_CD 1
#define PATH_DIR 2
#define PATH_HDF 3
#define PATH_FS 4
#define PATH_TAPE 5
#define PATH_GENLOCK_IMAGE 6
#define PATH_GENLOCK_VIDEO 7
#define PATH_GEO 8
#define PATH_ROM 9

struct strlist {
	struct strlist *next;
	TCHAR *option, *value;
	int unknown;
};

#define MAX_TOTAL_SCSI_DEVICES 8

/* maximum number native input devices supported (single type) */
#define MAX_INPUT_DEVICES 20
/* maximum number of native input device's buttons and axles supported */
#define MAX_INPUT_DEVICE_EVENTS 256
/* 4 different customization settings */
#define MAX_INPUT_SETTINGS 4
#define GAMEPORT_INPUT_SETTINGS 3 // last slot is for gameport panel mappings

#define MAX_INPUT_SUB_EVENT 8
#define MAX_INPUT_SUB_EVENT_ALL 9
#define SPARE_SUB_EVENT 8

#define INTERNALEVENT_COUNT 1

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

#define MAX_JPORTS_CUSTOM 6
#define MAX_JPORTS 4
#define NORMAL_JPORTS 2
#define MAX_JPORT_NAME 128
#define MAX_JPORT_CONFIG 256
struct jport_custom {
	TCHAR custom[MAX_DPATH];
};
struct inputdevconfig {
	TCHAR name[MAX_JPORT_NAME];
	TCHAR configname[MAX_JPORT_CONFIG];
	TCHAR shortid[16];
};
struct jport {
	int id{};
	int mode{}; // 0=default,1=wheel mouse,2=mouse,3=joystick,4=gamepad,5=analog joystick,6=cdtv,7=cd32
	int submode;
	int autofire{};
	struct inputdevconfig idc {};
	bool nokeyboardoverride{};
	bool changed{};
#ifdef AMIBERRY
	int mousemap{};
#endif
};

#define JPORT_UNPLUGGED -2
#define JPORT_NONE -1

#define JPORT_AF_NORMAL 1
#define JPORT_AF_TOGGLE 2
#define JPORT_AF_ALWAYS 3
#define JPORT_AF_TOGGLENOAF 4

#define KBTYPE_AMIGA 0
#define KBTYPE_PC1 1
#define KBTYPE_PC2 2

#define MAX_SPARE_DRIVES 20
#define MAX_CUSTOM_MEMORY_ADDRS 2

#define CONFIG_TYPE_ALL -1
#define CONFIG_TYPE_DEFAULT 0
#define CONFIG_TYPE_HARDWARE 1
#define CONFIG_TYPE_HOST 2
#define CONFIG_TYPE_NORESET 4
#define CONFIG_BLEN 2560

#define MOUSEUNTRAP_NONE 0
#define MOUSEUNTRAP_MIDDLEBUTTON 1
#define MOUSEUNTRAP_MAGIC 2
#define MOUSEUNTRAP_BOTH 3

#define TABLET_OFF 0
#define TABLET_MOUSEHACK 1
#define TABLET_REAL 2

#ifdef WITH_SLIRP
#define MAX_SLIRP_REDIRS 32
struct slirp_redir
{
	int proto;
	int srcport;
	int dstport;
	unsigned long addr;
};
#endif

struct cdslot
{
	TCHAR name[MAX_DPATH];
	bool inuse;
	bool delayed;
	bool temporary;
	int type;
};
struct floppyslot
{
	TCHAR df[MAX_DPATH];
	int dfxtype;
	int dfxsubtype;
	TCHAR dfxsubtypeid[32];
	TCHAR dfxprofile[32];
	int dfxclick;
	TCHAR dfxclickexternal[256];
	bool forcedwriteprotect;
};

#define ASPECTMULT 1024
#define WH_NATIVE 1
struct wh {
	int x, y;
	int width, height;
	int special;
};

#define MOUNT_CONFIG_SIZE 50
#define MAX_FILESYSTEM_UNITS 50
#define UAEDEV_DIR 0
#define UAEDEV_HDF 1
#define UAEDEV_CD 2
#define UAEDEV_TAPE 3

#define HD_LEVEL_SCSI_1 0
#define HD_LEVEL_SCSI_2 1
#define HD_LEVEL_SASI 2
#define HD_LEVEL_SASI_ENHANCED 2
#define HD_LEVEL_SASI_CHS 3

#define HD_LEVEL_ATA_1 0
#define HD_LEVEL_ATA_2 1
#define HD_LEVEL_ATA_2S 2

#define BOOTPRI_NOAUTOBOOT -128
#define BOOTPRI_NOAUTOMOUNT -129
#define ISAUTOBOOT(ci) ((ci)->bootpri > BOOTPRI_NOAUTOBOOT)
#define ISAUTOMOUNT(ci) ((ci)->bootpri > BOOTPRI_NOAUTOMOUNT)
#define MAX_UAEDEV_BADBLOCKS 8
struct uaedev_badblock
{
	uae_u32 first;
	uae_u32 last;
};
struct uaedev_config_info {
	int type;
	TCHAR devname[MAX_DPATH];
	TCHAR volname[MAX_DPATH];
	TCHAR rootdir[MAX_DPATH];
	bool readonly;
	bool lock;
	bool loadidentity;
	int bootpri;
	TCHAR filesys[MAX_DPATH];
	TCHAR geometry[MAX_DPATH];
	int lowcyl;
	int highcyl; // zero if detected from size
	int cyls; // calculated/corrected highcyl
	int surfaces;
	int sectors;
	int reserved;
	int blocksize;
	bool chs;
	uae_u64 max_lba;
	int controller_type;
	int controller_type_unit;
	int controller_unit;
	int controller_media_type; // 1 = CF IDE, 0 = normal
	int unit_feature_level;
	int unit_special_flags;
	bool physical_geometry; // if false: use defaults
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
	bool inject_icons;
	int badblock_num;
	struct uaedev_badblock badblocks[MAX_UAEDEV_BADBLOCKS];
	int uae_unitnum; // mountunit nr
};

struct uaedev_config_data
{
	struct uaedev_config_info ci;
	int configoffset; // HD config entry index
	int unitnum; // scsi unit number (if tape currently)
};

enum { CP_GENERIC = 1, CP_CDTV, CP_CDTVCR, CP_CD32, CP_A500, CP_A500P, CP_A600,
	CP_A1000, CP_A1200, CP_A2000, CP_A3000, CP_A3000T, CP_A4000, CP_A4000T,
	CP_VELVET, CP_CASABLANCA, CP_DRACO };

#define IDE_A600A1200 1
#define IDE_A4000 2

#define GFX_WINDOW 0
#define GFX_FULLSCREEN 1
#define GFX_FULLWINDOW 2

#define AUTOSCALE_NONE 0
#define AUTOSCALE_STATIC_AUTO 1
#define AUTOSCALE_STATIC_NOMINAL 2
#define AUTOSCALE_STATIC_MAX 3
#define AUTOSCALE_NORMAL 4
#define AUTOSCALE_RESIZE 5
#define AUTOSCALE_CENTER 6
#define AUTOSCALE_MANUAL 7 // use gfx_xcenter_pos and gfx_ycenter_pos
#define AUTOSCALE_INTEGER 8
#define AUTOSCALE_INTEGER_AUTOSCALE 9
#define AUTOSCALE_SEPARATOR 10
#define AUTOSCALE_OVERSCAN_BLANK 11

#define MONITOREMU_NONE 0
#define MONITOREMU_AUTO 1
#define MONITOREMU_A2024 2
#define MONITOREMU_GRAFFITI 3
#define MONITOREMU_HAM_E 4
#define MONITOREMU_HAM_E_PLUS 5
#define MONITOREMU_VIDEODAC18 6
#define MONITOREMU_AVIDEO12 7
#define MONITOREMU_AVIDEO24 8
#define MONITOREMU_FIRECRACKER24 9
#define MONITOREMU_DCTV 10
#define MONITOREMU_OPALVISION 11
#define MONITOREMU_COLORBURST 12

#define OVERSCANMODE_OVERSCAN 3
#define OVERSCANMODE_BROADCAST 4
#define OVERSCANMODE_EXTREME 5
#define OVERSCANMODE_ULTRA 6

#define AGNUSMODEL_AUTO 0
#define AGNUSMODEL_VELVET 1
#define AGNUSMODEL_A1000 2
#define AGNUSMODEL_OCS 3
#define AGNUSMODEL_ECS 4
#define AGNUSMODEL_AGA 5

#define AGNUSSIZE_AUTO 0
#define AGNUSSIZE_512 1
#define AGNUSSIZE_1M 2
#define AGNUSSIZE_2M 3

#define DENISEMODEL_AUTO 0
#define DENISEMODEL_VELVET 1
#define DENISEMODEL_A1000NOEHB 2
#define DENISEMODEL_A1000 3
#define DENISEMODEL_OCS 4
#define DENISEMODEL_ECS 5
#define DENISEMODEL_AGA 6

#define MAX_FILTERSHADERS 4

#define MAX_CHIPSET_REFRESH 10
#define MAX_CHIPSET_REFRESH_TOTAL (MAX_CHIPSET_REFRESH + 2)
#define CHIPSET_REFRESH_PAL (MAX_CHIPSET_REFRESH + 0)
#define CHIPSET_REFRESH_NTSC (MAX_CHIPSET_REFRESH + 1)
struct chipset_refresh
{
	bool inuse;
	int index;
	bool locked;
	bool rtg;
	bool exit;
	bool defaultdata;
	int horiz;
	int vert;
	int lace;
	int resolution;
	int resolution_pct;
	int ntsc;
	int vsync;
	int framelength;
	float rate;
	TCHAR label[16];
	TCHAR commands[256];
	TCHAR filterprofile[64];
};

#define APMODE_NATIVE 0
#define APMODE_RTG 1

struct apmode
{
	int gfx_fullscreen;
	int gfx_display;
	int gfx_vsync;
	// 0 = immediate flip
	// -1 = wait for flip, before frame ends
	// 1 = wait for flip, after new frame has started
	int gfx_vflip;
	// doubleframemode strobo
	bool gfx_strobo;
	int gfx_vsyncmode;
	int gfx_backbuffers;
	bool gfx_interlaced;
	int gfx_refreshrate;
};

#define MAX_LUA_STATES 16


#define MAX_FILTERDATA 3
#define GF_NORMAL 0
#define GF_RTG 1
#define GF_INTERLACE 2
struct gfx_filterdata
{
	int enable;
	int gfx_filter;
	TCHAR gfx_filtershader[2 * MAX_FILTERSHADERS + 1][MAX_DPATH];
	TCHAR gfx_filtermask[2 * MAX_FILTERSHADERS + 1][MAX_DPATH];
	TCHAR gfx_filteroverlay[MAX_DPATH];
	struct wh gfx_filteroverlay_pos;
	int gfx_filteroverlay_overscan;
	int gfx_filter_scanlines;
	int gfx_filter_scanlineratio;
	int gfx_filter_scanlinelevel;
	int gfx_filter_scanlineoffset;
	float gfx_filter_horiz_zoom, gfx_filter_vert_zoom;
	float gfx_filter_horiz_zoom_mult, gfx_filter_vert_zoom_mult;
	float gfx_filter_horiz_offset, gfx_filter_vert_offset;
	int gfx_filter_left_border, gfx_filter_right_border;
	int gfx_filter_top_border, gfx_filter_bottom_border;
	int gfx_filter_filtermodeh, gfx_filter_filtermodev;
	int gfx_filter_bilinear;
	int gfx_filter_noise, gfx_filter_blur;
	int gfx_filter_saturation, gfx_filter_luminance, gfx_filter_contrast;
	int gfx_filter_gamma, gfx_filter_gamma_ch[3];
	int gfx_filter_keep_aspect, gfx_filter_aspect;
	int gfx_filter_autoscale;
	int gfx_filter_integerscalelimit;
	int gfx_filter_keep_autoscale_aspect;
	int gfx_filter_rotation;
	bool changed;
};

#define MAX_DUPLICATE_EXPANSION_BOARDS 5
#define MAX_AVAILABLE_DUPLICATE_EXPANSION_BOARDS 4
#define MAX_EXPANSION_BOARDS 20
#define ROMCONFIG_CONFIGTEXT_LEN 256
struct boardromconfig;
struct romconfig
{
	TCHAR romfile[MAX_DPATH];
	TCHAR romident[256];
	uae_u32 board_ram_size;
	bool autoboot_disabled;
	bool inserted;
	bool dma24bit;
	int device_id;
	int device_settings;
	int subtype;
	void *unitdata;
	TCHAR configtext[ROMCONFIG_CONFIGTEXT_LEN];
	uae_u16 manufacturer;
	uae_u8 product;
	uae_u8 autoconfig[16];
	struct boardromconfig *back;
};
#define MAX_BOARD_ROMS 2
struct boardromconfig
{
	int device_type;
	int device_num;
	int device_order;
	struct romconfig roms[MAX_BOARD_ROMS];
};
#define MAX_RTG_BOARDS 4
struct rtgboardconfig
{
	int rtg_index;
	int rtgmem_type;
	uae_u32 rtgmem_size;
	int device_order;
	int monitor_id;
	bool autoswitch;
};
struct boardloadfile
{
	uae_u32 loadoffset;
	uae_u32 fileoffset, filesize;
	TCHAR loadfile[MAX_DPATH];
};
#define MAX_ROM_BOARDS 4
struct romboard
{
	uae_u32 size;
	uae_u32 start_address;
	uae_u32 end_address;
	struct boardloadfile lf;
};
#define MAX_RAM_BOARDS 4
struct ramboard
{
	uae_u32 size;
	uae_u16 manufacturer;
	uae_u8 product;
	uae_u8 autoconfig[16];
	bool autoconfig_inuse;
	bool manual_config;
	bool no_reset_unmap;
	int device_order;
	uae_u32 start_address;
	uae_u32 end_address;
	uae_u32 write_address;
	bool readonly;
	bool nodma;
	bool force16bit;
	bool chipramtiming;
	int fault;
	struct boardloadfile lf;
};
struct expansion_params
{
	int device_order;
};

#define Z3MAPPING_AUTO 0
#define Z3MAPPING_UAE 1
#define Z3MAPPING_REAL 2

#define GFX_SIZE_EXTRA_NUM 6
struct monconfig
{
	struct wh gfx_size_win;
	struct wh gfx_size_fs;
	struct wh gfx_size;
	struct wh gfx_size_win_xtra[GFX_SIZE_EXTRA_NUM];
	struct wh gfx_size_fs_xtra[GFX_SIZE_EXTRA_NUM];
};

#define KB_DISCONNECTED -1
#define KB_UAE 0
#define KB_A500_6570 1
#define KB_A600_6570 2
#define KB_A1000_6500 3
#define KB_A1000_6570 4
#define KB_A1200_6805 5
#define KB_A2000_8039 6
#define KB_Ax000_6570 7

#define DISPLAY_OPTIMIZATIONS_FULL 0
#define DISPLAY_OPTIMIZATIONS_PARTIAL 1
#define DISPLAY_OPTIMIZATIONS_NONE 2

#ifdef AMIBERRY
enum custom_type
{
	none,
	bool_type,
	bit_type,
	list_type
};
struct whdload_custom
{
	/**
	 * \brief The value of this custom field. This will be sent to WHDLoad
	 */
	int value = 0;
	/**
	 * \brief The type of the custom field: None, Boolean, Bit or List
	 */
	custom_type type;
	/**
	 * \brief Caption for this custom field function. Used in the label which describes what this option does
	 */
	std::string caption;
	/**
	 * \brief The list of labels to show in the dropdown. Used in List type custom fields
	 */
	std::vector<std::string> labels;
	/**
	 * \brief When the type is a Bit, it can contain multiple entries for the same custom field.
	 * Each entry has its own label and a different value that goes with it.
	 */
	std::vector<std::pair<std::string, int>> label_bit_pairs;
};
struct whdload_slave
{
	std::string filename;
	std::string data_path;
	whdload_custom custom1;
	whdload_custom custom2;
	whdload_custom custom3;
	whdload_custom custom4;
	whdload_custom custom5;

	whdload_custom& get_custom(const int index)
	{
		switch (index)
		{
		case 1:
			return custom1;
		case 2:
			return custom2;
		case 3:
			return custom3;
		case 4:
			return custom4;
		case 5:
			return custom5;
		default:
			return custom1;
		}
	}
};
struct whdload_options
{
	std::string whdload_filename;
	std::string filename;
	std::string game_name;
	std::string sub_path;
	std::string variant_uuid;
	int slave_count;
	std::string slave_default;
	bool slave_libraries;
	std::vector<whdload_slave> slaves;
	whdload_slave selected_slave;
	std::string custom;
	bool button_wait;
	bool show_splash;
	int config_delay;
	bool write_cache;
	bool quit_on_exit;
};
#endif

struct uae_prefs {

	struct strlist *all_lines;

	TCHAR description[256];
	TCHAR category[256];
	TCHAR tags[256];
	TCHAR info[256];
	int config_version;
	TCHAR config_hardware_path[MAX_DPATH];
	TCHAR config_host_path[MAX_DPATH];
	TCHAR config_all_path[MAX_DPATH];
	TCHAR config_path[MAX_DPATH];
	TCHAR config_window_title[256];
	int got_fs2_hdf2;

	bool illegal_mem;
	bool debug_mem;
	bool use_serial;
	bool serial_demand;
	bool serial_hwctsrts;
	bool serial_rtsctsdtrdtecd;
	bool serial_ri;
	bool serial_direct;
	int serial_stopbits;
	int serial_crlf;
	bool parallel_demand;
	int parallel_matrix_emulation;
	bool parallel_postscript_emulation;
	bool parallel_postscript_detection;
	int parallel_autoflush_time;
	TCHAR ghostscript_parameters[256];
	bool use_gfxlib;
	bool socket_emu;

	bool start_debugger;
	int debugging_features;
	TCHAR debugging_options[MAX_DPATH];
	bool start_gui;

	KbdLang keyboard_lang;

	int produce_sound;
	int sound_stereo;
	int sound_stereo_separation;
	int sound_mixed_stereo_delay;
	int sound_freq;
	int sound_maxbsiz;
	int sound_interpol;
	int sound_filter;
	int sound_filter_type;
	int sound_volume_master;
	int sound_volume_paula;
	int sound_volume_cd;
	int sound_volume_board;
	int sound_volume_midi;
	int sound_volume_genlock;
	bool sound_stereo_swap_paula;
	bool sound_stereo_swap_ahi;
	bool sound_auto;
	bool sound_volcnt;

	int sampler_freq;
	int sampler_buffer;
	bool sampler_stereo;

	int comptrustbyte;
	int comptrustword;
	int comptrustlong;
	int comptrustnaddr;
	bool compnf;
	bool compfpu;
	bool comp_hardflush;
	bool comp_constjump;
	bool comp_catchfault;
	int cachesize;
	bool cachesize_inhibit;
	TCHAR jitblacklist[MAX_DPATH];
	bool fpu_strict;
	int fpu_mode;

	struct monconfig gfx_monitor[MAX_AMIGADISPLAYS];
	int gfx_framerate, gfx_autoframerate;
	bool gfx_autoresolution_vga;
	int gfx_autoresolution;
	int gfx_autoresolution_delay;
	int gfx_autoresolution_minv, gfx_autoresolution_minh;
	bool gfx_scandoubler;
	struct apmode gfx_apmode[2];
	int gfx_resolution;
	int gfx_vresolution;
	int gfx_lores_mode;
	int gfx_pscanlines, gfx_iscanlines;
	int gfx_xcenter, gfx_ycenter;
	int gfx_xcenter_pos, gfx_ycenter_pos;
	int gfx_xcenter_size, gfx_ycenter_size;
	int gfx_max_horizontal, gfx_max_vertical;
	int gfx_saturation, gfx_luminance, gfx_contrast, gfx_gamma, gfx_gamma_ch[3];
	bool gfx_blackerthanblack;
	int gfx_threebitcolors;
	int gfx_api;
	int gfx_api_options;
	int gfx_extrawidth;
	int gfx_extraheight;
	bool lightboost_strobo;
	int lightboost_strobo_ratio;
	bool gfx_grayscale;
	bool lightpen_crosshair;
	int lightpen_offset[2];
	int gfx_display_sections;
	int gfx_variable_sync;
	bool gfx_windowed_resize;
	int gfx_overscanmode;
	int gfx_monitorblankdelay;
	int gfx_rotation;
	int gfx_ntscpixels;
	uae_u32 gfx_bordercolor;

	struct gfx_filterdata gf[3];

	float rtg_horiz_zoom_mult;
	float rtg_vert_zoom_mult;

	bool immediate_blits;
	int waiting_blits;
	float blitter_speed_throttle;
	unsigned int chipset_mask;
	bool chipset_hr;
	bool display_calibration;
	int keyboard_mode;
	bool keyboard_nkro;
	bool ntscmode;
	bool genlock;
	int genlock_image;
	int genlock_mix;
	int genlock_scale;
	int genlock_aspect;
	int genlock_effects;
	int genlock_offset_x, genlock_offset_y;
	uae_u64 ecs_genlock_features_colorkey_mask[4];
	uae_u8 ecs_genlock_features_plane_mask;
	bool genlock_alpha;
	TCHAR genlock_image_file[MAX_DPATH];
	TCHAR genlock_video_file[MAX_DPATH];
	TCHAR genlock_font[MAX_DPATH];
	int monitoremu;
	int monitoremu_mon;
	float chipset_refreshrate;
	struct chipset_refresh cr[MAX_CHIPSET_REFRESH + 2];
	int cr_selected;
	int collision_level;
	int leds_on_screen;
	int leds_on_screen_mask[2];
	int leds_on_screen_multiplier[2];
	int power_led_dim;
	struct wh osd_pos;
	int keyboard_leds[3];
	bool keyboard_leds_in_use;
	int scsi;
	bool sana2;
	bool uaeserial;
	int catweasel;
	int cpu_idle;
	int ppc_cpu_idle;
	bool cpu_cycle_exact;
	int cpu_clock_multiplier;
	int cpu_frequency;
	bool blitter_cycle_exact;
	bool cpu_memory_cycle_exact;
	int floppy_speed;
	int floppy_write_length;
	int floppy_random_bits_min;
	int floppy_random_bits_max;
	int floppy_auto_ext2;
	int cd_speed;
	bool tod_hack;
	uae_u32 maprom;
	int boot_rom;
	bool rom_readwrite;
	int turbo_emulation;
	int turbo_emulation_limit;
	bool turbo_boot;
	int turbo_boot_delay;
	bool headless;
	int filesys_limit;
	int filesys_max_name;
	int filesys_max_file_size;
	bool filesys_inject_icons;
	TCHAR filesys_inject_icons_tool[MAX_DPATH];
	TCHAR filesys_inject_icons_project[MAX_DPATH];
	TCHAR filesys_inject_icons_drawer[MAX_DPATH];
	int uaescsidevmode;
	bool reset_delay;
	bool crash_auto_reset;

	int cs_compatible;
	int cs_ciaatod;
	int cs_rtc;
	int cs_rtc_adjust;
	int cs_rtc_adjust_mode;
	bool cs_ksmirror_e0;
	bool cs_ksmirror_a8;
	bool cs_ciaoverlay;
	bool cs_cd32cd;
	bool cs_cd32c2p;
	bool cs_cd32nvram;
	bool cs_cd32fmv;
	int cs_cd32nvram_size;
	bool cs_cdtvcd;
	bool cs_cdtvram;
	int cs_ide;
	bool cs_pcmcia;
	bool cs_a1000ram;
	int cs_fatgaryrev;
	int cs_ramseyrev;
	int cs_agnusrev;
	int cs_deniserev;
	int cs_mbdmac;
	bool cs_cdtvcr;
	bool cs_df0idhw;
	bool cs_resetwarning;
	bool cs_ciatodbug;
	bool cs_z3autoconfig;
	bool cs_1mchipjumper;
	bool cs_cia6526;
	bool cs_bytecustomwritebug;
	bool cs_color_burst;
	bool cs_romisslow;
	bool cs_toshibagary;
	bool cs_bkpthang;
	int cs_unmapped_space;
	int cs_hacks;
	int cs_ciatype[2];
	int cs_kbhandshake;
	int cs_hvcsync;
	int cs_eclockphase;
	int cs_eclocksync;
	int cs_agnusmodel;
	int cs_agnussize;
	int cs_denisemodel;
	bool cs_memorypatternfill;
	bool cs_ipldelay;
	bool cs_floppydatapullup;
	uae_u32 seed;
	int cs_optimizations;

	struct boardromconfig expansionboard[MAX_EXPANSION_BOARDS];

	TCHAR romfile[MAX_DPATH];
	TCHAR romident[256];
	TCHAR romextfile[MAX_DPATH];
	uae_u32 romextfile2addr;
	TCHAR romextfile2[MAX_DPATH];
	TCHAR romextident[256];
	TCHAR flashfile[MAX_DPATH];
	TCHAR rtcfile[MAX_DPATH];
	TCHAR cartfile[MAX_DPATH];
	TCHAR cartident[256];
	int cart_internal;
	TCHAR pci_devices[256];
	TCHAR prtname[256];
	TCHAR sername[256];
	TCHAR a2065name[MAX_DPATH];
	TCHAR ne2000pciname[MAX_DPATH];
	TCHAR ne2000pcmcianame[MAX_DPATH];
	TCHAR picassoivromfile[MAX_DPATH];
	struct cdslot cdslots[MAX_TOTAL_SCSI_DEVICES];
	TCHAR quitstatefile[MAX_DPATH];
	TCHAR statefile[MAX_DPATH];
	TCHAR statefile_path[MAX_DPATH];
	TCHAR inprecfile[MAX_DPATH];
	TCHAR trainerfile[MAX_DPATH];
	bool inprec_autoplay;
	bool refresh_indicator;

	struct multipath path_floppy;
	struct multipath path_hardfile;
	struct multipath path_rom;
	struct multipath path_cd;

	int m68k_speed;
	float m68k_speed_throttle;
	float x86_speed_throttle;
	int cpu_model;
	int mmu_model;
	bool mmu_ec;
	int cpu060_revision;
	int fpu_model;
	int fpu_revision;
	int ppc_mode;
	TCHAR ppc_model[32];
	bool cpu_compatible;
	bool cpu_thread;
	bool int_no_unimplemented;
	bool fpu_no_unimplemented;
	bool address_space_24;
	bool cpu_data_cache;
	bool picasso96_nocustom;
	int picasso96_modeflags;
	bool picasso96_noautomodes;
	int cpu_model_fallback;

	uae_u32 z3autoconfig_start;
	struct ramboard z3fastmem[MAX_RAM_BOARDS];
	struct ramboard fastmem[MAX_RAM_BOARDS];
	struct romboard romboards[MAX_ROM_BOARDS];
	struct ramboard z3chipmem;
	struct ramboard chipmem;
	struct ramboard bogomem;
	struct ramboard mbresmem_low;
	struct ramboard mbresmem_high;
	struct ramboard mem25bit;
	uae_u32 debugmem_start;
	uae_u32 debugmem_size;
	int cpuboard_type;
	int cpuboard_subtype;
	int cpuboard_settings;
	struct ramboard cpuboardmem1;
	struct ramboard cpuboardmem2;
	int ppc_implementation;
	bool rtg_hardwareinterrupt;
	bool rtg_hardwaresprite;
	bool rtg_more_compatible;
	bool rtg_multithread;
	bool rtg_overlay;
	bool rtg_vgascreensplit;
	bool rtg_paletteswitch;
	bool rtg_dacswitch;
	struct rtgboardconfig rtgboards[MAX_RTG_BOARDS];
	uae_u32 custom_memory_addrs[MAX_CUSTOM_MEMORY_ADDRS];
	uae_u32 custom_memory_sizes[MAX_CUSTOM_MEMORY_ADDRS];
	uae_u32 custom_memory_mask[MAX_CUSTOM_MEMORY_ADDRS];
	int uaeboard;
	bool uaeboard_nodiag;
	int uaeboard_order;

	bool kickshifter;
	bool scsidevicedisable;
	bool filesys_no_uaefsdb;
	bool filesys_custom_uaefsdb;
	bool mmkeyboard;
	int uae_hide;
	bool clipboard_sharing;
	bool native_code;
	bool uae_hide_autoconfig;
	int z3_mapping_mode;
	bool autoconfig_custom_sort;
	bool obs_sound_toccata;
	bool obs_sound_toccata_mixer;
	bool obs_sound_es1370;
	bool obs_sound_fm801;
	bool cputester;

	int mountitems;
	struct uaedev_config_data mountconfig[MOUNT_CONFIG_SIZE];

	int nr_floppies;
	struct floppyslot floppyslots[4];
	bool floppy_read_only;
	bool harddrive_read_only;
	TCHAR dfxlist[MAX_SPARE_DRIVES][MAX_DPATH];
	int dfxclickvolume_disk[4];
	int dfxclickvolume_empty[4];
	int dfxclickchannelmask;

	TCHAR luafiles[MAX_LUA_STATES][MAX_DPATH];

	/* Target specific options */

	int kbd_led_num;
	int kbd_led_scr;
	int kbd_led_cap;

	bool gfx_auto_crop;
	bool gfx_manual_crop;
	int gfx_manual_crop_width;
	int gfx_manual_crop_height;
	int gfx_horizontal_offset;
	int gfx_vertical_offset;
	int gfx_correct_aspect;
	int scaling_method;

	bool gui_alwaysontop;
	bool main_alwaysontop;
	bool minimize_inactive;
	bool capture_always;
	bool start_minimized;
	bool start_uncaptured;

	int active_capture_priority;
	bool active_nocapture_pause;
	bool active_nocapture_nosound;
	int active_input;
	int inactive_priority;
	bool inactive_pause;
	bool inactive_nosound;
	int inactive_input;
	int minimized_priority;
	bool minimized_pause;
	bool minimized_nosound;
	int minimized_input;

	bool rtgallowscaling;
	int rtgscaleaspectratio;
	int rtgvblankrate;
	bool borderless;
	bool automount_removable;
	bool automount_cddrives;

	// We use the device name in Amiberry
//	int midioutdev;
//	int midiindev;
	TCHAR midioutdev[256];
	TCHAR midiindev[256];

	bool midirouter;
	int uaescsimode;
	int soundcard;
#ifdef AMIBERRY
	bool soundcard_default;
#endif
	int samplersoundcard;
	bool blankmonitors;
	bool right_control_is_right_win_key;
#ifdef WITH_SLIRP
	struct slirp_redir slirp_redirs[MAX_SLIRP_REDIRS];
#endif
	int statecapturerate, statecapturebuffersize;

	TCHAR open_gui[256];
	TCHAR quit_amiberry[256];
	TCHAR action_replay[256];
	TCHAR fullscreen_toggle[256];
	TCHAR minimize[256];
	TCHAR right_amiga[256];

	/* input */

	struct jport jports[MAX_JPORTS];
	struct jport_custom jports_custom[MAX_JPORTS_CUSTOM];
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
	bool tablet_library;
	int input_mouse_untrap;
	int input_magic_mouse_cursor;
	int input_keyboard_type;
	bool input_autoswitch;
	bool input_autoswitchleftright;
	bool input_advancedmultiinput;
	bool input_default_onscreen_keyboard;
	struct uae_input_device joystick_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device mouse_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device keyboard_settings[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
	struct uae_input_device internalevent_settings[MAX_INPUT_SETTINGS][INTERNALEVENT_COUNT];
	TCHAR input_config_name[GAMEPORT_INPUT_SETTINGS][256];
	int dongle;
	int input_contact_bounce;
	int input_device_match_mask;

#ifdef AMIBERRY
	bool vkbd_enabled;
	bool vkbd_hires;
	bool vkbd_exit;
	char vkbd_language[128];
	char vkbd_style[128];
	int vkbd_transparency;
	char vkbd_toggle[128];
	
	int drawbridge_driver;
	bool drawbridge_serial_auto;
	TCHAR drawbridge_serial_port[256];
	int drawbridge_drive_cable;
	bool drawbridge_smartspeed;
	bool drawbridge_autocache;
	bool alt_tab_release;
	int sound_pullmode;
	bool use_retroarch_quit;
	bool use_retroarch_menu;
	bool use_retroarch_reset;
	bool use_retroarch_statebuttons;
	bool use_retroarch_vkbd;

#endif
};

extern whdload_options whdload_prefs;

extern int config_changed, config_changed_flags;
extern void config_check_vsync(void);
extern void set_config_changed(int flags = 0);

/* Contains the filename of .uaerc */
//extern TCHAR optionsfile[];

extern void cfgfile_write(struct zfile*, const TCHAR* option, const TCHAR* format, ...);
extern void cfgfile_dwrite(struct zfile*, const TCHAR* option, const TCHAR* format, ...);
extern void cfgfile_target_write(struct zfile*, const TCHAR* option, const TCHAR* format, ...);
extern void cfgfile_target_dwrite(struct zfile*, const TCHAR* option, const TCHAR* format, ...);

extern void cfgfile_write_bool(struct zfile* f, const TCHAR* option, bool b);
extern void cfgfile_dwrite_bool(struct zfile* f, const  TCHAR* option, bool b);
extern void cfgfile_target_write_bool(struct zfile* f, const TCHAR* option, bool b);
extern void cfgfile_target_dwrite_bool(struct zfile* f, const TCHAR* option, bool b);

extern void cfgfile_write_str(struct zfile *f, const TCHAR *option, const TCHAR *value);
//extern void cfgfile_write_str_escape(struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_dwrite_str(struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_write_str(struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_dwrite_str(struct zfile *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_dwrite_str_escape(struct zfile *f, const TCHAR *option, const TCHAR *value);

extern void cfgfile_backup(const TCHAR* path);
extern struct uaedev_config_data* add_filesys_config(struct uae_prefs* p, int index, struct uaedev_config_info*);
extern bool get_hd_geometry(struct uaedev_config_info*);
extern void uci_set_defaults(struct uaedev_config_info* uci, bool rdb);

extern void error_log(const TCHAR*, ...);
extern TCHAR* get_error_log(void);
extern bool is_error_log(void);

extern void default_prefs(struct uae_prefs*, bool, int);
extern void discard_prefs(struct uae_prefs*, int);
extern void copy_prefs(const struct uae_prefs* src, struct uae_prefs* dst);
extern void copy_inputdevice_prefs(const struct uae_prefs *src, struct uae_prefs *dst);

#ifdef AMIBERRY
extern int bip_a500(struct uae_prefs* p, int rom);
extern int bip_a500plus(struct uae_prefs* p, int rom);
extern int bip_a600(struct uae_prefs* p, int rom);
extern int bip_a1000(struct uae_prefs* p, int rom);
extern int bip_a1200(struct uae_prefs* p, int rom);
extern int bip_a2000(struct uae_prefs* p, int rom);
extern int bip_a3000(struct uae_prefs* p, int rom);
extern int bip_a4000(struct uae_prefs* p, int rom);
extern int bip_cd32(struct uae_prefs* p, int rom);
extern int bip_cdtv(struct uae_prefs* p, int rom);
#endif

int parse_cmdline_option(struct uae_prefs*, TCHAR, const TCHAR*);

extern int cfgfile_separate_linea(const TCHAR* filename, char* line, TCHAR* line1b, TCHAR* line2b);
extern int cfgfile_yesno(const TCHAR* option, const TCHAR* value, const TCHAR* name, bool* location);
extern int cfgfile_intval(const TCHAR* option, const TCHAR* value, const TCHAR* name, int* location, int scale);
extern int cfgfile_strval(const TCHAR* option, const TCHAR* value, const TCHAR* name, int* location, const TCHAR* table[], int more);
extern int cfgfile_string(const TCHAR* option, const TCHAR* value, const TCHAR* name, TCHAR* location, int maxsz);
#ifdef AMIBERRY
extern int cfgfile_floatval(const TCHAR* option, const TCHAR* value, const TCHAR* name, float* location);
extern int cfgfile_string(const std::string& option, const std::string& value, const std::string& name, std::string& location);
#endif
extern int cfgfile_string_escape(const TCHAR* option, const TCHAR* value, const TCHAR* name, TCHAR* location, int maxsz);
extern bool cfgfile_option_find(const TCHAR* s, const TCHAR* option);
extern TCHAR* cfgfile_option_get(const TCHAR* s, const TCHAR* option);
extern TCHAR* cfgfile_subst_path(const TCHAR* path, const TCHAR* subst, const TCHAR* file);

extern TCHAR* target_expand_environment(const TCHAR* path, TCHAR* out, int maxlen);
extern int target_parse_option (struct uae_prefs *, const TCHAR *option, const TCHAR *value, int type);
extern void target_save_options(struct zfile*, struct uae_prefs*);
extern void target_default_options(struct uae_prefs*, int type);
extern void target_fixup_options(struct uae_prefs*);
extern int target_cfgfile_load(struct uae_prefs*, const TCHAR* filename, int type, int isdefault);
extern void cfgfile_save_options(struct zfile* f, struct uae_prefs* p, int type);
extern int target_get_display(const TCHAR*);
extern const TCHAR* target_get_display_name(int, bool);
extern void target_multipath_modified(struct uae_prefs*);
extern void cfgfile_resolve_path_out_load(const TCHAR* path, TCHAR* out, int size, int type);
extern void cfgfile_resolve_path_load(TCHAR* path, int size, int type);
extern void cfgfile_resolve_path_out_save(const TCHAR* path, TCHAR* out, int size, int type);
extern void cfgfile_resolve_path_save(TCHAR* path, int size, int type);

extern struct uae_prefs* cfgfile_open(const TCHAR* filename, int* type);
extern void cfgfile_close(struct uae_prefs* p);
extern int cfgfile_load(struct uae_prefs* p, const TCHAR* filename, int* type, int ignorelink, int userconfig);
extern int cfgfile_save(struct uae_prefs* p, const TCHAR* filename, int);
extern void cfgfile_parse_line(struct uae_prefs* p, TCHAR*, int);
extern void cfgfile_parse_lines(struct uae_prefs* p, const TCHAR*, int);
extern int cfgfile_parse_option(struct uae_prefs* p, const TCHAR* option, TCHAR* value, int);
extern int cfgfile_get_description(struct uae_prefs* p, const TCHAR* filename, TCHAR* description, TCHAR* category, TCHAR* tags, TCHAR* hostlink, TCHAR* hardwarelink, int* type);
extern void cfgfile_show_usage(void);
extern int cfgfile_searchconfig(const TCHAR* in, int index, TCHAR* out, int outsize);
extern uae_u32 cfgfile_uaelib(TrapContext* ctx, int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen);
extern uae_u32 cfgfile_uaelib_modify(TrapContext* ctx, uae_u32 mode, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize);
extern uae_u32 cfgfile_modify(uae_u32 index, const TCHAR* parms, uae_u32 size, TCHAR* out, uae_u32 outsize);
extern void cfgfile_addcfgparam(TCHAR*);
extern int built_in_prefs(struct uae_prefs* p, int model, int config, int compa, int romcheck);
extern int built_in_chipset_prefs(struct uae_prefs* p);
//extern int cmdlineparser (const TCHAR *s, TCHAR *outp[], int max);
extern void fixup_prefs_dimensions(struct uae_prefs* prefs);
extern void fixup_prefs(struct uae_prefs* prefs, bool userconfig);
extern void fixup_cpu(struct uae_prefs* prefs);
extern void cfgfile_compatibility_romtype(struct uae_prefs* p);
extern void cfgfile_compatibility_rtg(struct uae_prefs* p);
extern bool cfgfile_detect_art(struct uae_prefs* p, TCHAR* path);
extern const TCHAR *cfgfile_getconfigdata(size_t *len);
extern bool cfgfile_createconfigstore(struct uae_prefs* p);
extern void cfgfile_get_shader_config(struct uae_prefs* p, int rtg);

#ifdef AMIBERRY
extern void whdload_auto_prefs(struct uae_prefs* prefs, const char* filepath);
extern void cd_auto_prefs(struct uae_prefs* prefs, char* filepath);
extern void symlink_roms(struct uae_prefs* prefs);
extern void drawbridge_update_profiles(struct uae_prefs* prefs);
#endif

extern void check_prefs_changed_custom(void);
extern void check_prefs_changed_cpu(void);
extern void check_prefs_changed_audio(void);
extern void check_prefs_changed_cd(void);
extern int check_prefs_changed_gfx(void);

extern struct uae_prefs currprefs, changed_prefs;

extern int machdep_init(void);
extern void machdep_free(void);

struct cddlg_vals
{
	struct uaedev_config_info ci;
};
struct tapedlg_vals
{
	struct uaedev_config_info ci;
};
struct fsvdlg_vals
{
	struct uaedev_config_info ci;
	int rdb;
};

struct hfdlg_vals
{
	struct uaedev_config_info ci;
	bool original;
	uae_u64 size;
	uae_u32 dostype;
	int forcedcylinders;
	bool rdb;
};
extern struct cddlg_vals current_cddlg;
extern struct tapedlg_vals current_tapedlg;
extern struct fsvdlg_vals current_fsvdlg;
extern struct hfdlg_vals current_hfdlg;

extern void hardfile_testrdb(struct hfdlg_vals* hdf);
extern void default_tapedlg(struct tapedlg_vals* f);
extern void default_fsvdlg(struct fsvdlg_vals* f);
extern void default_hfdlg(struct hfdlg_vals* f, bool rdb);
extern void updatehdfinfo(bool force, bool defaults, bool realdrive, std::string& txtHdfInfo, std::string& txtHdfInfo2);

#ifdef AMIBERRY
struct amiberry_customised_layout
{
	// create structures for each 'function' button
	std::array<int, 15> none;
	std::array<int, 15> select;
};

struct hotkey_modifiers
{
	bool lctrl;
	bool rctrl;
	bool lalt;
	bool ralt;
	bool lshift;
	bool rshift;
	bool lgui;
	bool rgui;
};
struct amiberry_hotkey
{
	int scancode;
	Uint8 button;
	std::string key_name;
	std::string modifiers_string;
	hotkey_modifiers modifiers;
};

struct amiberry_gui_theme
{
	gcn::Color base_color;
	gcn::Color selector_inactive;
	gcn::Color selector_active;
	gcn::Color background_color;
	gcn::Color selection_color;
	gcn::Color foreground_color;
	std::string font_name;
	int font_size;
	gcn::Color font_color;
};

struct amiberry_options
{
	bool quickstart_start = true;
	bool read_config_descriptions = true;
	bool write_logfile = false;
	bool rctrl_as_ramiga = false;
	bool gui_joystick_control = true;
	bool default_multithreaded_drawing = true;
	int default_line_mode = 1;
	int input_default_mouse_speed = 100;
	bool input_keyboard_as_joystick_stop_keypresses = false;
	char default_open_gui_key[128] = "F12";
	char default_quit_key[128]{};
	char default_ar_key[128] = "Pause";
	char default_fullscreen_toggle_key[128]{};
	int rotation_angle = 0;
	bool default_horizontal_centering = false;
	bool default_vertical_centering = false;
	int default_scaling_method = -1;
	int default_gfx_autoresolution = 0;
	bool default_frameskip = false;
	bool default_correct_aspect_ratio = true;
	bool default_auto_crop = false;
	int default_width = 720;
	int default_height = 568;
	int default_fullscreen_mode = 0;
	int default_stereo_separation = 7;
	int default_sound_buffer = 8192;
	bool default_sound_pull = true;
	int default_sound_frequency = 44100;
	int default_joystick_deadzone = 33;
	bool default_retroarch_quit = true;
	bool default_retroarch_menu = true;
	bool default_retroarch_reset = false;
	bool default_retroarch_vkbd = false;
	char default_controller1[128] = "joy1";
	char default_controller2[128] = "joy2";
	char default_controller3[128]{};
	char default_controller4[128]{};
	char default_mouse1[128] = "mouse";
	char default_mouse2[128] = "joy0";
	bool default_whd_buttonwait = false;
	bool default_whd_showsplash = true;
	int default_whd_configdelay = 0;
	bool default_whd_writecache = false;
	bool default_whd_quit_on_exit = false;
	bool use_jst_instead_of_whd = false;
	bool disable_shutdown_button = true;
	bool allow_display_settings_from_xml = true;
	int default_soundcard = 0;
	bool default_vkbd_enabled;
	bool default_vkbd_hires;
	bool default_vkbd_exit;
	char default_vkbd_language[128] = "US";
	char default_vkbd_style[128] = "Original";
	int default_vkbd_transparency;
	char default_vkbd_toggle[128] = "guide";
	char gui_theme[128] = "Default.theme";
};

extern struct amiberry_options amiberry_options;
#endif

extern void import_joysticks(void);

#endif /* UAE_OPTIONS_H */
