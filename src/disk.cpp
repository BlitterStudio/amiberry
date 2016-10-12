 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Floppy disk emulation
  *
  * Copyright 1995 Hannu Rummukainen
  * Copyright 1995-2001 Bernd Schmidt
  * Copyright 2000-2003 Toni Wilen
  *
  * Original High Density Drive Handling by Dr. Adil Temel (C) 2001 [atemel1@hotmail.com]
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#define MFM_VALIDATOR 0

#include "uae.h"
#include "options.h"
#include "include/memory.h"
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
#include "fsdb.h"

static int longwritemode = 0;

/* support HD floppies */
#define FLOPPY_DRIVE_HD
/* writable track length with normal 2us bitcell/300RPM motor, 12667 PAL, 12797 NTSC */
#define FLOPPY_WRITE_LEN (currprefs.floppy_write_length > 256 ? currprefs.floppy_write_length / 2 : (currprefs.ntscmode ? (12798 / 2) : (12668 / 2)))
#define FLOPPY_WRITE_MAXLEN 0x3800
/* This works out to 350 */
#define FLOPPY_GAP_LEN (FLOPPY_WRITE_LEN - 11 * 544)
/* (cycles/bitcell) << 8, normal = ((2us/280ns)<<8) = ~1828.5714 */
#define NORMAL_FLOPPY_SPEED (currprefs.ntscmode ? 1812 : 1829)
/* max supported floppy drives, for small memory systems */
#define MAX_FLOPPY_DRIVES 4

#ifdef FLOPPY_DRIVE_HD
#define DDHDMULT 2
#else
#define DDHDMULT 1
#endif
#define MAX_SECTORS (DDHDMULT * 11)

/* UAE-1ADF (ADF_EXT2)
 * W	reserved
 * W	number of tracks (default 2*80=160)
 *
 * W	reserved
 * W type, 0=normal AmigaDOS track, 1 = raw MFM (upper byte = disk revolutions - 1)
 * L	available space for track in bytes (must be even)
 * L	track length in bits
 */

static int side, direction;
static uae_u8 selected = 15, disabled = 0;

static uae_u8 writebuffer[544 * MAX_SECTORS];

#define DISK_INDEXSYNC 1
#define DISK_WORDSYNC 2
#define DISK_REVOLUTION 4 /* 8,16,32,64 */

#define DSKREADY_UP_TIME 18
#define DSKREADY_DOWN_TIME 45

#define DSKDMA_OFF 0
#define DSKDMA_INIT 1
#define DSKDMA_READ 2
#define DSKDMA_WRITE 3

static int dskdmaen, dsklength, dsklength2, dsklen;
static uae_u16 dskbytr_val;
static uae_u32 dskpt;
static bool fifo_filled;
static uae_u16 fifo[3];
static int fifo_inuse[3];
static int dma_enable, bitoffset;
static uae_u16 word, dsksync;
static unsigned long dsksync_cycles;
#define WORDSYNC_TIME 11
/* Always carried through to the next line.  */
int disk_hpos;
static int disk_jitter;
static int indexdecay;

static uae_u8 prev_data;
static int prev_step;

