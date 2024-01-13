#ifndef UAE_ROMMGR_H
#define UAE_ROMMGR_H

#define MAX_ROMMGR_ROMS 320

extern int decode_cloanto_rom_do(uae_u8 *mem, int size, int real_size);

#define ROMTYPE_SUB_MASK    0x000000ff
#define ROMTYPE_GROUP_MASK  0x003fff00
#define ROMTYPE_MASK		0x003fffff

#define ROMTYPE_KICK		0x00000100
#define ROMTYPE_KICKCD32	0x00000200
#define ROMTYPE_EXTCD32		0x00000400
#define ROMTYPE_EXTCDTV		0x00000800
#define ROMTYPE_KEY			0x00001000
#define ROMTYPE_ARCADIABIOS	0x00002000
#define ROMTYPE_ARCADIAGAME	0x00004000
#define ROMTYPE_CD32CART	0x00008000
#define ROMTYPE_SPECIALKICK	0x00010000
#define ROMTYPE_ALG			0x00020000

#define ROMTYPE_CPUBOARD	0x00040000
#define ROMTYPE_CB_A3001S1	0x00040001
#define ROMTYPE_CB_APOLLO	0x00040002
#define ROMTYPE_CB_FUSION	0x00040003
#define ROMTYPE_CB_DKB		0x00040004
#define ROMTYPE_CB_WENGINE	0x00040005
#define ROMTYPE_CB_TEKMAGIC	0x00040006
#define ROMTYPE_CB_BLIZ1230	0x00040007
#define ROMTYPE_CB_BLIZ1260	0x00040008
#define ROMTYPE_CB_BLIZ2060	0x00040009
#define ROMTYPE_CB_A26x0	0x0004000a
#define ROMTYPE_CB_CSMK1	0x0004000b
#define ROMTYPE_CB_CSMK2	0x0004000c
#define ROMTYPE_CB_CSMK3	0x0004000d
#define ROMTYPE_CB_CSPPC	0x0004000e
#define ROMTYPE_CB_BLIZPPC	0x0004000f
#define ROMTYPE_CB_GOLEM030	0x00040010
#define ROMTYPE_CB_ACA500	0x00040011
#define ROMTYPE_CB_DBK_WF	0x00040012
#define ROMTYPE_CB_EMATRIX	0x00040013
#define ROMTYPE_CB_SX32PRO	0x00040014
#define ROMTYPE_CB_B1230MK2	0x00040015
#define ROMTYPE_CB_B1230MK3	0x00040016
#define ROMTYPE_CB_VECTOR	0x00040017
#define ROMTYPE_CB_ZEUS040	0x00040018
#define ROMTYPE_CB_MAGNUM40	0x00040019
#define ROMTYPE_CB_TQM		0x0004001a
#define ROMTYPE_CB_FALCON40	0x0004001b
#define ROMTYPE_CB_A1230S2	0x0004001c
#define ROMTYPE_CB_TYPHOON2	0x0004001d
#define ROMTYPE_CB_QUIKPAK	0x0004001e
#define ROMTYPE_CB_12GAUGE	0x0004001f
#define ROMTYPE_CB_HARMS3KP	0x00040020
#define ROMTYPE_CB_A1230S1	0x00040021
#define ROMTYPE_CB_DRACO	0x00040022
#define ROMTYPE_CB_CASAB	0x00040023

#define ROMTYPE_FREEZER		0x00080000
#define ROMTYPE_AR			0x00080001
#define ROMTYPE_AR2			0x00080002
#define ROMTYPE_HRTMON		0x00080003
#define ROMTYPE_NORDIC		0x00080004
#define ROMTYPE_XPOWER		0x00080005
#define ROMTYPE_SUPERIV		0x00080006

