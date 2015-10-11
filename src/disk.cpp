 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Floppy disk emulation
  *
  * Copyright 1995 Hannu Rummukainen
  * Copyright 1995-2001 Bernd Schmidt
  * Copyright 2000-2003 Toni Wilen
  *
  * High Density Drive Handling by Dr. Adil Temel (C) 2001 [atemel1@hotmail.com]
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "options.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "ersatz.h"
#include "disk.h"
#include "gui.h"
#include "zfile.h"
#include "newcpu.h"
#include "osemu.h"
#include "execlib.h"
#include "savestate.h"
#include "cia.h"
#include "debug.h"
#include "crc32.h"
#include "inputdevice.h"

static int longwritemode = 0;

/* support HD floppies */
#define FLOPPY_DRIVE_HD
/* writable track length with normal 2us bitcell/300RPM motor (PAL) */
#define FLOPPY_WRITE_LEN (currprefs.ntscmode ? (12798 / 2) : (12668 / 2)) /* 12667 PAL, 12797 NTSC */
#define FLOPPY_WRITE_MAXLEN 0x3800
/* This works out to 350 */
#define FLOPPY_GAP_LEN (FLOPPY_WRITE_LEN - 11 * 544)
/* (cycles/bitcell) << 8, normal = ((2us/280ns)<<8) = ~1830 */
#define NORMAL_FLOPPY_SPEED (currprefs.ntscmode ? 1810 : 1829)
/* max supported floppy drives, for small memory systems */
#define MAX_FLOPPY_DRIVES 4

#ifdef FLOPPY_DRIVE_HD
#define DDHDMULT 2
#else
#define DDHDMULT 1
#endif

/* UAE-1ADF (ADF_EXT2)
 * W	reserved
 * W	number of tracks (default 2*80=160)
 *
 * W	reserved
 * W	type, 0=normal AmigaDOS track, 1 = raw MFM
 * L	available space for track in bytes (must be even)
 * L	track length in bits
 */

static int side, direction;
static uae_u8 selected = 15, disabled = 0;

static uae_u8 writebuffer[544 * 11 * DDHDMULT];

#define DISK_INDEXSYNC 1
#define DISK_WORDSYNC 2
#define DISK_REVOLUTION 4 /* 8,16,32,64 */

#define DSKREADY_TIME 4
#define DSKREADY_DOWN_TIME 10

static int diskevent_flag;
static int disk_sync_cycle;

static int dskdmaen, dsklength, dsklength2, dsklen;
static uae_u16 dskbytr_val;
static uae_u32 dskpt;
static int dma_enable, bitoffset;
static uae_u16 word, dsksync;
/* Always carried through to the next line.  */
static int disk_hpos;
static int disk_jitter;

typedef enum { TRACK_AMIGADOS, TRACK_RAW, TRACK_RAW1, TRACK_PCDOS } image_tracktype;
typedef struct {
    uae_u16 len;
    uae_u32 offs;
    int bitlen, track;
    unsigned int sync;
    image_tracktype type;
} trackid;

#define MAX_TRACKS (2 * 83)

/* We have three kinds of Amiga floppy drives
 * - internal A500/A2000 drive:
 *   ID is always DRIVE_ID_NONE (S.T.A.G expects this)
 * - HD drive (A3000/A4000):
 *   ID is DRIVE_ID_35DD if DD floppy is inserted or drive is empty
 *   ID is DRIVE_ID_35HD if HD floppy is inserted
 * - regular external drive:
 *   ID is always DRIVE_ID_35DD
 */

#define DRIVE_ID_NONE  0x00000000
#define DRIVE_ID_35DD  0xFFFFFFFF
#define DRIVE_ID_35HD  0xAAAAAAAA
#define DRIVE_ID_525SD 0x55555555 /* 40 track 5.25 drive , kickstart does not recognize this */

typedef enum { ADF_NORMAL, ADF_EXT1, ADF_EXT2, ADF_FDI, ADF_IPF, ADF_CATWEASEL, ADF_PCDOS } drive_filetype;
typedef struct {
    struct zfile *diskfile;
    struct zfile *writediskfile;
    drive_filetype filetype;
    trackid trackdata[MAX_TRACKS];
    trackid writetrackdata[MAX_TRACKS];
    int buffered_cyl, buffered_side;
    int cyl;
    int motoroff;
    int motordelay; /* dskrdy needs some clock cycles before it changes after switching off motor */
    int state;
    int wrprot;
    uae_u16 bigmfmbuf[0x4000 * DDHDMULT];
    uae_u16 tracktiming[0x4000 * DDHDMULT];
    int multi_revolution;
    int skipoffset;
    int mfmpos;
    int indexoffset;
    int tracklen;
    int prevtracklen;
    int trackspeed;
    int dmalen;
    int num_tracks, write_num_tracks, num_secs;
    int hard_num_cyls;
    int dskchange;
    int dskchange_time;
    int dskready;
    int dskready_time;
    int dskready_down_time;
    int steplimit;
    frame_time_t steplimitcycle;
    int indexhack, indexhackmode;
    int ddhd; /* 1=DD 2=HD */
    int drive_id_scnt; /* drive id shift counter */
    int idbit;
    unsigned long drive_id; /* drive id to be reported */
    char newname[256]; /* storage space for new filename during eject delay */
    uae_u32 crc32;
    int useturbo;
    int floppybitcounter; /* number of bits left */
} drive;

#define MIN_STEPLIMIT_CYCLE (CYCLE_UNIT * 150)

static uae_u16 bigmfmbufw[0x4000 * DDHDMULT];
static drive floppy[MAX_FLOPPY_DRIVES];
static char dfxhistory[MAX_PREVIOUS_FLOPPIES][MAX_DPATH];

static uae_u8 exeheader[]={0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00};
static uae_u8 bootblock[]={
    0x44,0x4f,0x53,0x00,0xc0,0x20,0x0f,0x19,0x00,0x00,0x03,0x70,0x43,0xfa,0x00,0x18,
    0x4e,0xae,0xff,0xa0,0x4a,0x80,0x67,0x0a,0x20,0x40,0x20,0x68,0x00,0x16,0x70,0x00,
    0x4e,0x75,0x70,0xff,0x60,0xfa,0x64,0x6f,0x73,0x2e,0x6c,0x69,0x62,0x72,0x61,0x72,
    0x79
};

#define FS_OFS_DATABLOCKSIZE 488
#define FS_FLOPPY_BLOCKSIZE 512
#define FS_EXTENSION_BLOCKS 72
#define FS_FLOPPY_TOTALBLOCKS 1760
#define FS_FLOPPY_RESERVED 2

static void writeimageblock (struct zfile *dst, uae_u8 *sector, int offset)
{
    zfile_fseek (dst, offset, SEEK_SET);
    zfile_fwrite (sector, FS_FLOPPY_BLOCKSIZE, 1, dst);
}

static void disk_checksum(uae_u8 *p, uae_u8 *c)
{
    uae_u32 cs = 0;
    int i;
    for (i = 0; i < FS_FLOPPY_BLOCKSIZE; i+= 4) 
      cs += (p[i] << 24) | (p[i+1] << 16) | (p[i+2] << 8) | (p[i+3] << 0);
    cs = -cs;
    c[0] = cs >> 24; c[1] = cs >> 16; c[2] = cs >> 8; c[3] = cs >> 0;
}

static int dirhash (const char *name)
{
    unsigned long hash;
    int i;

    hash = strlen (name);
    for(i = 0; i < strlen (name); i++) {
	hash = hash * 13;
	hash = hash + toupper (name[i]);
	hash = hash & 0x7ff;
    }
    hash = hash % ((FS_FLOPPY_BLOCKSIZE / 4) - 56);
    return hash;
}

static void disk_date (uae_u8 *p)
{
    time_t t;
    struct tm *today;
    int year, days, minutes, ticks;
    char tmp[10];
    time (&t);
    today = localtime( &t );
    strftime (tmp, sizeof(tmp), "%Y", today);
    year = atol (tmp);
    strftime (tmp, sizeof(tmp), "%j", today);
    days = atol (tmp) - 1;
    strftime (tmp, sizeof(tmp), "%H", today);
    minutes = atol (tmp) * 60;
    strftime (tmp, sizeof(tmp), "%M", today);
    minutes += atol (tmp);
    strftime (tmp, sizeof(tmp), "%S", today);
    ticks = atol (tmp) * 50;
    while (year > 1978) {
	if ( !(year % 100) ? !(year % 400) : !(year % 4) ) days++;
	days += 365;
	year--;
    }
    p[0] = days >> 24; p[1] = days >> 16; p[2] = days >> 8; p[3] = days >> 0;
    p[4] = minutes >> 24; p[5] = minutes >> 16; p[6] = minutes >> 8; p[7] = minutes >> 0;
    p[8] = ticks >> 24; p[9] = ticks >> 16; p[10] = ticks >> 8; p[11] = ticks >> 0; 
}

static void createbootblock (uae_u8 *sector, int bootable)
{
    memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
    memcpy (sector, "DOS", 3);
    if (bootable)
	memcpy (sector, bootblock, sizeof(bootblock));
}

static void createrootblock (uae_u8 *sector, const char *disk_name)
{
    memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
    sector[0+3] = 2;
    sector[12+3] = 0x48;
    sector[312] = sector[313] = sector[314] = sector[315] = (uae_u8)0xff;
    sector[316+2] = 881 >> 8; sector[316+3] = 881 & 255;
    sector[432] = strlen (disk_name);
    strcpy ((char *)(sector + 433), disk_name);
    sector[508 + 3] = 1;
    disk_date (sector + 420);
    memcpy (sector + 472, sector + 420, 3 * 4);
    memcpy (sector + 484, sector + 420, 3 * 4);
}

static int getblock (uae_u8 *bitmap)
{
    int i = 0;
    while (bitmap[i] != 0xff) {
	if (bitmap[i] == 0) {
	    bitmap[i] = 1;
	    return i;
	}
	i++;
    }
    return -1;
}

static void pl (uae_u8 *sector, int offset, uae_u32 v)
{
    sector[offset + 0] = v >> 24;
    sector[offset + 1] = v >> 16;
    sector[offset + 2] = v >> 8;
    sector[offset + 3] = v >> 0;
}

static int createdirheaderblock (uae_u8 *sector, int parent, char *filename, uae_u8 *bitmap)
{
    int block = getblock (bitmap);

    memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
    pl (sector, 0, 2);
    pl (sector, 4, block);
    disk_date (sector + 512 - 92);
    sector[512 - 80] = strlen (filename);
    strcpy ((char *)(sector + 512 - 79), filename);
    pl (sector, 512 - 12, parent);
    pl (sector, 512 - 4, 2);
    return block;
}