typedef enum { TRACK_AMIGADOS, TRACK_RAW, TRACK_RAW1, TRACK_PCDOS, TRACK_DISKSPARE, TRACK_NONE } image_tracktype;
typedef struct {
  uae_u16 len;
  uae_u32 offs;
  int bitlen, track;
  uae_u16 sync;
  image_tracktype type;
	int revolutions;
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

typedef enum { ADF_NONE = -1, ADF_NORMAL, ADF_EXT1, ADF_EXT2, ADF_FDI, ADF_IPF, ADF_SCP, ADF_CATWEASEL, ADF_PCDOS, ADF_KICK, ADF_SKICK } drive_filetype;
typedef struct {
  struct zfile *diskfile;
  struct zfile *writediskfile;
  drive_filetype filetype;
  trackid trackdata[MAX_TRACKS];
  trackid writetrackdata[MAX_TRACKS];
  int buffered_cyl, buffered_side;
  int cyl;
  bool motoroff;
  int motordelay; /* dskrdy needs some clock cycles before it changes after switching off motor */
  bool state;
  bool wrprot;
	bool forcedwrprot;
  uae_u16 bigmfmbuf[0x4000 * DDHDMULT];
  uae_u16 tracktiming[0x4000 * DDHDMULT];
  int multi_revolution;
	int revolution_check;
  int skipoffset;
  int mfmpos;
  int indexoffset;
  int tracklen;
  int revolutions;
  int prevtracklen;
  int trackspeed;
  int num_tracks, write_num_tracks, num_secs;
  int hard_num_cyls;
  bool dskchange;
  int dskchange_time;
  bool dskready;
  int dskready_up_time;
  int dskready_down_time;
  int writtento;
  int steplimit;
  frame_time_t steplimitcycle;
  int indexhack, indexhackmode;
  int ddhd; /* 1=DD 2=HD */
  int drive_id_scnt; /* drive id shift counter */
  int idbit;
  unsigned long drive_id; /* drive id to be reported */
  TCHAR newname[256]; /* storage space for new filename during eject delay */
	bool newnamewriteprotected;
  uae_u32 crc32;
  int useturbo;
  int floppybitcounter; /* number of bits left */
	int lastdataacesstrack;
	int lastrev;
	bool track_access_done;
} drive;

#define MIN_STEPLIMIT_CYCLE (CYCLE_UNIT * 140)

static uae_u16 bigmfmbufw[0x4000 * DDHDMULT];
static drive floppy[MAX_FLOPPY_DRIVES];
static TCHAR dfxhistory[2][MAX_PREVIOUS_FLOPPIES][MAX_DPATH];

static uae_u8 exeheader[]={0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00};
static uae_u8 bootblock_ofs[]={
  0x44,0x4f,0x53,0x00,0xc0,0x20,0x0f,0x19,0x00,0x00,0x03,0x70,0x43,0xfa,0x00,0x18,
  0x4e,0xae,0xff,0xa0,0x4a,0x80,0x67,0x0a,0x20,0x40,0x20,0x68,0x00,0x16,0x70,0x00,
  0x4e,0x75,0x70,0xff,0x60,0xfa,0x64,0x6f,0x73,0x2e,0x6c,0x69,0x62,0x72,0x61,0x72,
  0x79
};
static uae_u8 bootblock_ffs[]={
	0x44, 0x4F, 0x53, 0x01, 0xE3, 0x3D, 0x0E, 0x72, 0x00, 0x00, 0x03, 0x70, 0x43, 0xFA, 0x00, 0x3E,
	0x70, 0x25, 0x4E, 0xAE, 0xFD, 0xD8, 0x4A, 0x80, 0x67, 0x0C, 0x22, 0x40, 0x08, 0xE9, 0x00, 0x06,
	0x00, 0x22, 0x4E, 0xAE, 0xFE, 0x62, 0x43, 0xFA, 0x00, 0x18, 0x4E, 0xAE, 0xFF, 0xA0, 0x4A, 0x80,
	0x67, 0x0A, 0x20, 0x40, 0x20, 0x68, 0x00, 0x16, 0x70, 0x00, 0x4E, 0x75, 0x70, 0xFF, 0x4E, 0x75,
	0x64, 0x6F, 0x73, 0x2E, 0x6C, 0x69, 0x62, 0x72, 0x61, 0x72, 0x79, 0x00, 0x65, 0x78, 0x70, 0x61,
	0x6E, 0x73, 0x69, 0x6F, 0x6E, 0x2E, 0x6C, 0x69, 0x62, 0x72, 0x61, 0x72, 0x79, 0x00, 0x00, 0x00,
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

static uae_u32 disk_checksum (uae_u8 *p, uae_u8 *c)
{
  uae_u32 cs = 0;
  int i;
  for (i = 0; i < FS_FLOPPY_BLOCKSIZE; i+= 4) 
    cs += (p[i] << 24) | (p[i+1] << 16) | (p[i+2] << 8) | (p[i+3] << 0);
  cs = -cs;
	if (c) {
    c[0] = cs >> 24; c[1] = cs >> 16; c[2] = cs >> 8; c[3] = cs >> 0;
  }
	return cs;
}

static int dirhash (const uae_char *name)
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
	static int pdays, pmins, pticks;
	int days, mins, ticks;
	struct timeval tv;
	struct mytimeval mtv;

	gettimeofday (&tv, NULL);
	tv.tv_sec -= _timezone;
	mtv.tv_sec = tv.tv_sec;
	mtv.tv_usec = tv.tv_usec;
	timeval_to_amiga (&mtv, &days, &mins, &ticks);
	if (days == pdays && mins == pmins && ticks == pticks) {
		ticks++;
		if (ticks >= 50 * 60) {
			ticks = 0;
			mins++;
			if (mins >= 24 * 60)
				days++;
    }
	}
	pdays = days;
	pmins = mins;
	pticks = ticks;
  p[0] = days >> 24; p[1] = days >> 16; p[2] = days >> 8; p[3] = days >> 0;
	p[4] = mins >> 24; p[5] = mins >> 16; p[6] = mins >> 8; p[7] = mins >> 0;
  p[8] = ticks >> 24; p[9] = ticks >> 16; p[10] = ticks >> 8; p[11] = ticks >> 0; 
}

static void createbootblock (uae_u8 *sector, int bootable)
{
  memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
  memcpy (sector, "DOS", 3);
  if (bootable)
  	memcpy (sector, bootblock_ofs, sizeof bootblock_ofs);
}

static void createrootblock (uae_u8 *sector, const TCHAR *disk_name)
{
	char *dn = ua (disk_name);
	char *dn2 = dn;
	if (strlen (dn2) >= 30)
		dn2[30] = 0;
	if (dn2[0] == 0)
		dn2 = "empty";
  memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
  sector[0+3] = 2;
  sector[12+3] = 0x48;
  sector[312] = sector[313] = sector[314] = sector[315] = (uae_u8)0xff;
  sector[316+2] = 881 >> 8; sector[316+3] = 881 & 255;
	sector[432] = strlen (dn2);
	strcpy ((char*)sector + 433, dn2);
  sector[508 + 3] = 1;
  disk_date (sector + 420);
  memcpy (sector + 472, sector + 420, 3 * 4);
  memcpy (sector + 484, sector + 420, 3 * 4);
	xfree (dn);
}

static int getblock (uae_u8 *bitmap, int *prev)
{
	int i = *prev;
  while (bitmap[i] != 0xff) {
  	if (bitmap[i] == 0) {
	    bitmap[i] = 1;
			*prev = i;
			return i;
		}
		i++;
	}
	i = 0;
	while (bitmap[i] != 0xff) {
		if (bitmap[i] == 0) {
			bitmap[i] = 1;
			*prev = i;
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

static int createdirheaderblock (uae_u8 *sector, int parent, const char *filename, uae_u8 *bitmap, int *prevblock)
{
	int block = getblock (bitmap, prevblock);

  memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
  pl (sector, 0, 2);
  pl (sector, 4, block);
  disk_date (sector + 512 - 92);
  sector[512 - 80] = strlen (filename);
  strcpy ((char *)sector + 512 - 79, filename);
  pl (sector, 512 - 12, parent);
  pl (sector, 512 - 4, 2);
  return block;
}

static int createfileheaderblock (struct zfile *z,uae_u8 *sector, int parent, const char *filename, struct zfile *src, uae_u8 *bitmap, int *prevblock)
{
  uae_u8 sector2[FS_FLOPPY_BLOCKSIZE];
  uae_u8 sector3[FS_FLOPPY_BLOCKSIZE];
	int block = getblock (bitmap, prevblock);
	int datablock = getblock (bitmap, prevblock);
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
  strcpy ((char *)sector + FS_FLOPPY_BLOCKSIZE - 79, filename);
  pl (sector, FS_FLOPPY_BLOCKSIZE - 12, parent);
  pl (sector, FS_FLOPPY_BLOCKSIZE - 4, -3);
  extensioncounter = 0;
  extensionblock = 0;

  while (size > 0) {
	  int datablock2 = datablock;
	  int extensionblock2 = extensionblock;
	  if (extensioncounter == FS_EXTENSION_BLOCKS) {
	    extensioncounter = 0;
			extensionblock = getblock (bitmap, prevblock);
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
		if (size > 0) datablock = getblock (bitmap, prevblock);
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
  int i, j;
  memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
	i = 0;
	for (;;) {
		uae_u32 mask = 0;
		for (j = 0; j < 32; j++) {
			if (bitmap[2 + i * 32 + j] == 0xff)
				break;
			if (!bitmap[2 + i * 32 + j])
				mask |= 1 << j;
  	}
		sector[4 + i * 4 + 0] = mask >> 24;
		sector[4 + i * 4 + 1] = mask >> 16;
		sector[4 + i * 4 + 2] = mask >>  8;
		sector[4 + i * 4 + 3] = mask >>  0;
		if (bitmap[2 + i * 32 + j] == 0xff)
			break;
		i++;
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
	TCHAR *fname1b = _T("runme.adf");
  char *fname2 = "startup-sequence";
  char *dirname1 = "s";
  struct zfile *ss;
	int prevblock;

  memset (bitmap, 0, sizeof bitmap);
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
	bitmap[1760] = -1;
	prevblock = 880;

	dblock1 = createdirheaderblock (sector2, 880, dirname1, bitmap, &prevblock);
  ss = zfile_fopen_empty (src, fname1b, strlen(fname1));
  zfile_fwrite (fname1, strlen(fname1), 1, ss);
	fblock1 = createfileheaderblock (dst, sector1,  dblock1, fname2, ss, bitmap, &prevblock);
  zfile_fclose (ss);
  pl (sector2, 24 + dirhash (fname2) * 4, fblock1);
  disk_checksum(sector2, sector2 + 20);
  writeimageblock (dst, sector2, dblock1 * FS_FLOPPY_BLOCKSIZE);

	fblock1 = createfileheaderblock (dst, sector1, 880, fname1, src, bitmap, &prevblock);

	createrootblock (sector1, zfile_getfilename (src));
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

static bool isfloppysound (drive *drv)
{
	return drv->useturbo == 0;
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

static TCHAR *drive_id_name (drive *drv)
{
  switch (drv->drive_id)
  {
	  case DRIVE_ID_35HD : return _T("3.5HD");
	  case DRIVE_ID_525SD: return _T("5.25SD");
	  case DRIVE_ID_35DD : return _T("3.5DD");
	  case DRIVE_ID_NONE : return _T("NONE");
  }
	return _T("UNKNOWN");
}

/* Simulate exact behaviour of an A3000T 3.5 HD disk drive.
 * The drive reports to be a 3.5 DD drive whenever there is no
 * disk or a 3.5 DD disk is inserted. Only 3.5 HD drive id is reported
 * when a real 3.5 HD disk is inserted. -Adil
 */
static void drive_settype_id (drive *drv)
{
  int t = currprefs.floppyslots[drv - &floppy[0]].dfxtype;

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
  drv->filetype = ADF_NONE;
  zfile_fclose (drv->diskfile);
  drv->diskfile = 0;
  zfile_fclose (drv->writediskfile);
  drv->writediskfile = 0;
}

static int drive_insert (drive * drv, struct uae_prefs *p, int dnum, const TCHAR *fname, bool fake, bool writeprotected);

static void reset_drive_gui(int num)
{
  gui_data.drive_disabled[num] = 0;
  gui_data.df[num][0] = 0;
  gui_data.crc32[num] = 0;
  if (currprefs.floppyslots[num].dfxtype < 0)
  	gui_data.drive_disabled[num] = 1;
}

static void reset_drive(int num)
{
  drive *drv = &floppy[num];

  drive_image_free (drv);
    
  drv->motordelay = 0;
  drv->state = 0;
  drv->wrprot = 0;
  drv->mfmpos = 0;
  drv->tracklen = drv->prevtracklen = 0;
  drv->trackspeed = 0;
  drv->num_tracks = drv->write_num_tracks = drv->write_num_tracks = 0;
  drv->dskready = 0;
  drv->steplimit = drv->steplimitcycle = 0;
  drv->indexhack = 0;
  drv->floppybitcounter = 0;
    
  drv->motoroff = 1;
  drv->idbit = 0;
  drv->drive_id = 0;
  drv->drive_id_scnt = 0;
	drv->lastdataacesstrack = -1;
  disabled &= ~(1 << num);
  if (currprefs.floppyslots[num].dfxtype < 0)
    disabled |= 1 << num;
  reset_drive_gui(num);
  /* most internal Amiga floppy drives won't enable
   * diskready until motor is running at full speed
   * and next indexsync has been passed
   */
  drv->indexhackmode = 0;
  if (num == 0 && currprefs.floppyslots[num].dfxtype == 0)
  	drv->indexhackmode = 1;
  drv->dskchange_time = 0;
  drv->dskchange = false;
  drv->dskready_down_time = 0;
  drv->dskready_up_time = 0;
  drv->buffered_cyl = -1;
  drv->buffered_side = -1;
  gui_led (num + LED_DF0, 0);
  drive_settype_id (drv);
  _tcscpy (currprefs.floppyslots[num].df, changed_prefs.floppyslots[num].df);
  drv->newname[0] = 0;
	drv->newnamewriteprotected = false;
	if (!drive_insert (drv, &currprefs, num, currprefs.floppyslots[num].df, false, false))
    disk_eject (num);
}

/* code for track display */
static void update_drive_gui (int num, bool force)
{
  if (num >= currprefs.nr_floppies) 
    return;
  drive *drv = floppy + num;
  bool writ = dskdmaen == DSKDMA_WRITE && drv->state && !((selected | disabled) & (1 << num));

	if (!force && drv->state == gui_data.drive_motor[num]
	  && drv->cyl == gui_data.drive_track[num]
	  && side == gui_data.drive_side
	  && drv->crc32 == gui_data.crc32[num]
	  && writ == gui_data.drive_writing[num]
	  && !_tcscmp (gui_data.df[num], currprefs.floppyslots[num].df))
	  return;
  _tcscpy (gui_data.df[num], currprefs.floppyslots[num].df);
  gui_data.crc32[num] = drv->crc32;
  gui_data.drive_motor[num] = drv->state;
  gui_data.drive_track[num] = drv->cyl;
  gui_data.drive_side = side;
  gui_data.drive_writing[num] = writ;
  gui_led (num + LED_DF0, (gui_data.drive_motor[num] ? 1 : 0) | (gui_data.drive_writing[num] ? 2 : 0));
}

static void drive_fill_bigbuf (drive *drv, int);

int DISK_validate_filename (struct uae_prefs *p, const TCHAR *fname, int leave_open, bool *wrprot, uae_u32 *crc32, struct zfile **zf)
{
	if (zf)
		*zf = NULL;
  if (crc32)
  	*crc32 = 0;
	if (wrprot)
		*wrprot = 0;
  if (leave_open || !zf) {
    struct zfile *f = zfile_fopen (fname, _T("r+b"), ZFD_NORMAL | ZFD_DISKHISTORY);
		if (!f) {
    	if (wrprot)
  	    *wrprot = 1;
    	f = zfile_fopen (fname, _T("rb"), ZFD_NORMAL | ZFD_DISKHISTORY);
    }
	  if (f && crc32)
	    *crc32 = zfile_crc32 (f);
		if (!zf)
			zfile_fclose (f);
		else
			*zf = f;
		return f ? 1 : 0;
  } else {
	  if (zfile_exists (fname)) {
	    if (wrprot)
		    *wrprot = 0;
	    if (crc32) {
		    struct zfile *f = zfile_fopen (fname, _T("rb"), ZFD_NORMAL | ZFD_DISKHISTORY);
		    if (f)
		      *crc32 = zfile_crc32 (f);
	      zfile_fclose (f);
	    }
			return 1;
	  } else {
	    if (wrprot)
		    *wrprot = 1;
	    return 0;
	  }
  }
}

static void updatemfmpos (drive *drv)
{
	if (drv->prevtracklen) {
  	drv->mfmpos = drv->mfmpos * (drv->tracklen * 1000 / drv->prevtracklen) / 1000;
		if (drv->mfmpos >= drv->tracklen)
			drv->mfmpos = drv->tracklen - 1;
	}
  drv->mfmpos %= drv->tracklen;
  drv->prevtracklen = drv->tracklen;
}

static void track_reset (drive *drv)
{
  drv->tracklen = FLOPPY_WRITE_LEN * drv->ddhd * 2 * 8;
	drv->revolutions = 1;
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
  if (strncmp ((char *)buffer, "UAE-1ADF", 8))
  	return 0;
  zfile_fread (buffer, 1, 4, diskfile);
  *num_tracks = buffer[2] * 256 + buffer[3];
  offs = 8 + 2 + 2 + (*num_tracks) * (2 + 2 + 4 + 4);

  for (i = 0; i < (*num_tracks); i++) {
    tid = trackdata + i;
    zfile_fread (buffer, 2 + 2 + 4 + 4, 1, diskfile);
    tid->type = (image_tracktype)buffer[3];
    tid->revolutions = buffer[2] + 1;
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

TCHAR *DISK_get_saveimagepath (const TCHAR *name)
{
  static TCHAR name1[MAX_DPATH];
  TCHAR name2[MAX_DPATH];
  TCHAR path[MAX_DPATH];
  int i;
    
  _tcscpy (name2, name);
  i = _tcslen (name2) - 1;
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
  fetch_saveimagepath (path, sizeof path / sizeof (TCHAR), 1);
	_stprintf (name1, _T("%s%s_save.adf"), path, name2 + i);
  return name1;
}

static struct zfile *getwritefile (struct uae_prefs *p, const TCHAR *name, bool *wrprot)
{
	struct zfile *zf;
	DISK_validate_filename (p, DISK_get_saveimagepath (name), 1, wrprot, NULL, &zf);
	return zf;
}

static int iswritefileempty (struct uae_prefs *p, const TCHAR *name)
{
  struct zfile *zf;
  bool wrprot;
  uae_char buffer[8];
  trackid td[MAX_TRACKS];
  int tracks, ddhd, i, ret;

  zf = getwritefile (p, name, &wrprot);
  if (!zf) return 1;
  zfile_fread (buffer, sizeof (char), 8, zf);
  if (strncmp ((uae_char*)buffer, "UAE-1ADF", 8))
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

static int openwritefile (struct uae_prefs *p, drive *drv, int create)
{
  bool wrprot = 0;

  drv->writediskfile = getwritefile (p, currprefs.floppyslots[drv - &floppy[0]].df, &wrprot);
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

static bool diskfile_iswriteprotect (struct uae_prefs *p, const TCHAR *fname, int *needwritefile, drive_type *drvtype)
{
  struct zfile *zf1, *zf2;
  bool wrprot1 = 0, wrprot2 = 1;
  uae_char buffer[25];
  
  *needwritefile = 0;
  *drvtype = DRV_35_DD;
  DISK_validate_filename (p, fname, 1, &wrprot1, NULL, &zf1);
  if (!zf1) 
    return 1;
  if (zfile_iscompressed (zf1)) {
    wrprot1 = 1;
    *needwritefile = 1;
  }
  zf2 = getwritefile (p, fname, &wrprot2);
  zfile_fclose (zf2);
  zfile_fread (buffer, sizeof (char), 25, zf1);
  zfile_fclose (zf1);
  if (strncmp ((uae_char *) buffer, "Formatted Disk Image file", 25) == 0) {
	  *needwritefile = 1;
	  return wrprot2;
  }
  if (strncmp ((uae_char *) buffer, "UAE-1ADF", 8) == 0) {
	  if (wrprot1)
	    return wrprot2;
	  return wrprot1;
  }
  if (strncmp ((uae_char *) buffer, "UAE--ADF", 8) == 0) {
	  *needwritefile = 1;
	  return wrprot2;
  }
  if (memcmp (exeheader, buffer, sizeof exeheader) == 0)
  	return 0;
  if (wrprot1)
    return wrprot2;
  return wrprot1;
}

static bool isrecognizedext (const TCHAR *name)
{
	const TCHAR *ext = _tcsrchr (name, '.');
	if (ext) {
		if (!_tcsicmp (ext + 1, _T("adf")) || !_tcsicmp (ext + 1, _T("adz")) || !_tcsicmp (ext + 1, _T("st")) || !_tcsicmp (ext + 1, _T("ima")) || !_tcsicmp (ext + 1, _T("img"))) 
			return true;
  }
	return false;
}

static int drive_insert (drive * drv, struct uae_prefs *p, int dnum, const TCHAR *fname, bool fake, bool forcedwriteprotect)
{
  uae_u8 buffer[2 + 2 + 4 + 4];
  trackid *tid;
  int num_tracks, size;
	int canauto;

  drive_image_free (drv);
	DISK_validate_filename (p, fname, 1, &drv->wrprot, &drv->crc32, &drv->diskfile);
	drv->forcedwrprot = forcedwriteprotect;
	if (drv->forcedwrprot)
		drv->wrprot = true;
  drv->ddhd = 1;
  drv->num_secs = 0;
  drv->hard_num_cyls = p->floppyslots[dnum].dfxtype == DRV_525_SD ? 40 : 80;
  drv->tracktiming[0] = 0;
  drv->useturbo = 0;
  drv->indexoffset = 0;

  gui_disk_image_change (dnum, fname, drv->wrprot);

  if (!drv->motoroff) {
		drv->dskready_up_time = DSKREADY_UP_TIME * 312 + (uaerand() & 511);
    drv->dskready_down_time = 0;
  }

  if (drv->diskfile == 0) {
  	track_reset (drv);
  	return 0;
  }

	if (!fake) {
    _tcsncpy (currprefs.floppyslots[dnum].df, fname, 255);
    currprefs.floppyslots[dnum].df[255] = 0;
	  currprefs.floppyslots[dnum].forcedwriteprotect = forcedwriteprotect;
    _tcsncpy (changed_prefs.floppyslots[dnum].df, fname, 255);
    changed_prefs.floppyslots[dnum].df[255] = 0;
	  changed_prefs.floppyslots[dnum].forcedwriteprotect = forcedwriteprotect;
    _tcscpy (drv->newname, fname);
	  drv->newnamewriteprotected = forcedwriteprotect;
    gui_filename (dnum, fname);
	}

  memset (buffer, 0, sizeof buffer);
  size = 0;
  if (drv->diskfile) {
  	zfile_fread (buffer, sizeof (char), 8, drv->diskfile);
  	zfile_fseek (drv->diskfile, 0, SEEK_END);
  	size = zfile_ftell (drv->diskfile);
  	zfile_fseek (drv->diskfile, 0, SEEK_SET);
  }

	canauto = 0;
	if (isrecognizedext (fname)) 
		canauto = 1;
	if (!canauto && drv->diskfile && isrecognizedext (zfile_getname (drv->diskfile)))
		canauto = 1;

  if (strncmp ((char*) buffer, "UAE-1ADF", 8) == 0) {

  	read_header_ext2 (drv->diskfile, drv->trackdata, &drv->num_tracks, &drv->ddhd);
  	drv->filetype = ADF_EXT2;
  	drv->num_secs = 11;
  	if (drv->ddhd > 1)
	    drv->num_secs = 22;

  } else if (strncmp ((char *) buffer, "UAE--ADF", 8) == 0) {
  	int offs = 160 * 4 + 8;
  	int i;

		drv->wrprot = true;
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
			tid->revolutions = 1;
	    if (tid->sync == 0) {
    		tid->type = TRACK_AMIGADOS;
    		tid->bitlen = 0;
	    } else {
    		tid->type = TRACK_RAW1;
    		tid->bitlen = tid->len * 8;
	    }
	    offs += tid->len;
  	}

  } else if (memcmp (exeheader, buffer, sizeof exeheader) == 0) {
  	int i;
  	struct zfile *z = zfile_fopen_empty (NULL, _T(""), 512 * 1760);
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
			tid->revolutions = 1;
  	}
  	drv->useturbo = 1;

	} else if (canauto && (

		// double sided
	  size == 9 * 80 * 2 * 512 || size == 18 * 80 * 2 * 512 || size == 10 * 80 * 2 * 512 || size == 20 * 80 * 2 * 512 ||
	  size == 9 * 81 * 2 * 512 || size == 18 * 81 * 2 * 512 || size == 10 * 81 * 2 * 512 || size == 20 * 81 * 2 * 512 ||
		size == 9 * 82 * 2 * 512 || size == 18 * 82 * 2 * 512 || size == 10 * 82 * 2 * 512 || size == 20 * 82 * 2 * 512 ||
		// single sided
		size == 9 * 80 * 1 * 512 || size == 18 * 80 * 1 * 512 || size == 10 * 80 * 1 * 512 || size == 20 * 80 * 1 * 512 ||
		size == 9 * 81 * 1 * 512 || size == 18 * 81 * 1 * 512 || size == 10 * 81 * 1 * 512 || size == 20 * 81 * 1 * 512 ||
		size == 9 * 82 * 1 * 512 || size == 18 * 82 * 1 * 512 || size == 10 * 82 * 1 * 512 || size == 20 * 82 * 1 * 512)) {
    	/* PC formatted image */
    	int i, side;

		  for (side = 2; side > 0; side--) {
			  if (       size ==  9 * 80 * side * 512 || size ==  9 * 81 * side * 512 || size ==  9 * 82 * side * 512) {
    	    drv->num_secs = 9;
    	    drv->ddhd = 1;
				  break;
			  } else if (size == 18 * 80 * side * 512 || size == 18 * 81 * side * 512 || size == 18 * 82 * side * 512) {
    	    drv->num_secs = 18;
    	    drv->ddhd = 2;
				  break;
			  } else if (size == 10 * 80 * side * 512 || size == 10 * 81 * side * 512 || size == 10 * 82 * side * 512) {
	        drv->num_secs = 10;
	        drv->ddhd = 1;
				  break;
			  } else if (size == 20 * 80 * side * 512 || size == 20 * 81 * side * 512 || size == 20 * 82 * side * 512) {
	        drv->num_secs = 20;
	        drv->ddhd = 2;
				  break;
			  }
	    }
      drv->num_tracks = size / (drv->num_secs * 512);

	    drv->filetype = ADF_PCDOS;
      tid = &drv->trackdata[0];
	    for (i = 0; i < drv->num_tracks; i++) {
	      tid->type = TRACK_PCDOS;
	      tid->len = 512 * drv->num_secs;
	      tid->bitlen = 0;
	      tid->offs = i * 512 * drv->num_secs;
			  if (side == 1) {
				  tid++;
				  tid->type = TRACK_NONE;
				  tid->len = 512 * drv->num_secs;
      	}
				tid->revolutions = 1;
			  tid++;

		  }
		  if (side == 1)
			  drv->num_tracks *= 2;

	  } else if ((size == 262144 || size == 524288) && buffer[0] == 0x11 && (buffer[1] == 0x11 || buffer[1] == 0x14)) {

		  // 256k -> KICK disk, 512k -> SuperKickstart disk
		  drv->filetype = size == 262144 ? ADF_KICK : ADF_SKICK;
		  drv->num_tracks = 1760 / (drv->num_secs = 11);
		  for (int i = 0; i < drv->num_tracks; i++) {
			  tid = &drv->trackdata[i];
			  tid->type = TRACK_AMIGADOS;
			  tid->len = 512 * drv->num_secs;
			  tid->bitlen = 0;
			  tid->offs = i * 512 * drv->num_secs - (drv->filetype == ADF_KICK ? 512 : 262144 + 1024);
			  tid->track = i;
			  tid->revolutions = 1;
		  }

    } else {
	    int i, ds;

    	ds = 0;
	    drv->filetype = ADF_NORMAL;

	    /* High-density or diskspare disk? */
	    drv->num_tracks = 0;
	    if (size > 160 * 11 * 512 + 511) { // larger than standard adf?
	      for (i = 80; i <= 83; i++) {
    	  	if (size == i * 22 * 512 * 2) { // HD
	    	    drv->ddhd = 2;
	          drv->num_tracks = size / (512 * (drv->num_secs = 22));
	    	    break;
	      	}
	      	if (size == i * 11 * 512 * 2) { // >80 cyl DD
	    	    drv->num_tracks = size / (512 * (drv->num_secs = 11));
	    	    break;
	      	}
	      	if (size == i * 12 * 512 * 2) { // ds 12 sectors
	    	    drv->num_tracks = size / (512 * (drv->num_secs = 12));
	    	    ds = 1;
	    	    break;
	      	}
	      	if (size == i * 24 * 512 * 2) { // ds 24 sectors
	    	    drv->num_tracks = size / (512 * (drv->num_secs = 24));
	          drv->ddhd = 2;
	    	    ds = 1;
	    	    break;
	    	}
      }
      if (drv->num_tracks == 0) {
	    	drv->num_tracks = size / (512 * (drv->num_secs = 22));
	    	drv->ddhd = 2;
	    }
	  } else {
      drv->num_tracks = size / (512 * (drv->num_secs = 11));
	  }

  	if (!ds &&drv->num_tracks > MAX_TRACKS)
      write_log (_T("Your diskfile is too big, %d bytes!\n"), size);
  	for (i = 0; i < drv->num_tracks; i++) {
      tid = &drv->trackdata[i];
      tid->type = ds ? TRACK_DISKSPARE : TRACK_AMIGADOS;
      tid->len = 512 * drv->num_secs;
      tid->bitlen = 0;
      tid->offs = i * 512 * drv->num_secs;
			tid->revolutions = 1;
  	}
  }
  openwritefile (p, drv, 0);
  drive_settype_id (drv);	/* Set DD or HD drive */
  drive_fill_bigbuf (drv, 1);
  drv->mfmpos = uaerand ();
  drv->mfmpos |= (uaerand () << 16);
  drv->mfmpos %= drv->tracklen;
  drv->prevtracklen = 0;
	if (!fake) {
  	update_drive_gui (drv - floppy, false);
	}
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

static void set_steplimit (drive *drv)
{
	// emulate step limit only if cycle-exact or approximate CPU speed
	if (currprefs.m68k_speed != 0)
		return;
	drv->steplimit = 4;
  drv->steplimitcycle = get_cycles ();
}

static int drive_empty (drive * drv)
{
  return drv->diskfile == 0 && drv->dskchange_time >= 0;
}

static void drive_step (drive * drv, int step_direction)
{
	if (!drive_empty (drv))
		drv->dskchange = 0;
  if (drv->steplimit && get_cycles() - drv->steplimitcycle < MIN_STEPLIMIT_CYCLE) {
  	return;
  }
  /* A1200's floppy drive needs at least 30 raster lines between steps
   * but we'll use very small value for better compatibility with faster CPU emulation
   * (stupid trackloaders with CPU delay loops)
   */
  set_steplimit (drv);
	if (step_direction) {
    if (drv->cyl) {
      drv->cyl--;
    }
  } else {
    int maxtrack = drv->hard_num_cyls;
    if (drv->cyl < maxtrack + 3) {
	    drv->cyl++;
    }
		if (drv->cyl >= maxtrack)
			write_log (_T("program tried to step over track %d\n"), maxtrack);
  }
  rand_shifter (drv);
}

static int drive_track0 (drive * drv)
{
  return drv->cyl == 0;
}

static int drive_writeprotected (drive * drv)
{
  return drv->wrprot || drv->forcedwrprot || drv->diskfile == NULL;
}

static int drive_running (drive * drv)
{
  return !drv->motoroff;
}

void DISK_motordelay_func(uae_u32 v)
{
  floppy[v].motordelay = 0;
}

static void drive_motor (drive * drv, bool off)
{
  if (drv->motoroff && !off) {
		drv->dskready_up_time = DSKREADY_UP_TIME * 312 + (uaerand() & 511);
    rand_shifter (drv);
  }
  if (!drv->motoroff && off) {
	  drv->drive_id_scnt = 0; /* Reset id shift reg counter */
	  drv->dskready_down_time = DSKREADY_DOWN_TIME * 312 + (uaerand() & 511);
	  if (currprefs.cpu_model <= 68010 && currprefs.m68k_speed == 0) {
	    drv->motordelay = 1;
	    event2_newevent(ev2_disk_motor0 + (drv - floppy), 30, drv - floppy);
	  }
  }
  drv->motoroff = off;
  if (drv->motoroff) {
	  drv->dskready = 0;
		drv->dskready_up_time = 0;
	} else {
		drv->dskready_down_time = 0;
  }
}

static void read_floppy_data (struct zfile *diskfile, int type, trackid *tid, int offset, uae_u8 *dst, int len)
{
  if (len == 0)
  	return;
	if (tid->track == 0) {
		if (type == ADF_KICK) {
			memset (dst, 0, len > 512 ? 512 : len);
			if (offset == 0) {
				memcpy (dst, "KICK", 4);
				len -= 512;
			}
		} else if (type == ADF_SKICK) {
			memset (dst, 0, len > 512 ? 512 : len);
			if (offset == 0) {
				memcpy (dst, "KICKSUP0", 8);
				len -= 1024;
			} else if (offset == 512) {
				len -= 512;
			}
		}
	}
	int off = tid->offs + offset;
	if (off >= 0 && len > 0) {
		zfile_fseek (diskfile, off, SEEK_SET);
    zfile_fread (dst, 1, len, diskfile);
	}
}

/* Megalomania does not like zero MFM words... */
static void mfmcode (uae_u16 * mfm, int words)
{
  uae_u32 lastword = 0;

  while (words--) {
		uae_u32 v = (*mfm) & 0x55555555;
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
  int i, len;
  int tr = drv->cyl * 2 + side;
  uae_u16 *dstmfmbuf, *mfm2;
  uae_u8 secbuf[1000];
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
		read_floppy_data (drv->diskfile, drv->filetype, ti, i * 512, &secbuf[60], 512);
	  crc16 = get_crc16(secbuf + 56, 3 + 1 + 512);
	  secbuf[60 + 512] = crc16 >> 8;
	  secbuf[61 + 512] = crc16 & 0xff;
    len = (tracklen / 2 - 96) / drv->num_secs - 574 / drv->ddhd;
	  if (len > 0)
    	memset(secbuf + 512 + 62, 0x4e, len);
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
	int prevbit;

  trackid *ti = drv->trackdata + tr;
	memset (dstmfmbuf, 0xaa, len * 2);
	dstmfmoffset += FLOPPY_GAP_LEN;
	drv->skipoffset = (FLOPPY_GAP_LEN * 8) / 3 * 2;
	drv->tracklen = len * 2 * 8;

	prevbit = 0;
  for (sec = 0; sec < drv->num_secs; sec++) {
  	uae_u8 secbuf[544];
    uae_u16 mfmbuf[544 + 1];
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

		read_floppy_data (drv->diskfile, drv->filetype, ti, sec * 512, &secbuf[32], 512);

		mfmbuf[0] = prevbit ? 0x2aaa : 0xaaaa;
		mfmbuf[1] = 0xaaaa;
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

		mfmbuf[544] = 0;

		mfmcode (mfmbuf + 4, 544 - 4 + 1);

    for (i = 0; i < 544; i++) {
  		dstmfmbuf[dstmfmoffset % len] = mfmbuf[i];
  		dstmfmoffset++;
  	}
		prevbit = mfmbuf[i - 1] & 1;
		// so that final word has correct MFM encoding
		dstmfmbuf[dstmfmoffset % len] = mfmbuf[i];
  }
}

/*
 * diskspare format
 *
 * 0 <4489> <4489> 0 track sector crchi, crclo, data[512] (520 bytes per sector)
 *
 * 0xAAAA 0x4489 0x4489 0x2AAA oddhi, oddlo, evenhi, evenlo, ...
 *
 * NOTE: data is MFM encoded using same method as ADOS header, not like ADOS data!
 *
 */

static void decode_diskspare (drive *drv)
{
  int tr = drv->cyl * 2 + side;
  int sec;
  int dstmfmoffset = 0;
  int size = 512 + 8;
  uae_u16 *dstmfmbuf = drv->bigmfmbuf;
  int len = drv->num_secs * size + FLOPPY_GAP_LEN;

  trackid *ti = drv->trackdata + tr;
  memset (dstmfmbuf, 0xaa, len * 2);
  dstmfmoffset += FLOPPY_GAP_LEN;
  drv->skipoffset = (FLOPPY_GAP_LEN * 8) / 3 * 2;
  drv->tracklen = len * 2 * 8;

  for (sec = 0; sec < drv->num_secs; sec++) {
	  uae_u8 secbuf[512 + 8];
	  uae_u16 mfmbuf[512 + 8];
	  int i;
	  uae_u32 deven, dodd;
	  uae_u16 chk;

	  secbuf[0] = tr;
	  secbuf[1] = sec;
	  secbuf[2] = 0;
	  secbuf[3] = 0;

		read_floppy_data (drv->diskfile, drv->filetype, ti, sec * 512, &secbuf[4], 512);

	  mfmbuf[0] = 0xaaaa;
	  mfmbuf[1] = 0x4489;
	  mfmbuf[2] = 0x4489;
	  mfmbuf[3] = 0x2aaa;

  	for (i = 0; i < 512; i += 4) {
	    deven = ((secbuf[i + 4] << 24) | (secbuf[i + 5] << 16)
			 | (secbuf[i + 6] << 8) | (secbuf[i + 7]));
	    dodd = deven >> 1;
	    deven &= 0x55555555;
	    dodd &= 0x55555555;
	    mfmbuf[i + 8 + 0] = dodd >> 16;
	    mfmbuf[i + 8 + 1] = dodd;
	    mfmbuf[i + 8 + 2] = deven >> 16;
	    mfmbuf[i + 8 + 3] = deven;
	  }
	  mfmcode (mfmbuf + 8, 512);

	  i = 8;
	  chk = mfmbuf[i++] & 0x7fff;
	  while (i < 512 + 8)
	    chk ^= mfmbuf[i++];
	  secbuf[2] = chk >> 8;
	  secbuf[3] = chk;

	  deven = ((secbuf[0] << 24) | (secbuf[1] << 16)
		  | (secbuf[2] << 8) | (secbuf[3]));
	  dodd = deven >> 1;
	  deven &= 0x55555555;
	  dodd &= 0x55555555;

	  mfmbuf[4] = dodd >> 16;
	  mfmbuf[5] = dodd;
	  mfmbuf[6] = deven >> 16;
	  mfmbuf[7] = deven;
	  mfmcode (mfmbuf + 4, 4);

  	for (i = 0; i < 512 + 8; i++) {
	    dstmfmbuf[dstmfmoffset % len] = mfmbuf[i];
	    dstmfmoffset++;
  	}
  }
}

static void drive_fill_bigbuf (drive * drv, int force)
{
  int tr = drv->cyl * 2 + side;
  trackid *ti = drv->trackdata + tr;
	bool retrytrack;
	int rev = -1;

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
  drv->revolutions = 1;
	retrytrack = drv->lastdataacesstrack == drv->cyl * 2 + side;
	if (!dskdmaen && !retrytrack)
		drv->track_access_done = false;
	//write_log (_T("%d:%d %d\n"), drv->cyl, side, retrytrack);

  if (drv->writediskfile && drv->writetrackdata[tr].bitlen > 0) {
	  int i;
	  trackid *wti = &drv->writetrackdata[tr];
	  drv->tracklen = wti->bitlen;
	  drv->revolutions = wti->revolutions;
		read_floppy_data (drv->writediskfile, drv->filetype, wti, 0, (uae_u8*)drv->bigmfmbuf, (wti->bitlen + 7) / 8);
	  for (i = 0; i < (drv->tracklen + 15) / 16; i++) {
	    uae_u16 *mfm = drv->bigmfmbuf + i;
	    uae_u8 *data = (uae_u8 *) mfm;
	    *mfm = 256 * *data + *(data + 1);
	  }

  } else if (ti->type == TRACK_PCDOS) {

	  decode_pcdos(drv);

  } else if (ti->type == TRACK_AMIGADOS) {

  	decode_amigados(drv);

  } else if (ti->type == TRACK_DISKSPARE) {

  	decode_diskspare (drv);

	} else if (ti->type == TRACK_NONE) {

	;

  } else {
	  int i;
	  int base_offset = ti->type == TRACK_RAW ? 0 : 1;
	  drv->tracklen = ti->bitlen + 16 * base_offset;
	  drv->bigmfmbuf[0] = ti->sync;
		read_floppy_data (drv->diskfile, drv->filetype, ti, 0, (uae_u8*)(drv->bigmfmbuf + base_offset), (ti->bitlen + 7) / 8);
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
static void diskfile_update (struct zfile *diskfile, trackid *ti, int len, image_tracktype type)
{
  uae_u8 buf[2 + 2 + 4 + 4], *zerobuf;

  ti->revolutions = 1;
  ti->bitlen = len;
  zfile_fseek (diskfile, 8 + 4 + (2 + 2 + 4 + 4) * ti->track, SEEK_SET);
  memset (buf, 0, sizeof buf);
  ti->type = type;
  buf[2] = 0;
  buf[3] = ti->type;
  do_put_mem_long ((uae_u32 *) (buf + 4), ti->len);
  do_put_mem_long ((uae_u32 *) (buf + 8), ti->bitlen);
  zfile_fwrite (buf, sizeof buf, 1, diskfile);
  if (ti->len > (len + 7) / 8) {
	  zerobuf = xmalloc (uae_u8, ti->len);
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

#if MFM_VALIDATOR
static void check_valid_mfm (uae_u16 *mbuf, int words, int sector)
{
	int prevbit = 0;
	for (int i = 0; i < words * 8; i++) {
		int wordoffset = i / 8;
		uae_u16 w = mbuf[wordoffset];
		uae_u16 wp = mbuf[wordoffset - 1];
		int bitoffset = (7 - (i & 7)) * 2;
		int clockbit = w & (1 << (bitoffset + 1));
		int databit = w & (1 << (bitoffset + 0));

		if ((clockbit && databit) || (clockbit && !databit && prevbit) || (!clockbit && !databit && !prevbit)) {
			write_log (L"illegal mfm sector %d data %04x %04x, bit %d:%d\n", sector, wp, w, wordoffset, bitoffset);
		}
		prevbit = databit;
	}
}
#endif

static int decode_buffer (uae_u16 *mbuf, int cyl, int drvsec, int ddhd, int filetype, int *drvsecp, int *sectable, int checkmode)
{
  int i, secwritten = 0;
  int fwlen = FLOPPY_WRITE_LEN * ddhd;
  int length = 2 * fwlen;
  uae_u32 odd, even, chksum, id, dlong;
  uae_u8 *secdata;
  uae_u8 secbuf[544];
	uae_u16 *mend = mbuf + length, *mstart;
	uae_u32 sechead[4];
  int shift = 0;
	bool issechead;

  memset (sectable, 0, MAX_SECTORS * sizeof (int));
	mstart = mbuf;
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
		  write_log (_T("Disk decode: weird sector number %d (%08X, %d)\n"), trackoffs, id, mbuf - mstart);
      if (filetype == ADF_EXT2)
	      return 2;
      continue;
	  }
#if MFM_VALIDATOR
		check_valid_mfm (mbuf - 4, 544 - 4 + 1, trackoffs);
#endif
		issechead = false;
  	chksum = odd ^ even;
  	for (i = 0; i < 4; i++) {
	    odd = getmfmlong (mbuf, shift);
	    even = getmfmlong (mbuf + 8, shift);
	    mbuf += 2;

	    dlong = (odd << 1) | even;
	    if (dlong && !checkmode) {
				issechead = true;
			}
			sechead[i] = dlong;
			chksum ^= odd ^ even;
		}
		if (issechead) {
    	if (filetype == ADF_EXT2)
  	    return 6;
    }
	  mbuf += 8;
	  odd = getmfmlong (mbuf, shift);
	  even = getmfmlong (mbuf + 2, shift);
	  mbuf += 4;
	  if (((odd << 1) | even) != chksum) {
			write_log (_T("Disk decode: checksum error on sector %d header\n"), trackoffs);
      if (filetype == ADF_EXT2)
	      return 3;
      continue;
	  }
    if (((id & 0x00ff0000) >> 16) != cyl * 2 + side) {
			write_log (_T("Disk decode: mismatched track (%d <> %d) on sector %d header (%08X)\n"), (id & 0x00ff0000) >> 16, cyl * 2 + side, trackoffs, id);
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
			write_log (_T("Disk decode: sector %d, data checksum error\n"), trackoffs);
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
  int sectable[18];
  uae_u8 secbuf[3 + 1 + 512];
  uae_u8 mark;
  uae_u16 crc;

  memset (sectable, 0, sizeof sectable);
  memcpy (mbuf + fwlen, mbuf, fwlen * sizeof (uae_u16));
  mend -= 518;
  secbuf[0] = secbuf[1] = secbuf[2] = 0xa1;
  secbuf[3] = 0xfb;

  while (secwritten < drvsec) {
		int mfmcount;

		mfmcount = 0;
  	while (getmfmword (mbuf, shift) != 0x4489) {
			mfmcount++;
	    if (mbuf >= mend)
    		return 1;
	    shift++;
	    if (shift == 16) {
		    shift = 0;
		    mbuf++;
	    }
			if (sector >= 0 && mfmcount / 16 >= 43)
				sector = -1;
  	}

		mfmcount = 0;
  	while (getmfmword (mbuf, shift) == 0x4489) {
			mfmcount++;
	    if (mbuf >= mend)
    		return 1;
	    mbuf++;
  	}
		if (mfmcount < 3) // ignore if less than 3 sync markers
			continue;

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

			// skip 28 bytes
			for (i = 0; i < 28; i++)
				mfmdecode (&mbuf, shift);

	    if (get_crc16(tmp, 8) != crc || cyl != drv->cyl || head != side || size != 2 || sector < 1 || sector > drv->num_secs) {
				write_log (_T("PCDOS: track %d, corrupted sector header\n"), drv->cyl * 2 + side);
    		return 1;
	    }
	    sector--;
	    continue;
	  }
	  if (mark != 0xfb && mark != 0xfa) {
			write_log (_T("PCDOS: track %d: unknown address mark %02X\n"), drv->cyl * 2 + side, mark);
	    continue;
	  }
	  if (sector < 0)
	    continue;
	  for (i = 0; i < 512; i++)
	    secbuf[i + 4] = mfmdecode(&mbuf, shift);
	  crc = (mfmdecode(&mbuf, shift) << 8) | mfmdecode(&mbuf, shift);
	  if (get_crc16(secbuf, 3 + 1 + 512) != crc) {
			write_log (_T("PCDOS: track %d, sector %d data checksum error\n"),
				drv->cyl * 2 + side, sector + 1);
	    continue;
	  }
	  sectable[sector] = 1;
	  secwritten++;
	  zfile_fseek (drv->diskfile, drv->trackdata[drv->cyl * 2 + side].offs + sector * 512, SEEK_SET);
	  zfile_fwrite (secbuf + 4, sizeof (uae_u8), 512, drv->diskfile);
		write_log (_T("PCDOS: track %d sector %d written\n"), drv->cyl * 2 + side, sector + 1);
	  sector = -1;
  }
	if (secwritten != drv->num_secs)
		write_log (_T("PCDOS: track %d, %d corrupted sectors ignored\n"),
		drv->cyl * 2 + side, drv->num_secs - secwritten);
  return 0;
}

static int drive_write_adf_amigados (drive * drv)
{
  int drvsec, i;
  int sectable[MAX_SECTORS];

  if (decode_buffer (drv->bigmfmbuf, drv->cyl, drv->num_secs, drv->ddhd, drv->filetype, &drvsec, sectable, 0))
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
		write_log (_T("disk raw write: image file's track %d is too small (%d < %d)!\n"), ti->track, ti->len, len);
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
	int tr = drv->cyl * 2 + side;

  if (drive_writeprotected (drv) || drv->trackdata[tr].type == TRACK_NONE) {
    /* read original track back because we didn't really write anything */
    drv->buffered_side = 2;
    return;
  }
  if (drv->writediskfile) {
	  drive_write_ext2 (drv->bigmfmbuf, drv->writediskfile, &drv->writetrackdata[tr],
    longwritemode ? dsklength2 * 8 : drv->tracklen);
  }
  switch (drv->filetype) {
  case ADF_NORMAL:
  	if (drive_write_adf_amigados (drv)) {
      static int warned;
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
		  write_log (_T("not an amigados track %d (error %d), writing as raw track\n"), drv->cyl * 2 + side, ret);
		  drive_write_ext2 (drv->bigmfmbuf, drv->diskfile, &drv->trackdata[drv->cyl * 2 + side],
			  longwritemode ? dsklength2 * 8 : drv->tracklen);
    }
    return;
  case ADF_IPF:
   	break;
  case ADF_PCDOS:
	  ret = drive_write_pcdos (drv);
		if (ret)
			write_log (_T("not a PC formatted track %d (error %d)\n"), drv->cyl * 2 + side, ret);
	  break;
  }
  drv->tracktiming[0] = 0;
}

static void drive_eject (drive * drv)
{
  gui_disk_image_change (drv - floppy, NULL, drv->wrprot);
  drive_image_free (drv);
  drv->dskchange = true;
  drv->ddhd = 1;
  drv->dskchange_time = 0;
  drv->dskready = 0;
  drv->dskready_up_time = 0;
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

static void floppy_get_bootblock (uae_u8 *dst, bool ffs, bool bootable)
{
	strcpy ((char*)dst, "DOS");
	dst[3] = ffs ? 1 : 0;
	if (bootable)
		memcpy (dst, ffs ? bootblock_ffs : bootblock_ofs, ffs ? sizeof bootblock_ffs : sizeof bootblock_ofs);
}
static void floppy_get_rootblock (uae_u8 *dst, int block, const TCHAR *disk_name, drive_type adftype)
{
	dst[0+3] = 2;
	dst[12+3] = 0x48;
	dst[312] = dst[313] = dst[314] = dst[315] = (uae_u8)0xff;
	dst[316+2] = (block + 1) >> 8; dst[316+3] = (block + 1) & 255;
	char *s = ua ((disk_name && _tcslen (disk_name) > 0) ? disk_name : _T("empty"));
	dst[432] = strlen (s);
	strcpy ((char*)dst + 433, s);
	xfree (s);
	dst[508 + 3] = 1;
	disk_date (dst + 420);
	memcpy (dst + 472, dst + 420, 3 * 4);
	memcpy (dst + 484, dst + 420, 3 * 4);
	disk_checksum (dst, dst + 20);
	/* bitmap block */
	memset (dst + 512 + 4, 0xff, 2 * block / 8);
	if (adftype == 0)
		dst[512 + 0x72] = 0x3f;
	else
		dst[512 + 0xdc] = 0x3f;
	disk_checksum (dst + 512, dst + 512);
}

/* type: 0=regular, 1=ext2adf */
/* adftype: 0=DD,1=HD,2=DD PC,3=HD PC,4=525SD */
bool disk_creatediskfile (const TCHAR *name, int type, drive_type adftype, const TCHAR *disk_name, bool ffs, bool bootable, struct zfile *copyfrom)
{
	int size = 32768;
  struct zfile *f;
  int i, l, file_size, tracks, track_len, sectors;
	uae_u8 *chunk = NULL;
	int ddhd = 1;
	bool ok = false;
	uae_u64 pos;

  if (type == 1)
  	tracks = 2 * 83;
  else
    tracks = 2 * 80;
  file_size = 880 * 1024;
  sectors = 11;
  if (adftype == 2 || adftype == 3) {
	  file_size = 720 * 1024;
	  sectors = 9;
  }
  track_len = FLOPPY_WRITE_LEN * 2;
  if (adftype == 1 || adftype == 3) {
	  file_size *= 2;
	  track_len *= 2;
		ddhd = 2;
  } else if (adftype == 4) {
    file_size /= 2;
    tracks /= 2;
  }

	if (copyfrom) {
		pos = zfile_ftell (copyfrom);
		zfile_fseek (copyfrom, 0, SEEK_SET);
	}

	f = zfile_fopen (name, _T("wb"), 0);
  chunk = xmalloc (uae_u8, size);
  if (f && chunk) {
	  int cylsize = sectors * 2 * 512;
	  memset(chunk, 0 ,size);
	  if (type == 0) {
	    for (i = 0; i < file_size; i += cylsize) {
		    memset(chunk, 0, cylsize);
		    if (adftype <= 1) {
  		    if (i == 0) {
		        /* boot block */
						floppy_get_bootblock (chunk, ffs, bootable);
		      } else if (i == file_size / 2) {
    		    /* root block */
					  floppy_get_rootblock (chunk, file_size / 1024, disk_name, adftype);
          }
    		}
    		zfile_fwrite (chunk, cylsize, 1, f);
      }
			ok = true;
	  } else {
			uae_u8 root[4];
			uae_u8 rawtrack[3 * 4], dostrack[3 * 4];
      l = track_len;
			zfile_fwrite ("UAE-1ADF", 8, 1, f);
			root[0] = 0; root[1] = 0; /* flags (reserved) */
			root[2] = 0; root[3] = tracks; /* number of tracks */
			zfile_fwrite (root, 4, 1, f);
			rawtrack[0] = 0; rawtrack[1] = 0; /* flags (reserved) */
			rawtrack[2] = 0; rawtrack[3] = 1; /* track type */
			rawtrack[4] = 0; rawtrack[5] = 0; rawtrack[6]=(uae_u8)(l >> 8); rawtrack[7] = (uae_u8)l;
			rawtrack[8] = 0; rawtrack[9] = 0; rawtrack[10] = 0; rawtrack[11] = 0;
			memcpy (dostrack, rawtrack, sizeof rawtrack);
			dostrack[3] = 0;
			dostrack[9] = (l * 8) >> 16; dostrack[10] = (l * 8) >> 8; dostrack[11] = (l * 8) >> 0;
			bool dodos = ffs || bootable || (disk_name && _tcslen (disk_name) > 0);
			for (i = 0; i < tracks; i++) {
				uae_u8 tmp[3 * 4];
				memcpy (tmp, rawtrack, sizeof rawtrack);
				if (dodos || copyfrom)
					memcpy (tmp, dostrack, sizeof dostrack);
				zfile_fwrite (tmp, sizeof tmp, 1, f);
			}
			for (i = 0; i < tracks; i++) {
				memset (chunk, 0, size);
				if (copyfrom) {
					zfile_fread (chunk, 11 * ddhd, 512, copyfrom);
				} else {
				  if (dodos) {
					  if (i == 0)
						  floppy_get_bootblock (chunk, ffs, bootable);
					  else if (i == 80)
						  floppy_get_rootblock (chunk, 80 * 11 * ddhd, disk_name, adftype);
				  }
				}
        zfile_fwrite (chunk, l, 1, f);
			}
			ok = true;
  	}
  }
  xfree (chunk);
  zfile_fclose (f);
	if (copyfrom)
		zfile_fseek (copyfrom, pos, SEEK_SET);
	return ok;
}

int disk_getwriteprotect (struct uae_prefs *p, const TCHAR *name)
{
  int needwritefile;
  drive_type drvtype;
  return diskfile_iswriteprotect (p, name, &needwritefile, &drvtype);
}

static void diskfile_readonly (const TCHAR *name, bool readonly)
{
	struct mystat st;
  int mode, oldmode;
    
	if (!my_stat (name, &st)) {
		write_log (_T("failed to access '%s'\n"), name);
  	return;
	}
	oldmode = mode = st.mode;
  mode &= ~FILEFLAG_WRITE;
  if (!readonly) 
    mode |= FILEFLAG_WRITE;
  if (mode != oldmode) {
		if (!my_chmod (name, mode))
			write_log(_T("chmod failed!\n"));
  }
}

static void setdskchangetime(drive *drv, int dsktime)
{
  int i;
  /* prevent multiple disk insertions at the same time */
  if (drv->dskchange_time > 0)
  	return;
  for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
  	if (&floppy[i] != drv && floppy[i].dskchange_time > 0 && floppy[i].dskchange_time + 1 >= dsktime) {
	    dsktime = floppy[i].dskchange_time + 1;
  	}
  }
  drv->dskchange_time = dsktime;
}

void DISK_reinsert (int num)
{
  drive_eject (&floppy[num]);
	setdskchangetime (&floppy[num], 2 * 50 * 312);
}

int disk_setwriteprotect (struct uae_prefs *p, int num, const TCHAR *name, bool writeprotected)
{
  int needwritefile, oldprotect;
  struct zfile *zf1, *zf2;
  bool wrprot1, wrprot2;
  int i;
  TCHAR *name2;
  drive_type drvtype;

	write_log(_T("disk_setwriteprotect %d '%s' %d\n"), num, name, writeprotected);

  oldprotect = diskfile_iswriteprotect (p, name, &needwritefile, &drvtype);
  DISK_validate_filename (p, name, 1, &wrprot1, NULL, &zf1);
  if (!zf1) 
    return 0;

	write_log(_T("old = %d writeprot = %d\n"), oldprotect, wrprot1);

  if (zfile_iscompressed (zf1)) 
    wrprot1 = 1;
  zfile_fclose (zf1);
  zf2 = getwritefile (p, name, &wrprot2);
  name2 = DISK_get_saveimagepath (name);

  if (needwritefile && zf2 == 0)
		disk_creatediskfile (name2, 1, drvtype, NULL, false, false, NULL);
  zfile_fclose (zf2);
  if (writeprotected && iswritefileempty (p, name)) {
  	for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
	    if (!_tcscmp (name, floppy[i].newname))
    		drive_eject (&floppy[i]);
  	}
  	_wunlink (name2);
  }

  if (!needwritefile)
    diskfile_readonly (name, writeprotected);
  diskfile_readonly (name2, writeprotected);
  DISK_reinsert (num);
  return 1;
}

void disk_eject (int num)
{
  gui_filename (num, _T(""));
  drive_eject (floppy + num);
  *currprefs.floppyslots[num].df = *changed_prefs.floppyslots[num].df = 0;
  floppy[num].newname[0] = 0;
	update_drive_gui (num, true);
}

int DISK_history_add (const TCHAR *name, int idx, int type, int donotcheck)
{
  int i;

  if (idx >= MAX_PREVIOUS_FLOPPIES)
  	return 0;
  if (name == NULL) {
		dfxhistory[type][idx][0] = 0;
  	return 1;
  }
  if (name[0] == 0)
	  return 0;
  if (!donotcheck) {
		if (!zfile_exists (name)) {
			for (i = 0; i < MAX_PREVIOUS_FLOPPIES; i++) {
				if (!_tcsicmp (dfxhistory[type][i], name)) {
					while (i < MAX_PREVIOUS_FLOPPIES - 1) {
						_tcscpy (dfxhistory[type][i], dfxhistory[type][i + 1]);
						i++;
					}
					dfxhistory[type][MAX_PREVIOUS_FLOPPIES - 1][0] = 0;
					break;
				}
			}
	    return 0;
		}
  }
  if (idx >= 0) {
  	if (idx >= MAX_PREVIOUS_FLOPPIES)
	    return 0;
		dfxhistory[type][idx][0] = 0;
	  for (i = 0; i < MAX_PREVIOUS_FLOPPIES; i++) {
			if (!_tcsicmp (dfxhistory[type][i], name))
		    return 0;
    }
		_tcscpy (dfxhistory[type][idx], name);
	  return 1;
  }
  for (i = 0; i < MAX_PREVIOUS_FLOPPIES; i++) {
		if (!_tcscmp (dfxhistory[type][i], name)) {
	    while (i < MAX_PREVIOUS_FLOPPIES - 1) {
				_tcscpy (dfxhistory[type][i], dfxhistory[type][i + 1]);
		    i++;
	    }
			dfxhistory[type][MAX_PREVIOUS_FLOPPIES - 1][0] = 0;
	    break;
  	}
  }
  for (i = MAX_PREVIOUS_FLOPPIES - 2; i >= 0; i--)
		_tcscpy (dfxhistory[type][i + 1], dfxhistory[type][i]);
	_tcscpy (dfxhistory[type][0], name);
  return 1;
}

TCHAR *DISK_history_get (int idx, int type)
{
  if (idx >= MAX_PREVIOUS_FLOPPIES)
		return NULL;
	return dfxhistory[type][idx];
}

static void disk_insert_2 (int num, const TCHAR *name, bool forced, bool forcedwriteprotect)
{
	drive *drv = floppy + num;
  if (forced) {
		drive_insert (drv, &currprefs, num, name, false, forcedwriteprotect);
	  return;
  }
  if (!_tcscmp (currprefs.floppyslots[num].df, name))
  	return;
  _tcscpy (drv->newname, name);
	drv->newnamewriteprotected = forcedwriteprotect;
  _tcscpy (currprefs.floppyslots[num].df, name);
	currprefs.floppyslots[num].forcedwriteprotect = forcedwriteprotect;
  DISK_history_add (name, -1, HISTORY_FLOPPY, 0);
	if (name[0] == 0) {
		disk_eject (num);
	} else if (!drive_empty(drv) || drv->dskchange_time > 0) {
		drive_eject (drv);
		/* set dskchange_time, disk_insert() will be
		 * called from DISK_check_change() after 2 second delay
	 	* this makes sure that all programs detect disk change correctly
	 	*/
		setdskchangetime (drv, 2 * 50 * 312);
	} else {
		setdskchangetime (drv, 1 * 312);
	}
}

void disk_insert (int num, const TCHAR *name, bool forcedwriteprotect)
{
	disk_insert_2 (num, name, 0, forcedwriteprotect);
}

void disk_insert (int num, const TCHAR *name)
{
  disk_insert_2 (num, name, 0, false);
}
void disk_insert_force (int num, const TCHAR *name, bool forcedwriteprotect)
{
	disk_insert_2 (num, name, 1, forcedwriteprotect);
}

static void DISK_check_change (void)
{
  if (currprefs.floppy_speed != changed_prefs.floppy_speed)
  	currprefs.floppy_speed = changed_prefs.floppy_speed;
  for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
  	drive *drv = floppy + i;
  	if (currprefs.floppyslots[i].dfxtype != changed_prefs.floppyslots[i].dfxtype) {
	    currprefs.floppyslots[i].dfxtype = changed_prefs.floppyslots[i].dfxtype;
	    reset_drive (i);
	  }
	}
}

void DISK_vsync (void)
{
	DISK_check_change ();
	for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
		drive *drv = floppy + i;
  	if (drv->dskchange_time == 0 && _tcscmp (currprefs.floppyslots[i].df, changed_prefs.floppyslots[i].df))
			disk_insert (i, changed_prefs.floppyslots[i].df, changed_prefs.floppyslots[i].forcedwriteprotect);
  }
}

int disk_empty (int num)
{
    return drive_empty (floppy + num);
}

static TCHAR *tobin (uae_u8 v)
{
	static TCHAR buf[9];
	for (int i = 7; i >= 0; i--)
		buf[7 - i] = v & (1 << i) ? '1' : '0';
	return buf;
}

static void fetch_DISK_select(uae_u8 data)
{
	selected = (data >> 3) & 15;
	side = 1 - ((data >> 2) & 1);
	direction = (data >> 1) & 1;
}

void DISK_select_set (uae_u8 data)
{
	prev_data = data;
	prev_step = data & 1;

	fetch_DISK_select (data);
}

void DISK_select (uae_u8 data)
{
  int step_pulse, prev_selected, dr;

  prev_selected = selected;

  fetch_DISK_select (data);
  step_pulse = data & 1;

  if ((prev_data & 0x80) != (data & 0x80)) {
  	for (dr = 0; dr < 4; dr++) {
      if (floppy[dr].indexhackmode > 1 && !(selected & (1 << dr))) {
		    floppy[dr].indexhack = 1;
	    }
	  }
  }

	// step goes high and drive was selected when step pulse changes: step
  if (prev_step != step_pulse) {
  	prev_step = step_pulse;
  	if (prev_step && !savestate_state) {
	    for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
    		if (!((prev_selected | disabled) & (1 << dr))) {
  				drive_step (floppy + dr, direction);
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
	    if (!(selected & (1 << dr)) && (prev_selected & (1 << dr)) ) {
		    drv->drive_id_scnt++;
		    drv->drive_id_scnt &= 31;
		    drv->idbit = (drv->drive_id & (1L << (31 - drv->drive_id_scnt))) ? 1 : 0;
		    if (!(disabled & (1 << dr))) {
  		    if ((prev_data & 0x80) == 0 || (data & 0x80) == 0) {
      			/* motor off: if motor bit = 0 in prevdata or data -> turn motor on */
      			drive_motor (drv, 0);
  		    } else if (prev_data & 0x80) {
			      /* motor on: if motor bit = 1 in prevdata only (motor flag state in data has no effect)
	         -> turn motor off */
			      drive_motor (drv, 1);
  		    }
        }
				if (/*!currprefs.cs_df0idhw &&*/ dr == 0)
		      drv->idbit = 0;
      }
    }
  }

  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	  floppy[dr].state = (!(selected & (1 << dr))) | !floppy[dr].motoroff;
		update_drive_gui (dr, false);
  }
  prev_data = data;
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
      		if (drv->dskready && !drv->indexhack && currprefs.floppyslots[dr].dfxtype != DRV_35_DD_ESCOM)
		        st &= ~0x20;
        }
	    } else {
				if (/*currprefs.cs_df0idhw ||*/ dr > 0) {
		      /* report drive ID */
		      if (drv->idbit && currprefs.floppyslots[dr].dfxtype != DRV_35_DD_ESCOM)
		        st &= ~0x20;
		    } else {
		      /* non-ID internal drive: mirror real dskready */
		      if (drv->dskready)
		      	st &= ~0x20;
		    }
		    /* dskrdy needs some cycles after switching the motor off.. (Pro Tennis Tour) */
				if (/*!currprefs.cs_df0idhw &&*/ dr == 0 && drv->motordelay)
		      st &= ~0x20;
	    }
	    if (drive_track0 (drv))
    		st &= ~0x10;
	    if (drive_writeprotected (drv))
    		st &= ~8;
  	  if (drv->dskchange && currprefs.floppyslots[dr].dfxtype != DRV_525_SD) {
  		  st &= ~4;
  	  }
  	} else if (!(selected & (1 << dr))) {
	    if (drv->idbit)
    		st &= ~0x20;
  	}
  }
  return st;
}

static bool unformatted (drive *drv)
{
  int tr = drv->cyl * 2 + side;
  if (tr >= drv->num_tracks)
  	return true;
  if (drv->filetype == ADF_EXT2 && drv->trackdata[tr].bitlen == 0 && drv->trackdata[tr].type != TRACK_AMIGADOS)
  	return true;
	if (drv->trackdata[tr].type == TRACK_NONE)
		return true;
  return false;
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
  dskdmaen = DSKDMA_OFF;
	dsklength = 0;
}

static void fetchnextrevolution (drive *drv)
{
	if (drv->revolution_check)
		return;
  drv->trackspeed = get_floppy_speed2 (drv);
	drv->revolution_check = 2;
  if (!drv->multi_revolution)
  	return;
}

static void do_disk_index (void)
{
	if (!indexdecay) {
		indexdecay = 2;
		cia_diskindex ();
	}
}

void DISK_handler (uae_u32 data)
{
	int flag = data & 255;
	int disk_sync_cycle = data >> 8;
	int hpos = current_hpos ();

  event2_remevent(ev2_disk);
  DISK_update (disk_sync_cycle);
	if (!dskdmaen) {
    if (flag & (DISK_REVOLUTION << 0))
      fetchnextrevolution (&floppy[0]);
    if (flag & (DISK_REVOLUTION << 1))
      fetchnextrevolution (&floppy[1]);
    if (flag & (DISK_REVOLUTION << 2))
      fetchnextrevolution (&floppy[2]);
    if (flag & (DISK_REVOLUTION << 3))
      fetchnextrevolution (&floppy[3]);
	}
  if (flag & DISK_WORDSYNC)
    INTREQ (0x8000 | 0x1000);
	if (flag & DISK_INDEXSYNC)
		do_disk_index ();
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
    if (dmaen (DMA_DISK) && dskdmaen == DSKDMA_WRITE && dsklength > 0 && fifo_filled) {
      bitoffset++;
      bitoffset &= 15;
      if (!bitoffset) {
				// fast disk modes, fill the fifo instantly
				if (currprefs.floppy_speed > 100 && !fifo_inuse[0] && !fifo_inuse[1] && !fifo_inuse[2]) {
					while (!fifo_inuse[2]) {
						uae_u16 w = chipmem_wget_indirect (dskpt);
						DSKDAT (w);
						dskpt += 2;
					}
				}
				if (disk_fifostatus () >= 0) {
				  uae_u16 w = DSKDATR ();
	        for (dr = 0; dr < MAX_FLOPPY_DRIVES ; dr++) {
            drive *drv2 = &floppy[dr];
            if (drives[dr]) {
	            drv2->bigmfmbuf[drv2->mfmpos >> 4] = w;
        			drv2->bigmfmbuf[(drv2->mfmpos >> 4) + 1] = 0x5555;
	            drv2->writtento = 1;
    		    }
	        }
	        dsklength--;
	        if (dsklength <= 0) {
	          disk_dmafinished ();
					  for (int dr = 0; dr < MAX_FLOPPY_DRIVES ; dr++) {
						  drive *drv = &floppy[dr];
						  drv->writtento = 0;
						  if (drv->motoroff)
							  continue;
						  if (selected & (1 << dr))
							  continue;
						  drive_write_data (drv);
	          }
	        }
        }
      }
    }
    floppybits -= drv->trackspeed;
  }
}

static void update_jitter (void)
{
	disk_jitter = ((uaerand () >> 4) % 3) + 1;
}

static void updatetrackspeed (drive *drv, int mfmpos)
{
	if (dskdmaen < DSKDMA_WRITE) {
		int t = drv->tracktiming[mfmpos / 8];
		int ts = get_floppy_speed2 (drv) * t / 1000;
  	if (ts < 700 || ts > 3000) {
		} else {
			drv->trackspeed = ts;
		}
  }
}

static void disk_doupdate_predict (int startcycle)
{
	int finaleventcycle = maxhpos << 8;
	int finaleventflag = 0;

	for (int dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (drv->motoroff)
			continue;
		if (!drv->trackspeed)
			continue;
		if (selected & (1 << dr))
			continue;
		int mfmpos = drv->mfmpos;
		if (drv->tracktiming[0])
			updatetrackspeed (drv, mfmpos);
		int diskevent_flag = 0;
    uae_u32 tword = word;
		//int diff = drv->floppybitcounter % drv->trackspeed;
		int countcycle = startcycle; // + (diff ? drv->trackspeed - diff : 0);
		while (countcycle < (maxhpos << 8)) {
    	if (drv->tracktiming[0])
	      updatetrackspeed (drv, mfmpos);
			countcycle += drv->trackspeed;
			if (dskdmaen != DSKDMA_WRITE || (dskdmaen == DSKDMA_WRITE && !dma_enable)) {
	      tword <<= 1;
	      if (!drive_empty (drv)) {
	        if (unformatted (drv))
		        tword |= (uaerand() & 0x1000) ? 1 : 0;
	        else
	          tword |= getonebit (drv->bigmfmbuf, mfmpos);
	      }
				if (dskdmaen != DSKDMA_READ && (tword & 0xffff) == dsksync && dsksync != 0)
		      diskevent_flag |= DISK_WORDSYNC;
	    }
	    mfmpos++;
	    mfmpos %= drv->tracklen;
			if (!dskdmaen) {
	      if (mfmpos == 0)
          diskevent_flag |= DISK_REVOLUTION << (drv - floppy);
			  if (mfmpos == drv->indexoffset)
          diskevent_flag |= DISK_INDEXSYNC;
			}
			if (dskdmaen != DSKDMA_WRITE && mfmpos == drv->skipoffset) {
			  update_jitter ();
        int skipcnt = disk_jitter;
	      while (skipcnt-- > 0) {
		      mfmpos++;
		      mfmpos %= drv->tracklen;
					if (!dskdmaen) {
		        if (mfmpos == 0)
	            diskevent_flag |= DISK_REVOLUTION << (drv - floppy);
					  if (mfmpos == drv->indexoffset)
	            diskevent_flag |= DISK_INDEXSYNC;
	        }
      	}
			}
			if (diskevent_flag)
				break;
    }
    if (drv->tracktiming[0])
        updatetrackspeed (drv, drv->mfmpos);
		if (diskevent_flag && countcycle < finaleventcycle) {
			finaleventcycle = countcycle;
			finaleventflag = diskevent_flag;
		}
	}
	if (finaleventflag && (finaleventcycle >> 8) < maxhpos) {
		event2_newevent (ev2_disk, (finaleventcycle - startcycle) >> 8, ((finaleventcycle >> 8) << 8) | finaleventflag);
  }
}

int disk_fifostatus (void)
{
	if (fifo_inuse[0] && fifo_inuse[1] && fifo_inuse[2])
		return 1;
	if (!fifo_inuse[0] && !fifo_inuse[1] && !fifo_inuse[2])
		return -1;
	return 0;
}

static int doreaddma (void)
{
	if (dmaen (DMA_DISK) && bitoffset == 15 && dma_enable && dskdmaen == DSKDMA_READ && dsklength >= 0) {
		if (dsklength > 0) {
			// DSKLEN == 1: finish without DMA transfer.
			if (dsklength == 1 && dsklength2 == 1) {
				disk_dmafinished ();
				return 0;
			}
			// fast disk modes, just flush the fifo
			if (currprefs.floppy_speed > 100 && fifo_inuse[0] && fifo_inuse[1] && fifo_inuse[2]) {
				while (fifo_inuse[0]) {
					uae_u16 w = DSKDATR ();
					chipmem_wput_indirect (dskpt, w);
					dskpt += 2;
				}
			}
			if (disk_fifostatus () > 0) {
				write_log (_T("doreaddma() fifo overflow detected, retrying..\n"));
				return -1;
			} else {
			  DSKDAT (word);
			  dsklength--;
		  }
	  }
		return 1;
	}
	return 0;
}

static void disk_doupdate_read_nothing (int floppybits)
{
  int j = 0, k = 1, l = 0;

  while (floppybits >= get_floppy_speed()) {
  	word <<= 1;
	  doreaddma ();
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
		if (doreaddma () < 0) {
			word >>= 1;
			return;
		}
	  drv->mfmpos++;
	  drv->mfmpos %= drv->tracklen;
	  if (drv->mfmpos == drv->indexoffset) {
	    drv->indexhack = 0;
			do_disk_index ();
	  }
		if (drv->mfmpos == 0) {
			fetchnextrevolution (drv);
			if (drv->tracktiming[0])
				updatetrackspeed (drv, drv->mfmpos);
		}
	  if (drv->mfmpos == drv->skipoffset) {
			update_jitter ();
			int skipcnt = disk_jitter;
			while (skipcnt-- > 0) {
				drv->mfmpos++;
  	    drv->mfmpos %= drv->tracklen;
				if (drv->mfmpos == drv->indexoffset) {
					drv->indexhack = 0;
					do_disk_index ();
    	  }
				if (drv->mfmpos == 0) {
					fetchnextrevolution (drv);
					if (drv->tracktiming[0])
						updatetrackspeed (drv, drv->mfmpos);
				}
			}
	  }
  	if ((bitoffset & 7) == 7) {
	    dskbytr_val = word & 0xff;
	    dskbytr_val |= 0x8000;
  	}
  	if (word == dsksync) {
			dsksync_cycles = get_cycles () + WORDSYNC_TIME * CYCLE_UNIT;
	    if (dskdmaen != DSKDMA_OFF) {
		    dma_enable = 1;
				INTREQ (0x8000 | 0x1000);
	    }
			if (adkcon & 0x400) {
				bitoffset = 15;
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
  if (word == dsksync && cycles_in_range (dsksync_cycles))
    v |= 0x1000;
  if (dskdmaen != DSKDMA_OFF && dmaen (DMA_DISK))
    v |= 0x4000;
  if (dsklen & 0x4000)
    v |= 0x2000;
	for (int dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (drv->motoroff)
			continue;
		if (!(selected & (1 << dr))) {
			drv->lastdataacesstrack = drv->cyl * 2 + side;
			drv->track_access_done = true;
		}
	}
  return v;
}

static void DISK_start (void)
{
  int dr;

	for (int i = 0; i < 3; i++)
		fifo_inuse[i] = false;
	fifo_filled = 0;
  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	  drive *drv = &floppy[dr];
	  if (!(selected & (1 << dr))) {
	    int tr = drv->cyl * 2 + side;
	    trackid *ti = drv->trackdata + tr;

			if (dskdmaen == DSKDMA_WRITE) {
		    drv->tracklen = longwritemode ? FLOPPY_WRITE_MAXLEN : FLOPPY_WRITE_LEN * drv->ddhd * 8 * 2;
		    drv->trackspeed = get_floppy_speed();
		    drv->skipoffset = -1;
		    updatemfmpos (drv);
	    }
	    /* Ugh.  A nasty hack.  Assume ADF_EXT1 tracks are always read
	       from the start.  */
	    if (ti->type == TRACK_RAW1) {
    		drv->mfmpos = 0;
				bitoffset = 0;
			}
  	}
    drv->floppybitcounter = 0;
  }
	dma_enable = (adkcon & 0x400) ? 0 : 1;
}

static int linecounter;

void DISK_hsync (void)
{
  int dr;

  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
  	drive *drv = &floppy[dr];
  	if (drv->steplimit)
	    drv->steplimit--;
		if (drv->revolution_check)
			drv->revolution_check--;

		if (drv->dskready_down_time > 0)
			drv->dskready_down_time--;
		/* emulate drive motor turn on time */
		if (drv->dskready_up_time > 0 && !drive_empty (drv)) {
			drv->dskready_up_time--;
			if (drv->dskready_up_time == 0 && !drv->motoroff)
				drv->dskready = true;
		}
		/* delay until new disk image is inserted */
		if (drv->dskchange_time > 0) {
			drv->dskchange_time--;
			if (drv->dskchange_time == 0) {
				drive_insert (drv, &currprefs, dr, drv->newname, false, drv->newnamewriteprotected);
				update_drive_gui (dr, false);
			}
		}
  }
	if (indexdecay)
		indexdecay--;
  if (linecounter) {
  	linecounter--;
  	if (! linecounter)
     disk_dmafinished ();
	  return;
  }
	DISK_update (maxhpos);
}

void DISK_update (int tohpos)
{
  int dr;
	int cycles;
  int startcycle = disk_hpos;

	if (disk_hpos < 0) {
		disk_hpos = - disk_hpos;
		return;
	}

	cycles = (tohpos << 8) - disk_hpos;
  if (cycles <= 0)
  	return;
  disk_hpos += cycles;
  if (disk_hpos >= (maxhpos << 8))
		disk_hpos %= 1 << 8;

  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
    drive *drv = &floppy[dr];

    if (drv->motoroff || !drv->tracklen || !drv->trackspeed)
      continue;
    drv->floppybitcounter += cycles;
    if (selected & (1 << dr)) {
      drv->mfmpos += drv->floppybitcounter / drv->trackspeed;
      drv->mfmpos %= drv->tracklen;
      drv->floppybitcounter %= drv->trackspeed;
      continue;
    }
    if (drv->diskfile)
      drive_fill_bigbuf (drv, 0);
    drv->mfmpos %= drv->tracklen;
  }
	int didaccess = 0;
  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
    drive *drv = &floppy[dr];
    if (drv->motoroff || !drv->trackspeed)
      continue;
    if (selected & (1 << dr))
      continue;
		/* write dma and wordsync enabled: read until wordsync match found */
		if (dskdmaen == DSKDMA_WRITE && dma_enable)
      disk_doupdate_write (drv, drv->floppybitcounter);
    else
      disk_doupdate_read (drv, drv->floppybitcounter);
    drv->floppybitcounter %= drv->trackspeed;
		didaccess = 1;
  }
  /* no floppy selected but read dma */
	if (!didaccess && dskdmaen == DSKDMA_READ) {
	  disk_doupdate_read_nothing (cycles);
  }

	/* instantly finish dma if dsklen==0 and wordsync detected */
	if (dskdmaen != DSKDMA_OFF && dma_enable && dsklength2 == 0 && dsklength == 0)
		disk_dmafinished ();

	disk_doupdate_predict (disk_hpos);
}

void DSKLEN (uae_u16 v, int hpos)
{
  int dr, prev = dsklen;
  int noselected = 0;
  int motormask;

  DISK_update (hpos);
  if ((v & 0x8000) && (dsklen & 0x8000)) {
  	dskdmaen = DSKDMA_READ;
    DISK_start ();
  }
  if (!(v & 0x8000)) {
		if (dskdmaen != DSKDMA_OFF) {
      /* Megalomania and Knightmare does this */
      if (dskdmaen == DSKDMA_WRITE) {
				write_log (_T("warning: Disk write DMA aborted, %d words left PC=%x\n"), dsklength, M68K_GETPC);
	      // did program write something that needs to be stored to file?
        for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
	        drive *drv2 = &floppy[dr];
	        if (!drv2->writtento)
		        continue;
	        drive_write_data (drv2);
	      }
      }
			dskdmaen = DSKDMA_OFF;
    }
  }
  dsklen = v;
  dsklength2 = dsklength = dsklen & 0x3fff;

	if (dskdmaen == DSKDMA_OFF)
    return;

  if (dsklength == 0 && dma_enable) {
	  disk_dmafinished ();
	  return;
  }

  if ((v & 0x4000) && (prev & 0x4000)) {
    if (dsklength == 0)
      return;
    if (dsklength == 1) {
      disk_dmafinished();
      return;
    }
		dskdmaen = DSKDMA_WRITE;
    DISK_start ();
  }

	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (drv->motoroff)
			continue;
		if (selected & (1 << dr))
			continue;
		if (dskdmaen == DSKDMA_READ) {
			drv->lastdataacesstrack = drv->cyl * 2 + side;
			drv->track_access_done = true;
		}
	}


  motormask = 0;
  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
    drive *drv = &floppy[dr];
    drv->writtento = 0;
    if (drv->motoroff)
	    continue;
    motormask |= 1 << dr;
    if ((selected & (1 << dr)) == 0)
      break;
  }
  if (dr == 4) {
		write_log (_T("disk %s DMA started, drvmask=%x motormask=%x PC=%08x\n"),
			dskdmaen == DSKDMA_WRITE ? _T("write") : _T("read"), selected ^ 15, motormask, M68K_GETPC);
    noselected = 1;
  }

  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++)
		update_drive_gui (dr, false);

  /* Try to make floppy access from Kickstart faster.  */
	if (dskdmaen != DSKDMA_READ && dskdmaen != DSKDMA_WRITE)
    return;

  for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
    drive *drv = &floppy[dr];
    if (selected & (1 << dr))
      continue;
		if (drv->filetype != ADF_NORMAL && drv->filetype != ADF_KICK && drv->filetype != ADF_SKICK)
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

			if (dskdmaen == DSKDMA_READ) { /* TURBO read */
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
	        chipmem_wput_indirect (dskpt, drv->bigmfmbuf[pos >> 4]);
	        dskpt += 2;
	        pos += 16;
	        pos %= drv->tracklen;
	      }
				drv->mfmpos = pos;
	      INTREQ (0x8000 | 0x1000);
	      done = 1;

			} else if (dskdmaen == DSKDMA_WRITE) { /* TURBO write */

		    for (i = 0; i < dsklength; i++) {
          uae_u16 w = chipmem_wget_indirect (dskpt + i * 2);
		      drv->bigmfmbuf[pos >> 4] = w;
		      pos += 16;
		      pos %= drv->tracklen;
		    }
				drv->mfmpos = pos;
		    drive_write_data (drv);
		    done = 1;
	    }
	  }
	  if (!done && noselected) {
	    while (dsklength-- > 0) {
		    if (dskdmaen == DSKDMA_WRITE) {
		        uae_u16 w = chipmem_wget_indirect (dskpt);
    		} else {
	        chipmem_wput_indirect (dskpt, 0);
		    }
	      dskpt += 2;
	    }
	    INTREQ (0x8000 | 0x1000);
	    done = 1;
	  }

	  if (done) {
		  linecounter = 2;
			dskdmaen = DSKDMA_OFF;
		  return;
	  }
  }
}

