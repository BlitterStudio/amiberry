extern int decode_cloanto_rom_do (uae_u8 *mem, int size, int real_size);

#define ROMTYPE_KICK	    0x000001
#define ROMTYPE_KICKCD32    0x000002
#define ROMTYPE_EXTCD32	    0x000004
#define ROMTYPE_EXTCDTV	    0x000008
#define ROMTYPE_A2091BOOT   0x000010
#define ROMTYPE_A4091BOOT   0x000020
#define ROMTYPE_AR	    0x000040
#define ROMTYPE_SUPERIV	    0x000080
#define ROMTYPE_KEY	    0x000100
#define ROMTYPE_ARCADIABIOS 0x000200
#define ROMTYPE_ARCADIAGAME 0x000400
#define ROMTYPE_HRTMON	    0x000800
#define ROMTYPE_NORDIC	    0x001000
#define ROMTYPE_XPOWER	    0x002000
#define ROMTYPE_CD32CART    0x004000
#define ROMTYPE_SPECIALKICK 0x008000
#define ROMTYPE_MASK	    0x01ffff
#define ROMTYPE_EVEN	    0x020000
#define ROMTYPE_ODD	    0x040000
#define ROMTYPE_8BIT	    0x080000
#define ROMTYPE_BYTESWAP    0x100000
#define ROMTYPE_CD32	    0x200000
#define ROMTYPE_SCRAMBLED   0x400000
#define ROMTYPE_NONE		0x800000

#define ROMTYPE_ALL_KICK (ROMTYPE_KICK | ROMTYPE_KICKCD32 | ROMTYPE_CD32)
#define ROMTYPE_ALL_EXT (ROMTYPE_EXTCD32 | ROMTYPE_EXTCDTV)
#define ROMTYPE_ALL_CART (ROMTYPE_AR | ROMTYPE_HRTMON | ROMTYPE_NORDIC | ROMTYPE_XPOWER | ROMTYPE_CD32CART)

struct romheader {
  TCHAR *name;
  int id;
};

struct romdata {
  TCHAR *name;
  int ver, rev;
  int subver, subrev;
  TCHAR *model;
  uae_u32 size;
  int id;
  int cpu;
  int cloanto;
  int type;
  int group;
  int title;
  TCHAR *partnumber;
  uae_u32 crc32;
  uae_u32 sha1[5];
  TCHAR *configname;
};

struct romlist {
  TCHAR *path;
  struct romdata *rd;
};

extern struct romdata *getromdatabypath (const TCHAR *path);
extern struct romdata *getromdatabycrc (uae_u32 crc32);
extern struct romdata *getromdatabycrc (uae_u32 crc32, bool);
extern struct romdata *getromdatabydata (uae_u8 *rom, int size);
extern struct romdata *getromdatabyid (int id);
extern struct romdata *getromdatabyidgroup (int id, int group, int subitem);
extern struct romdata *getromdatabyzfile (struct zfile *f);
extern struct romlist **getarcadiaroms (void);
extern struct romdata *getarcadiarombyname (const TCHAR *name);
extern struct romlist **getromlistbyident (int ver, int rev, int subver, int subrev, const TCHAR *model, int romflags, bool all);
extern void getromname (const struct romdata*, TCHAR*);
extern struct romdata *getromdatabyname (const TCHAR*);
extern struct romlist *getromlistbyids(const int *ids);
extern void romwarning(const int *ids);
extern struct romlist *getromlistbyromdata(const struct romdata *rd);
extern void romlist_add (const TCHAR *path, struct romdata *rd);
extern TCHAR *romlist_get (const struct romdata *rd);
extern void romlist_clear (void);
extern struct zfile *read_rom (struct romdata *rd);
extern struct zfile *read_rom_name (const TCHAR *filename);

extern int load_keyring (struct uae_prefs *p, const TCHAR *path);
extern uae_u8 *target_load_keyfile (struct uae_prefs *p, const TCHAR *path, int *size, TCHAR *name);
extern void free_keyring (void);
extern int get_keyring (void);
extern void kickstart_fix_checksum (uae_u8 *mem, int size);
extern int kickstart_checksum (uae_u8 *mem, int size);
extern int decode_rom (uae_u8 *mem, int size, int mode, int real_size);
extern struct zfile *rom_fopen (const TCHAR *name, const TCHAR *mode, int mask);
extern struct zfile *read_rom_name_guess (const TCHAR *filename);
extern void addkeydir (const TCHAR *path);
extern void addkeyfile (const TCHAR *path);
extern int romlist_count (void);
extern struct romlist *romlist_getit (void);
extern int configure_rom (struct uae_prefs *p, const int *rom, int msg);