static int createfileheaderblock (struct zfile *z,uae_u8 *sector, int parent, char *filename, struct zfile *src, uae_u8 *bitmap)
{
    uae_u8 sector2[FS_FLOPPY_BLOCKSIZE];
    uae_u8 sector3[FS_FLOPPY_BLOCKSIZE];
    int block = getblock (bitmap);
    int datablock = getblock (bitmap);
    int datasec = 1;
    int extensions;
    int extensionblock, extensioncounter, headerextension = 1;
    int size;

    zfile_fseek (src, 0, SEEK_END);
    size = zfile_ftell (src);
    zfile_fseek (src, 0, SEEK_SET);
    extensions = (size + FS_OFS_DATABLOCKSIZE - 1) / FS_OFS_DATABLOCKSIZE;

    memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
    pl (sector, 0, 2);
    pl (sector, 4, block);
    pl (sector, 8, extensions > FS_EXTENSION_BLOCKS ? FS_EXTENSION_BLOCKS : extensions);
    pl (sector, 16, datablock);
    pl (sector, FS_FLOPPY_BLOCKSIZE - 188, size);
    disk_date (sector + FS_FLOPPY_BLOCKSIZE - 92);
    sector[FS_FLOPPY_BLOCKSIZE - 80] = strlen (filename);
    strcpy ((char *)(sector + FS_FLOPPY_BLOCKSIZE - 79), filename);
    pl (sector, FS_FLOPPY_BLOCKSIZE - 12, parent);
    pl (sector, FS_FLOPPY_BLOCKSIZE - 4, -3);
    extensioncounter = 0;
    extensionblock = 0;

    while (size > 0) {
	int datablock2 = datablock;
	int extensionblock2 = extensionblock;
	if (extensioncounter == FS_EXTENSION_BLOCKS) {
	    extensioncounter = 0;
	    extensionblock = getblock (bitmap);
	    if (datasec > FS_EXTENSION_BLOCKS + 1) {
	        pl (sector3, 8, FS_EXTENSION_BLOCKS);
	        pl (sector3, FS_FLOPPY_BLOCKSIZE - 8, extensionblock);
	        pl (sector3, 4, extensionblock2);
	        disk_checksum(sector3, sector3 + 20);
	        writeimageblock (z, sector3, extensionblock2 * FS_FLOPPY_BLOCKSIZE);
	    } else {
	        pl (sector, 512 - 8, extensionblock);
	    }
	    memset (sector3, 0, FS_FLOPPY_BLOCKSIZE);
	    pl (sector3, 0, 16);
	    pl (sector3, FS_FLOPPY_BLOCKSIZE - 12, block);
	    pl (sector3, FS_FLOPPY_BLOCKSIZE - 4, -3);
	}
	memset (sector2, 0, FS_FLOPPY_BLOCKSIZE);
	pl (sector2, 0, 8);
	pl (sector2, 4, block);
	pl (sector2, 8, datasec++);
	pl (sector2, 12, size > FS_OFS_DATABLOCKSIZE ? FS_OFS_DATABLOCKSIZE : size);
	zfile_fread (sector2 + 24, size > FS_OFS_DATABLOCKSIZE ? FS_OFS_DATABLOCKSIZE : size, 1, src);
	size -= FS_OFS_DATABLOCKSIZE;
	datablock = 0;
	if (size > 0) datablock = getblock (bitmap);
	pl (sector2, 16, datablock);
        disk_checksum(sector2, sector2 + 20);
        writeimageblock (z, sector2, datablock2 * FS_FLOPPY_BLOCKSIZE);
	if (datasec <= FS_EXTENSION_BLOCKS + 1)
	    pl (sector, 512 - 204 - extensioncounter * 4, datablock2);
	else
	    pl (sector3, 512 - 204 - extensioncounter * 4, datablock2);
	extensioncounter++;
    }
    if (datasec > FS_EXTENSION_BLOCKS) {
	pl (sector3, 8, extensioncounter);
        disk_checksum(sector3, sector3 + 20);
        writeimageblock (z, sector3, extensionblock * FS_FLOPPY_BLOCKSIZE);
    }
    disk_checksum(sector, sector + 20);
    writeimageblock (z, sector, block * FS_FLOPPY_BLOCKSIZE);
    return block;
}

static void createbitmapblock (uae_u8 *sector, uae_u8 *bitmap)
{
    uae_u8 mask;
    int i, j;
    memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
    for (i = FS_FLOPPY_RESERVED; i < FS_FLOPPY_TOTALBLOCKS; i += 8) {
	mask = 0;
	for (j = 0; j < 8; j++) {
	    if (bitmap[i + j]) mask |= 1 << j;
	}
	sector[4 + i / 8] = mask;
    }
    disk_checksum(sector, sector + 0);
}

static int createimagefromexe (struct zfile *src, struct zfile *dst)
{
    uae_u8 sector1[FS_FLOPPY_BLOCKSIZE], sector2[FS_FLOPPY_BLOCKSIZE];
    uae_u8 bitmap[FS_FLOPPY_TOTALBLOCKS + 8];
    int exesize;
    int blocksize = FS_OFS_DATABLOCKSIZE;
    int blocks, extensionblocks;
    int totalblocks;
    int fblock1, dblock1;
    char *fname1 = "runme.exe";
    char *fname2 = "startup-sequence";
    char *dirname1 = "s";
    struct zfile *ss;

    memset (bitmap, 0, sizeof (bitmap));
    zfile_fseek (src, 0, SEEK_END);
    exesize = zfile_ftell (src);
    blocks = (exesize + blocksize - 1) / blocksize;
    extensionblocks = (blocks + FS_EXTENSION_BLOCKS - 1) / FS_EXTENSION_BLOCKS;
    /* bootblock=2, root=1, bitmap=1, startup-sequence=1+1, exefileheader=1 */
    totalblocks = 2 + 1 + 1 + 2 + 1 + blocks + extensionblocks;
    if (totalblocks > FS_FLOPPY_TOTALBLOCKS)
	return 0;

    bitmap[880] = 1;
    bitmap[881] = 1;
    bitmap[0] = 1;
    bitmap[1] = 1;

    dblock1 = createdirheaderblock (sector2, 880, dirname1, bitmap);
    ss = zfile_fopen_empty (fname1, strlen(fname1));
    zfile_fwrite (fname1, strlen(fname1), 1, ss);
    fblock1 = createfileheaderblock (dst, sector1,  dblock1, fname2, ss, bitmap);
    zfile_fclose (ss);
    pl (sector2, 24 + dirhash (fname2) * 4, fblock1);
    disk_checksum(sector2, sector2 + 20);
    writeimageblock (dst, sector2, dblock1 * FS_FLOPPY_BLOCKSIZE);

    fblock1 = createfileheaderblock (dst, sector1, 880, fname1, src, bitmap);

    createrootblock (sector1, "empty");
    pl (sector1, 24 + dirhash (fname1) * 4, fblock1);
    pl (sector1, 24 + dirhash (dirname1) * 4, dblock1);
    disk_checksum(sector1, sector1 + 20);
    writeimageblock (dst, sector1, 880 * FS_FLOPPY_BLOCKSIZE);
    
    createbitmapblock (sector1, bitmap);
    writeimageblock (dst, sector1, 881 * FS_FLOPPY_BLOCKSIZE);

    createbootblock (sector1, 1);
    writeimageblock (dst, sector1, 0 * FS_FLOPPY_BLOCKSIZE);

    return 1;
}

static int get_floppy_speed (void)
{
    int m = currprefs.floppy_speed;
    if (m <= 10) 
      m = 100;
    m = NORMAL_FLOPPY_SPEED * 100 / m;
    return m;
}

static int get_floppy_speed2 (drive *drv)
{
    int m = get_floppy_speed () * drv->tracklen / (2 * 8 * FLOPPY_WRITE_LEN * drv->ddhd);
    if (m <= 0)
	m = 1;
    return m;
}

static const char *drive_id_name (drive *drv)
{
    switch (drv->drive_id)
    {
    case DRIVE_ID_35HD : return "3.5HD";
    case DRIVE_ID_525SD: return "5.25SD";
    case DRIVE_ID_35DD : return "3.5DD";
    case DRIVE_ID_NONE : return "NONE";
    }
    return "UNKNOWN";
}

/* Simulate exact behaviour of an A3000T 3.5 HD disk drive.
 * The drive reports to be a 3.5 DD drive whenever there is no
 * disk or a 3.5 DD disk is inserted. Only 3.5 HD drive id is reported
 * when a real 3.5 HD disk is inserted. -Adil
 */
static void drive_settype_id (drive *drv)
{
    int t = currprefs.dfxtype[drv - &floppy[0]];

    switch (t)
    {
    case DRV_35_HD:
#ifdef FLOPPY_DRIVE_HD
	if (!drv->diskfile || drv->ddhd <= 1)
	    drv->drive_id = DRIVE_ID_35DD;
	else
	    drv->drive_id = DRIVE_ID_35HD;
#else
  drv->drive_id = DRIVE_ID_35DD;
#endif
	break;
    case DRV_35_DD_ESCOM:
    case DRV_35_DD:
    default:
	drv->drive_id = DRIVE_ID_35DD;
	break;
    case DRV_525_SD:
	drv->drive_id = DRIVE_ID_525SD;
	break;
    case DRV_NONE:
	drv->drive_id = DRIVE_ID_NONE;
	break;
    }
}

static void drive_image_free (drive *drv)
{
    drv->filetype = (drive_filetype)-1;
    zfile_fclose (drv->diskfile);
    drv->diskfile = 0;
    zfile_fclose (drv->writediskfile);
    drv->writediskfile = 0;
}

static int drive_insert (drive * drv, struct uae_prefs *p, int dnum, const char *fname);

static void reset_drive_gui(int i)
{
    gui_data.drive_disabled[i] = 0;
    gui_data.df[i][0] = 0;
    gui_data.crc32[i] = 0;
    if (currprefs.dfxtype[i] < 0)
	gui_data.drive_disabled[i] = 1;
}

static void reset_drive(int i)
{
    drive *drv = &floppy[i];

    drive_image_free (drv);
    
    drv->motordelay = 0;
    drv->state = 0;
    drv->wrprot = 0;
    drv->mfmpos = 0;
    drv->tracklen = drv->prevtracklen = 0;
    drv->trackspeed = 0;
    drv->num_tracks = drv->write_num_tracks = drv->write_num_tracks = 0;
    drv->dskchange = drv->dskready = drv->dskready_time = drv->dskready_down_time = 0;
    drv->steplimit = drv->steplimitcycle = 0;
    drv->indexhack = 0;
    drv->drive_id_scnt = 0;
    drv->idbit = 0;
    drv->floppybitcounter = 0;
    
    drv->motoroff = 1;
    disabled &= ~(1 << i);
    if (currprefs.dfxtype[i] < 0)
        disabled |= 1 << i;
    reset_drive_gui(i);
    /* most internal Amiga floppy drives won't enable
     * diskready until motor is running at full speed
     * and next indexsync has been passed
     */
    drv->indexhackmode = 0;
    if (i == 0 && currprefs.dfxtype[i] == 0)
    	drv->indexhackmode = 1;
    drv->dskchange_time = 0;
    drv->buffered_cyl = -1;
    drv->buffered_side = -1;
    gui_led (i + 1, 0);
    drive_settype_id (drv);
    strcpy (currprefs.df[i], changed_prefs.df[i]);
    drv->newname[0] = 0;
    if (!drive_insert (drv, &currprefs, i, currprefs.df[i]))
        disk_eject (i);
}

/* code for track display */
static void update_drive_gui (int num)
{
  if (num>=currprefs.nr_floppies) 
    return;
  drive *drv = floppy + num;
  int writ = dskdmaen == 3 && drv->state && !(selected & (1 << num));

    if (drv->state == gui_data.drive_motor[num]
	&& drv->cyl == gui_data.drive_track[num]
	&& side == gui_data.drive_side
	&& drv->crc32 == gui_data.crc32[num]
	&& writ == gui_data.drive_writing[num])
	return;
    strcpy (gui_data.df[num], currprefs.df[num]);
    gui_data.crc32[num] = drv->crc32;
    gui_data.drive_motor[num] = drv->state;
    gui_data.drive_track[num] = drv->cyl;
    gui_data.drive_side = side;
    gui_data.drive_writing[num] = writ;
    gui_led (num + 1, gui_data.drive_motor[num]);
}

static void drive_fill_bigbuf (drive *drv, int);

struct zfile *DISK_validate_filename (const char *fname, int leave_open, int *wrprot, uae_u32 *crc32)
{
  if (crc32)
  	*crc32 = 0;
  if (leave_open) {
    struct zfile *f = zfile_fopen (fname, "r+b");
    if (f) {
    	if (wrprot)
  	    *wrprot = 0;
    } else {
    	if (wrprot)
  	    *wrprot = 1;
    	f = zfile_fopen (fname, "rb");
    }
	  if (f && crc32)
	    *crc32 = zfile_crc32 (f);
    return f;
  } else {
	  if (zfile_exists (fname)) {
	    if (wrprot)
		    *wrprot = 0;
	    if (crc32) {
		    struct zfile *f = zfile_fopen (fname, "rb");
		    if (f)
		      *crc32 = zfile_crc32 (f);
	      zfile_fclose (f);
	    }
	    return (struct zfile *)1;
	  } else {
	    if (wrprot)
		    *wrprot = 1;
	    return 0;
	  }
  }
}

static void updatemfmpos (drive *drv)
{
    if (drv->prevtracklen)
	drv->mfmpos = drv->mfmpos * (drv->tracklen * 1000 / drv->prevtracklen) / 1000;
    drv->mfmpos %= drv->tracklen;
    drv->prevtracklen = drv->tracklen;
}