void DISK_update_adkcon (int hpos, uae_u16 v)
{
	uae_u16 vold = adkcon;
	uae_u16 vnew = adkcon;
	if (v & 0x8000)
		 vnew |= v & 0x7FFF;
	else
		vnew &= ~v;
	if ((vnew & 0x400) && !(vold & 0x400))
		bitoffset = 0;
}

void DSKSYNC (int hpos, uae_u16 v)
{
  if (v == dsksync)
  	return;
  DISK_update (hpos);
  dsksync = v;
}

STATIC_INLINE bool iswrite (void)
{
	return dskdmaen == 3;
}

void DSKDAT (uae_u16 v)
{
	if (fifo_inuse[2]) {
		write_log (_T("DSKDAT: FIFO overflow!\n"));
		return;
	}
	fifo_inuse[2] = fifo_inuse[1];
	fifo[2] = fifo[1];
	fifo_inuse[1] = fifo_inuse[0];
	fifo[1] = fifo[0];
	fifo_inuse[0] = iswrite () ? 2 : 1;
	fifo[0] = v;
	fifo_filled = 1;
}
uae_u16 DSKDATR (void)
{
	int i;
	uae_u16 v = 0;
	for (i = 2; i >= 0; i--) {
		if (fifo_inuse[i]) {
			fifo_inuse[i] = 0;
			v = fifo[i];
			break;
		}
	}
	if (i < 0) {
		write_log (_T("DSKDATR: FIFO underflow!\n"));
	} else 	if (dskdmaen > 0 && dskdmaen < 3 && dsklength <= 0 && disk_fifostatus () < 0) {
		disk_dmafinished ();
	}
	return v;
}

