 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Autoconfig device support
  *
  * (c) 1996 Ed Hanway
  */

#ifndef UAE_AUTOCONF_H
#define UAE_AUTOCONF_H

#include "uae/types.h"

#define AFTERDOS_INIT_PRI ((-121) & 0xff)
#define AFTERDOS_PRI ((-122) & 0xff)

#define RTAREA_DEFAULT 0xf00000
#define RTAREA_BACKUP  0xef0000
#define RTAREA_BACKUP_2 0xdb0000
#define RTAREA_SIZE 0x10000

#define RTAREA_TRAPS 0x4000
#define RTAREA_RTG 0x4800
#define RTAREA_TRAMPOLINE 0x4a00
#define RTAREA_DATAREGION 0xF000

#define RTAREA_FSBOARD 0xFFEC
#define RTAREA_HEARTBEAT 0xFFF0
#define RTAREA_TRAPTASK 0xFFF4
#define RTAREA_EXTERTASK 0xFFF8
#define RTAREA_INTREQ 0xFFFC

#define RTAREA_TRAP_DATA 0x8000
#define RTAREA_TRAP_DATA_SIZE 0x4000
#define RTAREA_TRAP_DATA_SLOT_SIZE 0x1000 // 4096
#define RTAREA_TRAP_DATA_SECOND 80
#define RTAREA_TRAP_DATA_TASKWAIT (RTAREA_TRAP_DATA_SECOND - 4)
#define RTAREA_TRAP_DATA_EXTRA 144
#define RTAREA_TRAP_DATA_EXTRA_SIZE (RTAREA_TRAP_DATA_SLOT_SIZE - RTAREA_TRAP_DATA_EXTRA)

#define RTAREA_TRAP_STATUS 0xF000
#define RTAREA_TRAP_STATUS_SIZE 8
#define RTAREA_TRAP_STATUS_SECOND 4

#define RTAREA_VARIABLES 0x7F00
#define RTAREA_VARIABLES_SIZE 0x0100
#define RTAREA_SYSBASE 0x7FFC
#define RTAREA_GFXBASE 0x7FF8
#define RTAREA_INTBASE 0x7FF4
#define RTAREA_INTXY 0x7FF0

#define RTAREA_TRAP_DATA_NUM (RTAREA_TRAP_DATA_SIZE / RTAREA_TRAP_DATA_SLOT_SIZE)
#define RTAREA_TRAP_DATA_SEND_NUM 1

#define RTAREA_TRAP_SEND_STATUS (RTAREA_TRAP_STATUS + RTAREA_TRAP_STATUS_SIZE * RTAREA_TRAP_DATA_NUM)
#define RTAREA_TRAP_SEND_DATA (RTAREA_TRAP_DATA + RTAREA_TRAP_DATA_SLOT_SIZE * RTAREA_TRAP_DATA_NUM)

#define UAEBOARD_DATAREGION_START 0x4000
#define UAEBOARD_DATAREGION_SIZE 0xc000

extern uae_u32 addr (int);
extern void db (uae_u8);
extern void dw (uae_u16);
extern void dl (uae_u32);
extern void df(uae_u8 b, int len);
extern uae_u32 dsf(uae_u8, int);
extern uae_u32 ds_ansi(const uae_char*);
extern uae_u32 ds (const TCHAR*);
extern uae_u32 ds_bstr_ansi (const uae_char*);
extern uae_u8 dbg (uaecptr);
extern void calltrap (uae_u32);
extern void org (uae_u32);
extern uae_u32 here (void);
extern uaecptr makedatatable (uaecptr resid, uaecptr resname, uae_u8 type, uae_s8 priority, uae_u16 ver, uae_u16 rev);
extern uae_u32 boot_rom_copy(TrapContext*, uaecptr, int);
extern void add_rom_absolute(uaecptr addr);
extern void save_rom_absolute(uaecptr addr);

extern void align (int);

extern volatile uae_atomic uae_int_requested;
extern void rtarea_reset(void);

#define RTS 0x4e75
#define RTE 0x4e73

extern uaecptr EXPANSION_explibname, EXPANSION_doslibname, EXPANSION_uaeversion;
extern uaecptr EXPANSION_explibbase, EXPANSION_uaedevname, EXPANSION_haveV36;
extern uaecptr EXPANSION_bootcode, EXPANSION_nullfunc;