#define ROMTYPE_SCSI		0x00100000
#define ROMTYPE_A2091		0x00100001
#define ROMTYPE_A4091		0x00100002
#define ROMTYPE_BLIZKIT4	0x00100003
#define ROMTYPE_FASTLANE	0x00100004
#define ROMTYPE_OKTAGON		0x00100005
#define ROMTYPE_GVPS1		0x00100006
#define ROMTYPE_GVPS12		0x00100007
#define ROMTYPE_GVPS2		0x00100008
#define ROMTYPE_AMAX		0x00100009
#define ROMTYPE_ALFA		0x0010000a
#define ROMTYPE_ALFAPLUS	0x0010000b
#define ROMTYPE_APOLLO		0x0010000c
#define ROMTYPE_MASOBOSHI	0x0010000d
#define ROMTYPE_SUPRA		0x0010000e
#define ROMTYPE_A2090		0x0010000f
#define ROMTYPE_GOLEM		0x00100010
#define ROMTYPE_STARDRIVE	0x00100011
#define ROMTYPE_KOMMOS		0x00100012
#define ROMTYPE_VECTOR		0x00100013
#define ROMTYPE_ADIDE		0x00100014
#define ROMTYPE_MTEC		0x00100015
#define ROMTYPE_PROTAR		0x00100016
#define ROMTYPE_ADD500		0x00100017
#define ROMTYPE_KRONOS		0x00100018
#define ROMTYPE_ADSCSI		0x00100019
#define ROMTYPE_ROCHARD		0x0010001a
#define ROMTYPE_CLTDSCSI	0x0010001b
#define ROMTYPE_PTNEXUS		0x0010001c
#define ROMTYPE_DATAFLYERP	0x0010001d
#define ROMTYPE_SUPRADMA	0x0010001e
#define ROMTYPE_GREX		0x0010001f
#define ROMTYPE_PROMETHEUS	0x00100020
#define ROMTYPE_MEDIATOR	0x00100021
#define ROMTYPE_TECMAR		0x00100022
#define ROMTYPE_XEBEC		0x00100023
#define ROMTYPE_MICROFORGE	0x00100024
#define ROMTYPE_PARADOX		0x00100025
#define ROMTYPE_HDA506		0x00100026
#define ROMTYPE_ALF1		0x00100027
#define ROMTYPE_PROMIGOS	0x00100028
#define ROMTYPE_SYSTEM2000	0x00100029
#define ROMTYPE_A1060		0x0010002a
#define ROMTYPE_A2088		0x0010002b
#define ROMTYPE_A2088T		0x0010002c
#define ROMTYPE_A2286		0x0010002d
#define ROMTYPE_A2386		0x0010002e
#define ROMTYPE_OMTIADAPTER	0x0010002f
#define ROMTYPE_X86_RT1000	0x00100030
#define ROMTYPE_X86_AT_HD1	0x00100031
#define ROMTYPE_X86_AT_HD2	0x00100032
#define ROMTYPE_X86_XT_IDE	0x00100033
#define ROMTYPE_PICASSOIV	0x00100034
#define ROMTYPE_x86_VGA		0x00100035
#define ROMTYPE_APOLLOHD	0x00100036
#define ROMTYPE_MEVOLUTION	0x00100037
#define ROMTYPE_GOLEMFAST	0x00100038
#define ROMTYPE_PHOENIXB	0x00100039
#define ROMTYPE_IVSTPRO		0x0010003A
#define ROMTYPE_TOCCATA		0x0010003B
#define ROMTYPE_ES1370		0x0010003C
#define ROMTYPE_FM801		0x0010003D
#define ROMTYPE_UAESNDZ2	0x0010003E
#define ROMTYPE_UAESNDZ3	0x0010003F
#define ROMTYPE_RAMZ2		0x00100040
#define ROMTYPE_RAMZ3		0x00100041
#define ROMTYPE_CATWEASEL	0x00100042
#define ROMTYPE_CDTVSCSI	0x00100043
#define ROMTYPE_MB_IDE		0x00100044
#define ROMTYPE_SCSI_A3000	0x00100045
#define ROMTYPE_SCSI_A4000T	0x00100046
#define ROMTYPE_MB_PCMCIA	0x00100047
#define ROMTYPE_MEGACHIP	0x00100048
#define ROMTYPE_A2065		0x00100049
#define ROMTYPE_NE2KPCI		0x0010004a
#define ROMTYPE_NE2KPCMCIA	0x0010004b
#define ROMTYPE_CDTVDMAC	0x0010004c
#define ROMTYPE_CDTVCR		0x0010004d
#define ROMTYPE_IVSVECTOR	0x0010004e
#define ROMTYPE_BUDDHA		0x0010004f
#define ROMTYPE_NE2KISA		0x00100050
#define ROMTYPE_BLIZKIT3	0x00100051
#define ROMTYPE_SCRAM5380	0x00100052
#define ROMTYPE_SCRAM5394	0x00100053
#define ROMTYPE_OSSI		0x00100054
#define ROMTYPE_HARLEQUIN	0x00100055
#define ROMTYPE_DATAFLYER	0x00100056
#define ROMTYPE_ARIADNE2	0x00100057
#define ROMTYPE_XSURF		0x00100058
#define ROMTYPE_XSURF100Z2	0x00100059
#define ROMTYPE_XSURF100Z3	0x0010005a
#define ROMTYPE_HYDRA		0x0010005b
#define ROMTYPE_LANROVER	0x0010005c
#define ROMTYPE_ARIADNE		0x0010005d
#define ROMTYPE_HARDFRAME	0x0010005e
#define ROMTYPE_ATEAM		0x0010005f
#define ROMTYPE_PMX			0x00100060
#define ROMTYPE_COMSPEC		0x00100061
#define ROMTYPE_MALIBU		0x00100062
#define ROMTYPE_RAPIDFIRE	0x00100063
#define ROMTYPE_UAEBOARDZ2	0x00100064
#define ROMTYPE_UAEBOARDZ3	0x00100065
#define ROMTYPE_ADDHARD		0x00100066
#define ROMTYPE_A2090B		0x00100067
#define ROMTYPE_FASTATA4K	0x00100068
#define ROMTYPE_INMATE		0x00100069
#define ROMTYPE_EMPLANT		0x0010006a
#define ROMTYPE_CUBO		0x0010006b
#define ROMTYPE_GOLEMHD3000	0x0010006c
#define ROMTYPE_WEDGE		0x0010006d
#define ROMTYPE_ARRIBA		0x0010006e
#define ROMTYPE_EVESHAMREF	0x0010006f
#define ROMTYPE_PROFEX		0x00100070
#define ROMTYPE_ELSATHD		0x00100071
#define ROMTYPE_ALF3		0x00100072
#define ROMTYPE_FASTTRAK	0x00100073
#define ROMTYPE_CDTVSRAM	0x00100074
#define ROMTYPE_SBISA		0x00100075
#define ROMTYPE_X86MOUSE	0x00100076
#define ROMTYPE_ACCESSX		0x00100077
#define ROMTYPE_OVERDRIVE	0x00100078
#define ROMTYPE_IVSTC		0x00100079
#define ROMTYPE_IVST500AT	0x0010007a
#define ROMTYPE_TRIFECTA	0x0010007b
#define ROMTYPE_PRELUDE		0x0010007c
#define ROMTYPE_PRELUDE1200	0x0010007d
#define ROMTYPE_TANDEM		0x0010007e
#define ROMTYPE_ARCHOSHD	0x0010007f
#define ROMTYPE_PCMCIASRAM	0x00100080
#define ROMTYPE_PCMCIAIDE	0x00100081
#define ROMTYPE_SSQUIRREL	0x00100082
#define ROMTYPE_MASTERCARD	0x00100083
#define ROMTYPE_DOTTO		0x00100084
#define ROMTYPE_ALF2		0x00100085
#define ROMTYPE_SYNTHESIS	0x00100086
#define ROMTYPE_MASTFB		0x00100087
#define ROMTYPE_VOODOO3		0x00100088
#define ROMTYPE_S3VIRGE		0x00100089
#define ROMTYPE_VOODOO5		0x0010008a
#define ROMTYPE_HD20A		0x0010008b
#define ROMTYPE_DEVHD		0x0010008c
#define ROMTYPE_CSMK1SCSI	0x0010008d
#define ROMTYPE_GVPA1208	0x0010008e
#define ROMTYPE_DSP3210		0x0010008f
#define ROMTYPE_ALTAIS		0x00100090