uae_u16 disk_dmal (void)
{
	uae_u16 dmal = 0;
	if (dskdmaen) {
		if (dskdmaen == 3) {
			dmal = (1 + 2) * (fifo_inuse[0] ? 1 : 0) + (4 + 8) * (fifo_inuse[1] ? 1 : 0) + (16 + 32) * (fifo_inuse[2] ? 1 : 0);
			dmal ^= 63;
			if (dsklength == 2)
				dmal &= ~(16 + 32);
			if (dsklength == 1)
				dmal &= ~(16 + 32 + 4 + 8);
		} else {
			dmal = 16 * (fifo_inuse[0] ? 1 : 0) + 4 * (fifo_inuse[1] ? 1 : 0) + 1 * (fifo_inuse[2] ? 1 : 0);
		}
	}
	return dmal;
}
uaecptr disk_getpt (void)
{
	uaecptr pt = dskpt;
	dskpt += 2;
	return pt;
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
		if (!drive_insert (drv, &currprefs, dr, currprefs.floppyslots[dr].df, false, currprefs.floppyslots[dr].forcedwriteprotect))
	    disk_eject (dr);
  }
	if (disk_empty (0))
		write_log (_T("No disk in drive 0.\n"));
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

static void load_track (int num, int cyl, int side, int *sectable)
{
	int oldcyl, oldside, drvsec;

	drive *drv = &floppy[num];

	oldcyl = drv->cyl;
	oldside = side;
	drv->cyl = cyl;
	side = 0;
	drv->buffered_cyl = -1;
	drive_fill_bigbuf (drv, 1);
	decode_buffer (drv->bigmfmbuf, drv->cyl, 11, drv->ddhd, drv->filetype, &drvsec, sectable, 1);
	drv->cyl = oldcyl;
	side = oldside;
	drv->buffered_cyl = -1;
}