static void track_reset (drive *drv)
{
    drv->tracklen = FLOPPY_WRITE_LEN * drv->ddhd * 2 * 8;
    drv->trackspeed = get_floppy_speed ();
    drv->buffered_side = -1;
    drv->skipoffset = -1;
    drv->tracktiming[0] = 0;
    memset (drv->bigmfmbuf, 0xaa, FLOPPY_WRITE_LEN * 2 * drv->ddhd);
    updatemfmpos (drv);
}

static int read_header_ext2 (struct zfile *diskfile, trackid *trackdata, int *num_tracks, int *ddhd)
{
    uae_u8 buffer[2 + 2 + 4 + 4];
    trackid *tid;
    int offs;
    int i;

    zfile_fseek (diskfile, 0, SEEK_SET);
    zfile_fread (buffer, 1, 8, diskfile);
    if (strncmp ((const char *)buffer, "UAE-1ADF", 8))
	return 0;
    zfile_fread (buffer, 1, 4, diskfile);
    *num_tracks = buffer[2] * 256 + buffer[3];
    offs = 8 + 2 + 2 + (*num_tracks) * (2 + 2 + 4 + 4);

    for (i = 0; i < (*num_tracks); i++) {
        tid = trackdata + i;
        zfile_fread (buffer, 2 + 2 + 4 + 4, 1, diskfile);
        tid->type = (image_tracktype)(buffer[2] * 256 + buffer[3]);
        tid->len = buffer[5] * 65536 + buffer[6] * 256 + buffer[7];
        tid->bitlen = buffer[9] * 65536 + buffer[10] * 256 + buffer[11];
        tid->offs = offs;
        if (tid->len > 20000 && ddhd)
	    *ddhd = 2;
	tid->track = i;
	offs += tid->len;
    }
    return 1;
}

char *DISK_get_saveimagepath (const char *name)
{
    static char name1[MAX_DPATH];
    char name2[MAX_DPATH];
    char path[MAX_DPATH];
    int i;
    
    strcpy (name2, name);
    i = strlen (name2) - 1;
    while (i > 0) {
	if (name2[i] == '.') {
	    name2[i] = 0;
	    break;
	}
	i--;
    }
    while (i > 0) {
	if (name2[i] == '/' || name2[i] == '\\') {
	    i++;
	    break;
	}
	i--;
    }
    fetch_saveimagepath (path, sizeof (path), 1);
    sprintf (name1, "%s%s_save.adf", path, name2 + i);
    return name1;
}

static struct zfile *getwritefile (const char *name, int *wrprot)
{
    return DISK_validate_filename (DISK_get_saveimagepath (name), 1, wrprot, NULL);
}

static int iswritefileempty (const char *name)
{
    struct zfile *zf;
    int wrprot;
    uae_u8 buffer[8];
    trackid td[MAX_TRACKS];
    int tracks, ddhd, i, ret;

    zf = getwritefile (name, &wrprot);
    if (!zf) return 1;
    zfile_fread (buffer, sizeof (char), 8, zf);
    if (strncmp ((char *) buffer, "UAE-1ADF", 8))
	return 0;
    ret = read_header_ext2 (zf, td, &tracks, &ddhd);
    zfile_fclose (zf);
    if (!ret)
	return 1;
    for (i = 0; i < tracks; i++) {
	if (td[i].bitlen) return 0;
    }
    return 1;
}

static int openwritefile (drive *drv, int create)
{
  int wrprot = 0;

  drv->writediskfile = getwritefile (currprefs.df[drv - &floppy[0]], &wrprot);
  if (drv->writediskfile) {
    drv->wrprot = wrprot;
	  if (!read_header_ext2 (drv->writediskfile, drv->writetrackdata, &drv->write_num_tracks, 0)) {
	    zfile_fclose (drv->writediskfile);
	    drv->writediskfile = 0;
	    drv->wrprot = 1;
	  } else {
	    if (drv->write_num_tracks > drv->num_tracks)
		    drv->num_tracks = drv->write_num_tracks;
	  }
  } else if (zfile_iscompressed (drv->diskfile)) {
	  drv->wrprot = 1;
  }
  return drv->writediskfile ? 1 : 0;
}

static int diskfile_iswriteprotect (const char *fname, int *needwritefile, drive_type *drvtype)
{
    struct zfile *zf1, *zf2;
    int wrprot1 = 0, wrprot2 = 1;
    unsigned char buffer[25];
    
    *needwritefile = 0;
    *drvtype = DRV_35_DD;
    zf1 = DISK_validate_filename (fname, 1, &wrprot1, NULL);
    if (!zf1) 
      return 1;
    if (zfile_iscompressed (zf1)) {
	    wrprot1 = 1;
	    *needwritefile = 1;
    }
    zf2 = getwritefile (fname, &wrprot2);
    zfile_fclose (zf2);
    zfile_fread (buffer, sizeof (char), 25, zf1);
    zfile_fclose (zf1);
    if (strncmp ((char *) buffer, "Formatted Disk Image file", 25) == 0) {
	*needwritefile = 1;
	return wrprot2;
    }
    if (strncmp ((char *) buffer, "UAE-1ADF", 8) == 0) {
	if (wrprot1)
	  return wrprot2;
	return wrprot1;
    }
    if (strncmp ((char *) buffer, "UAE--ADF", 8) == 0) {
	*needwritefile = 1;
	return wrprot2;
    }
    if (memcmp (exeheader, buffer, sizeof(exeheader)) == 0)
	return 0;
    if (wrprot1)
	    return wrprot2;
    return wrprot1;
}

static int drive_insert (drive *drv, struct uae_prefs *p, int dnum, const char *fname)
{
    unsigned char buffer[2 + 2 + 4 + 4];
    trackid *tid;
    int num_tracks, size;

    drive_image_free (drv);
    drv->diskfile = DISK_validate_filename (fname, 1, &drv->wrprot, &drv->crc32);
    drv->ddhd = 1;
    drv->num_secs = 0;
    drv->hard_num_cyls = p->dfxtype[dnum] == DRV_525_SD ? 40 : 80;
    drv->tracktiming[0] = 0;
    drv->useturbo = 0;
    drv->indexoffset = 0;

    if (!drv->motoroff) {
	    drv->dskready_time = DSKREADY_TIME;
	    drv->dskready_down_time = 0;
    }

    if (drv->diskfile == 0) {
	track_reset (drv);
	return 0;
    }

    strncpy (currprefs.df[dnum], fname, 255);
    currprefs.df[dnum][255] = 0;
    strncpy (changed_prefs.df[dnum], fname, 255);
    changed_prefs.df[dnum][255] = 0;
    strcpy (drv->newname, fname);
    gui_filename (dnum, fname);

    memset (buffer, 0, sizeof (buffer));
    size = 0;
    if (drv->diskfile) {
	zfile_fread (buffer, sizeof (char), 8, drv->diskfile);
	zfile_fseek (drv->diskfile, 0, SEEK_END);
	size = zfile_ftell (drv->diskfile);
	zfile_fseek (drv->diskfile, 0, SEEK_SET);
    }

    if (strncmp ((char *) buffer, "UAE-1ADF", 8) == 0) {

	read_header_ext2 (drv->diskfile, drv->trackdata, &drv->num_tracks, &drv->ddhd);
	drv->filetype = ADF_EXT2;
	drv->num_secs = 11;
	if (drv->ddhd > 1)
	    drv->num_secs = 22;

    } else if (strncmp ((char *) buffer, "UAE--ADF", 8) == 0) {
	int offs = 160 * 4 + 8;
	int i;

	drv->wrprot = 1;
	drv->filetype = ADF_EXT1;
	drv->num_tracks = 160;
	drv->num_secs = 11;

	zfile_fseek (drv->diskfile, 8, SEEK_SET);
	for (i = 0; i < 160; i++) {
	    tid = &drv->trackdata[i];
	    zfile_fread (buffer, 4, 1, drv->diskfile);
	    tid->sync = buffer[0] * 256 + buffer[1];
	    tid->len = buffer[2] * 256 + buffer[3];
	    tid->offs = offs;
	    if (tid->sync == 0) {
		tid->type = TRACK_AMIGADOS;
		tid->bitlen = 0;
	    } else {
		tid->type = TRACK_RAW1;
		tid->bitlen = tid->len * 8;
	    }
	    offs += tid->len;
	}

    } else if (memcmp (exeheader, buffer, sizeof(exeheader)) == 0) {
	int i;
	struct zfile *z = zfile_fopen_empty ("", 512 * 1760);
	createimagefromexe (drv->diskfile, z);
	drv->filetype = ADF_NORMAL;
	zfile_fclose (drv->diskfile);
	drv->diskfile = z;
	drv->num_tracks = 160;
	drv->num_secs = 11;
	for (i = 0; i < drv->num_tracks; i++) {
	    tid = &drv->trackdata[i];
	    tid->type = TRACK_AMIGADOS;
	    tid->len = 512 * drv->num_secs;
	    tid->bitlen = 0;
	    tid->offs = i * 512 * drv->num_secs;
	}
	drv->useturbo = 1;
    } else if (size == 720 * 1024 || size == 1440 * 1024) {
	/* PC formatted image */
	int i;

	drv->filetype = ADF_PCDOS;
	drv->num_tracks = 160;
	drv->ddhd = size == 1440 * 1024 ? 2 : 1;
	drv->num_secs = drv->ddhd == 2 ? 18 : 9;
	for (i = 0; i < drv->num_tracks; i++) {
	    tid = &drv->trackdata[i];
	    tid->type = TRACK_PCDOS;
	    tid->len = 512 * drv->num_secs;
	    tid->bitlen = 0;
	    tid->offs = i * 512 * drv->num_secs;
	}

    } else {
	int i;
	drv->filetype = ADF_NORMAL;

	/* High-density disk? */
	if (size >= 160 * 22 * 512) {
	    drv->num_tracks = size / (512 * (drv->num_secs = 22));
	    drv->ddhd = 2;
	} else
	    drv->num_tracks = size / (512 * (drv->num_secs = 11));

	if (drv->num_tracks > MAX_TRACKS)
	    write_log ("Your diskfile is too big, %d bytes!\n", size);
	for (i = 0; i < drv->num_tracks; i++) {
	    tid = &drv->trackdata[i];
	    tid->type = TRACK_AMIGADOS;
	    tid->len = 512 * drv->num_secs;
	    tid->bitlen = 0;
	    tid->offs = i * 512 * drv->num_secs;
	}
    }
    openwritefile (drv, 0);
    drive_settype_id (drv);	/* Set DD or HD drive */
    drive_fill_bigbuf (drv, 1);
    drv->mfmpos = uaerand ();
    drv->mfmpos |= (uaerand () << 16);
    drv->mfmpos %= drv->tracklen;
    drv->prevtracklen = 0;
    return 1;
}

static void rand_shifter (drive *drv)
{
    int r = ((uaerand () >> 4) & 7) + 1;
    while (r-- > 0) {
	word <<= 1;
	word |= (uaerand () & 0x1000) ? 1 : 0;
	bitoffset++;
	bitoffset &= 15;
    }
}

static int drive_empty (drive * drv)
{
    return drv->diskfile == 0;
}

static void drive_step (drive * drv)
{
    if (drv->steplimit && get_cycles() - drv->steplimitcycle < MIN_STEPLIMIT_CYCLE) {
	return;
    }
    /* A1200's floppy drive needs at least 30 raster lines between steps
     * but we'll use very small value for better compatibility with faster CPU emulation
     * (stupid trackloaders with CPU delay loops)
     */
    drv->steplimit = 10;
    drv->steplimitcycle = get_cycles ();
    if (!drive_empty (drv))
	drv->dskchange = 0;
    if (direction) {
	    if (drv->cyl) {
	      drv->cyl--;
      }
  } else {
    int maxtrack = drv->hard_num_cyls;
    if (drv->cyl < maxtrack + 3) {
	    drv->cyl++;
    }
  }
  rand_shifter (drv);
}

static int drive_track0 (drive * drv)
{
    return drv->cyl == 0;
}

static int drive_writeprotected (drive * drv)
{
    return drv->wrprot || drv->diskfile == NULL;
}

static int drive_running (drive * drv)
{
    return !drv->motoroff;
}

static void motordelay_func(uae_u32 v)
{
    floppy[v].motordelay = 0;
}