extern uaecptr ROM_filesys_resname, ROM_filesys_resid;
extern uaecptr ROM_filesys_diagentry;
extern uaecptr ROM_hardfile_resname, ROM_hardfile_resid;
extern uaecptr ROM_hardfile_init;
extern uaecptr filesys_initcode, filesys_initcode_ptr, filesys_initcode_real;

extern int is_hardfile(int unit_no);
extern int nr_units(void);
extern int nr_directory_units(struct uae_prefs*);
extern uaecptr need_uae_boot_rom(struct uae_prefs*);

struct mountedinfo
{
    uae_s64 size;
    bool ismounted;
    bool ismedia;
	int error;
    int nrcyls;
	TCHAR rootdir[MAX_DPATH];
};

extern int get_filesys_unitconfig (struct uae_prefs *p, int index, struct mountedinfo*);
extern int kill_filesys_unitconfig (struct uae_prefs *p, int nr);
extern int move_filesys_unitconfig (struct uae_prefs *p, int nr, int to);
extern TCHAR *validatedevicename (TCHAR *s, const TCHAR *def);
extern TCHAR *validatevolumename (TCHAR *s, const TCHAR *def);

int filesys_insert(int nr, const TCHAR *volume, const TCHAR *rootdir, bool readonly, int flags);
int filesys_eject(int nr);
int filesys_media_change(const TCHAR *rootdir, int inserted, struct uaedev_config_data *uci);
int filesys_media_change_queue(const TCHAR *rootdir, int total);

extern TCHAR *filesys_createvolname (const TCHAR *volname, const TCHAR *rootdir, struct zvolume *zv, const TCHAR *def);
extern int target_get_volume_name (struct uaedev_mount_info *mtinf, struct uaedev_config_info *ci, bool inserted, bool fullcheck, int cnt);

extern int sprintf_filesys_unit (TCHAR *buffer, int num);

extern void filesys_reset (void);
extern void filesys_cleanup (void);
extern void filesys_prepare_reset (void);
extern void filesys_start_threads (void);
extern void filesys_flush_cache (void);
extern void filesys_free_handles (void);
extern void filesys_vsync (void);
extern bool filesys_heartbeat(void);

extern void filesys_install (void);
extern void filesys_install_code (void);
extern void create_ks12_boot(void);
extern void create_68060_nofpu(void);
extern uaecptr filesys_get_entry(int);
extern void filesys_store_devinfo (uae_u8 *);
extern void hardfile_install (void);
extern void hardfile_reset (void);
extern void emulib_install (void);
extern uae_u32 uaeboard_demux (uae_u32*);
extern void expansion_init (void);
extern void expansion_cleanup (void);
extern void expansion_clear (void);
extern uaecptr expansion_startaddress(struct uae_prefs*, uaecptr addr, uae_u32 size);
extern bool expansion_is_next_board_fastram(void);
extern uaecptr uaeboard_alloc_ram(uae_u32);
extern uae_u8 *uaeboard_map_ram(uaecptr);
extern void expansion_scan_autoconfig(struct uae_prefs*, bool);
extern void expansion_generate_autoconfig_info(struct uae_prefs *p);
extern struct autoconfig_info *expansion_get_autoconfig_info(struct uae_prefs*, int romtype, int devnum);
extern struct autoconfig_info *expansion_get_autoconfig_data(struct uae_prefs *p, int index);
extern struct autoconfig_info *expansion_get_autoconfig_by_address(struct uae_prefs *p, uaecptr addr, int index);
extern struct autoconfig_info *expansion_get_bank_data(struct uae_prefs *p, uaecptr *addr);
extern void expansion_set_autoconfig_sort(struct uae_prefs *p);
extern int expansion_autoconfig_move(struct uae_prefs *p, int index, int direction, bool test);
extern bool expansion_can_move(struct uae_prefs *p, int index);
extern bool alloc_expansion_bank(addrbank *bank, struct autoconfig_info *aci);
extern void free_expansion_bank(addrbank *bank);
extern void expansion_map(void);
extern uae_u32 expansion_board_size(addrbank *ab);

extern void uaegfx_install_code (uaecptr);

extern uae_u32 emulib_target_getcpurate (uae_u32, uae_u32*);