int DISK_examine_image (struct uae_prefs *p, int num, struct diskinfo *di)
{
  int drvsec;
  int ret, i;
  drive *drv = &floppy[num];
  uae_u32 dos, crc, crc2;
  int wasdelayed = drv->dskchange_time;
  int sectable[MAX_SECTORS];
	int oldcyl, oldside;
  uae_u32 v;

  ret = 0;
	memset (di, 0, sizeof (struct diskinfo));
	di->unreadable = true;
	oldcyl = drv->cyl;
	oldside = side;
  drv->cyl = 0;
  side = 0;
	if (!drive_insert (drv, p, num, p->floppyslots[num].df, true, true) || !drv->diskfile) {
		drv->cyl = oldcyl;
		side = oldside;
    return 1;
	}
	di->crc32 = zfile_crc32 (drv->diskfile);
	di->unreadable = false;
  decode_buffer (drv->bigmfmbuf, drv->cyl, 11, drv->ddhd, drv->filetype, &drvsec, sectable, 1);
	di->hd = drvsec == 22;
	drv->cyl = oldcyl;
	side = oldside;
  if (sectable[0] == 0 || sectable[1] == 0) {
    ret = 2;
		goto end2;
  }
  crc = crc2 = 0;
  for (i = 0; i < 1024; i += 4) {
		di->bootblock[i + 0] = writebuffer[i + 0];
		di->bootblock[i + 1] = writebuffer[i + 1];
		di->bootblock[i + 2] = writebuffer[i + 2];
		di->bootblock[i + 3] = writebuffer[i + 3];
  	v = (writebuffer[i] << 24) | (writebuffer[i + 1] << 16) | (writebuffer[i + 2] << 8) | writebuffer[i + 3];
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
	di->bb_crc_valid = true;
	writebuffer[4] = writebuffer[5] = writebuffer[6] = writebuffer[7] = 0;
	if (get_crc32 (writebuffer, 0x31) == 0xae5e282c) {
		di->bootblocktype = 1;
	}
  if (dos == 0x444f5300)
  	ret = 10;
  else if (dos == 0x444f5301 || dos == 0x444f5302 || dos == 0x444f5303)
  	ret = 11;
  else if (dos == 0x444f5304 || dos == 0x444f5305 || dos == 0x444f5306 || dos == 0x444f5307)
  	ret = 12;
  else
  	ret = 4;
	v = get_crc32 (writebuffer + 8, 0x5c - 8);
	if (ret >= 10 && v == 0xe158ca4b) {
		di->bootblocktype = 2;
	}
end:
	load_track (num, 40, 0, sectable);
	if (sectable[0]) {
		if (!disk_checksum (writebuffer, NULL) &&
			writebuffer[0] == 0 && writebuffer[1] == 0 && writebuffer[2] == 0 && writebuffer[3] == 2 &&
			writebuffer[508] == 0 && writebuffer[509] == 0 && writebuffer[510] == 0 && writebuffer[511] == 1) {
			writebuffer[512 - 20 * 4 + 1 + writebuffer[512 - 20 * 4]] = 0;
			TCHAR *n = au ((const char*)(writebuffer + 512 - 20 * 4 + 1));
			if (_tcslen (n) >= sizeof (di->diskname))
				n[sizeof (di->diskname) - 1] = 0;
			_tcscpy (di->diskname, n);
			xfree (n);
		}
	}
end2:
  drive_image_free (drv);
  if (wasdelayed > 1) {
	  drive_eject (drv);
	  currprefs.floppyslots[num].df[0] = 0;
	  drv->dskchange_time = wasdelayed;
	  disk_insert (num, drv->newname);
  }
  return ret;
}

/* Disk save/restore code */

#if defined SAVESTATE || defined DEBUGGER

void DISK_save_custom (uae_u32 *pdskpt, uae_u16 *pdsklength, uae_u16 *pdsksync, uae_u16 *pdskbytr)
{
  if (pdskpt) 
    *pdskpt = dskpt;
  if (pdsklength) 
    *pdsklength = dsklen;
  if (pdsksync) 
    *pdsksync = dsksync;
  if(pdskbytr) 
    *pdskbytr = dskbytr_val;
}

#endif /* SAVESTATE || DEBUGGER */

static uae_u32 getadfcrc (drive *drv)
{
	uae_u8 *b;
	uae_u32 crc32;
	int size;

	if (!drv->diskfile)
		return 0;
	zfile_fseek (drv->diskfile, 0, SEEK_END);
	size = zfile_ftell (drv->diskfile);
	b = xmalloc (uae_u8, size);
	if (!b)
		return 0;
	zfile_fseek (drv->diskfile, 0, SEEK_SET);
	zfile_fread (b, 1, size, drv->diskfile);
	crc32 = get_crc32 (b, size);
	free (b);
	return crc32;
}

#ifdef SAVESTATE

void DISK_restore_custom (uae_u32 pdskpt, uae_u16 pdsklength, uae_u16 pdskbytr)
{
  dskpt = pdskpt;
  dsklen = pdsklength;
  dskbytr_val = pdskbytr;
}

void restore_disk_finish (void)
{
	int cnt = 0;
	for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
		if (currprefs.floppyslots[i].dfxtype >= 0) {
			update_drive_gui (i, true);
			cnt++;
		}
	}
	currprefs.nr_floppies = changed_prefs.nr_floppies = cnt;
	DISK_check_change ();
}