static void drive_motor (drive * drv, int off)
{
  if (drv->motoroff && !off) {
    drv->dskready_time = DSKREADY_TIME;
    rand_shifter (drv);
  }
  if (!drv->motoroff && off) {
	  drv->drive_id_scnt = 0; /* Reset id shift reg counter */
	  drv->dskready_down_time = DSKREADY_DOWN_TIME;
	  if (currprefs.cpu_model <= 68010 && currprefs.m68k_speed == 0) {
	      drv->motordelay = 1;
	      event2_newevent2(30, drv - floppy, motordelay_func);
	  }
  }
  drv->motoroff = off;
  if (drv->motoroff) {
	  drv->dskready = 0;
	  drv->dskready_time = 0;
  }
}

static void read_floppy_data (struct zfile *diskfile, trackid *tid, int offset, unsigned char *dst, int len)
{
    if (len == 0)
	return;
    zfile_fseek (diskfile, tid->offs + offset, SEEK_SET);
    zfile_fread (dst, 1, len, diskfile);
}

/* Megalomania does not like zero MFM words... */
static void mfmcode (uae_u16 * mfm, int words)
{
    uae_u32 lastword = 0;

    while (words--) {
	uae_u32 v = *mfm;
	uae_u32 lv = (lastword << 16) | v;
	uae_u32 nlv = 0x55555555 & ~lv;
	uae_u32 mfmbits = (nlv << 1) & (nlv >> 1);

	*mfm++ = v | mfmbits;
	lastword = v;
    }
}

static uae_u8 mfmencodetable[16] = {
    0x2a, 0x29, 0x24, 0x25, 0x12, 0x11, 0x14, 0x15,
    0x4a, 0x49, 0x44, 0x45, 0x52, 0x51, 0x54, 0x55
};

static uae_u16 dos_encode_byte(uae_u8 byte)
{           
    uae_u8 b2, b1;        
    uae_u16 word;

    b1 = byte;
    b2 = b1 >> 4;
    b1 &= 15;
    word = mfmencodetable[b2] <<8 | mfmencodetable[b1];
    return (word | ((word & (256 | 64)) ? 0 : 128));
}

static uae_u16 *mfmcoder(uae_u8 *src, uae_u16 *dest, int len) 
{
    int i;

    for (i = 0; i < len; i++) {
	*dest = dos_encode_byte(*src++);
	*dest |= ((dest[-1] & 1)||(*dest & 0x4000)) ? 0: 0x8000;            
	dest++;
    }
    return dest;
}


static void decode_pcdos (drive *drv)
{
    int i;
    int tr = drv->cyl * 2 + side;
    uae_u16 *dstmfmbuf, *mfm2;
    uae_u8 secbuf[700];
    uae_u16 crc16;
    trackid *ti = drv->trackdata + tr;
    int tracklen = 12500;

    mfm2 = drv->bigmfmbuf;
    *mfm2++ = 0x9254;
    memset (secbuf, 0x4e, 40);
    memset (secbuf + 40, 0x00, 12);
    secbuf[52] = 0xc2;
    secbuf[53] = 0xc2;
    secbuf[54] = 0xc2;
    secbuf[55] = 0xfc;
    memset (secbuf + 56, 0x4e, 40);
    dstmfmbuf = mfmcoder (secbuf, mfm2, 96);
    mfm2[52] = 0x5224;
    mfm2[53] = 0x5224;
    mfm2[54] = 0x5224;
    for (i = 0; i < drv->num_secs; i++) {
        mfm2 = dstmfmbuf;
	memset (secbuf, 0x00, 12);
	secbuf[12] = 0xa1;
	secbuf[13] = 0xa1;
	secbuf[14] = 0xa1;
	secbuf[15] = 0xfe;
	secbuf[16] = drv->cyl;
	secbuf[17] = side;
	secbuf[18] = 1 + i;
	secbuf[19] = 2; // 128 << 2 = 512
	crc16 = get_crc16(secbuf + 12, 3 + 1 + 4);
	secbuf[20] = crc16 >> 8;
	secbuf[21] = crc16 & 0xff;
	memset(secbuf + 22, 0x4e, 22);
	memset(secbuf + 44, 0x00, 12);
	secbuf[56] = 0xa1;
	secbuf[57] = 0xa1;
	secbuf[58] = 0xa1;
	secbuf[59] = 0xfb;
	read_floppy_data (drv->diskfile, ti, i * 512, &secbuf[60], 512);
	crc16 = get_crc16(secbuf + 56, 3 + 1 + 512);
	secbuf[60 + 512] = crc16 >> 8;
	secbuf[61 + 512] = crc16 & 0xff;
	memset(secbuf + 512 + 62, 0x4e, (tracklen / 2 - 96) / drv->num_secs - 574);
	dstmfmbuf = mfmcoder(secbuf, mfm2, 60 + 512 + 2 + 76 / drv->ddhd);
	mfm2[12] = 0x4489;
	mfm2[13] = 0x4489;
	mfm2[14] = 0x4489;
	mfm2[56] = 0x4489;
	mfm2[57] = 0x4489;
	mfm2[58] = 0x4489;
    }
    while (dstmfmbuf - drv->bigmfmbuf < tracklen / 2)
        *dstmfmbuf++ = 0x9254;
    drv->skipoffset = 0;
    drv->tracklen = (dstmfmbuf - drv->bigmfmbuf) * 16;
}

static void decode_amigados (drive *drv)
{
    /* Normal AmigaDOS format track */
    int tr = drv->cyl * 2 + side;
  int sec;
	int dstmfmoffset = 0;
  uae_u16 *dstmfmbuf = drv->bigmfmbuf;
  int len = drv->num_secs * 544 + FLOPPY_GAP_LEN;

    trackid *ti = drv->trackdata + tr;
	memset (dstmfmbuf, 0xaa, len * 2);
	dstmfmoffset += FLOPPY_GAP_LEN;
	drv->skipoffset = (FLOPPY_GAP_LEN * 8) / 3 * 2;
	drv->tracklen = len * 2 * 8;

    for (sec = 0; sec < drv->num_secs; sec++) {
	uae_u8 secbuf[544];
	    uae_u16 mfmbuf[544];
	int i;
	uae_u32 deven, dodd;
	uae_u32 hck = 0, dck = 0;

	secbuf[0] = secbuf[1] = 0x00;
	secbuf[2] = secbuf[3] = 0xa1;
	secbuf[4] = 0xff;
	secbuf[5] = tr;
	secbuf[6] = sec;
	secbuf[7] = drv->num_secs - sec;

	for (i = 8; i < 24; i++)
	    secbuf[i] = 0;

	read_floppy_data (drv->diskfile, ti, sec * 512, &secbuf[32], 512);

	mfmbuf[0] = mfmbuf[1] = 0xaaaa;
	mfmbuf[2] = mfmbuf[3] = 0x4489;

	deven = ((secbuf[4] << 24) | (secbuf[5] << 16)
		 | (secbuf[6] << 8) | (secbuf[7]));
	dodd = deven >> 1;
	deven &= 0x55555555;
	dodd &= 0x55555555;

	mfmbuf[4] = dodd >> 16;
	mfmbuf[5] = dodd;
	mfmbuf[6] = deven >> 16;
	mfmbuf[7] = deven;

	for (i = 8; i < 48; i++)
	    mfmbuf[i] = 0xaaaa;
	for (i = 0; i < 512; i += 4) {
	    deven = ((secbuf[i + 32] << 24) | (secbuf[i + 33] << 16)
		     | (secbuf[i + 34] << 8) | (secbuf[i + 35]));
	    dodd = deven >> 1;
	    deven &= 0x55555555;
	    dodd &= 0x55555555;
	    mfmbuf[(i >> 1) + 32] = dodd >> 16;
	    mfmbuf[(i >> 1) + 33] = dodd;
	    mfmbuf[(i >> 1) + 256 + 32] = deven >> 16;
	    mfmbuf[(i >> 1) + 256 + 33] = deven;
	}

	for (i = 4; i < 24; i += 2)
	    hck ^= (mfmbuf[i] << 16) | mfmbuf[i + 1];

	deven = dodd = hck;
	dodd >>= 1;
	mfmbuf[24] = dodd >> 16;
	mfmbuf[25] = dodd;
	mfmbuf[26] = deven >> 16;
	mfmbuf[27] = deven;

	for (i = 32; i < 544; i += 2)
	    dck ^= (mfmbuf[i] << 16) | mfmbuf[i + 1];

	deven = dodd = dck;
	dodd >>= 1;
	mfmbuf[28] = dodd >> 16;
	mfmbuf[29] = dodd;
	mfmbuf[30] = deven >> 16;
	mfmbuf[31] = deven;
	mfmcode (mfmbuf + 4, 544 - 4);

  for (i = 0; i < 544; i++) {
		dstmfmbuf[dstmfmoffset % len] = mfmbuf[i];
		dstmfmoffset++;
	}
     }
}

static void drive_fill_bigbuf (drive * drv, int force)
{
    int tr = drv->cyl * 2 + side;
    trackid *ti = drv->trackdata + tr;

    if (!drv->diskfile || tr >= drv->num_tracks) {
    	track_reset (drv);
    	return;
    }

    if (!force && drv->buffered_cyl == drv->cyl && drv->buffered_side == side)
	return;
    drv->indexoffset = 0;
    drv->multi_revolution = 0;
    drv->tracktiming[0] = 0;
    drv->skipoffset = -1;

    if (drv->writediskfile && drv->writetrackdata[tr].bitlen > 0) {
	int i;
	trackid *wti = &drv->writetrackdata[tr];
	drv->tracklen = wti->bitlen;
	read_floppy_data (drv->writediskfile, wti, 0, (unsigned char *) drv->bigmfmbuf, (wti->bitlen + 7) / 8);
	for (i = 0; i < (drv->tracklen + 15) / 16; i++) {
	    uae_u16 *mfm = drv->bigmfmbuf + i;
	    uae_u8 *data = (uae_u8 *) mfm;
	    *mfm = 256 * *data + *(data + 1);
	}
    } else if (ti->type == TRACK_PCDOS) {

	decode_pcdos(drv);


    } else if (ti->type == TRACK_AMIGADOS) {

	decode_amigados(drv);

    } else {
	int i;
	int base_offset = ti->type == TRACK_RAW ? 0 : 1;
	drv->tracklen = ti->bitlen + 16 * base_offset;
	drv->bigmfmbuf[0] = ti->sync;
	read_floppy_data (drv->diskfile, ti, 0, (unsigned char *) (drv->bigmfmbuf + base_offset), (ti->bitlen + 7) / 8);
	for (i = base_offset; i < (drv->tracklen + 15) / 16; i++) {
	    uae_u16 *mfm = drv->bigmfmbuf + i;
	    uae_u8 *data = (uae_u8 *) mfm;
	    *mfm = 256 * *data + *(data + 1);
	}
    }
    drv->buffered_side = side;
    drv->buffered_cyl = drv->cyl;
    if (drv->tracklen == 0) {
    	drv->tracklen = FLOPPY_WRITE_LEN * drv->ddhd * 2 * 8;
	    memset (drv->bigmfmbuf, 0, FLOPPY_WRITE_LEN * 2 * drv->ddhd);
    }
    drv->trackspeed = get_floppy_speed2 (drv);
    updatemfmpos (drv);
}

/* Update ADF_EXT2 track header */
static void diskfile_update (struct zfile *diskfile, trackid *ti, int len, uae_u8 type)
{
    uae_u8 buf[2 + 2 + 4 + 4], *zerobuf;

    ti->bitlen = len;
    zfile_fseek (diskfile, 8 + 4 + (2 + 2 + 4 + 4) * ti->track, SEEK_SET);
    memset (buf, 0, sizeof buf);
    ti->type = (image_tracktype) type;
    buf[3] = ti->type;
    do_put_mem_long ((uae_u32 *) (buf + 4), ti->len);
    do_put_mem_long ((uae_u32 *) (buf + 8), ti->bitlen);
    zfile_fwrite (buf, sizeof (buf), 1, diskfile);
    if (ti->len > (len + 7) / 8) {
	zerobuf = (uae_u8 *)xmalloc (ti->len);
	memset (zerobuf, 0, ti->len);
	zfile_fseek (diskfile, ti->offs, SEEK_SET);
	zfile_fwrite (zerobuf, 1, ti->len, diskfile);
	free (zerobuf);
    }
}