typedef bool (*DEVICE_INIT)(struct autoconfig_info*);
typedef void(*DEVICE_ADD)(int, struct uaedev_config_info*, struct romconfig*);
typedef bool(*E8ACCESS)(int, uae_u32*, int, bool);
typedef void(*DEVICE_MEMORY_CALLBACK)(struct romconfig*, uae_u8*, int);
#define EXPANSIONTYPE_SCSI 1
#define EXPANSIONTYPE_IDE 2
#define EXPANSIONTYPE_24BIT 4
#define EXPANSIONTYPE_SASI 16
#define EXPANSIONTYPE_CUSTOM 32
#define EXPANSIONTYPE_PCI_BRIDGE 64
#define EXPANSIONTYPE_PARALLEL_ADAPTER 128
#define EXPANSIONTYPE_X86_BRIDGE 0x100
#define EXPANSIONTYPE_CUSTOM_SECONDARY 0x200
#define EXPANSIONTYPE_RTG 0x400
#define EXPANSIONTYPE_SOUND 0x800
#define EXPANSIONTYPE_FLOPPY 0x1000
#define EXPANSIONTYPE_NET 0x2000
#define EXPANSIONTYPE_INTERNAL 0x4000
#define EXPANSIONTYPE_FALLBACK_DISABLE 0x8000
#define EXPANSIONTYPE_HAS_FALLBACK 0x10000
#define EXPANSIONTYPE_X86_EXPANSION 0x20000
#define EXPANSIONTYPE_PCMCIA 0x40000
#define EXPANSIONTYPE_CUSTOMDISK 0x80000
#define EXPANSIONTYPE_CLOCKPORT 0x100000
#define EXPANSIONTYPE_DMA24 0x200000

#define EXPANSIONBOARD_CHECKBOX 0
#define EXPANSIONBOARD_MULTI 1
#define EXPANSIONBOARD_STRING 2

struct expansionboardsettings
{
	const TCHAR *name;
	const TCHAR *configname;
	int type;
	bool invert;
	int bitshift;
};
struct expansionsubromtype
{
	const TCHAR *name;
	const TCHAR *configname;
	uae_u32 romtype;
	int memory_mid, memory_pid;
	uae_u32 memory_serial;
	bool memory_after;
	int deviceflags;
	uae_u8 autoconfig[16];
};
struct expansionromtype
{
	const TCHAR *name;
	const TCHAR *friendlyname;
	const TCHAR *friendlymanufacturer;
	DEVICE_INIT preinit, init, init2;
	DEVICE_ADD add;
	uae_u32 romtype;
	uae_u32 romtype_extra;
	uae_u32 parentromtype;
	int zorro;
	bool singleonly;
	const struct expansionsubromtype *subtypes;
	int defaultsubtype;
	bool autoboot_jumper;
	int deviceflags;
	int memory_mid, memory_pid;
	uae_u32 memory_serial;
	bool memory_after;
	DEVICE_MEMORY_CALLBACK memory_callback;
	bool id_jumper;
	int extrahdports;
	const struct expansionboardsettings *settings;
	uae_u8 autoconfig[16];
};
extern const struct expansionromtype expansionroms[];
struct cpuboardsubtype
{
	const TCHAR *name;
	const TCHAR *configname;
	uae_u32 romtype;
	int romtype_extra;
	int cputype;
	DEVICE_ADD add;
	int deviceflags;
	int memorytype;
	int maxmemory;
	int z3extra;
	DEVICE_INIT init, init2;
	int initzorro;
	int initflag;
	const struct expansionboardsettings *settings;
	E8ACCESS e8;
	// if includes Z2 or Z3 RAM
	int memory_mid, memory_pid;
	uae_u32 memory_serial;
	bool memory_after;
	uae_u8 autoconfig[16];
};
struct cpuboardtype
{
	int id;
	const TCHAR *name;
	const struct cpuboardsubtype *subtypes;
	int defaultsubtype;
};
extern const struct cpuboardtype cpuboards[];
struct memoryboardtype
{
	const TCHAR *man;
	const TCHAR *name;
	uae_u8 z;
	uae_u32 address;
	uae_u16 manufacturer;
	uae_u8 product;
	uae_u8 autoconfig[16];
};
extern const struct memoryboardtype memoryboards[];

// More information in first revision HRM Appendix_G
#define BOARD_PROTOAUTOCONFIG 1
#define BOARD_AUTOCONFIG_Z2 2
#define BOARD_AUTOCONFIG_Z3 3
#define BOARD_NONAUTOCONFIG_BEFORE 4
#define BOARD_NONAUTOCONFIG_AFTER_Z2 5
#define BOARD_NONAUTOCONFIG_AFTER_Z3 6
#define BOARD_PCI 7
#define BOARD_IGNORE 8

#endif /* UAE_AUTOCONF_H */