uae_u8 *restore_disk(int num,uae_u8 *src)
{
  drive *drv;
  int state;
  TCHAR old[MAX_DPATH];
  TCHAR *s;
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
  	changed_prefs.floppyslots[num].dfxtype = -1;
  } else {
 		drv->motoroff = (state & 1) ? 0 : 1;
		drv->idbit = (state & 4) ? 1 : 0;
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
    currprefs.floppyslots[num].dfxtype = changed_prefs.floppyslots[num].dfxtype = dfxtype;
	}
	drv->dskchange = (state & 8) != 0;
  side = (state & 16) ? 1 : 0;
  drv->indexhackmode = 0;
  if (num == 0 && currprefs.floppyslots[num].dfxtype == 0)
  	drv->indexhackmode = 1;
  drv->buffered_cyl = -1;
  drv->buffered_side = -1;
  drv->cyl = restore_u8 ();
  drv->dskready = restore_u8 () != 0;
  drv->drive_id_scnt = restore_u8 ();
  int mfmpos = restore_u32 ();
  drv->dskchange_time = 0;
  restore_u32 ();
  s = restore_path (SAVESTATE_PATH_FLOPPY);
  _tcscpy (old, currprefs.floppyslots[num].df);
  _tcsncpy(changed_prefs.floppyslots[num].df, s,255);
  xfree (s);
  int dskready_up_time = restore_u16 ();
  int dskready_down_time = restore_u16 ();
  newis = changed_prefs.floppyslots[num].df[0] ? 1 : 0;
  if (!(disabled & (1 << num))) {
  	if (!newis && old[0]) {
			*currprefs.floppyslots[num].df = *changed_prefs.floppyslots[num].df = 0;
			drv->dskchange = false;
		} else if (newis) {
			drive_insert (floppy + num, &currprefs, num, changed_prefs.floppyslots[num].df, false, false);
      if (drive_empty (floppy + num)) {
      	if (newis && old[0]) {
    	    _tcscpy (changed_prefs.floppyslots[num].df, old);
					drive_insert (floppy + num, &currprefs, num, changed_prefs.floppyslots[num].df, false, false);
    	    if (drive_empty (floppy + num))
						drv->dskchange = true;
				} else {
					drv->dskchange_time = -1;
        }
      }
	  }
  }
	drv->mfmpos = mfmpos;
	drv->prevtracklen = drv->tracklen;
	drv->dskready_up_time = dskready_up_time;
	drv->dskready_down_time = dskready_down_time;
  reset_drive_gui(num);
  return src;
}