#define MFMMASK 0x55555555
static uae_u16 getmfmword (uae_u16 *mbuf, int shift)
{
    return (mbuf[0] << shift) | (mbuf[1] >> (16 - shift));
}

static uae_u32 getmfmlong (uae_u16 *mbuf, int shift)
{
    return ((getmfmword (mbuf, shift) << 16) | getmfmword (mbuf + 1, shift)) & MFMMASK;
}

static int decode_buffer (uae_u16 *mbuf, int cyl, int drvsec, int ddhd, int filetype, int *drvsecp, int checkmode)
{
  int i, secwritten = 0;
  int fwlen = FLOPPY_WRITE_LEN * ddhd;
  int length = 2 * fwlen;
  uae_u32 odd, even, chksum, id, dlong;
  uae_u8 *secdata;
  uae_u8 secbuf[544];
  uae_u16 *mend = mbuf + length;
  char sectable[22];
  int shift = 0;

  memset (sectable, 0, sizeof (sectable));
  memcpy (mbuf + fwlen, mbuf, fwlen * sizeof (uae_u16));
  mend -= (4 + 16 + 8 + 512);
  while (secwritten < drvsec) {
	  int trackoffs;

	  while (getmfmword (mbuf, shift) != 0x4489) {
		  if (mbuf >= mend)
		      return 1;
	    shift++;
	    if (shift == 16) {
		    shift = 0;
		    mbuf++;
	    }
	  }
  	while (getmfmword (mbuf, shift) == 0x4489) {
	    if (mbuf >= mend)
	      return 1;
	    mbuf++;
	  }

	  odd = getmfmlong (mbuf, shift);
	  even = getmfmlong (mbuf + 2, shift);
	  mbuf += 4;
	  id = (odd << 1) | even;

	  trackoffs = (id & 0xff00) >> 8;
	  if (trackoffs + 1 > drvsec) {
	      if (filetype == ADF_EXT2)
		      return 2;
	      continue;
	  }
  	chksum = odd ^ even;
  	for (i = 0; i < 4; i++) {
	    odd = getmfmlong (mbuf, shift);
	    even = getmfmlong (mbuf + 8, shift);
	    mbuf += 2;

	    dlong = (odd << 1) | even;
	    if (dlong && !checkmode) {
	    	if (filetype == ADF_EXT2)
	  	    return 6;
	    	secwritten = -200;
	    }
	    chksum ^= odd ^ even;
	  }			/* could check here if the label is nonstandard */
	  mbuf += 8;
	  odd = getmfmlong (mbuf, shift);
	  even = getmfmlong (mbuf + 2, shift);
	  mbuf += 4;
	  if (((odd << 1) | even) != chksum || ((id & 0x00ff0000) >> 16) != cyl * 2 + side) {
      if (filetype == ADF_EXT2)
	      return 3;
      continue;
	  }
	  odd = getmfmlong (mbuf, shift);
	  even = getmfmlong (mbuf + 2, shift);
	  mbuf += 4;
	  chksum = (odd << 1) | even;
	  secdata = secbuf + 32;
	  for (i = 0; i < 128; i++) {
	    odd = getmfmlong (mbuf, shift);
	    even = getmfmlong (mbuf + 256, shift);
	    mbuf += 2;
	    dlong = (odd << 1) | even;
	    *secdata++ = dlong >> 24;
	    *secdata++ = dlong >> 16;
	    *secdata++ = dlong >> 8;
	    *secdata++ = dlong;
	    chksum ^= odd ^ even;
  	}
	  if (chksum) {
	    if (filetype == ADF_EXT2)
		    return 4;
	    continue;
  	}
	  mbuf += 256;
	  sectable[trackoffs] = 1;
	  secwritten++;
	  memcpy (writebuffer + trackoffs * 512, secbuf + 32, 512);
  }
  if (filetype == ADF_EXT2 && (secwritten == 0 || secwritten < 0))
  	return 5;
  *drvsecp = drvsec;
  return 0;
}

static uae_u8 mfmdecode(uae_u16 **mfmp, int shift)
{
    uae_u16 mfm = getmfmword (*mfmp, shift);
    uae_u8 out = 0;
    int i;

    (*mfmp)++;
    mfm &= 0x5555;
    for (i = 0; i < 8; i++) {
	out >>= 1;
	if (mfm & 1)
	    out |= 0x80;
	mfm >>= 2;
    }
    return out;
}

static int drive_write_pcdos (drive *drv)
{
    int i;
    int drvsec = drv->num_secs;
    int fwlen = FLOPPY_WRITE_LEN * drv->ddhd;
    int length = 2 * fwlen;
    uae_u16 *mbuf = drv->bigmfmbuf;
    uae_u16 *mend = mbuf + length;
    int secwritten = 0, shift = 0, sector = -1;
    char sectable[18];
    uae_u8 secbuf[3 + 1 + 512];
    uae_u8 mark;
    uae_u16 crc;

    memset (sectable, 0, sizeof (sectable));
    memcpy (mbuf + fwlen, mbuf, fwlen * sizeof (uae_u16));
    mend -= 518;
    secbuf[0] = secbuf[1] = secbuf[2] = 0xa1;
    secbuf[3] = 0xfb;

    while (secwritten < drvsec) {
	while (getmfmword (mbuf, shift) != 0x4489) {
	    if (mbuf >= mend)
		return 1;
	    shift++;
	    if (shift == 16) {
		shift = 0;
		mbuf++;
	    }
	}
	while (getmfmword (mbuf, shift) == 0x4489) {
	    if (mbuf >= mend)
		return 1;
	    mbuf++;
	}
	mark = mfmdecode(&mbuf, shift);
	if (mark == 0xfe) {
	    uae_u8 tmp[8];
	    uae_u8 cyl, head, size;

	    cyl = mfmdecode(&mbuf, shift);
	    head = mfmdecode(&mbuf, shift);
	    sector = mfmdecode(&mbuf, shift);
	    size = mfmdecode(&mbuf, shift);
	    crc = (mfmdecode(&mbuf, shift) << 8) | mfmdecode(&mbuf, shift);

	    tmp[0] = 0xa1; tmp[1] = 0xa1; tmp[2] = 0xa1; tmp[3] = mark;
	    tmp[4] = cyl; tmp[5] = head; tmp[6] = sector; tmp[7] = size;
	    if (get_crc16(tmp, 8) != crc || cyl != drv->cyl || head != side || size != 2 || sector < 1 || sector > drv->num_secs) {
		return 1;
	    }
	    sector--;
	    continue;
	}
	if (mark != 0xfb) {
	    continue;
	}
	if (sector < 0)
	    continue;
	for (i = 0; i < 512; i++)
	    secbuf[i + 4] = mfmdecode(&mbuf, shift);
	crc = (mfmdecode(&mbuf, shift) << 8) | mfmdecode(&mbuf, shift);
	if (get_crc16(secbuf, 3 + 1 + 512) != crc) {
	    continue;
	}
	sectable[sector] = 1;
	secwritten++;
	zfile_fseek (drv->diskfile, drv->trackdata[drv->cyl * 2 + side].offs + sector * 512, SEEK_SET);
	zfile_fwrite (secbuf + 4, sizeof (uae_u8), 512, drv->diskfile);
	sector = -1;
    }
    return 0;
}

static int drive_write_adf_amigados (drive * drv)
{
    int drvsec, i;

    if (decode_buffer (drv->bigmfmbuf, drv->cyl, drv->num_secs, drv->ddhd, drv->filetype, &drvsec, 0))
	return 2;
    if (!drvsec)
	return 2;

    if (drv->filetype == ADF_EXT2)
      diskfile_update (drv->diskfile, &drv->trackdata[drv->cyl * 2 + side], drvsec * 512 * 8, TRACK_AMIGADOS);
    for (i = 0; i < drvsec; i++) {
	zfile_fseek (drv->diskfile, drv->trackdata[drv->cyl * 2 + side].offs + i * 512, SEEK_SET);
	zfile_fwrite (writebuffer + i * 512, sizeof (uae_u8), 512, drv->diskfile);
    }

    return 0;
}

/* write raw track to disk file */
static int drive_write_ext2 (uae_u16 *bigmfmbuf, struct zfile *diskfile, trackid *ti, int tracklen)
{
    int len, i;

    len = (tracklen + 7) / 8;
    if (len > ti->len) {
	len = ti->len;
    }
    diskfile_update (diskfile, ti, tracklen, TRACK_RAW);
    for (i = 0; i < ti->len / 2; i++) {
	uae_u16 *mfm = bigmfmbuf + i;
	uae_u16 *mfmw = bigmfmbufw + i;
	uae_u8 *data = (uae_u8 *) mfm;
	*mfmw = 256 * *data + *(data + 1);
    }
    zfile_fseek (diskfile, ti->offs, SEEK_SET);
    zfile_fwrite (bigmfmbufw, 1, len, diskfile);
    return 1;
}

static void drive_write_data (drive * drv)
{
    int ret = -1;
    static int warned;

    if (drive_writeprotected (drv)) {
	    /* read original track back because we didn't really write anything */
	    drv->buffered_side = 2;
	    return;
    }
    if (drv->writediskfile) {
	drive_write_ext2 (drv->bigmfmbuf, drv->writediskfile, &drv->writetrackdata[drv->cyl * 2 + side], 
      longwritemode ? dsklength2 * 8 : drv->tracklen);
    }
    switch (drv->filetype) {
    case ADF_NORMAL:
	if (drive_write_adf_amigados (drv)) {
	    if (!warned)
		notify_user (NUMSG_NEEDEXT2);
	    warned = 1;
	}
	return;
    case ADF_EXT1:
	    break;
    case ADF_EXT2:
	if (!longwritemode)
	    ret = drive_write_adf_amigados (drv);
	    if (ret) {
	      drive_write_ext2 (drv->bigmfmbuf, drv->diskfile, &drv->trackdata[drv->cyl * 2 + side],
		      longwritemode ? dsklength2 * 8 : drv->tracklen);
	    }
	    return;
 
   case ADF_IPF:
    	break;
    case ADF_PCDOS:
	ret = drive_write_pcdos (drv);
	break;
    }
    drv->tracktiming[0] = 0;
}

static void drive_eject (drive * drv)
{
    drive_image_free (drv);
    drv->dskchange = 1;
    drv->ddhd = 1;
    drv->dskchange_time = 0;
    drv->dskready = 0;
    drv->dskready_time = 0;
    drv->dskready_down_time = 0;
    drv->crc32 = 0;
    drive_settype_id (drv); /* Back to 35 DD */
}

/* We use this function if we have no Kickstart ROM.
 * No error checking - we trust our luck. */
void DISK_ersatz_read (int tr, int sec, uaecptr dest)
{
    uae_u8 *dptr = get_real_address (dest);
    zfile_fseek (floppy[0].diskfile, floppy[0].trackdata[tr].offs + sec * 512, SEEK_SET);
    zfile_fread (dptr, 1, 512, floppy[0].diskfile);
}