#define ROMTYPE_NOT			0x00800000
#define ROMTYPE_QUAD		0x01000000
#define ROMTYPE_EVEN		0x02000000
#define ROMTYPE_ODD			0x04000000
#define ROMTYPE_8BIT		0x08000000
#define ROMTYPE_BYTESWAP	0x10000000
#define ROMTYPE_CD32		0x20000000
#define ROMTYPE_SCRAMBLED	0x40000000
#define ROMTYPE_NONE		0x80000000

#define ROMTYPE_ALL_KICK (ROMTYPE_KICK | ROMTYPE_KICKCD32 | ROMTYPE_CD32)
#define ROMTYPE_ALL_EXT (ROMTYPE_EXTCD32 | ROMTYPE_EXTCDTV | ROMTYPE_ARCADIABIOS)
#define ROMTYPE_ALL_CART (ROMTYPE_AR | ROMTYPE_HRTMON | ROMTYPE_NORDIC | ROMTYPE_XPOWER | ROMTYPE_CD32CART | ROMTYPE_ARCADIAGAME | ROMTYPE_ALG)

struct romheader {
	const TCHAR *name;
	int id;
};

struct romdata {
	const TCHAR *name;
	int ver, rev;
	int subver, subrev;
	const TCHAR *model;
	uae_u32 size;
	int id;
	int cpu;
	int cloanto;
	uae_u32 type;
	int group;
	int title;
	const TCHAR *partnumber;
	uae_u32 crc32;
	uae_u32 sha1[5];
	const TCHAR *configname;
	const TCHAR *defaultfilename;
	int sortpriority;
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
extern struct romdata *getromdatabytype (int romtype);
extern struct romdata *getromdatabyidgroup (int id, int group, int subitem);
extern struct romdata *getromdatabyzfile (struct zfile *f);
extern struct romdata *getfrombydefaultname(const TCHAR *name, int size);
extern struct romlist **getarcadiaroms(int);
extern struct romdata *getarcadiarombyname (const TCHAR *name);
extern struct romlist **getromlistbyident (int ver, int rev, int subver, int subrev, const TCHAR *model, int romflags, bool all);
extern void getromname (const struct romdata*, TCHAR*);
extern struct romdata *getromdatabyname (const TCHAR*);
extern struct romlist *getromlistbyids (const int *ids, const TCHAR *romname);
extern struct romlist *getromlistbyromtype(uae_u32 romtype, const TCHAR *romname);
extern void romwarning(const int *ids);
extern void romwarning(int romtype);
extern struct romlist *getromlistbyromdata (const struct romdata *rd);
extern void romlist_add (const TCHAR *path, struct romdata *rd);
extern TCHAR *romlist_get (const struct romdata *rd);
extern void romlist_clear (void);
extern struct zfile *read_rom(struct romdata *rd, bool rw = false);
extern struct zfile *read_rom_name(const TCHAR *filename, bool rw = false);
extern struct zfile *read_device_from_romconfig(struct romconfig *rc, uae_u32 romtype, bool rw = false);

extern int load_keyring (struct uae_prefs *p, const TCHAR *path);
extern uae_u8 *target_load_keyfile (struct uae_prefs *p, const TCHAR *path, int *size, TCHAR *name);
extern void free_keyring (void);
extern int get_keyring (void);
extern void kickstart_fix_checksum (uae_u8 *mem, int size);
extern void descramble_nordicpro (uae_u8*, int, int);
extern int kickstart_checksum (uae_u8 *mem, int size);
extern int decode_rom (uae_u8 *mem, int size, int mode, int real_size);
extern struct zfile *rom_fopen (const TCHAR *name, const TCHAR *mode, int mask);
extern struct zfile *read_rom_name_guess (const TCHAR *filename, TCHAR *out);
extern void addkeydir (const TCHAR *path);
extern void addkeyfile (const TCHAR *path);
extern int romlist_count (void);
extern struct romlist *romlist_getit (void);
extern int configure_rom (struct uae_prefs *p, const int *rom, int msg);

int is_device_rom(struct uae_prefs *p, int romtype, int devnum);
struct zfile *read_device_rom(struct uae_prefs *p, int romtype, int devnum, int *roms);
struct romconfig *get_device_romconfig(struct uae_prefs *p, int romtype, int devnum);
struct boardromconfig *get_device_rom(struct uae_prefs *p, int romtype, int devnum, int *index);
void set_device_rom(struct uae_prefs *p, const TCHAR *path, int romtype, int devnum);
const struct expansionromtype *get_device_expansion_rom(int romtype);
const struct expansionromtype *get_unit_expansion_rom(int hdunit);
struct boardromconfig *get_device_rom_new(struct uae_prefs *p, int romtype, int devnum, int *index);
void clear_device_rom(struct uae_prefs *p, int romtype, int devnum, bool deleteDevice);
struct boardromconfig *get_boardromconfig(struct uae_prefs *p, int romtype, int *index);
bool is_board_enabled(struct uae_prefs *p, int romtype, int devnum);
void board_prefs_changed(int romtype, int devnum);

#define LOADROM_FILL 1
#define LOADROM_EVENONLY 2
#define LOADROM_EVENONLY_ODDONE ((255 << 16) | LOADROM_EVENONLY)
#define LOADROM_ONEFILL 4
#define LOADROM_ZEROFILL 8
#define LOADROM_ODDFILL(x) ((x << 16) | LOADROM_EVENONLY)
bool load_rom_rc(struct romconfig *rc, uae_u32 romtype, int maxfilesize, int fileoffset, uae_u8 *rom, int maxromsize, int flags);
struct zfile *load_rom_rc_zfile(struct romconfig *rc, uae_u32 romtype, int maxfilesize, int fileoffset, uae_u8 *rom, int maxromsize, int flags);
struct zfile *flashromfile_open(const TCHAR *name);

#define EXPANSION_ORDER_MAX 10000

#endif /* UAE_ROMMGR_H */