uae_u8 *restore_disk2 (int num,uae_u8 *src)
{
	drive *drv = &floppy[num];
	uae_u32 m = restore_u32 ();
	if (m) {
		drv->floppybitcounter = restore_u16 ();
		drv->tracklen = restore_u32 ();
		drv->trackspeed = restore_u16 ();
		drv->skipoffset = restore_u32 ();
		drv->indexoffset = restore_u32 ();
		drv->buffered_cyl = drv->cyl;
		drv->buffered_side = side;
		for (int j = 0; j < (drv->tracklen + 15) / 16; j++) {
			drv->bigmfmbuf[j] = restore_u16 ();
			if (m & 2)
				drv->tracktiming[j] = restore_u16 ();
		}
		drv->revolutions = restore_u16 ();
	}
  return src;
}

uae_u8 *save_disk (int num, int *len, uae_u8 *dstptr, bool usepath)
{
  uae_u8 *dstbak,*dst;
	drive *drv = &floppy[num];

  if (dstptr)
  	dstbak = dst = dstptr;
  else
	  dstbak = dst = xmalloc (uae_u8, 2 + 1 + 1 + 1 + 1 + 4 + 4 + 256);
  save_u32 (drv->drive_id);	    /* drive type ID */
	save_u8 ((drv->motoroff ? 0 : 1) | ((disabled & (1 << num)) ? 2 : 0) | (drv->idbit ? 4 : 0) | (drv->dskchange ? 8 : 0) | (side ? 16 : 0) | (drv->wrprot ? 32 : 0));
  save_u8 (drv->cyl);		    /* cylinder */
  save_u8 (drv->dskready);	    /* dskready */
  save_u8 (drv->drive_id_scnt);   /* id mode position */
  save_u32 (drv->mfmpos);	    /* disk position */
  save_u32 (getadfcrc (drv));	    /* CRC of disk image */
	save_path (usepath ? currprefs.floppyslots[num].df : _T(""), SAVESTATE_PATH_FLOPPY);/* image name */
	save_u16 (drv->dskready_up_time);
	save_u16 (drv->dskready_down_time);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_disk2 (int num, int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;
	drive *drv = &floppy[num];

	int m = 0;
	int size = 0;
	if (drv->motoroff == 0 && drv->buffered_side >= 0 && drv->tracklen > 0) {
		m = 1;
		if (drv->tracktiming[0])
			m |= 2;
		size += ((drv->tracklen + 15) * 2) / 8;
	}
	if (!m)
		return NULL;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 4 + 2 + 4 + 2 + 4 + 4 + size + 2);

	save_u32 (m);
	save_u16 (drv->floppybitcounter);
	save_u32 (drv->tracklen);
	save_u16 (drv->trackspeed);
	save_u32 (drv->skipoffset);
	save_u32 (drv->indexoffset);
	for (int j = 0; j < (drv->tracklen + 15) / 16; j++) {
		save_u16 (drv->bigmfmbuf[j]);
		if (drv->tracktiming[0])
			save_u16 (drv->tracktiming[j]);
	}
	save_u16 (drv->revolutions);

  *len = dst - dstbak;
  return dstbak;
}