/* type: 0=regular, 1=ext2adf */
/* adftype: 0=DD,1=HD,2=525SD */
void disk_creatediskfile (char *name, int type, drive_type adftype, char *disk_name)
{
    struct zfile *f;
    int i, l, file_size, tracks, track_len;
    char *chunk = NULL;
    uae_u8 tmp[3*4];
    if (disk_name == NULL || strlen(disk_name) == 0)
	disk_name = (char *)"empty";

    if (type == 1)
    	tracks = 2 * 83;
    else
	    tracks = 2 * 80;
    file_size = 880 * 1024;
    track_len = FLOPPY_WRITE_LEN * 2;
    if (adftype == 1) {
	file_size *= 2;
	track_len *= 2;
    } else if (adftype == 2) {
	file_size /= 2;
	tracks /= 2;
    }

    f = zfile_fopen (name, "wb");
    chunk = (char *)xmalloc (16384);
    if (f && chunk) {
	memset(chunk,0,16384);
	if (type == 0 && adftype < 2) {
	    for (i = 0; i < file_size; i += 11264) {
		memset(chunk, 0, 11264);
		if (i == 0) {
		    /* boot block */
		    strcpy (chunk, "DOS");
		} else if (i == file_size / 2) {
		    int block = file_size / 1024;
		    /* root block */
		    chunk[0+3] = 2;
		    chunk[12+3] = 0x48;
		    chunk[312] = chunk[313] = chunk[314] = chunk[315] = (uae_u8)0xff;
		    chunk[316+2] = (block + 1) >> 8; chunk[316+3] = (block + 1) & 255;
		    chunk[432] = strlen (disk_name);
		    strcpy (chunk + 433, disk_name);
		    chunk[508 + 3] = 1;
		    disk_date ((uae_u8 *)(chunk + 420));
		    memcpy (chunk + 472, chunk + 420, 3 * 4);
		    memcpy (chunk + 484, chunk + 420, 3 * 4);
		    disk_checksum((uae_u8 *)chunk, (uae_u8 *)(chunk + 20));
		    /* bitmap block */
		    memset (chunk + 512 + 4, 0xff, 2 * file_size / (1024 * 8));
		    if (adftype == 0)
			    chunk[512 + 0x72] = 0x3f;
		    else
			    chunk[512 + 0xdc] = 0x3f;
		    disk_checksum((uae_u8 *)(chunk + 512), (uae_u8 *)(chunk + 512));

		}
		zfile_fwrite (chunk, 11264, 1, f);
	    }
	} else {
	    l = track_len;
	    zfile_fwrite ((void *)"UAE-1ADF", 8, 1, f);
	    tmp[0] = 0; tmp[1] = 0; /* flags (reserved) */
	    tmp[2] = 0; tmp[3] = tracks; /* number of tracks */
	    zfile_fwrite (tmp, 4, 1, f);
	    tmp[0] = 0; tmp[1] = 0; /* flags (reserved) */
	    tmp[2] = 0; tmp[3] = 1; /* track type */
	    tmp[4] = 0; tmp[5] = 0; tmp[6]=(uae_u8)(l >> 8); tmp[7] = (uae_u8)l;
	    tmp[8] = 0; tmp[9] = 0; tmp[10] = 0; tmp[11] = 0;
	    for (i = 0; i < tracks; i++) zfile_fwrite (tmp, sizeof (tmp), 1, f);
	    for (i = 0; i < tracks; i++) zfile_fwrite (chunk, l, 1, f);
	}
    }
    free (chunk);
    zfile_fclose (f);
}

int disk_getwriteprotect (const char *name)
{
    int needwritefile;
    drive_type drvtype;
    return diskfile_iswriteprotect (name, &needwritefile, &drvtype);
}

static void diskfile_readonly (const char *name, int readonly)
{
    struct stat st;
    int mode, oldmode;
    
    if (stat (name, &st))
	return;
    oldmode = mode = st.st_mode;
    mode &= ~FILEFLAG_WRITE;
    if (!readonly) mode |= FILEFLAG_WRITE;
    if (mode != oldmode)
	chmod (name, mode);
}

static void setdskchangetime(drive *drv, int dsktime)
{
    int i;
    /* prevent multiple disk insertions at the same time */
    if (drv->dskchange_time > 0)
	return;
    for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
	if (&floppy[i] != drv && floppy[i].dskchange_time > 0 && floppy[i].dskchange_time + 5 >= dsktime) {
	    dsktime = floppy[i].dskchange_time + 5;
	}
    }
    drv->dskchange_time = dsktime;
}

void DISK_reinsert (int num)
{
    drive_eject (&floppy[num]);
    setdskchangetime (&floppy[num], 20);
}

int disk_setwriteprotect (int num, const char *name, int protect)
{
    int needwritefile, oldprotect;
    struct zfile *zf1, *zf2;
    int wrprot1, wrprot2, i;
    char *name2;
    drive_type drvtype;
 
    oldprotect = diskfile_iswriteprotect (name, &needwritefile, &drvtype);
    zf1 = DISK_validate_filename (name, 1, &wrprot1, NULL);
    if (!zf1) 
      return 0;
    if (zfile_iscompressed (zf1)) 
      wrprot1 = 1;
    zfile_fclose (zf1);
    zf2 = getwritefile (name, &wrprot2);
    name2 = DISK_get_saveimagepath (name);

    if (needwritefile && zf2 == 0)
	disk_creatediskfile (name2, 1, drvtype, NULL);
    zfile_fclose (zf2);
    if (protect && iswritefileempty (name)) {
	for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
	    if (!strcmp (name, floppy[i].newname))
		drive_eject (&floppy[i]);
	}
	unlink (name2);
    }

    if (!needwritefile)
        diskfile_readonly (name, protect);
    diskfile_readonly (name2, protect);
    DISK_reinsert (num);
    return 1;
}

void disk_eject (int num)
{
    gui_filename (num, "");
    drive_eject (floppy + num);
    *currprefs.df[num] = *changed_prefs.df[num] = 0;
    floppy[num].newname[0] = 0;
    update_drive_gui (num);
}

int DISK_history_add (const char *name, int idx)
{
    int i;

    if (idx >= MAX_PREVIOUS_FLOPPIES)
	return 0;
    if (name == NULL) {
	dfxhistory[idx][0] = 0;
	return 1;
    }
  if (name[0] == 0)
	  return 0;
  if (!zfile_exists (name))
	  return 0;
  if (idx >= 0) {
  	if (idx >= MAX_PREVIOUS_FLOPPIES)
	    return 0;
	    dfxhistory[idx][0] = 0;
	    for (i = 0; i < MAX_PREVIOUS_FLOPPIES; i++) {
	      if (!strcmp (dfxhistory[i], name))
		      return 0;
      }
	  strcpy (dfxhistory[idx], name);
	  return 1;
  }
  for (i = 0; i < MAX_PREVIOUS_FLOPPIES; i++) {
	  if (!strcmp (dfxhistory[i], name)) {
	    while (i < MAX_PREVIOUS_FLOPPIES - 1) {
		    strcpy (dfxhistory[i], dfxhistory[i + 1]);
		    i++;
	    }
	    dfxhistory[MAX_PREVIOUS_FLOPPIES - 1][0] = 0;
	    break;
  	}
  }
  for (i = MAX_PREVIOUS_FLOPPIES - 2; i >= 0; i--)
  	strcpy (dfxhistory[i + 1], dfxhistory[i]);
  strcpy (dfxhistory[0], name);
  return 1;
}

char *DISK_history_get (int idx)
{
  if (idx >= MAX_PREVIOUS_FLOPPIES)
  	return 0;
  return dfxhistory[idx];
}

static void disk_insert_2 (int num, const char *name, int forced)
{
  	drive *drv = floppy + num;
    if (forced) {
	drive_insert (drv, &currprefs, num, name);
	return;
    }
    if (!strcmp (currprefs.df[num], name))
    	return;
    strcpy (drv->newname, name);
    strcpy (currprefs.df[num], name);
    DISK_history_add (name, -1);
    	if (name[0] == 0) {
		disk_eject (num);
    	} else if (!drive_empty(drv) || drv->dskchange_time > 0) {
		drive_eject (drv);
		/* set dskchange_time, disk_insert() will be
		 * called from DISK_check_change() after 2 second delay
	 	* this makes sure that all programs detect disk change correctly
	 	*/
	  setdskchangetime (drv, 20);
    	} else {
	  setdskchangetime (drv, 1);
    	}
}

void disk_insert (int num, const char *name)
{
    disk_insert_2 (num, name, 0);
}

void DISK_check_change (void)
{
    int i;

    if (currprefs.floppy_speed != changed_prefs.floppy_speed)
	currprefs.floppy_speed = changed_prefs.floppy_speed;
    for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
	drive *drv = floppy + i;
	gui_lock ();
	if (currprefs.dfxtype[i] != changed_prefs.dfxtype[i]) {
	    currprefs.dfxtype[i] = changed_prefs.dfxtype[i];
	    reset_drive (i);
	}
	if (drv->dskchange_time == 0 && strcmp (currprefs.df[i], changed_prefs.df[i]))
	    disk_insert (i, changed_prefs.df[i]);
	gui_unlock ();
	if (drv->dskready_down_time > 0)
	    drv->dskready_down_time--;
	/* emulate drive motor turn on time */
	if (drv->dskready_time && !drive_empty(drv)) {
	    drv->dskready_time--;
	    if (drv->dskready_time == 0)
		drv->dskready = 1;
	}
	/* delay until new disk image is inserted */
	if (drv->dskchange_time) {
	    drv->dskchange_time--;
	    if (drv->dskchange_time == 0) {
		drive_insert (drv, &currprefs, i, drv->newname);
		update_drive_gui (i);
	    }
	}
    }
}

int disk_empty (int num)
{
    return drive_empty (floppy + num);
}

void DISK_select (uae_u8 data)
{
    int step_pulse, lastselected, dr;
    static uae_u8 prevdata;
    static int step;

    lastselected = selected;
    selected = (data >> 3) & 15;
    side = 1 - ((data >> 2) & 1);

    direction = (data >> 1) & 1;
    step_pulse = data & 1;

    if ((prevdata & 0x80) != (data & 0x80)) {
	for (dr = 0; dr < 4; dr++) {
	    if (floppy[dr].indexhackmode > 1 && !(selected & (1 << dr))) {
		floppy[dr].indexhack = 1;
	    }
	}
    }

    if (step != step_pulse) {
	step = step_pulse;
	if (step && !savestate_state) {
	    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		if (!((selected | disabled) & (1 << dr))) {
		    drive_step (floppy + dr);
		    if (floppy[dr].indexhackmode > 1 && (data & 0x80))
			floppy[dr].indexhack = 1;
		}
	    }
	}
    }
    if (!savestate_state) {
	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	    drive *drv = floppy + dr;
	    /* motor on/off workings tested with small assembler code on real Amiga 1200. */
	    /* motor/id flipflop is set only when drive select goes from high to low */
	    if (!(selected & (1 << dr)) && (lastselected & (1 << dr)) ) {
		drv->drive_id_scnt++;
		drv->drive_id_scnt &= 31;
		drv->idbit = (drv->drive_id & (1L << (31 - drv->drive_id_scnt))) ? 1 : 0;
		if (!(disabled & (1 << dr))) {
		    if ((prevdata & 0x80) == 0 || (data & 0x80) == 0) {
			/* motor off: if motor bit = 0 in prevdata or data -> turn motor on */
			drive_motor (drv, 0);
		    } else if (prevdata & 0x80) {
			    /* motor on: if motor bit = 1 in prevdata only (motor flag state in data has no effect)
			       -> turn motor off */
			    drive_motor (drv, 1);
		    }
    }
		if (!currprefs.cs_df0idhw && dr == 0)
		    drv->idbit = 0;
		    }
	    }
	}

    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	floppy[dr].state = (!(selected & (1 << dr))) | !floppy[dr].motoroff;
	update_drive_gui (dr);
    }
    prevdata = data;
}

uae_u8 DISK_status (void)
{
    uae_u8 st = 0x3c;
    int dr;

    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	drive *drv = floppy + dr;
    if (!((selected | disabled) & (1 << dr))) {
	    if (drive_running (drv)) {
    {
  		if (drv->dskready && !drv->indexhack && currprefs.dfxtype[dr] != DRV_35_DD_ESCOM)
		    st &= ~0x20;
		}
	    } else {
		/* report drive ID */
		if (drv->idbit && currprefs.dfxtype[dr] != DRV_35_DD_ESCOM)
		    st &= ~0x20;
		/* dskrdy needs some cycles after switching the motor off.. (Pro Tennis Tour) */
		if (drv->motordelay) {
		    st &= ~0x20;
		    drv->motordelay = 0;
		}
	    }
	    if (drive_track0 (drv))
		st &= ~0x10;
	    if (drive_writeprotected (drv))
		st &= ~8;
	  if (drv->dskchange && currprefs.dfxtype[dr] != DRV_525_SD) {
		  st &= ~4;
	  }
	} else if (!(selected & (1 << dr))) {
	    if (drv->idbit)
		st &= ~0x20;
	}
    }
    return st;
}

static int unformatted (drive *drv)
{
    int tr = drv->cyl * 2 + side;
    if (tr >= drv->num_tracks)
	return 1;
    if (drv->filetype == ADF_EXT2 && drv->trackdata[tr].bitlen == 0)
	return 1;
    return 0;
}

/* get one bit from MFM bit stream */
STATIC_INLINE uae_u32 getonebit (uae_u16 * mfmbuf, int mfmpos)
{
    uae_u16 *buf;

    buf = &mfmbuf[mfmpos >> 4];
    return (buf[0] & (1 << (15 - (mfmpos & 15)))) ? 1 : 0;
}

static void disk_dmafinished (void)
{
    INTREQ (0x8000 | 0x0002);
    longwritemode = 0;
    dskdmaen = 0;
}

static void fetchnextrevolution (drive *drv)
{
    drv->trackspeed = get_floppy_speed2 (drv);
    if (!drv->multi_revolution)
	return;
}

void DISK_handler (uae_u32 data)
{
    int flag = diskevent_flag;

    event2_remevent(ev2_disk);
	  DISK_update (disk_sync_cycle);
    if (flag & (DISK_REVOLUTION << 0))
      fetchnextrevolution (&floppy[0]);
    if (flag & (DISK_REVOLUTION << 1))
      fetchnextrevolution (&floppy[1]);
    if (flag & (DISK_REVOLUTION << 2))
      fetchnextrevolution (&floppy[2]);
    if (flag & (DISK_REVOLUTION << 3))
      fetchnextrevolution (&floppy[3]);
    if (flag & DISK_WORDSYNC)
      INTREQ (0x8000 | 0x1000);
    if (flag & DISK_INDEXSYNC)
    	cia_diskindex ();
}

static void disk_doupdate_write (drive * drv, int floppybits)
{
    int dr;
    int drives[4];
    
    for (dr = 0; dr < MAX_FLOPPY_DRIVES ; dr++) {
        drive *drv2 = &floppy[dr];
        drives[dr] = 0;
        if (drv2->motoroff)
	        continue;
        if (selected & (1 << dr))
	        continue;
	      drives[dr] = 1;
    }

    while (floppybits >= drv->trackspeed) {
	    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	        if (drives[dr]) {
		    floppy[dr].mfmpos++;
		    floppy[dr].mfmpos %= drv->tracklen;
	        }
	    }
	    if ((dmacon & 0x210) == 0x210 && dskdmaen == 3 && dsklength > 0) {
	      bitoffset++;
	      bitoffset &= 15;
	      if (!bitoffset) {
		      for (dr = 0; dr < MAX_FLOPPY_DRIVES ; dr++) {
		          drive *drv2 = &floppy[dr];
              uae_u16 w = get_word (dskpt);
		          if (drives[dr]) {
			          drv2->bigmfmbuf[drv2->mfmpos >> 4] = w;
          			drv2->bigmfmbuf[(drv2->mfmpos >> 4) + 1] = 0x5555;
      		    }
		      }
		      dskpt += 2;
		      dsklength--;
		      if (dsklength == 0) {
		        disk_dmafinished ();
		        for (dr = 0; dr < MAX_FLOPPY_DRIVES ; dr++) {
			        drive *drv2 = &floppy[dr];
			        if (drives[dr])
			          drive_write_data (drv2);
		        }
		      }
	      }
	    }
	    floppybits -= drv->trackspeed;
    }
}

static void updatetrackspeed (drive *drv, int mfmpos)
{
  if (dskdmaen < 3) {
    uae_u16 *p = drv->tracktiming;
    p += mfmpos / 8;
	  drv->trackspeed = get_floppy_speed2 (drv);
    drv->trackspeed = drv->trackspeed * p[0] / 1000;
  	if (drv->trackspeed < 700 || drv->trackspeed > 3000) {
	    drv->trackspeed = 1000;
  	}
  }
}

static void disk_doupdate_predict (drive * drv, int startcycle)
{
    int is_sync = 0;
    int firstcycle = startcycle;
    uae_u32 tword = word;
    int mfmpos = drv->mfmpos;
    int indexhack = drv->indexhack;

    diskevent_flag = 0;
    while (startcycle < (maxhpos << 8) && !diskevent_flag) {
	int cycle = startcycle >> 8;
	if (drv->tracktiming[0])
	    updatetrackspeed (drv, mfmpos);
	if (dskdmaen != 3) {
	    tword <<= 1;
	    if (!drive_empty (drv)) {
	      if (unformatted (drv))
		      tword |= (uaerand() & 0x1000) ? 1 : 0;
	      else
	        tword |= getonebit (drv->bigmfmbuf, mfmpos);
	    }
	    if ((tword & 0xffff) == dsksync)
		    diskevent_flag |= DISK_WORDSYNC;
	}
	mfmpos++;
	mfmpos %= drv->tracklen;
	if (mfmpos == 0)
	    diskevent_flag |= DISK_REVOLUTION << (drv - floppy);
	if (mfmpos == drv->indexoffset) {
	    diskevent_flag |= DISK_INDEXSYNC;
	    indexhack = 0;
	}   
	if (dskdmaen != 3 && mfmpos == drv->skipoffset) {
      int skipcnt = disk_jitter;
	    while (skipcnt-- > 0) {
		    mfmpos++;
		    mfmpos %= drv->tracklen;
		    if (mfmpos == 0)
		        diskevent_flag |= DISK_REVOLUTION << (drv - floppy);
		    if (mfmpos == drv->indexoffset) {
		        diskevent_flag |= DISK_INDEXSYNC;
		        indexhack = 0;
		    }
	    }
	}
	startcycle += drv->trackspeed;
    }
    if (drv->tracktiming[0])
        updatetrackspeed (drv, drv->mfmpos);
    if (diskevent_flag) {
	      disk_sync_cycle = startcycle >> 8;
        event2_newevent(ev2_disk, (startcycle - firstcycle) / CYCLE_UNIT);
    }
}

static void disk_doupdate_read_nothing (int floppybits)
{
    int j = 0, k = 1, l = 0;

    while (floppybits >= get_floppy_speed()) {
	word <<= 1;
	if (bitoffset == 15 && dma_enable && dskdmaen == 2 && dsklength >= 0) {
	    if (dsklength > 0) {
		put_word (dskpt, word);
		dskpt += 2;
	    }
	    dsklength--;
	    if (dsklength <= 0)
		disk_dmafinished ();
	}
	if ((bitoffset & 7) == 7) {
	    dskbytr_val = word & 0xff;
	    dskbytr_val |= 0x8000;
	}
	bitoffset++;
	bitoffset &= 15;
	floppybits -= get_floppy_speed();
    }
}

static void disk_doupdate_read (drive * drv, int floppybits)
{
  int j = 0, k = 1, l = 0;

  while (floppybits >= drv->trackspeed) {
      if (drv->tracktiming[0])
    updatetrackspeed (drv, drv->mfmpos);
      word <<= 1;
      if (!drive_empty (drv)) {
        if (unformatted (drv))
		      word |= (uaerand() & 0x1000) ? 1 : 0;
        else
    	    word |= getonebit (drv->bigmfmbuf, drv->mfmpos);
      }
	//write_log ("%08.8X bo=%d mfmpos=%d dma=%d\n", (word & 0xffffff), bitoffset, drv->mfmpos,dma_enable);
	drv->mfmpos++;
	drv->mfmpos %= drv->tracklen;
	if (drv->mfmpos == drv->indexoffset) {
	    drv->indexhack = 0;
	}
	if (drv->mfmpos == drv->skipoffset) {
	    drv->mfmpos += disk_jitter;
	    drv->mfmpos %= drv->tracklen;
	}
  if (bitoffset == 15 && dma_enable && dskdmaen == 2 && dsklength >= 0) {
	  if (dsklength > 0) {
		  put_word (dskpt, word);
		  dskpt += 2;
	  }
	    dsklength--;
	    if (dsklength <= 0)
	        disk_dmafinished ();
	}
	if ((bitoffset & 7) == 7) {
	    dskbytr_val = word & 0xff;
	    dskbytr_val |= 0x8000;
	}
	if (word == dsksync) {
	    if (adkcon & 0x400)
	        bitoffset = 15;
	    if (dskdmaen) {
		    dma_enable = 1;
	    }
	}
	bitoffset++;
	bitoffset &= 15;
	floppybits -= drv->trackspeed;
    }
}

/* this is very unoptimized. DSKBYTR is used very rarely, so it should not matter. */

uae_u16 DSKBYTR (int hpos)
{
    uae_u16 v;

    DISK_update (hpos);
    v = dskbytr_val;
    dskbytr_val &= ~0x8000;
    if (word == dsksync)
	    v |= 0x1000;
    if (dskdmaen && (dmacon & 0x210) == 0x210)
	    v |= 0x4000;
    if (dsklen & 0x4000)
	    v |= 0x2000;
    return v;
}

static void DISK_start (void)
{
    int dr;

    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	drive *drv = &floppy[dr];
	if (!(selected & (1 << dr))) {
	    int tr = drv->cyl * 2 + side;
	    trackid *ti = drv->trackdata + tr;

	    if (dskdmaen == 3) {
		drv->tracklen = longwritemode ? FLOPPY_WRITE_MAXLEN : FLOPPY_WRITE_LEN * drv->ddhd * 8 * 2;
		drv->trackspeed = get_floppy_speed();
		drv->skipoffset = -1;
		updatemfmpos (drv);
	    }
	    /* Ugh.  A nasty hack.  Assume ADF_EXT1 tracks are always read
	       from the start.  */
	    if (ti->type == TRACK_RAW1)
		drv->mfmpos = 0;
	}
        drv->floppybitcounter = 0;
    }
    dma_enable = (adkcon & 0x400) ? 0 : 1;
}

static int linecounter;

void DISK_hsync (int tohpos)
{
    int dr;

    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	drive *drv = &floppy[dr];
	if (drv->steplimit)
	    drv->steplimit--;
    }
    if (linecounter) {
	linecounter--;
	if (! linecounter)
	    disk_dmafinished ();
	return;
    }
    DISK_update (tohpos);
}

void DISK_update (int tohpos)
{
    int dr;
    int cycles = (tohpos << 8) - disk_hpos;
    int startcycle = disk_hpos;
    int didread;

    disk_jitter = ((uaerand () >> 4) & 3) + 1;
    if (disk_jitter > 2)
    	disk_jitter = 1;
    if (cycles <= 0)
	return;
    disk_hpos += cycles;
    if (disk_hpos >= (maxhpos << 8))
	disk_hpos -= maxhpos << 8;

    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	    drive *drv = &floppy[dr];
	    if (drv->motoroff)
	        continue;
            drv->floppybitcounter += cycles;
	    if (selected & (1 << dr)) {
	        drv->mfmpos += drv->floppybitcounter / drv->trackspeed;
	        drv->mfmpos %= drv->tracklen;
          drv->floppybitcounter %= drv->trackspeed;
	        continue;
	    }
	    drive_fill_bigbuf (drv, 0);
	    drv->mfmpos %= drv->tracklen;
    }
    didread = 0;
    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	    drive *drv = &floppy[dr];
	    if (drv->motoroff)
	        continue;
	    if (selected & (1 << dr))
	        continue;
	    if (dskdmaen == 3)
	        disk_doupdate_write (drv, drv->floppybitcounter);
	    else
	        disk_doupdate_read (drv, drv->floppybitcounter);
	    disk_doupdate_predict (drv, disk_hpos);
      drv->floppybitcounter %= drv->trackspeed;
    	didread = 1;
	    break;
    }
     /* no floppy selected but read dma */
    if (!didread && dskdmaen == 2) {
	disk_doupdate_read_nothing (cycles);
    }

}