/* internal floppy controller variables */

uae_u8 *restore_floppy(uae_u8 *src)
{
  word = restore_u16();
  bitoffset = restore_u8();
  dma_enable = restore_u8();
	disk_hpos = restore_u8 () & 0xff;
  dskdmaen = restore_u8();
	for (int i = 0; i < 3; i++) {
		fifo[i] = restore_u16 ();
		fifo_inuse[i] = restore_u8 ();
		if (dskdmaen == 0)
			fifo_inuse[i] = false;
	}
	fifo_filled = fifo_inuse[0] || fifo_inuse[1] || fifo_inuse[2];
	dsklength = restore_u16 ();
  return src;
}

uae_u8 *save_floppy(int *len, uae_u8 *dstptr)
{
  uae_u8 *dstbak, *dst;

  if (dstptr)
  	dstbak = dst = dstptr;
  else
		dstbak = dst = xmalloc (uae_u8, 100);
	save_u16 (word);			/* shift register */
  save_u8 (bitoffset);	/* dma bit offset */
  save_u8 (dma_enable);	/* disk sync found */
	save_u8 (disk_hpos & 0xff);	/* next bit read position */
  save_u8 (dskdmaen);		/* dma status */
	for (int i = 0; i < 3; i++) {
		save_u16 (fifo[i]);
		save_u8 (fifo_inuse[i]);
	}
	save_u16 (dsklength);
  *len = dst - dstbak;
  return dstbak;
}

#endif /* SAVESTATE */