void DSKLEN (uae_u16 v, int hpos)
{
    int dr, prev = dsklen;
    int noselected = 0;
    int motormask;

    DISK_update (hpos);
    if ((v & 0x8000) && (dsklen & 0x8000)) {
	    dskdmaen = 2;
      DISK_start ();
    }
    if (!(v & 0x8000)) {
	    if (dskdmaen) {
	    /* Megalomania and Knightmare does this */
	      dskdmaen = 0;
      }
    }
    dsklen = v;
    dsklength2 = dsklength = dsklen & 0x3fff;

    if (dskdmaen == 0)
	    return;

    if ((v & 0x4000) && (prev & 0x4000)) {
	    if (dsklength == 0)
	      return;
	    if (dsklength == 1) {
	        disk_dmafinished();
	        return;
	    }
	    dskdmaen = 3;
	    DISK_start ();
    }

    if (dsklength == 1)
	dsklength = 0;

    motormask = 0;
    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
      drive *drv = &floppy[dr];
      if (drv->motoroff)
  	    continue;
	    motormask |= 1 << dr;
	    if ((selected & (1 << dr)) == 0)
	      break;
    }
    if (dr == 4) {
	    noselected = 1;
    }

    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++)
	update_drive_gui (dr);

    /* Try to make floppy access from Kickstart faster.  */
    if (dskdmaen != 2 && dskdmaen != 3)
	    return;
    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
        drive *drv = &floppy[dr];
        if (selected & (1 << dr))
	        continue;
	      if (drv->filetype != ADF_NORMAL)
	      break;
    }
    if (dr < MAX_FLOPPY_DRIVES) /* no turbo mode if any selected drive has non-standard ADF */
	    return;
    {
  	int done = 0;
	  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	    drive *drv = &floppy[dr];
	    int pos, i;

	    if (drv->motoroff)
		    continue;
	    if (!drv->useturbo && currprefs.floppy_speed > 0)
		    continue;
	    if (selected & (1 << dr))
		    continue;

	    pos = drv->mfmpos & ~15;
		  drive_fill_bigbuf (drv, 0);

	    if (dskdmaen == 2) { /* TURBO read */
		    if (adkcon & 0x400) {
		      for (i = 0; i < drv->tracklen; i += 16) {
			      pos += 16;
			      pos %= drv->tracklen;
			      if (drv->bigmfmbuf[pos >> 4] == dsksync) {
			        /* must skip first disk sync marker */
			        pos += 16;
			        pos %= drv->tracklen;
			        break;
			      }
		      }
		      if (i >= drv->tracklen)
			      return;
		    }
		    while (dsklength-- > 0) {
		      put_word (dskpt, drv->bigmfmbuf[pos >> 4]);
		      dskpt += 2;
		      pos += 16;
		      pos %= drv->tracklen;
		    }
		    INTREQ_f (0x8000 | 0x1000);
		    done = 1;

	    } else if (dskdmaen == 3) { /* TURBO write */

		    for (i = 0; i < dsklength; i++) {
          uae_u16 w = get_word (dskpt + i * 2);
		      drv->bigmfmbuf[pos >> 4] = w;
		      pos += 16;
		      pos %= drv->tracklen;
		    }
		    drive_write_data (drv);
		    done = 1;
	    }
	  }
	if (!done && noselected) {
	    while (dsklength-- > 0) {
		    if (dskdmaen == 3) {
		        uae_u16 w = get_word (dskpt);
    		} else {
	        put_word (dskpt, 0);
		    }
	        dskpt += 2;
	    }
	    INTREQ_f (0x8000 | 0x1000);
	    done = 1;
	}

	  if (done) {
		  linecounter = 2;
		  dskdmaen = 0;
		  return;
	  }
  }
}

void DSKSYNC (int hpos, uae_u16 v)
{
    if (v == dsksync)
    	return;
    DISK_update (hpos);
    dsksync = v;
}

void DSKPTH (uae_u16 v)
{
    dskpt = (dskpt & 0xffff) | ((uae_u32) v << 16);
}

void DSKPTL (uae_u16 v)
{
    dskpt = (dskpt & ~0xffff) | (v);
}

void DISK_free (void)
{
    int dr;
    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	    drive *drv = &floppy[dr];
	    drive_image_free (drv);
    }
}

void DISK_init (void)
{
  int dr;

  longwritemode = side = direction = 0;
  diskevent_flag = 0;
  disk_sync_cycle = 0;
  dsklength = dsklength2 = dsklen = 0;
  dskbytr_val = 0;
  dskpt = 0;
  dma_enable =0;
  bitoffset = word = 0;
  dsksync = disk_jitter = linecounter = 0;
  selected = 15;

  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
  	drive *drv = &floppy[dr];
	  /* reset all drive types to 3.5 DD */
	  drive_settype_id (drv);
	  if (!drive_insert (drv, &currprefs, dr, currprefs.df[dr]))
	    disk_eject (dr);
  }
}

void DISK_reset (void)
{
    int i;

    if (savestate_state)
	    return;

    disk_hpos = 0;
    dskdmaen = 0;
    disabled = 0;
    for (i = 0; i < MAX_FLOPPY_DRIVES; i++)
	    reset_drive (i);
}

int DISK_examine_image (struct uae_prefs *p, int num, uae_u32 *crc32)
{
    int drvsec;
    int ret, i;
    drive *drv = &floppy[num];
    uae_u32 dos, crc, crc2;
    int wasdelayed = drv->dskchange_time;

    ret = 0;
    drv->cyl = 0;
    side = 0;
    *crc32 = 0;
    if (!drive_insert (drv, p, num, p->df[num]))
	return 1;
    if (!drv->diskfile)
	return 1;
    *crc32 = zfile_crc32 (drv->diskfile);
    if (decode_buffer (drv->bigmfmbuf, drv->cyl, 11, drv->ddhd, drv->filetype, &drvsec, 1)) {
	ret = 2;
	goto end;
    }
    crc = crc2 = 0;
    for (i = 0; i < 1024; i += 4) {
	uae_u32 v = (writebuffer[i] << 24) | (writebuffer[i + 1] << 16) | (writebuffer[i + 2] << 8) | writebuffer[i + 3];
	if (i == 0)
	    dos = v;
	if (i == 4) {
	    crc2 = v;
	    v = 0;
	}
	if (crc + v < crc)
	    crc++;
	crc += v;
    }
    if (dos == 0x4b49434b) { /* KICK */
	ret = 10;
	goto end;
    }
    crc ^= 0xffffffff;
    if (crc != crc2) {
	ret = 3;
	goto end;
    }
    if (dos == 0x444f5300)
	ret = 10;
    else if (dos == 0x444f5301 || dos == 0x444f5302 || dos == 0x444f5303)
	ret = 11;
    else if (dos == 0x444f5304 || dos == 0x444f5305 || dos == 0x444f5306 || dos == 0x444f5307)
	ret = 12;
    else
	ret = 4;
end:
    drive_image_free (drv);
    if (wasdelayed > 1) {
	drive_eject (drv);
	currprefs.df[num][0] = 0;
	drv->dskchange_time = wasdelayed;
	disk_insert (num, drv->newname);
    }
    return ret;
}

/* Disk save/restore code */

#if defined SAVESTATE || defined DEBUGGER

void DISK_save_custom (uae_u32 *pdskpt, uae_u16 *pdsklength, uae_u16 *pdsksync, uae_u16 *pdskbytr)
{
    if (pdskpt) *pdskpt = dskpt;
    if (pdsklength) *pdsklength = dsklength;
    if (pdsksync) *pdsksync = dsksync;
    if(pdskbytr) *pdskbytr = dskbytr_val;
}

#endif /* SAVESTATE || DEBUGGER */

#ifdef SAVESTATE

void DISK_restore_custom (uae_u32 pdskpt, uae_u16 pdsklength, uae_u16 pdskbytr)
{
    dskpt = pdskpt;
    dsklength = pdsklength;
    dskbytr_val = pdskbytr;
}

void restore_disk_finish (void)
{
}

uae_u8 *restore_disk(int num,uae_u8 *src)
{
    drive *drv;
    int state;
    char old[MAX_DPATH];
    int newis;
    drive_type dfxtype;

    drv = &floppy[num];
    disabled &= ~(1 << num);
    drv->drive_id = restore_u32 ();
    drv->motoroff = 1;
    drv->idbit = 0;
    state = restore_u8 ();
    if (state & 2) {
       	disabled |= 1 << num;
      	if (changed_prefs.nr_floppies > num)
      	    changed_prefs.nr_floppies = num;
      	changed_prefs.dfxtype[num] = (drive_type) -1;
    } else {
       		drv->motoroff = (state & 1) ? 0 : 1;
			drv->idbit = (state & 4) ? 1 : 0;
	    if (changed_prefs.nr_floppies < num)
	      changed_prefs.nr_floppies = num;
  	  switch (drv->drive_id)
  	  {
  	    case DRIVE_ID_35HD:
  	    dfxtype = DRV_35_HD;
  	    break;
  	    case DRIVE_ID_525SD:
  	    dfxtype = DRV_525_SD;
  	    break;
  	    default:
  	    dfxtype = DRV_35_DD;
  	    break;
  	  }
	    changed_prefs.dfxtype[num] = dfxtype;
		}
    drv->indexhackmode = 0;
    if (num == 0 && currprefs.dfxtype[num] == 0)
	drv->indexhackmode = 1;
    drv->buffered_cyl = -1;
    drv->buffered_side = -1;
    drv->cyl = restore_u8 ();
    drv->dskready = restore_u8 ();
    drv->drive_id_scnt = restore_u8 ();
    drv->mfmpos = restore_u32 ();
    drv->dskchange = 0;
    drv->dskchange_time = 0;
    restore_u32 ();
    strcpy (old, currprefs.df[num]);
    strncpy(changed_prefs.df[num], (char *)src,255);
    newis = changed_prefs.df[num][0] ? 1 : 0;
    src+=strlen((char *)src)+1;
    if (!(disabled & (1 << num))) {
	if (!newis) {
	    drv->dskchange = 1;
	} else {
      drive_insert (floppy + num, &currprefs, num, changed_prefs.df[num]);
      if (drive_empty (floppy + num)) {
  	if (newis && old[0]) {
  	    strcpy (changed_prefs.df[num], old);
  	    drive_insert (floppy + num, &currprefs, num, changed_prefs.df[num]);
  	    if (drive_empty (floppy + num))
  		drv->dskchange = 1;
          }
      }
	}
    }
    reset_drive_gui(num);
    return src;
}

static uae_u32 getadfcrc (drive *drv)
{
    uae_u8 *b;
    uae_u32 crc32;
    int size;

    if (!drv->diskfile)
	return 0;
    zfile_fseek (drv->diskfile, 0, SEEK_END);
    size = zfile_ftell (drv->diskfile);
    b = (uae_u8 *)malloc (size);
    if (!b)
	return 0;
    zfile_fseek (drv->diskfile, 0, SEEK_SET);
    zfile_fread (b, 1, size, drv->diskfile);
    crc32 = get_crc32 (b, size);
    free (b);
    return crc32;
}

uae_u8 *save_disk(int num, int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak,*dst;
    drive *drv;

    drv = &floppy[num];
    if (dstptr)
    	dstbak = dst = dstptr;
    else
      dstbak = dst = (uae_u8 *)malloc (2+1+1+1+1+4+4+256);
    save_u32 (drv->drive_id);	    /* drive type ID */
    save_u8 ((drv->motoroff ? 0:1) | ((disabled & (1 << num)) ? 2 : 0) | (drv->idbit ? 4 : 0) | (drv->dskchange ? 8 : 0));
    save_u8 (drv->cyl);		    /* cylinder */
    save_u8 (drv->dskready);	    /* dskready */
    save_u8 (drv->drive_id_scnt);   /* id mode position */
    save_u32 (drv->mfmpos);	    /* disk position */
    save_u32 (getadfcrc (drv));	    /* CRC of disk image */
    strcpy ((char *)dst, currprefs.df[num]);/* image name */
    dst += strlen((char *)dst) + 1;

    *len = dst - dstbak;
    return dstbak;
}

/* internal floppy controller variables */

uae_u8 *restore_floppy(uae_u8 *src)
{
    word = restore_u16();
    bitoffset = restore_u8();
    dma_enable = restore_u8();
    disk_hpos = restore_u8() << 8;
    dskdmaen = restore_u8();
    restore_u16(); 
    //word |= restore_u16() << 16;

    return src;
}

uae_u8 *save_floppy(int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak, *dst;

    if (dstptr)
    	dstbak = dst = dstptr;
    else
      dstbak = dst = (uae_u8 *)malloc(2+1+1+1+1+2);
    save_u16 (word);		/* current fifo (low word) */
    save_u8 (bitoffset);	/* dma bit offset */
    save_u8 (dma_enable);	/* disk sync found */
    save_u8 (disk_hpos >> 8);	/* next bit read position */
    save_u8 (dskdmaen);		/* dma status */
    save_u16 (0);		/* was current fifo (high word), but it was wrong???? */

    *len = dst - dstbak;
    return dstbak;
}

#endif /* SAVESTATE */
