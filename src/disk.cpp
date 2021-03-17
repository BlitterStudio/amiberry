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

int disk_debug_logging = 0;
int disk_debug_mode = 0;
int disk_debug_track = -1;

#define REVOLUTION_DEBUG 0
#define MFM_VALIDATOR 0

#include "uae.h"
#include "options.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "disk.h"
#include "gui.h"
#include "zfile.h"
#include "newcpu.h"
//#include "osemu.h"
//#include "execlib.h"
#include "savestate.h"
#include "cia.h"
//#include "debug.h"
#ifdef FDI2RAW
#include "fdi2raw.h"
#endif
//#include "catweasel.h"
#ifdef DRIVESOUND
#include "driveclick.h"
#endif
#ifdef CAPS
#include "uae/caps.h"
#endif
#ifdef SCP
#include "scp.h"
#endif
#include "crc32.h"
//#include "inputrecord.h"
//#include "amax.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#include "fsdb.h"
#include "statusline.h"
#include "rommgr.h"
#include "tinyxml2.h"

#undef CATWEASEL

int floppy_writemode = 0;

/* support HD floppies */
#define FLOPPY_DRIVE_HD
/* Writable track length with normal 2us bitcell/300RPM motor, 12688 PAL, 12784 NTSC */
/* DMA clock / (7 clocks per bit * 5 revs per second * 8 bits per byte) */
#define FLOPPY_WRITE_LEN_PAL 12668 // 3546895 / (7 * 5 * 8)
#define FLOPPY_WRITE_LEN_NTSC 12784 // 3579545 / (7 * 5 * 8)
#define FLOPPY_WRITE_LEN (currprefs.floppy_write_length > 256 ? currprefs.floppy_write_length / 2 : (currprefs.ntscmode ? (FLOPPY_WRITE_LEN_NTSC / 2) : (FLOPPY_WRITE_LEN_PAL / 2)))
#define FLOPPY_WRITE_MAXLEN 0x3800
/* This works out to 350 PAL, 408 NTSC */
#define FLOPPY_GAP_LEN (FLOPPY_WRITE_LEN - 11 * 544)
/* 7 CCK per bit */
#define NORMAL_FLOPPY_SPEED (7 * 256)
/* max supported floppy drives, for small memory systems */
#define MAX_FLOPPY_DRIVES 4

#ifdef FLOPPY_DRIVE_HD
#define DDHDMULT 2
#else
#define DDHDMULT 1
#endif
#define MAX_SECTORS (DDHDMULT * 11)

#undef DEBUG_DRIVE_ID

/* UAE-1ADF (ADF_EXT2)
* W reserved
* W number of tracks (default 2*80=160)
*
* W reserved
* W type, 0=normal AmigaDOS track, 1 = raw MFM (upper byte = disk revolutions - 1)
* L available space for track in bytes (must be even)
* L track length in bits
*/

static int side, direction, reserved_side;
static uae_u8 selected = 15, disabled, reserved;

static uae_u8 writebuffer[544 * MAX_SECTORS];
static uae_u8 writesecheadbuffer[16 * MAX_SECTORS];

#define DISK_INDEXSYNC 1
#define DISK_WORDSYNC 2
#define DISK_REVOLUTION 4 /* 8,16,32,64 */

#define DSKREADY_UP_TIME 18
#define DSKREADY_DOWN_TIME 24

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
static int dma_enable, bitoffset, syncoffset;
static uae_u16 word, dsksync;
static unsigned long dsksync_cycles;
#define WORDSYNC_TIME 11
/* Always carried through to the next line.  */
int disk_hpos;
static int disk_jitter;
static int indexdecay;
static uae_u8 prev_data;
static int prev_step;
static bool initial_disk_statusline;
static struct diskinfo disk_info_data = { 0 };
static bool amax_enabled;

typedef enum { TRACK_AMIGADOS, TRACK_RAW, TRACK_RAW1, TRACK_PCDOS, TRACK_DISKSPARE, TRACK_NONE } image_tracktype;
typedef struct {
	uae_u16 len;
	int offs, extraoffs;
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

typedef enum { ADF_NONE = -1, ADF_NORMAL, ADF_EXT1, ADF_EXT2, ADF_FDI, ADF_IPF, ADF_SCP, ADF_CATWEASEL, ADF_PCDOS, ADF_KICK, ADF_SKICK, ADF_NORMAL_HEADER } drive_filetype;
typedef struct {
	struct zfile *diskfile;
	struct zfile *writediskfile;
	struct zfile *pcdecodedfile;
	drive_filetype filetype;
	trackid trackdata[MAX_TRACKS];
	trackid writetrackdata[MAX_TRACKS];
	int buffered_cyl, buffered_side;
	int cyl;
	bool motoroff;
	int motordelay; /* dskrdy needs some clock cycles before it changes after switching off motor */
	bool state;
	int selected_delay;
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
	int num_tracks, write_num_tracks, num_secs, num_heads;
	int hard_num_cyls;
	bool dskeject;
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
#ifdef FDI2RAW
	FDI *fdi;
#endif
	int useturbo;
	int floppybitcounter; /* number of bits left */
#ifdef CATWEASEL
	catweasel_drive *catweasel;
#else
	int catweasel;
	int amax;
	int lastdataacesstrack;
	int lastrev;
	bool track_access_done;
	bool fourms;
#endif
} drive;

#define MIN_STEPLIMIT_CYCLE (CYCLE_UNIT * 140)

static uae_u16 bigmfmbufw[0x4000 * DDHDMULT];
static drive floppy[MAX_FLOPPY_DRIVES];
static TCHAR dfxhistory[HISTORY_MAX][MAX_PREVIOUS_IMAGES][MAX_DPATH];

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
	cs = (~cs) + 1;
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
#ifndef __PSP2__
	tv.tv_sec -= _timezone;
#endif
	mtv.tv_sec = tv.tv_sec;
	mtv.tv_usec = tv.tv_usec;
	timeval_to_amiga (&mtv, &days, &mins, &ticks, 50);
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
	if (strlen (dn) >= 30)
		dn[30] = 0;
	const char *dn2 = dn;
	if (dn2[0] == 0)
		dn2 = "empty";
	memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
	sector[0+3] = 2;
	sector[12+3] = 0x48;
	sector[312] = sector[313] = sector[314] = sector[315] = (uae_u8)0xff;
	sector[316+2] = 881 >> 8; sector[316+3] = 881 & 255;
	sector[432] = (uae_u8)strlen(dn2);
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
	sector[512 - 80] = (uae_u8)strlen(filename);
	strcpy ((char*)sector + 512 - 79, filename);
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
	size = (int)zfile_ftell(src);
	zfile_fseek (src, 0, SEEK_SET);
	extensions = (size + FS_OFS_DATABLOCKSIZE - 1) / FS_OFS_DATABLOCKSIZE;

	memset (sector, 0, FS_FLOPPY_BLOCKSIZE);
	pl (sector, 0, 2);
	pl (sector, 4, block);
	pl (sector, 8, extensions > FS_EXTENSION_BLOCKS ? FS_EXTENSION_BLOCKS : extensions);
	pl (sector, 16, datablock);
	pl (sector, FS_FLOPPY_BLOCKSIZE - 188, size);
	disk_date (sector + FS_FLOPPY_BLOCKSIZE - 92);
	sector[FS_FLOPPY_BLOCKSIZE - 80] = (uae_u8)strlen(filename);
	strcpy ((char*)sector + FS_FLOPPY_BLOCKSIZE - 79, filename);
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
	const char *fname1 = "runme.exe";
	const TCHAR *fname1b = _T("runme.adf");
	const char *fname2 = "startup-sequence";
	const char *dirname1 = "s";
	struct zfile *ss;
	int prevblock;

	memset (bitmap, 0, sizeof bitmap);
	zfile_fseek (src, 0, SEEK_END);
	exesize = (int)zfile_ftell(src);
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
	ss = zfile_fopen_empty (src, fname1b, strlen (fname1));
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

static int get_floppy_speed_from_image(drive *drv)
{
	int l, m;
	
	m = get_floppy_speed();
	l = drv->tracklen;
	drv->fourms = false;
	if (!drv->tracktiming[0]) {
		m = m * l / (2 * 8 * FLOPPY_WRITE_LEN * drv->ddhd);
	}
	// 4us track?
	if (l < (FLOPPY_WRITE_LEN_PAL * 8) * 4 / 6) {
		m *= 2;
		drv->fourms = true;
	}
	if (m <= 0) {
		m = 1;
	}

	return m;
}

static const TCHAR *drive_id_name(drive *drv)
{
	switch(drv->drive_id)
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
	case DRV_525_DD:
	default:
		drv->drive_id = DRIVE_ID_35DD;
		break;
	case DRV_525_SD:
		drv->drive_id = DRIVE_ID_525SD;
		break;
	case DRV_NONE:
	case DRV_PC_525_ONLY_40:
	case DRV_PC_525_40_80:
	case DRV_PC_35_ONLY_80:
		drv->drive_id = DRIVE_ID_NONE;
		break;
	}
#ifdef DEBUG_DRIVE_ID
	write_log (_T("drive_settype_id: DF%d: set to %s\n"), drv-floppy, drive_id_name(drv));
#endif
}

static void drive_image_free (drive *drv)
{
	switch (drv->filetype)
	{
	case ADF_IPF:
#ifdef CAPS
		caps_unloadimage (drv - floppy);
#endif
		break;
	case ADF_SCP:
#ifdef SCP
		scp_close (drv - floppy);
#endif
		break;
	case ADF_FDI:
#ifdef FDI2RAW
		fdi2raw_header_free (drv->fdi);
		drv->fdi = 0;
#endif
		break;
	}
	drv->filetype = ADF_NONE;
	zfile_fclose(drv->diskfile);
	drv->diskfile = NULL;
	zfile_fclose(drv->writediskfile);
	drv->writediskfile = NULL;
	zfile_fclose(drv->pcdecodedfile);
	drv->pcdecodedfile = NULL;
}

static int drive_insert (drive * drv, struct uae_prefs *p, int dnum, const TCHAR *fname, bool fake, bool writeprotected);

static void reset_drive_gui (int num)
{
	struct gui_info_drive *gid = &gui_data.drives[num];
	gid->drive_disabled = 0;
	gid->df[0] = 0;
	gid->crc32 = 0;
	if (currprefs.floppyslots[num].dfxtype < 0)
		gid->drive_disabled = 1;
}

static void setamax (void)
{
#ifdef AMAX
	amax_enabled = false;
	if (is_device_rom(&currprefs, ROMTYPE_AMAX, 0) > 0) {
		/* Put A-Max as last drive in drive chain */
		int j;
		amax_enabled = true;
		for (j = 0; j < MAX_FLOPPY_DRIVES; j++)
			if (floppy[j].amax)
				return;
		for (j = 0; j < MAX_FLOPPY_DRIVES; j++) {
			if ((1 << j) & disabled) {
				floppy[j].amax = 1;
				write_log (_T("AMAX: drive %d\n"), j);
				return;
			}
		}
	}
#endif
}

static bool ispcbridgedrive(int num)
{
	int type = currprefs.floppyslots[num].dfxtype;
	return type == DRV_PC_525_ONLY_40 || type == DRV_PC_35_ONLY_80 || type == DRV_PC_525_40_80;
}

static bool drive_writeprotected(drive *drv)
{
#ifdef CATWEASEL
	if (drv->catweasel)
		return 1;
#endif
	return currprefs.floppy_read_only || drv->wrprot || drv->forcedwrprot || drv->diskfile == NULL;
}

static void reset_drive (int num)
{
	drive *drv = &floppy[num];

	drv->amax = 0;
	drive_image_free (drv);
	drv->motoroff = 1;
	drv->idbit = 0;
	drv->drive_id = 0;
	drv->drive_id_scnt = 0;
	drv->lastdataacesstrack = -1;
	disabled &= ~(1 << num);
	reserved &= ~(1 << num);
	if (currprefs.floppyslots[num].dfxtype < 0 || ispcbridgedrive(num))
		disabled |= 1 << num;
	if (ispcbridgedrive(num))
		reserved |= 1 << num;
	reset_drive_gui (num);
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
	gui_led (num + LED_DF0, 0, -1);
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
	drive *drv = floppy + num;
	bool writ = dskdmaen == DSKDMA_WRITE && drv->state && !((selected | disabled) & (1 << num));
	struct gui_info_drive *gid = &gui_data.drives[num];

	if (!force && drv->state == gid->drive_motor
		&& drv->cyl == gid->drive_track
		&& side == gui_data.drive_side
		&& drv->crc32 == gid->crc32
		&& writ == gid->drive_writing
		&& drive_writeprotected(drv) == gid->floppy_protected
		&& !_tcscmp (gid->df, currprefs.floppyslots[num].df))
		return;
	_tcscpy (gid->df, currprefs.floppyslots[num].df);
	gid->crc32 = drv->crc32;
	gid->drive_motor = drv->state;
	gid->drive_track = drv->cyl;
	if (reserved & (1 << num))
		gui_data.drive_side = reserved_side;
	else
		gui_data.drive_side = side;
	gid->drive_writing = writ;
	gid->floppy_protected = drive_writeprotected(drv);
	gui_led (num + LED_DF0, (gid->drive_motor ? 1 : 0) | (gid->drive_writing ? 2 : 0), -1);
}

static void drive_fill_bigbuf (drive * drv,int);

int DISK_validate_filename (struct uae_prefs *p, const TCHAR *fname_in, TCHAR *outfname, int leave_open, bool *wrprot, uae_u32 *crc32, struct zfile **zf)
{
	TCHAR outname[MAX_DPATH];

	if (zf)
		*zf = NULL;
	if (crc32)
		*crc32 = 0;
	if (wrprot)
		*wrprot = p->floppy_read_only ? 1 : 0;

	cfgfile_resolve_path_out_load(fname_in, outname, MAX_DPATH, PATH_FLOPPY);
	if (outfname)
		_tcscpy(outfname, outname);

	if (leave_open || !zf) {
		struct zfile *f = zfile_fopen (outname, _T("r+b"), ZFD_NORMAL | ZFD_DISKHISTORY);
		if (!f) {
			if (wrprot)
				*wrprot = 1;
			f = zfile_fopen (outname, _T("rb"), ZFD_NORMAL | ZFD_DISKHISTORY);
		}
		if (f && crc32)
			*crc32 = zfile_crc32 (f);
		if (!zf)
			zfile_fclose (f);
		else
			*zf = f;
		return f ? 1 : 0;
	} else {
		if (zfile_exists (outname)) {
			if (wrprot && !p->floppy_read_only)
				*wrprot = 0;
			if (crc32) {
				struct zfile *f = zfile_fopen (outname, _T("rb"), ZFD_NORMAL | ZFD_DISKHISTORY);
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
	if (strncmp ((char*)buffer, "UAE-1ADF", 8))
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

static void saveimagecutpathpart(TCHAR *name)
{
	int i;

	i = _tcslen (name) - 1;
	while (i > 0) {
		if (name[i] == '/' || name[i] == '\\') {
			name[i] = 0;
			break;
		}
		if (name[i] == '.') {
			name[i] = 0;
			break;
		}
		i--;
	}
	while (i > 0) {
		if (name[i] == '/' || name[i] == '\\') {
			name[i] = 0;
			break;
		}
		i--;
	}
}

static void saveimagecutfilepart(TCHAR *name)
{
	TCHAR tmp[MAX_DPATH];
	int i;

	_tcscpy(tmp, name);
	i = _tcslen (tmp) - 1;
	while (i > 0) {
		if (tmp[i] == '/' || tmp[i] == '\\') {
			_tcscpy(name, tmp + i + 1);
			break;
		}
		if (tmp[i] == '.') {
			tmp[i] = 0;
			break;
		}
		i--;
	}
	while (i > 0) {
		if (tmp[i] == '/' || tmp[i] == '\\') {
			_tcscpy(name, tmp + i + 1);
			break;
		}
		i--;
	}
}

static void saveimageaddfilename(TCHAR *dst, const TCHAR *src, int type)
{
	_tcscat(dst, src);
	if (type)
		_tcscat(dst, _T(".save_adf"));
	else
		_tcscat(dst, _T("_save.adf"));
}

static TCHAR *DISK_get_default_saveimagepath (const TCHAR *name)
{
	TCHAR name1[MAX_DPATH];
	TCHAR path[MAX_DPATH];
	_tcscpy(name1, name);
	saveimagecutfilepart(name1);
	get_saveimage_path (path, sizeof path / sizeof (TCHAR), 1);
	saveimageaddfilename(path, name1, 0);
	return my_strdup(path);
}

// -2 = existing, if not, use 0.
// -1 = as configured
// 0 = saveimages-dir
// 1 = image dir
TCHAR *DISK_get_saveimagepath(const TCHAR *name, int type)
{
	int typev = type;

	for (int i = 0; i < 2; i++) {
		if (typev == 1 || (typev == -1 && saveimageoriginalpath) || (typev == -2 && (saveimageoriginalpath || i == 1))) {
			TCHAR si_name[MAX_DPATH], si_path[MAX_DPATH];
			_tcscpy(si_name, name);
			_tcscpy(si_path, name);
			saveimagecutfilepart(si_name);
			saveimagecutpathpart(si_path);
			_tcscat(si_path, FSDB_DIR_SEPARATOR_S);
			saveimageaddfilename(si_path, si_name, 1);
			if (typev != -2 || (typev == -2 && zfile_exists(si_path)))
				return my_strdup(si_path);
		}
		if (typev == 2 || (typev == -1 && !saveimageoriginalpath) || (typev == -2 && (!saveimageoriginalpath || i == 1))) {
			TCHAR *p = DISK_get_default_saveimagepath(name);
			if (typev != -2 || (typev == -2 && zfile_exists(p)))
				return p;
			xfree(p);
		}
	}
	return DISK_get_saveimagepath(name, -1);
}

static struct zfile *getexistingwritefile(struct uae_prefs *p, const TCHAR *name, bool *wrprot)
{
	struct zfile *zf = NULL;
	TCHAR *path;
	TCHAR outname[MAX_DPATH];

	path = DISK_get_saveimagepath(name, saveimageoriginalpath);
	DISK_validate_filename (p, path, outname, 1, wrprot, NULL, &zf);
	xfree(path);
	if (zf)
		return zf;
	path = DISK_get_saveimagepath(name, !saveimageoriginalpath);
	DISK_validate_filename (p, path, outname, 1, wrprot, NULL, &zf);
	xfree(path);
	return zf;
}

static int iswritefileempty (struct uae_prefs *p, const TCHAR *name)
{
	struct zfile *zf;
	bool wrprot;
	uae_char buffer[8];
	trackid td[MAX_TRACKS];
	int tracks, ddhd, i, ret;

	zf = getexistingwritefile (p, name, &wrprot);
	if (!zf)
		return 1;
	zfile_fread (buffer, sizeof (char), 8, zf);
	if (strncmp((uae_char*)buffer, "UAE-1ADF", 8)) {
		zfile_fclose(zf);
		return 0;
	}
	ret = read_header_ext2 (zf, td, &tracks, &ddhd);
	zfile_fclose (zf);
	if (!ret)
		return 1;
	for (i = 0; i < tracks; i++) {
		if (td[i].bitlen)
			return 0;
	}
	return 1;
}

static int openwritefile (struct uae_prefs *p, drive *drv, int create)
{
	bool wrprot = 0;

	drv->writediskfile = getexistingwritefile(p, currprefs.floppyslots[drv - &floppy[0]].df, &wrprot);
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

static bool diskfile_iswriteprotect (struct uae_prefs *p, const TCHAR *fname_in, int *needwritefile, drive_type *drvtype)
{
	struct zfile *zf1, *zf2;
	bool wrprot1 = 0, wrprot2 = 1;
	uae_char buffer[25];
	TCHAR outname[MAX_DPATH];

	*needwritefile = 0;
	*drvtype = DRV_35_DD;
	DISK_validate_filename (p, fname_in, outname, 1, &wrprot1, NULL, &zf1);
	if (!zf1)
		return 1;
	if (zfile_iscompressed (zf1)) {
		wrprot1 = 1;
		*needwritefile = 1;
	}
	zf2 = getexistingwritefile(p, fname_in, &wrprot2);
	zfile_fclose (zf2);
	zfile_fread (buffer, sizeof (char), 25, zf1);
	zfile_fclose (zf1);
	if (strncmp ((uae_char*) buffer, "CAPS", 4) == 0) {
		*needwritefile = 1;
		return wrprot2;
	}
	if (strncmp ((uae_char*) buffer, "SCP", 3) == 0) {
		*needwritefile = 1;
		return wrprot2;
	}
	if (strncmp ((uae_char*) buffer, "Formatted Disk Image file", 25) == 0) {
		*needwritefile = 1;
		return wrprot2;
	}
	if (strncmp ((uae_char*) buffer, "UAE-1ADF", 8) == 0) {
		if (wrprot1)
			return wrprot2;
		return wrprot1;
	}
	if (strncmp ((uae_char*) buffer, "UAE--ADF", 8) == 0) {
		*needwritefile = 1;
		return wrprot2;
	}
	if (memcmp (exeheader, buffer, sizeof exeheader) == 0)
		return 0;
	if (wrprot1)
		return wrprot2;
	return wrprot1;
}

static bool isrecognizedext(const TCHAR *name)
{
	const TCHAR *ext = _tcsrchr(name, '.');
	if (ext) {
		ext++;
		if (!_tcsicmp(ext, _T("adf")) || !_tcsicmp(ext, _T("adz")) || !_tcsicmp(ext, _T("st")) ||
			!_tcsicmp(ext, _T("ima")) || !_tcsicmp(ext, _T("img")) || !_tcsicmp(ext, _T("dsk")))
			return true;
	}
	return false;
}

static void update_disk_statusline(int num)
{
	//drive *drv = &floppy[num];
	//if (!drv->diskfile)
	//	return;
	//const TCHAR *fname = zfile_getoriginalname(drv->diskfile);
	//if (!fname)
	//	fname = zfile_getname(drv->diskfile);
	//if (!fname)
	//	fname = _T("?");
	//if (disk_info_data.diskname[0])
	//	statusline_add_message(STATUSTYPE_FLOPPY, _T("DF%d: [%s] %s"), num, disk_info_data.diskname, my_getfilepart(fname));
	//else
	//	statusline_add_message(STATUSTYPE_FLOPPY, _T("DF%d: %s"), num, my_getfilepart(fname));
}

static int drive_insert (drive * drv, struct uae_prefs *p, int dnum, const TCHAR *fname_in, bool fake, bool forcedwriteprotect)
{
	uae_u8 buffer[2 + 2 + 4 + 4];
	trackid *tid;
	int num_tracks, size;
	int canauto;
	TCHAR outname[MAX_DPATH];

	drive_image_free (drv);
	if (!fake)
		DISK_examine_image(p, dnum, &disk_info_data, false);
	DISK_validate_filename (p, fname_in, outname, 1, &drv->wrprot, &drv->crc32, &drv->diskfile);
	drv->forcedwrprot = forcedwriteprotect;
	if (drv->forcedwrprot)
		drv->wrprot = true;
	drv->ddhd = 1;
	drv->num_heads = 2;
	drv->num_secs = 0;
	drv->hard_num_cyls = p->floppyslots[dnum].dfxtype == DRV_525_SD ? 40 : 80;
	drv->tracktiming[0] = 0;
	drv->useturbo = 0;
	drv->indexoffset = 0;
	if (!fake) {
		drv->dskeject = false;
		//gui_disk_image_change (dnum, outname, drv->wrprot);
	}

	if (!drv->motoroff) {
		drv->dskready_up_time = DSKREADY_UP_TIME * 312 + (uaerand() & 511);
		drv->dskready_down_time = 0;
	}

	if (drv->diskfile == NULL && !drv->catweasel) {
		track_reset (drv);
		return 0;
	}

	if (!fake) {
		//inprec_recorddiskchange (dnum, fname_in, drv->wrprot);

		if (currprefs.floppyslots[dnum].df != fname_in) {
			_tcsncpy (currprefs.floppyslots[dnum].df, fname_in, 255);
			currprefs.floppyslots[dnum].df[255] = 0;
		}
		currprefs.floppyslots[dnum].forcedwriteprotect = forcedwriteprotect;
		_tcsncpy (changed_prefs.floppyslots[dnum].df, fname_in, 255);
		changed_prefs.floppyslots[dnum].df[255] = 0;
		changed_prefs.floppyslots[dnum].forcedwriteprotect = forcedwriteprotect;
		_tcscpy (drv->newname, fname_in);
		drv->newnamewriteprotected = forcedwriteprotect;
		gui_filename (dnum, outname);
	}

	memset (buffer, 0, sizeof buffer);
	size = 0;
	if (drv->diskfile) {
		zfile_fread (buffer, sizeof (char), 8, drv->diskfile);
		zfile_fseek (drv->diskfile, 0, SEEK_END);
		size = (int)zfile_ftell(drv->diskfile);
		zfile_fseek (drv->diskfile, 0, SEEK_SET);
	}

	canauto = 0;
	if (isrecognizedext (outname))
		canauto = 1;
	if (!canauto && drv->diskfile && isrecognizedext (zfile_getname (drv->diskfile)))
		canauto = 1;
	// if PC-only drive, make sure PC-like floppies are always detected
	if (!canauto && ispcbridgedrive(dnum))
		canauto = 1;

	if (drv->catweasel) {

		drv->wrprot = true;
		drv->filetype = ADF_CATWEASEL;
		drv->num_tracks = 80;
		drv->ddhd = 1;

#ifdef CAPS
	} else if (strncmp ((char*)buffer, "CAPS", 4) == 0) {

		drv->wrprot = true;
		if (!caps_loadimage (drv->diskfile, drv - floppy, &num_tracks)) {
			zfile_fclose (drv->diskfile);
			drv->diskfile = 0;
			return 0;
		}
		drv->num_tracks = num_tracks;
		drv->filetype = ADF_IPF;
#endif
#ifdef SCP
	} else if (strncmp ((char*)buffer, "SCP", 3) == 0) {
		drv->wrprot = true;
		if (!scp_open (drv->diskfile, drv - floppy, &num_tracks)) {
			zfile_fclose (drv->diskfile);
			drv->diskfile = 0;
			return 0;
		}
		drv->num_tracks = num_tracks;
		drv->filetype = ADF_SCP;
#endif
#ifdef FDI2RAW
	} else if ((drv->fdi = fdi2raw_header (drv->diskfile))) {

		drv->wrprot = true;
		drv->num_tracks = fdi2raw_get_last_track (drv->fdi);
		drv->num_secs = fdi2raw_get_num_sector (drv->fdi);
		drv->filetype = ADF_FDI;
#endif
	} else if (strncmp ((char*)buffer, "UAE-1ADF", 8) == 0) {

		read_header_ext2 (drv->diskfile, drv->trackdata, &drv->num_tracks, &drv->ddhd);
		drv->filetype = ADF_EXT2;
		drv->num_secs = 11;
		if (drv->ddhd > 1)
			drv->num_secs = 22;

	} else if (strncmp ((char*)buffer, "UAE--ADF", 8) == 0) {
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

	} else if (memcmp (exeheader, buffer, sizeof exeheader) == 0 && size <= 512 * (1760 - 7) && !canauto) {
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

		// 320k double sided
		size == 8 * 40 * 2 * 512 ||
		// 320k single sided
		size == 8 * 40 * 1 * 512 ||

		// 360k double sided
		size == 9 * 40 * 2 * 512 ||
		// 360k single sided
		size == 9 * 40 * 1 * 512 ||

		// 1.2M double sided
		size == 15 * 80 * 2 * 512 ||

		// 720k/1440k double sided
		size == 9 * 80 * 2 * 512 || size == 18 * 80 * 2 * 512 || size == 10 * 80 * 2 * 512 || size == 20 * 80 * 2 * 512 || size == 21 * 80 * 2 * 512 ||
		size == 9 * 81 * 2 * 512 || size == 18 * 81 * 2 * 512 || size == 10 * 81 * 2 * 512 || size == 20 * 81 * 2 * 512 || size == 21 * 81 * 2 * 512 ||
		size == 9 * 82 * 2 * 512 || size == 18 * 82 * 2 * 512 || size == 10 * 82 * 2 * 512 || size == 20 * 82 * 2 * 512 || size == 21 * 82 * 2 * 512 ||
		// 720k/1440k single sided
		size == 9 * 80 * 1 * 512 || size == 18 * 80 * 1 * 512 || size == 10 * 80 * 1 * 512 || size == 20 * 80 * 1 * 512 ||
		size == 9 * 81 * 1 * 512 || size == 18 * 81 * 1 * 512 || size == 10 * 81 * 1 * 512 || size == 20 * 81 * 1 * 512 ||
		size == 9 * 82 * 1 * 512 || size == 18 * 82 * 1 * 512 || size == 10 * 82 * 1 * 512 || size == 20 * 82 * 1 * 512)) {
			/* PC formatted image */
			int side, sd;
			int dfxtype = p->floppyslots[dnum].dfxtype;

			drv->num_secs = 9;
			drv->ddhd = 1;
			sd = 0;

			bool can40 = dfxtype == DRV_525_DD || dfxtype == DRV_PC_525_ONLY_40 || dfxtype == DRV_PC_525_40_80;
			bool can80 = dfxtype == DRV_35_HD || dfxtype == DRV_PC_35_ONLY_80 || dfxtype == DRV_PC_525_40_80;
			bool drv525 = dfxtype == DRV_525_DD || dfxtype == DRV_PC_525_ONLY_40 || dfxtype == DRV_PC_525_40_80;
	
			for (side = 2; side > 0; side--) {
				if (drv->hard_num_cyls >= 80 && can80) {
					if (       size ==  9 * 80 * side * 512 || size ==  9 * 81 * side * 512 || size ==  9 * 82 * side * 512) {
						drv->num_secs = 9;
						drv->ddhd = 1;
						break;
					} else if (!drv525 && (size == 18 * 80 * side * 512 || size == 18 * 81 * side * 512 || size == 18 * 82 * side * 512)) {
						drv->num_secs = 18;
						drv->ddhd = 2;
						break;
					} else if (!drv525 && (size == 10 * 80 * side * 512 || size == 10 * 81 * side * 512 || size == 10 * 82 * side * 512)) {
						drv->num_secs = 10;
						drv->ddhd = 1;
						break;
					} else if (!drv525 && (size == 20 * 80 * side * 512 || size == 20 * 81 * side * 512 || size == 20 * 82 * side * 512)) {
						drv->num_secs = 20;
						drv->ddhd = 2;
						break;
					} else if (!drv525 && (size == 21 * 80 * side * 512 || size == 21 * 81 * side * 512 || size == 21 * 82 * side * 512)) {
						drv->num_secs = 21;
						drv->ddhd = 2;
						break;
					} else if (size == 15 * 80 * side * 512) {
						drv->num_secs = 15;
						drv->ddhd = 1;
						break;
					}
				}
				if (drv->hard_num_cyls == 40 || can40) {
					if (size == 9 * 40 * side * 512) {
						drv->num_secs = 9;
						drv->ddhd = 1;
						sd = 1;
						break;
					} else if (size == 8 * 40 * side * 512) {
						drv->num_secs = 8;
						drv->ddhd = 1;
						sd = 1;
						break;
					}
				}
			}

			drv->num_tracks = size / (drv->num_secs * 512);

			// SD disk in 5.25 drive = duplicate each track
			if (sd && p->floppyslots[dnum].dfxtype == DRV_525_DD) {
				drv->num_tracks *= 2;
			} else {
				sd = 0;
			}

			drv->filetype = ADF_PCDOS;
			tid = &drv->trackdata[0];
			for (int i = 0; i < drv->num_tracks; i++) {
				tid->type = TRACK_PCDOS;
				tid->len = 512 * drv->num_secs;
				tid->bitlen = 0;
				tid->offs = (sd ? i / 2 : i) * 512 * drv->num_secs;
				if (side == 1) {
					tid++;
					tid->type = TRACK_NONE;
					tid->len = 512 * drv->num_secs;
				}
				tid->revolutions = 1;
				tid++;

			}
			drv->num_heads = side;
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

		int ds;

		ds = 0;
		drv->filetype = ADF_NORMAL;

		/* High-density, diskspare disk or sector headers included? */
		drv->num_tracks = 0;
		if (size > 160 * 11 * 512 + 511) { // larger than standard adf?
			for (int i = 80; i <= 83; i++) {
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
				if (size == i * 11 * (512 + 16) * 2) { // 80+ cyl DD + headers
					drv->num_tracks = size / ((512 + 16) * (drv->num_secs = 11));
					drv->filetype = ADF_NORMAL_HEADER;
					break;
				}
				if (size == i * 22 * (512 + 16) * 2) { // 80+ cyl HD + headers
					drv->num_tracks = size / ((512 + 16) * (drv->num_secs = 22));
					drv->filetype = ADF_NORMAL_HEADER;
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

		if (!ds && drv->num_tracks > MAX_TRACKS)
			write_log (_T("Your diskfile is too big, %d bytes!\n"), size);
		for (int i = 0; i < drv->num_tracks; i++) {
			tid = &drv->trackdata[i];
			tid->type = ds ? TRACK_DISKSPARE : TRACK_AMIGADOS;
			tid->len = 512 * drv->num_secs;
			tid->bitlen = 0;
			tid->offs = i * 512 * drv->num_secs;
			tid->revolutions = 1;
			if (drv->filetype == ADF_NORMAL_HEADER) {
				tid->extraoffs = drv->num_tracks * 512 * drv->num_secs + i * 16 * drv->num_secs;
			} else {
				tid->extraoffs = -1;
			}
		}
	}
	openwritefile (p, drv, 0);
	drive_settype_id (drv); /* Set DD or HD drive */
	drive_fill_bigbuf (drv, 1);
	drv->mfmpos = uaerand ();
	drv->mfmpos |= (uaerand () << 16);
	drv->mfmpos %= drv->tracklen;
	drv->prevtracklen = 0;
	if (!fake) {
#ifdef DRIVESOUND
		if (isfloppysound (drv))
			driveclick_insert (drv - floppy, 0);
#endif
		update_drive_gui (drv - floppy, false);
		update_disk_statusline(drv - floppy);
	}
	return 1;
}

static void rand_shifter (drive *drv)
{
	if (selected & (1 << (drv - floppy)))
		return;

	int r = ((uaerand() >> 4) & 7) + 1;
	while (r-- > 0) {
		word <<= 1;
		word |= (uaerand() & 0x1000) ? 1 : 0;
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
#ifdef CATWEASEL
	if (drv->catweasel)
		return catweasel_disk_changed (drv->catweasel) == 0;
#endif
	return drv->diskfile == 0 && drv->dskchange_time >= 0;
}

static void drive_step (drive * drv, int step_direction)
{
#ifdef CATWEASEL
	if (drv->catweasel) {
		int dir = direction ? -1 : 1;
		catweasel_step (drv->catweasel, dir);
		drv->cyl += dir;
		if (drv->cyl < 0)
			drv->cyl = 0;
		write_log (_T("%d -> %d\n"), dir, drv->cyl);
		return;
	}
#endif
	if (!drive_empty (drv))
		drv->dskchange = 0;
	if (drv->steplimit && get_cycles() - drv->steplimitcycle < MIN_STEPLIMIT_CYCLE) {
		write_log (_T(" step ignored drive %ld, %lu\n"),
			drv - floppy, (get_cycles() - drv->steplimitcycle) / CYCLE_UNIT);
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
#ifdef DRIVESOUND
			if (isfloppysound (drv))
				driveclick_click (drv - floppy, drv->cyl);
#endif
		}
		/*	else
		write_log (_T("program tried to step beyond track zero\n"));
		"no-click" programs does that
		*/
	} else {
		int maxtrack = drv->hard_num_cyls;
		if (drv->cyl < maxtrack + 3) {
			drv->cyl++;
#ifdef CATWEASEL
			if (drv->catweasel)
				catweasel_step (drv->catweasel, 1);
#endif
		}
		if (drv->cyl >= maxtrack)
			write_log (_T("program tried to step over track %d PC %08x\n"), maxtrack, M68K_GETPC);
#ifdef DRIVESOUND
		if (isfloppysound (drv))
			driveclick_click (drv - floppy, drv->cyl);
#endif
	}
	rand_shifter (drv);
	if (disk_debug_logging > 2)
		write_log (_T(" ->step %d"), drv->cyl);
}

static int drive_track0 (drive * drv)
{
#ifdef CATWEASEL
	if (drv->catweasel)
		return catweasel_track0 (drv->catweasel);
#endif
	return drv->cyl == 0;
}

static int drive_running (drive * drv)
{
	return !drv->motoroff;
}

static void motordelay_func (uae_u32 v)
{
	floppy[v].motordelay = 0;
}

static void drive_motor (drive * drv, bool off)
{
	if (drv->motoroff && !off) {
		drv->dskready_up_time = DSKREADY_UP_TIME * 312 + (uaerand() & 511);
		rand_shifter (drv);
#ifdef DRIVESOUND
		if (isfloppysound (drv))
			driveclick_motor (drv - floppy, drv->dskready_down_time == 0 ? 2 : 1);
#endif
		if (disk_debug_logging > 2)
			write_log (_T(" ->motor on"));
	}
	if (!drv->motoroff && off) {
		drv->drive_id_scnt = 0; /* Reset id shift reg counter */
		drv->dskready_down_time = DSKREADY_DOWN_TIME * 312 + (uaerand() & 511);
#ifdef DRIVESOUND
		driveclick_motor (drv - floppy, 0);
#endif
#ifdef DEBUG_DRIVE_ID
		write_log (_T("drive_motor: Selected DF%d: reset id shift reg.\n"),drv-floppy);
#endif
		if (disk_debug_logging > 2)
			write_log (_T(" ->motor off"));
		if (currprefs.cpu_model <= 68010 && currprefs.m68k_speed == 0) {
			drv->motordelay = 1;
			event2_newevent2 (30, drv - floppy, motordelay_func);
		}
	}
	drv->motoroff = off;
	if (drv->motoroff) {
		drv->dskready = 0;
		drv->dskready_up_time = 0;
	} else {
		drv->dskready_down_time = 0;
	}
#ifdef CATWEASEL
	if (drv->catweasel)
		catweasel_set_motor (drv->catweasel, !drv->motoroff);
#endif
}

static void read_floppy_data(struct zfile *diskfile, int type, trackid *tid, int sector, uae_u8 *dst, uae_u8 *secheaddst, int len)
{
	if (len <= 0)
		return;
	if (secheaddst) {
		memset(secheaddst, 0, 16);
	}
	if (tid->track == 0) {
		if (type == ADF_KICK) {
			memset (dst, 0, len > 512 ? 512 : len);
			if (sector == 0) {
				memcpy (dst, "KICK", 4);
				len -= 512;
			}
		} else if (type == ADF_SKICK) {
			memset (dst, 0, len > 512 ? 512 : len);
			if (sector == 0) {
				memcpy (dst, "KICKSUP0", 8);
				len -= 1024;
			} else if (sector == 1) {
				len -= 512;
			}
		}
	}
	if (tid->offs < 0 || sector < 0)
		return;
	if (type == ADF_NORMAL_HEADER && tid->extraoffs > 0) {
		zfile_fseek(diskfile, tid->extraoffs + sector * 16, SEEK_SET);
		zfile_fread(secheaddst, 1, 16, diskfile);
	}
	zfile_fseek(diskfile, tid->offs + sector * 512, SEEK_SET);
	zfile_fread(dst, 1, len, diskfile);
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

static const uae_u8 mfmencodetable[16] = {
	0x2a, 0x29, 0x24, 0x25, 0x12, 0x11, 0x14, 0x15,
	0x4a, 0x49, 0x44, 0x45, 0x52, 0x51, 0x54, 0x55
};


static uae_u16 dos_encode_byte (uae_u8 byte)
{
	uae_u8 b2, b1;
	uae_u16 word;

	b1 = byte;
	b2 = b1 >> 4;
	b1 &= 15;
	word = mfmencodetable[b2] <<8 | mfmencodetable[b1];
	return (word | ((word & (256 | 64)) ? 0 : 128));
}

static uae_u16 *mfmcoder (uae_u8 *src, uae_u16 *dest, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		*dest = dos_encode_byte (*src++);
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
		read_floppy_data (drv->diskfile, drv->filetype, ti, i, &secbuf[60], NULL, 512);
		crc16 = get_crc16 (secbuf + 56, 3 + 1 + 512);
		secbuf[60 + 512] = crc16 >> 8;
		secbuf[61 + 512] = crc16 & 0xff;
		len = (tracklen / 2 - 96) / drv->num_secs - 574 / drv->ddhd;
		if (len > 0)
			memset(secbuf + 512 + 62, 0x4e, len);
		dstmfmbuf = mfmcoder (secbuf, mfm2, 60 + 512 + 2 + 76 / drv->ddhd);
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
	if (disk_debug_logging > 0)
		write_log (_T("pcdos read track %d\n"), tr);
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
		uae_u8 secheadbuf[16];
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

		read_floppy_data (drv->diskfile, drv->filetype, ti, sec, &secbuf[32], secheadbuf, 512);

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

		for (i = 0; i < 16; i += 4) {
			deven = ((secheadbuf[i] << 24) | (secheadbuf[i + 1] << 16)
				| (secheadbuf[i + 2] << 8) | (secheadbuf[i + 3]));
			dodd = deven >> 1;
			deven &= 0x55555555;
			dodd &= 0x55555555;
			mfmbuf[(i >> 1) + 8] = dodd >> 16;
			mfmbuf[(i >> 1) + 9] = dodd;
			mfmbuf[(i >> 1) + 8 + 8] = deven >> 16;
			mfmbuf[(i >> 1) + 8 + 9] = deven;
		}
		for (i = 4; i < 24; i += 2) {
			hck ^= (mfmbuf[i] << 16) | mfmbuf[i + 1];
		}
		deven = dodd = hck;
		dodd >>= 1;
		mfmbuf[24] = dodd >> 16;
		mfmbuf[25] = dodd;
		mfmbuf[26] = deven >> 16;
		mfmbuf[27] = deven;

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
		for (i = 32; i < 544; i += 2) {
			dck ^= (mfmbuf[i] << 16) | mfmbuf[i + 1];
		}
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

	if (disk_debug_logging > 0)
		write_log (_T("amigados read track %d\n"), tr);
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

		read_floppy_data (drv->diskfile, drv->filetype, ti, sec, &secbuf[4], NULL, 512);

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
		secbuf[3] = (uae_u8)chk;

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

	if (disk_debug_logging > 0)
		write_log (_T("diskspare read track %d\n"), tr);
}

static void drive_fill_bigbuf (drive * drv, int force)
{
	int tr = drv->cyl * 2 + side;
	trackid *ti = drv->trackdata + tr;
	bool retrytrack;
	int rev = -1;

	if ((!drv->diskfile && !drv->catweasel) || tr >= drv->num_tracks) {
		track_reset (drv);
		return;
	}

	if (!force && drv->catweasel) {
		drv->buffered_cyl = -1;
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
		read_floppy_data (drv->writediskfile, drv->filetype, wti, 0, (uae_u8*)drv->bigmfmbuf, NULL, (wti->bitlen + 7) / 8);
		for (i = 0; i < (drv->tracklen + 15) / 16; i++) {
			uae_u16 *mfm = drv->bigmfmbuf + i;
			uae_u8 *data = (uae_u8 *) mfm;
			*mfm = 256 * *data + *(data + 1);
		}
		if (disk_debug_logging > 0)
			write_log (_T("track %d, length %d read from \"saveimage\"\n"), tr, drv->tracklen);
	} else if (drv->filetype == ADF_CATWEASEL) {
#ifdef CATWEASEL
		drv->tracklen = 0;
		if (!catweasel_disk_changed (drv->catweasel)) {
			drv->tracklen = catweasel_fillmfm (drv->catweasel, drv->bigmfmbuf, side, drv->ddhd, 0);
		}
		drv->buffered_cyl = -1;
		if (!drv->tracklen) {
			track_reset (drv);
			return;
		}
#endif
	} else if (drv->filetype == ADF_IPF) {

#ifdef CAPS
		caps_loadtrack (drv->bigmfmbuf, drv->tracktiming, drv - floppy, tr, &drv->tracklen, &drv->multi_revolution, &drv->skipoffset, &drv->lastrev, retrytrack);
#endif

	} else if (drv->filetype == ADF_SCP) {

#ifdef SCP
		scp_loadtrack (drv->bigmfmbuf, drv->tracktiming, drv - floppy, tr, &drv->tracklen, &drv->multi_revolution, &drv->skipoffset, &drv->lastrev, retrytrack);
#endif

	} else if (drv->filetype == ADF_FDI) {

#ifdef FDI2RAW
		fdi2raw_loadtrack (drv->fdi, drv->bigmfmbuf, drv->tracktiming, tr, &drv->tracklen, &drv->indexoffset, &drv->multi_revolution, 1);
#endif

	} else if (ti->type == TRACK_PCDOS) {

		decode_pcdos (drv);

	} else if (ti->type == TRACK_AMIGADOS) {

		decode_amigados (drv);

	} else if (ti->type == TRACK_DISKSPARE) {

		decode_diskspare (drv);

	} else if (ti->type == TRACK_NONE) {

		;

	} else {
		int i;
		int base_offset = ti->type == TRACK_RAW ? 0 : 1;
		drv->tracklen = ti->bitlen + 16 * base_offset;
		drv->bigmfmbuf[0] = ti->sync;
		read_floppy_data (drv->diskfile, drv->filetype, ti, 0, (uae_u8*)(drv->bigmfmbuf + base_offset), NULL, (ti->bitlen + 7) / 8);
		for (i = base_offset; i < (drv->tracklen + 15) / 16; i++) {
			uae_u16 *mfm = drv->bigmfmbuf + i;
			uae_u8 *data = (uae_u8 *) mfm;
			*mfm = 256 * *data + *(data + 1);
		}
		if (disk_debug_logging > 2)
			write_log (_T("rawtrack %d image offset=%x\n"), tr, ti->offs);
	}
	drv->buffered_side = side;
	drv->buffered_cyl = drv->cyl;
	if (drv->tracklen == 0) {
		drv->tracklen = FLOPPY_WRITE_LEN * drv->ddhd * 2 * 8;
		memset (drv->bigmfmbuf, 0, FLOPPY_WRITE_LEN * 2 * drv->ddhd);
	}

	drv->trackspeed = get_floppy_speed_from_image(drv);
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
	if (disk_debug_logging > 0)
		write_log (_T("track %d, raw track length %d written (total size %d)\n"), ti->track, (ti->bitlen + 7) / 8, ti->len);
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
			write_log (_T("illegal mfm sector %d data %04x %04x, bit %d:%d\n"), sector, wp, w, wordoffset, bitoffset);
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
			write_log (_T("Disk decode: weird sector number %d (%08x, %ld)\n"), trackoffs, id, mbuf - mstart);
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
			if (filetype != ADF_NORMAL_HEADER) {
				write_log(_T("Disk decode: sector %d header: %08X %08X %08X %08X\n"),
					trackoffs, sechead[0], sechead[1], sechead[2], sechead[3]);
				if (filetype == ADF_EXT2)
					return 6;
			}
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
		//write_log (_T("Sector %d ok\n"), trackoffs);
		sectable[trackoffs] = 1;
		secwritten++;
		memcpy (writebuffer + trackoffs * 512, secbuf + 32, 512);
		uae_u8 *secheadbuf = writesecheadbuffer + trackoffs * 16;
		for (i = 0; i < 4; i++) {
			*secheadbuf++ = sechead[i] >> 24;
			*secheadbuf++ = sechead[i] >> 16;
			*secheadbuf++ = sechead[i] >>  8;
			*secheadbuf++ = sechead[i] >>  0;
		}
	}
	if (filetype == ADF_EXT2 && (secwritten == 0 || secwritten < 0))
		return 5;
	if (secwritten == 0)
		write_log (_T("Disk decode: unsupported format\n"));
	if (secwritten < 0)
		write_log (_T("Disk decode: sector labels ignored\n"));
	*drvsecp = drvsec;
	return 0;
}

static uae_u8 mfmdecode (uae_u16 **mfmp, int shift)
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

static int drive_write_pcdos (drive *drv, struct zfile *zf, bool count)
{
	int i;
	int drvsec = drv->num_secs;
	int fwlen = FLOPPY_WRITE_LEN * drv->ddhd;
	int length = 2 * fwlen;
	uae_u16 *mbuf = drv->bigmfmbuf;
	uae_u16 *mend = mbuf + length;
	int secwritten = 0, seccnt = 0;
	int shift = 0, sector = -1;
	int sectable[24];
	uae_u8 secbuf[3 + 1 + 512];
	uae_u8 mark;
	uae_u16 crc;

	memset (sectable, 0, sizeof sectable);
	memcpy (mbuf + fwlen, mbuf, fwlen * sizeof (uae_u16));
	mend -= 518;
	secbuf[0] = secbuf[1] = secbuf[2] = 0xa1;
	secbuf[3] = 0xfb;

	while (seccnt < drvsec) {
		int mfmcount;

		mfmcount = 0;
		while (getmfmword (mbuf, shift) != 0x4489) {
			mfmcount++;
			if (mbuf >= mend)
				return -1;
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
				return -1;
			mbuf++;
		}
		if (mfmcount < 3) // ignore if less than 3 sync markers
			continue;

		mark = mfmdecode(&mbuf, shift);
		if (mark == 0xfe) {
			uae_u8 tmp[8];
			uae_u8 cyl, head, size;

			cyl = mfmdecode (&mbuf, shift);
			head = mfmdecode (&mbuf, shift);
			sector = mfmdecode (&mbuf, shift);
			size = mfmdecode (&mbuf, shift);
			crc = (mfmdecode (&mbuf, shift) << 8) | mfmdecode (&mbuf, shift);

			tmp[0] = 0xa1; tmp[1] = 0xa1; tmp[2] = 0xa1; tmp[3] = mark;
			tmp[4] = cyl; tmp[5] = head; tmp[6] = sector; tmp[7] = size;

			// skip 28 bytes
			for (i = 0; i < 28; i++)
				mfmdecode (&mbuf, shift);

			if (get_crc16 (tmp, 8) != crc || cyl != drv->cyl || head != side || size != 2 || sector < 1 || sector > drv->num_secs || sector >= sizeof sectable) {
				write_log (_T("PCDOS: track %d, corrupted sector header\n"), drv->cyl * 2 + side);
				return -1;
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
			secbuf[i + 4] = mfmdecode (&mbuf, shift);
		crc = (mfmdecode (&mbuf, shift) << 8) | mfmdecode (&mbuf, shift);
		if (get_crc16 (secbuf, 3 + 1 + 512) != crc) {
			write_log (_T("PCDOS: track %d, sector %d data checksum error\n"),
				drv->cyl * 2 + side, sector + 1);
			continue;
		}
		seccnt++;
		if (count && sectable[sector])
			break;
		if (!sectable[sector]) {
			secwritten++;
			sectable[sector] = 1;
			zfile_fseek (zf, drv->trackdata[drv->cyl * 2 + side].offs + sector * 512, SEEK_SET);
			zfile_fwrite (secbuf + 4, sizeof (uae_u8), 512, zf);
			//write_log (_T("PCDOS: track %d sector %d written\n"), drv->cyl * 2 + side, sector + 1);
		}
		sector = -1;
	}
	if (!count && secwritten != drv->num_secs)
		write_log (_T("PCDOS: track %d, %d corrupted sectors ignored\n"),
		drv->cyl * 2 + side, drv->num_secs - secwritten);
	return secwritten;
}

static int drive_write_adf_amigados (drive *drv)
{
	int drvsec;
	int sectable[MAX_SECTORS];

	if (decode_buffer (drv->bigmfmbuf, drv->cyl, drv->num_secs, drv->ddhd, drv->filetype, &drvsec, sectable, 0))
		return 2;
	if (!drvsec)
		return 2;

	if (drv->filetype == ADF_EXT2) {
		diskfile_update(drv->diskfile, &drv->trackdata[drv->cyl * 2 + side], drvsec * 512 * 8, TRACK_AMIGADOS);
	}
	trackid *tid = &drv->trackdata[drv->cyl * 2 + side];
	if (drv->filetype == ADF_NORMAL_HEADER && tid->extraoffs > 0) {
		zfile_fseek(drv->diskfile, tid->extraoffs, SEEK_SET);
		zfile_fwrite(writesecheadbuffer, sizeof(uae_u8), drvsec * 16, drv->diskfile);
	}
	zfile_fseek (drv->diskfile, tid->offs, SEEK_SET);
	zfile_fwrite (writebuffer, sizeof (uae_u8), drvsec * 512, drv->diskfile);
	return 0;
}

/* write raw track to disk file */
static int drive_write_ext2 (uae_u16 *bigmfmbuf, struct zfile *diskfile, trackid *ti, int tracklen)
{
	int len, i, offset;

	offset = 0;
	len = (tracklen + 7) / 8;
	if (len > ti->len) {
		write_log (_T("disk raw write: image file's track %d is too small (%d < %d)!\n"), ti->track, ti->len, len);
		offset = (len - ti->len) / 2;
		len = ti->len;
	}
	diskfile_update (diskfile, ti, tracklen, TRACK_RAW);
	for (i = 0; i < ti->len / 2; i++) {
		uae_u16 *mfm = bigmfmbuf + i + offset;
		uae_u16 *mfmw = bigmfmbufw + i + offset;
		uae_u8 *data = (uae_u8 *) mfm;
		*mfmw = 256 * *data + *(data + 1);
	}
	zfile_fseek (diskfile, ti->offs, SEEK_SET);
	zfile_fwrite (bigmfmbufw, 1, len, diskfile);
	return 1;
}

static void drive_write_data (drive * drv);

static bool convert_adf_to_ext2 (drive *drv, int mode)
{
	TCHAR name[MAX_DPATH];
	bool hd = drv->ddhd == 2;
	struct zfile *f;

	if (drv->filetype != ADF_NORMAL)
		return false;
	_tcscpy (name, currprefs.floppyslots[drv - floppy].df);
	if (!name[0])
		return false;
	if (mode == 1) {
		TCHAR *p = _tcsrchr (name, '.');
		if (!p)
			p = name + _tcslen (name);
		_tcscpy (p, _T(".extended.adf"));
		if (!disk_creatediskfile (&currprefs, name, 1, hd ? DRV_35_HD : DRV_35_DD, -1, NULL, false, false, drv->diskfile))
			return false;
	} else if (mode == 2) {
		struct zfile *tmp = zfile_fopen_load_zfile (drv->diskfile);
		if (!tmp)
			return false;
		zfile_fclose (drv->diskfile);
		drv->diskfile = NULL;
		if (!disk_creatediskfile (&currprefs, name, 1, hd ? DRV_35_HD : DRV_35_DD, -1, NULL, false, false, tmp)) {
			zfile_fclose (tmp);
			return false;
		}
		zfile_fclose(tmp);
	} else {
		return false;
	}
	f = zfile_fopen (name, _T("r+b"));
	if (!f)
		return false;
	_tcscpy (currprefs.floppyslots[drv - floppy].df, name);
	_tcscpy (changed_prefs.floppyslots[drv - floppy].df, name);
	zfile_fclose (drv->diskfile);

	drv->diskfile = f;
	drv->filetype = ADF_EXT2;
	read_header_ext2 (drv->diskfile, drv->trackdata, &drv->num_tracks, &drv->ddhd);

	drive_write_data (drv);
#ifdef RETROPLATFORM
	rp_disk_image_change (drv - &floppy[0], name, false);
#endif
	drive_fill_bigbuf (drv, 1);

	return true;
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
			floppy_writemode > 0 ? dsklength2 * 8 : drv->tracklen);
	}
	switch (drv->filetype) {
	case ADF_NORMAL:
	case ADF_NORMAL_HEADER:
		if (drv->ddhd > 1 && currprefs.floppyslots[drv - &floppy[0]].dfxtype != DRV_35_HD) {
			// HD image in DD drive: ignore writing.
			drv->buffered_side = 2;
		} else {
			if (drive_write_adf_amigados (drv)) {
				if (currprefs.floppy_auto_ext2) {
					convert_adf_to_ext2 (drv, currprefs.floppy_auto_ext2);
				} else {
					static int warned;
					if (!warned)
						notify_user (NUMSG_NEEDEXT2);
					warned = 1;
				}
			}
		}
		return;
	case ADF_EXT1:
		break;
	case ADF_EXT2:
		if (!floppy_writemode)
			ret = drive_write_adf_amigados (drv);
		if (ret) {
			write_log (_T("not an amigados track %d (error %d), writing as raw track\n"), drv->cyl * 2 + side, ret);
			drive_write_ext2 (drv->bigmfmbuf, drv->diskfile, &drv->trackdata[drv->cyl * 2 + side],
				floppy_writemode > 0 ? dsklength2 * 8 : drv->tracklen);
		}
		return;
	case ADF_IPF:
		break;
	case ADF_SCP:
		break;
	case ADF_PCDOS:
		ret = drive_write_pcdos (drv, drv->diskfile, 0);
		if (ret < 0)
			write_log (_T("not a PC formatted track %d (error %d)\n"), drv->cyl * 2 + side, ret);
		break;
	}
	drv->tracktiming[0] = 0;
}

static void drive_eject (drive * drv)
{
#ifdef DRIVESOUND
	if (isfloppysound (drv))
		driveclick_insert (drv - floppy, 1);
#endif
	//if (drv->diskfile || drv->filetype >= 0)
		//statusline_add_message(STATUSTYPE_FLOPPY, _T("DF%d: -"), drv - floppy);
	drive_image_free (drv);
	drv->dskeject = false;
	drv->dskchange = true;
	drv->ddhd = 1;
	drv->dskchange_time = 0;
	drv->dskready = 0;
	drv->dskready_up_time = 0;
	drv->dskready_down_time = 0;
	drv->forcedwrprot = false;
	drv->crc32 = 0;
	drive_settype_id (drv); /* Back to 35 DD */
	if (disk_debug_logging > 0)
		write_log (_T("eject drive %ld\n"), drv - &floppy[0]);
	//gui_disk_image_change(drv - floppy, NULL, drv->wrprot);
	//inprec_recorddiskchange (drv - floppy, NULL, false);
}

static void floppy_get_bootblock (uae_u8 *dst, bool ffs, bool bootable)
{
	strcpy ((char*)dst, "DOS");
	dst[3] = ffs ? 1 : 0;
	if (bootable)
		memcpy (dst, ffs ? bootblock_ffs : bootblock_ofs, ffs ? sizeof bootblock_ffs : sizeof bootblock_ofs);
}
static void floppy_get_rootblock (uae_u8 *dst, int block, const TCHAR *disk_name, bool hd)
{
	dst[0+3] = 2; // primary type
	dst[12+3] = 0x48; // size of hash table
	dst[312] = dst[313] = dst[314] = dst[315] = (uae_u8)0xff; // bitmap valid
	dst[316+2] = (block + 1) >> 8; dst[316+3] = (block + 1) & 255; // bitmap pointer
	char *s = ua ((disk_name && _tcslen (disk_name) > 0) ? disk_name : _T("empty"));
	dst[432] = (uae_u8)strlen(s); // name length
	strcpy ((char*)dst + 433, s); // name
	xfree (s);
	dst[508 + 3] = 1; // secondary type
	disk_date (dst + 420); // root modification date
	disk_date(dst + 484); // creation date
	disk_checksum (dst, dst + 20);
	/* bitmap block */
	memset (dst + 512 + 4, 0xff, 2 * block / 8);
	if (!hd)
		dst[512 + 0x72] = 0x3f;
	else
		dst[512 + 0xdc] = 0x3f;
	disk_checksum (dst + 512, dst + 512);
}

/* type: 0=regular, 1=ext2adf, 2=regular+headers */
bool disk_creatediskfile (struct uae_prefs *p, const TCHAR *name, int type, drive_type adftype, int hd, const TCHAR *disk_name, bool ffs, bool bootable, struct zfile *copyfrom)
{
	int size = 32768;
	struct zfile *f;
	int i, l, file_size, data_size, tracks, track_len, sectors;
	uae_u8 *chunk = NULL;
	int ddhd = 1;
	bool ok = false;
	uae_u64 pos;

	if (type == 1)
		tracks = 2 * 83;
	else
		tracks = 2 * 80;
	file_size = 880 * 2 * 512;
	data_size = file_size;
	if (type == 2)
		file_size = 880 * 2 * (512 + 16);
	sectors = 11;
	if (adftype == DRV_PC_525_ONLY_40 || adftype == DRV_PC_35_ONLY_80 || adftype == DRV_PC_525_40_80) {
		file_size = 720 * 1024;
		sectors = 9;
	}
	// largest needed
	track_len = FLOPPY_WRITE_LEN_NTSC;
	if (p->floppy_write_length > track_len && p->floppy_write_length < 2 * FLOPPY_WRITE_LEN_NTSC)
		track_len = p->floppy_write_length;
	if (adftype == DRV_35_HD || hd > 0) {
		file_size *= 2;
		track_len *= 2;
		ddhd = 2;
		if (adftype == DRV_PC_525_40_80) {
			file_size = 15 * 512 * 80 * 2;
		}
	} else if (adftype == DRV_PC_525_ONLY_40) {
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
		memset (chunk, 0, size);
		if (type == 0 || type == 2) {
			int dataoffset = 0;
			if (type == 2) {
				cylsize = sectors * 2 * (512 + 16);
				dataoffset += 16;
			}
			for (i = 0; i < file_size; i += cylsize) {
				memset(chunk, 0, cylsize);
				if (adftype == DRV_35_DD || adftype == DRV_35_HD) {
					if (i == 0) {
						/* boot block */
						floppy_get_bootblock (chunk + dataoffset, ffs, bootable);
					} else if (i == file_size / 2) {
						/* root block */
						floppy_get_rootblock (chunk + dataoffset, data_size / 1024, disk_name, ddhd > 1);
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
							floppy_get_rootblock (chunk, 80 * 11 * ddhd, disk_name, adftype == DRV_35_HD);
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
	write_log(_T("'%s': old mode = %x\n"), name, st.mode);
	oldmode = mode = st.mode;
	mode &= ~FILEFLAG_WRITE;
	if (!readonly)
		mode |= FILEFLAG_WRITE;
	if (mode != oldmode) {
		if (!my_chmod (name, mode))
			write_log(_T("chmod failed!\n"));
	}
	write_log(_T("'%s': new mode = %x\n"), name, mode);
}

static void setdskchangetime (drive *drv, int dsktime)
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
	if (disk_debug_logging > 0)
		write_log (_T("delayed insert enable %d\n"), dsktime);
}

void DISK_reinsert (int num)
{
	drive_eject (&floppy[num]);
	setdskchangetime (&floppy[num], 2 * 50 * 312);
}

int disk_setwriteprotect (struct uae_prefs *p, int num, const TCHAR *fname_in, bool writeprotected)
{
	int needwritefile, oldprotect;
	struct zfile *zf1, *zf2;
	bool wrprot1, wrprot2;
	int i;
	TCHAR *name2;
	drive_type drvtype;
	TCHAR outfname[MAX_DPATH];

	write_log(_T("disk_setwriteprotect %d '%s' %d\n"), num, fname_in, writeprotected);

	oldprotect = diskfile_iswriteprotect (p, fname_in, &needwritefile, &drvtype);
	DISK_validate_filename (p, fname_in, outfname, 1, &wrprot1, NULL, &zf1);
	if (!zf1)
		return 0;

	write_log(_T("old = %d writeprot = %d master = %d\n"), oldprotect, wrprot1, p->floppy_read_only);

	if (wrprot1 && p->floppy_read_only) {
		zfile_fclose(zf1);
		return 0;
	}
	if (zfile_iscompressed (zf1))
		wrprot1 = 1;
	zfile_fclose (zf1);
	zf2 = getexistingwritefile(p, fname_in, &wrprot2);
	name2 = DISK_get_saveimagepath(fname_in, -2);

	if (needwritefile && zf2 == NULL)
		disk_creatediskfile (p, name2, 1, drvtype, -1, NULL, false, false, NULL);
	zfile_fclose (zf2);
	if (writeprotected && iswritefileempty (p, fname_in)) {
		for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
			if (!_tcscmp (fname_in, floppy[i].newname))
				drive_eject (&floppy[i]);
		}
		_wunlink (name2);
	}

	if (!needwritefile)
		diskfile_readonly (outfname, writeprotected);
	diskfile_readonly (name2, writeprotected);

	floppy[num].forcedwrprot = false;
	floppy[num].newnamewriteprotected = false;

	return 1;
}

void disk_eject (int num)
{
	set_config_changed ();
	gui_filename (num, _T(""));
	drive_eject (floppy + num);
	*currprefs.floppyslots[num].df = *changed_prefs.floppyslots[num].df = 0;
	floppy[num].newname[0] = 0;
	update_drive_gui (num, true);
}

int DISK_history_add (const TCHAR *name, int idx, int type, int donotcheck)
{
	int i;

	if (idx >= MAX_PREVIOUS_IMAGES)
		return 0;
	if (name == NULL) {
		if (idx < 0)
			return 0;
		dfxhistory[type][idx][0] = 0;
		return 1;
	}
	if (name[0] == 0)
		return 0;
#if 0
	if (!donotcheck) {
		if (!zfile_exists (name)) {
			for (i = 0; i < MAX_PREVIOUS_IMAGES; i++) {
				if (!_tcsicmp (dfxhistory[type][i], name)) {
					while (i < MAX_PREVIOUS_IMAGES - 1) {
						_tcscpy (dfxhistory[type][i], dfxhistory[type][i + 1]);
						i++;
					}
					dfxhistory[type][MAX_PREVIOUS_IMAGES - 1][0] = 0;
					break;
				}
			}
			return 0;
		}
	}
#endif
	if (idx >= 0) {
		if (idx >= MAX_PREVIOUS_IMAGES)
			return 0;
		dfxhistory[type][idx][0] = 0;
		for (i = 0; i < MAX_PREVIOUS_IMAGES; i++) {
			if (!_tcsicmp (dfxhistory[type][i], name))
				return 0;
		}
		_tcscpy (dfxhistory[type][idx], name);
		return 1;
	}
	for (i = 0; i < MAX_PREVIOUS_IMAGES; i++) {
		if (!_tcscmp (dfxhistory[type][i], name)) {
			while (i < MAX_PREVIOUS_IMAGES - 1) {
				_tcscpy (dfxhistory[type][i], dfxhistory[type][i + 1]);
				i++;
			}
			dfxhistory[type][MAX_PREVIOUS_IMAGES - 1][0] = 0;
			break;
		}
	}
	for (i = MAX_PREVIOUS_IMAGES - 2; i >= 0; i--)
		_tcscpy (dfxhistory[type][i + 1], dfxhistory[type][i]);
	_tcscpy (dfxhistory[type][0], name);
	return 1;
}

TCHAR *DISK_history_get (int idx, int type)
{
	if (idx >= MAX_PREVIOUS_IMAGES)
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
	drv->dskeject = false;
	_tcscpy(drv->newname, name);
	drv->newnamewriteprotected = forcedwriteprotect;
	_tcscpy (currprefs.floppyslots[num].df, name);
	currprefs.floppyslots[num].forcedwriteprotect = forcedwriteprotect;
	DISK_history_add (name, -1, HISTORY_FLOPPY, 0);
	if (name[0] == 0) {
		disk_eject (num);
	} else if (!drive_empty(drv) || drv->dskchange_time > 0) {
		// delay eject so that it is always called when emulation is active
		drv->dskeject = true;
	} else {
		setdskchangetime (drv, 1 * 312);
	}
}

void disk_insert (int num, const TCHAR *name, bool forcedwriteprotect)
{
	set_config_changed ();
	target_addtorecent (name, 0);
	disk_insert_2 (num, name, 0, forcedwriteprotect);
}

void disk_insert (int num, const TCHAR *name)
{
	set_config_changed ();
	target_addtorecent (name, 0);
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
	if (currprefs.floppy_read_only != changed_prefs.floppy_read_only)
		currprefs.floppy_read_only = changed_prefs.floppy_read_only;
	for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
		drive *drv = floppy + i;
		if (drv->dskeject) {
			drive_eject(drv);
			/* set dskchange_time, disk_insert() will be
			* called from DISK_check_change() after 2 second delay
			* this makes sure that all programs detect disk change correctly
			*/
			setdskchangetime(drv, 2 * 50 * 312);
		}
		if (currprefs.floppyslots[i].dfxtype != changed_prefs.floppyslots[i].dfxtype) {
			currprefs.floppyslots[i].dfxtype = changed_prefs.floppyslots[i].dfxtype;
			reset_drive (i);
#ifdef RETROPLATFORM
			rp_floppy_device_enable (i, currprefs.floppyslots[i].dfxtype >= 0);
#endif
		}
	}
	if (config_changed) {
		for (int i = 0; i < MAX_SPARE_DRIVES; i++) {
			_tcscpy(currprefs.dfxlist[i], changed_prefs.dfxlist[i]);
		}
	}
}

void DISK_vsync (void)
{
	DISK_check_change ();
	for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
		drive *drv = floppy + i;
		if (drv->selected_delay > 0) {
			drv->selected_delay--;
		}
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
	if (currprefs.cs_compatible == CP_VELVET) {
		selected = (data >> 3) & 3;
	} else {
		selected = (data >> 3) & 15;
	}
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
	bool velvet = currprefs.cs_compatible == CP_VELVET;
	int step_pulse, prev_selected, dr;

	prev_selected = selected;

	fetch_DISK_select (data);
	step_pulse = data & 1;

	if (disk_debug_logging > 2) {
		if (velvet) {
			write_log (_T("%08X %02X->%02X %s drvmask=%x"), M68K_GETPC, prev_data, data, tobin(data), selected ^ 3);
		} else {
			write_log (_T("%08X %02X->%02X %s drvmask=%x"), M68K_GETPC, prev_data, data, tobin(data), selected ^ 15);
		}
	}

#ifdef AMAX
	if (amax_enabled) {
		for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
			drive *drv = floppy + dr;
			if (drv->amax)
				amax_disk_select (data, prev_data, dr);
		}
	}
#endif

	if (!velvet) {
		if ((prev_data & 0x80) != (data & 0x80)) {
			for (dr = 0; dr < 4; dr++) {
				if (floppy[dr].indexhackmode > 1 && !((selected | disabled) & (1 << dr))) {
					floppy[dr].indexhack = 1;
					if (disk_debug_logging > 2)
						write_log (_T(" indexhack!"));
				}
			}
		}
	}

	if (disk_debug_logging > 2) {
		if (velvet) {
			write_log (_T(" %d%d"), (selected & 1) ? 0 : 1, (selected & 2) ? 0 : 1);
			if ((prev_data & 0x08) != (data & 0x08))
				write_log (_T(" dsksel0>%d"), (data & 0x08) ? 0 : 1);
			if ((prev_data & 0x10) != (data & 0x10))
				write_log (_T(" dsksel1>%d"), (data & 0x10) ? 0 : 1);
			if ((prev_data & 0x20) != (data & 0x20))
				write_log (_T(" dskmotor0>%d"), (data & 0x20) ? 0 : 1);
			if ((prev_data & 0x40) != (data & 0x40))
				write_log (_T(" dskmotor1>%d"), (data & 0x40) ? 0 : 1);
			if ((prev_data & 0x02) != (data & 0x02))
				write_log (_T(" direct>%d"), (data & 0x02) ? 1 : 0);
			if ((prev_data & 0x04) != (data & 0x04))
				write_log (_T(" side>%d"), (data & 0x04) ? 1 : 0);
		} else {
			write_log (_T(" %d%d%d%d"), (selected & 1) ? 0 : 1, (selected & 2) ? 0 : 1, (selected & 4) ? 0 : 1, (selected & 8) ? 0 : 1);
			for (int i = 0; i < 4; i++) {
				int im = 1 << i;
				if ((selected & im) && !(prev_selected & im))
					write_log(_T(" sel%d>0"), i);
				if (!(selected & im) && (prev_selected & im))
					write_log(_T(" sel%d>1"), i);
			}
			if ((prev_data & 0x80) != (data & 0x80))
				write_log (_T(" dskmotor>%d"), (data & 0x80) ? 1 : 0);
			if ((prev_data & 0x02) != (data & 0x02))
				write_log (_T(" direct>%d"), (data & 0x02) ? 1 : 0);
			if ((prev_data & 0x04) != (data & 0x04))
				write_log (_T(" side>%d"), (data & 0x04) ? 1 : 0);
		}
	}

	// step goes high and drive was selected when step pulse changes: step
	if (prev_step != step_pulse) {
		if (disk_debug_logging > 2)
			write_log (_T(" dskstep %d "), step_pulse);
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
		if (velvet) {
			for (dr = 0; dr < 2; dr++) {
				drive *drv = floppy + dr;
				int motormask = 0x20 << dr;
				int selectmask = 0x08 << dr;
				if (!(selected & (1 << dr)) && !(disabled & (1 << dr))) {
					if (!(prev_data & motormask) && (data & motormask)) {
						drive_motor(drv, 1);
					} else if ((prev_data & motormask) && !(data & motormask)) {
						drive_motor(drv, 0);
					}
				}
			}
		} else {
			for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
				drive *drv = floppy + dr;
				/* motor on/off workings tested with small assembler code on real Amiga 1200. */
				/* motor/id flipflop is set only when drive select goes from high to low */
				if (!((selected | disabled) & (1 << dr)) && (prev_selected & (1 << dr)) ) {
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
					if (!currprefs.cs_df0idhw && dr == 0)
						drv->idbit = 0;
#ifdef DEBUG_DRIVE_ID
					write_log (_T("DISK_status: sel %d id %s (%08X) [0x%08x, bit #%02d: %d]\n"),
						dr, drive_id_name(drv), drv->drive_id, drv->drive_id << drv->drive_id_scnt, 31 - drv->drive_id_scnt, drv->idbit);
#endif
				}
			}
		}
	}

	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		// selected
		if (!(selected & (1 << dr)) && floppy[dr].selected_delay < 0) {
			floppy[dr].selected_delay = 2;
		}
		// not selected
		if ((selected & (1 << dr))) {
			floppy[dr].selected_delay = -1;
		}
		// external drives usually (always?) light activity led when selected. Internal only when motor is running.
		bool selected_led = !(selected & (1 << dr)) && floppy[dr].selected_delay == 0 && dr > 0;
		floppy[dr].state = selected_led || !floppy[dr].motoroff;
		update_drive_gui (dr, false);
	}
	prev_data = data;
	if (disk_debug_logging > 2)
		write_log (_T("\n"));
}

uae_u8 DISK_status_ciab(uae_u8 st)
{
	if (currprefs.cs_compatible == CP_VELVET) {
		st |= 0x80;
		for (int dr = 0; dr < 2; dr++) {
			drive *drv = floppy + dr;
			if (!(((selected >> 3) | disabled) & (1 << dr))) {
				if (drive_writeprotected (drv))
					st &= ~0x80;
			}
		}
		if (disk_debug_logging > 2) {
			write_log(_T("DISK_STATUS_CIAB %08x %02x\n"), M68K_GETPC, st);
		}
	}

	return st;
}

uae_u8 DISK_status_ciaa(void)
{
	uae_u8 st = 0x3c;

	if (currprefs.cs_compatible == CP_VELVET) {
		for (int dr = 0; dr < 2; dr++) {
			drive *drv = floppy + dr;
			if (!(((selected >> 3) | disabled) & (1 << dr))) {
				if (drv->dskchange)
					st &= ~0x20;
				if (drive_track0 (drv))
					st &= ~0x10;
			}
		}
		if (disk_debug_logging > 2) {
			write_log(_T("DISK_STATUS_CIAA %08x %02x\n"), M68K_GETPC, st);
		}
		return st;
	}

	for (int dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = floppy + dr;
		if (drv->amax) {
			//if (amax_active())
				//st = amax_disk_status (st);
		} else if (!((selected | disabled) & (1 << dr))) {
			if (drive_running (drv)) {
				if (drv->catweasel) {
#ifdef CATWEASEL
					if (catweasel_diskready (drv->catweasel))
						st &= ~0x20;
#endif
				} else {
					if (drv->dskready && !drv->indexhack && currprefs.floppyslots[dr].dfxtype != DRV_35_DD_ESCOM)
						st &= ~0x20;
				}
			} else {
				if (currprefs.cs_df0idhw || dr > 0) {
					/* report drive ID */
					if (drv->idbit && currprefs.floppyslots[dr].dfxtype != DRV_35_DD_ESCOM)
						st &= ~0x20;
				} else {
					/* non-ID internal drive: mirror real dskready */
					if (drv->dskready)
						st &= ~0x20;
				}
				/* dskrdy needs some cycles after switching the motor off.. (Pro Tennis Tour) */
				if (!currprefs.cs_df0idhw && dr == 0 && drv->motordelay)
					st &= ~0x20;
			}
			if (drive_track0 (drv))
				st &= ~0x10;
			if (drive_writeprotected (drv))
				st &= ~8;
			if (drv->catweasel) {
#ifdef CATWEASEL
				if (catweasel_disk_changed (drv->catweasel))
					st &= ~4;
#endif
			} else if (drv->dskchange && currprefs.floppyslots[dr].dfxtype != DRV_525_SD) {
				st &= ~4;
			}
		} else if (!((selected | disabled) & (1 << dr))) {
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

static int nextbit(drive *drv)
{
	return drv && !drv->fourms && !(adkcon & 0x0100) ? 2 : 1;
}

/* get one bit from bit stream */
static uae_u32 getonebit(drive *drv, uae_u16 *mfmbuf, int mfmpos, int *inc)
{
	uae_u16 *buf;

	if (inc)
		*inc = 1;
	if (inc && nextbit(drv) == 2) {
		// 2us -> 4us
		int b1 = getonebit(NULL, mfmbuf, mfmpos, NULL);
		int b2 = getonebit(NULL, mfmbuf, (mfmpos + 1) % drv->tracklen, NULL);
		if (!b1 && b2) {
			*inc = 3;
			return 1;
		}
		if (b1 && !b2) {
			*inc = 2;
			return 1;
		}
		if (b1 && b2) {
			*inc = 3;
			return 1;
		}
		*inc = 2;
		return 0;
	}
	buf = &mfmbuf[mfmpos >> 4];
	return (buf[0] & (1 << (15 - (mfmpos & 15)))) ? 1 : 0;
}

void dumpdisk (const TCHAR *name)
{
	//int i, j, k;
	//uae_u16 w;

	//for (i = 0; i < MAX_FLOPPY_DRIVES; i++) {
	//	drive *drv = &floppy[i];
	//	if (!(disabled & (1 << i))) {
	//		console_out_f (_T("%s: drive %d motor %s cylinder %2d sel %s %s mfmpos %d/%d\n"),
	//			name, i, drv->motoroff ? _T("off") : _T(" on"), drv->cyl, (selected & (1 << i)) ? _T("no") : _T("yes"),
	//			drive_writeprotected (drv) ? _T("ro") : _T("rw"), drv->mfmpos, drv->tracklen);
	//		if (drv->motoroff == 0) {
	//			w = 0;
	//			for (j = -4; j < 13; j++) {
	//				for (k = 0; k < 16; k++) {
	//					int pos = drv->mfmpos + j * 16 + k;
	//					if (pos < 0)
	//						pos += drv->tracklen;
	//					w <<= 1;
	//					w |= getonebit (drv->bigmfmbuf, pos);
	//				}
	//				console_out_f(_T("%04X%c"), w, j == -1 ? '|' : ' ');
	//			}
	//			console_out (_T("\n"));
	//		}
	//	}
	//}
	//console_out_f (_T("side %d dma %d off %d word %04X pt %08X len %04X bytr %04X adk %04X sync %04X\n"),
	//	side, dskdmaen, bitoffset, word, dskpt, dsklen, dskbytr_val, adkcon, dsksync);
}

static void disk_dmafinished (void)
{
	INTREQ (0x8000 | 0x0002);
	if (floppy_writemode > 0)
		floppy_writemode = 0;
	dskdmaen = DSKDMA_OFF;
	dsklength = 0;
	dsklen = 0;
	if (disk_debug_logging > 0) {
		int dr;
		write_log (_T("disk dma finished %08X MFMpos="), dskpt);
		for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++)
			write_log (_T("%d%s"), floppy[dr].mfmpos, dr < MAX_FLOPPY_DRIVES - 1 ? _T(",") : _T(""));
		write_log (_T("\n"));
	}
}

static void fetchnextrevolution (drive *drv)
{
	if (drv->revolution_check)
		return;
	drv->trackspeed = get_floppy_speed_from_image(drv);
#if REVOLUTION_DEBUG
	if (1 || drv->mfmpos != 0) {
		write_log (_T("REVOLUTION: DMA=%d %d %d/%d %d %d\n"), dskdmaen, drv->trackspeed, drv->mfmpos, drv->tracklen, drv->indexoffset, drv->floppybitcounter);
	}
#endif
	drv->revolution_check = 2;
	if (!drv->multi_revolution)
		return;
	switch (drv->filetype)
	{
	case ADF_IPF:
#ifdef CAPS
		caps_loadrevolution (drv->bigmfmbuf, drv->tracktiming, drv - floppy, drv->cyl * 2 + side, &drv->tracklen, &drv->lastrev, drv->track_access_done);
#endif
		break;
	case ADF_SCP:
#ifdef SCP
		scp_loadrevolution (drv->bigmfmbuf, drv - floppy, drv->tracktiming, &drv->tracklen);
#endif
		break;
	case ADF_FDI:
#ifdef FDI2RAW
		fdi2raw_loadrevolution(drv->fdi, drv->bigmfmbuf, drv->tracktiming, drv->cyl * 2 + side, &drv->tracklen, 1);
#endif
		break;
	}
}

static void do_disk_index (void)
{
#if REVOLUTION_DEBUG
	write_log(_T("INDEX %d\n"), indexdecay);
#endif
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

	event2_remevent (ev2_disk);
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

static void disk_doupdate_write(int floppybits, int trackspeed)
{
	int dr;
	int drives[4];

	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv2 = &floppy[dr];
		drives[dr] = 0;
		if (drv2->motoroff)
			continue;
		if ((selected | disabled) & (1 << dr))
			continue;
		drives[dr] = 1;
	}

	while (floppybits >= trackspeed) {
		for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
			if (drives[dr]) {
				drive *drv2 = &floppy[dr];
				drv2->mfmpos++;
				drv2->mfmpos %= drv2->tracklen;
				if (drv2->mfmpos == drv2->indexoffset) {
					do_disk_index();
				}
			}
		}
		if (dmaen(DMA_DISK) && dmaen(DMA_MASTER) && dskdmaen == DSKDMA_WRITE && dsklength > 0 && fifo_filled) {
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
					for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
						drive *drv2 = &floppy[dr];
						if (drives[dr]) {
							drv2->bigmfmbuf[drv2->mfmpos >> 4] = w;
							drv2->bigmfmbuf[(drv2->mfmpos >> 4) + 1] = 0x5555;
							drv2->writtento = 1;
						}
#ifdef AMAX
						if (amax_enabled)
							amax_diskwrite (w);
#endif
					}
					dsklength--;
					if (dsklength <= 0) {
						disk_dmafinished ();
						for (int dr = 0; dr < MAX_FLOPPY_DRIVES ; dr++) {
							drive *drv = &floppy[dr];
							drv->writtento = 0;
							if (drv->motoroff)
								continue;
							if ((selected | disabled) & (1 << dr))
								continue;
							drive_write_data (drv);
						}
					}
				}
			}
		}
		floppybits -= trackspeed;
	}
}

static void update_jitter (void)
{
	if (currprefs.floppy_random_bits_max > 0 && currprefs.floppy_random_bits_max >= currprefs.floppy_random_bits_min)
		disk_jitter = ((uaerand () >> 4) % (currprefs.floppy_random_bits_max - currprefs.floppy_random_bits_min + 1)) + currprefs.floppy_random_bits_min;
	else
		disk_jitter = 0;
}

static void updatetrackspeed (drive *drv, int mfmpos)
{
	if (dskdmaen < DSKDMA_WRITE) {
		int t = drv->tracktiming[mfmpos / 8];
		int ts = get_floppy_speed_from_image(drv) * t / 1000;
		if (ts < 700 || ts > 3000) {
			static int warned;
			warned++;
			if (warned < 50)
				write_log (_T("corrupted trackspeed value %d %d (%d/%d)\n"), t, ts, mfmpos, drv->tracklen);
		} else {
			drv->trackspeed = ts;
		}
	}
}

static void disk_doupdate_predict (int startcycle)
{
	int finaleventcycle = maxhpos << 8;
	int finaleventflag = 0;
	bool noselected = true;

	for (int dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (drv->motoroff)
			continue;
		if (!drv->trackspeed)
			continue;
		if ((selected | disabled) & (1 << dr))
			continue;
		int mfmpos = drv->mfmpos;
		if (drv->tracktiming[0])
			updatetrackspeed (drv, mfmpos);
		int diskevent_flag = 0;
		uae_u32 tword = word;
		noselected = false;
		int countcycle = startcycle;
		while (countcycle < (maxhpos << 8)) {
			int inc = nextbit(drv);
			if (drv->tracktiming[0])
				updatetrackspeed (drv, mfmpos);
			countcycle += drv->trackspeed;
			if (dskdmaen != DSKDMA_WRITE || (dskdmaen == DSKDMA_WRITE && !dma_enable)) {
				tword <<= 1;
				if (!drive_empty (drv)) {
					if (unformatted (drv))
						tword |= (uaerand () & 0x1000) ? 1 : 0;
					else
						tword |= getonebit(drv, drv->bigmfmbuf, mfmpos, &inc);
				}
				if (dskdmaen != DSKDMA_READ && (tword & 0xffff) == dsksync && dsksync != 0)
					diskevent_flag |= DISK_WORDSYNC;
			}
			mfmpos += inc;
			mfmpos %= drv->tracklen;
			if (!dskdmaen) {
				if (mfmpos == 0)
					diskevent_flag |= DISK_REVOLUTION << (drv - floppy);
				if (mfmpos == drv->indexoffset)
					diskevent_flag |= DISK_INDEXSYNC;
			}
			if (dskdmaen != DSKDMA_WRITE && mfmpos == drv->skipoffset) {
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
	if (dmaen(DMA_DISK) && dmaen(DMA_MASTER) && bitoffset == 15 && dma_enable && dskdmaen == DSKDMA_READ && dsklength >= 0) {
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
#if 0
				if (dsklength < 1 && (adkcon & 0x200))
					activate_debugger();
#endif
			}
		}
		return 1;
	}
	return 0;
}

static void wordsync_detected(bool startup)
{
	dsksync_cycles = get_cycles() + WORDSYNC_TIME * CYCLE_UNIT;
	if (dskdmaen != DSKDMA_OFF) {
		if (disk_debug_logging && dma_enable == 0) {
			int pos = -1;
			for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
				drive *drv = &floppy[i];
				if (!(disabled & (1 << i)) && !drv->motoroff) {
					pos = drv->mfmpos;
					break;
				}
			}
			write_log(_T("Sync match %04x mfmpos %d enable %d wordsync %d\n"), dsksync, pos, dma_enable, (adkcon & 0x0400) != 0);
			if (disk_debug_logging > 1)
				dumpdisk(_T("SYNC"));
		}
		if (!startup)
			dma_enable = 1;
		INTREQ(0x8000 | 0x1000);
	}
	if (adkcon & 0x0400) { // WORDSYNC
		bitoffset = 15;
	}
}

static void disk_doupdate_read_reallynothing(int floppybits, bool state)
{
	// Only because there is at least one demo that checks wrong bit
	// and hangs unless DSKSYNC bit it set with zero DSKSYNC value...
	if (INTREQR() & 0x1000)
		return;
	while (floppybits >= get_floppy_speed()) {
		bool skipbit = false;
		word <<= 1;
		word |= (state ? 1 : 0);
		// MSBSYNC
		if (adkcon & 0x200) {
			if ((word & 0x0001) == 0 && bitoffset == 0) {
				word = 0;
				skipbit = true;
			}
			if ((word & 0x0001) == 0 && bitoffset == 8) {
				word >>= 1;
				skipbit = true;
			}
		}
		if (!skipbit && (bitoffset & 7) == 7) {
			dskbytr_val = word & 0xff;
			dskbytr_val |= 0x8000;
		}
		if (!(adkcon & 0x200) && word == dsksync) {
			INTREQ(0x8000 | 0x1000);
		}
		bitoffset++;
		bitoffset &= 15;
		floppybits -= get_floppy_speed();
	}
}

static void disk_doupdate_read_nothing(int floppybits)
{
	while (floppybits >= get_floppy_speed()) {
		bool skipbit = false;
		word <<= 1;
		word |= (uaerand() & 0x1000) ? 1 : 0;
		doreaddma();
		// MSBSYNC
		if (adkcon & 0x200) {
			if ((word & 0x0001) == 0 && bitoffset == 0) {
				word = 0;
				skipbit = true;
			}
			if ((word & 0x0001) == 0 && bitoffset == 8) {
				word >>= 1;
				skipbit = true;
			}
		}
		if (!skipbit && (bitoffset & 7) == 7) {
			dskbytr_val = word & 0xff;
			dskbytr_val |= 0x8000;
		}
		if (!(adkcon & 0x200) && word == dsksync) {
			wordsync_detected(false);
		}
		bitoffset++;
		bitoffset &= 15;
		floppybits -= get_floppy_speed();
	}
}

static void disk_doupdate_read (drive * drv, int floppybits)
{
	/*
	uae_u16 *mfmbuf = drv->bigmfmbuf;
	dsksync = 0x4444;
	adkcon |= 0x400;
	drv->mfmpos = 0;
	memset (mfmbuf, 0, 1000);
	cycles = 0x1000000;
	// 4444 4444 4444 aaaa aaaaa 4444 4444 4444
	// 4444 aaaa aaaa 4444
	mfmbuf[0] = 0x4444;
	mfmbuf[1] = 0x4444;
	mfmbuf[2] = 0x4444;
	mfmbuf[3] = 0xaaaa;
	mfmbuf[4] = 0xaaaa;
	mfmbuf[5] = 0x4444;
	mfmbuf[6] = 0x4444;
	mfmbuf[7] = 0x4444;
	*/
	while (floppybits >= drv->trackspeed) {
		bool skipbit = false;
		int inc = nextbit(drv);

		if (drv->tracktiming[0])
			updatetrackspeed (drv, drv->mfmpos);

		word <<= 1;

		if (!drive_empty (drv)) {
			if (unformatted (drv))
				word |= (uaerand () & 0x1000) ? 1 : 0;
			else
				word |= getonebit(drv, drv->bigmfmbuf, drv->mfmpos, &inc);
		}

		//write_log (_T("%08X bo=%d so=%d mfmpos=%d dma=%d\n"), (word & 0xffffff), bitoffset, syncoffset, drv->mfmpos, dma_enable);
		if (doreaddma () < 0) {
			word >>= 1;
			return;
		}
		drv->mfmpos += inc;
		drv->mfmpos %= drv->tracklen;
		if (drv->mfmpos == drv->indexoffset) {
			if (disk_debug_logging > 2 && drv->indexhack)
				write_log (_T("indexhack cleared\n"));
			drv->indexhack = 0;
			do_disk_index ();
		}
		if (drv->mfmpos == 0) {
			fetchnextrevolution (drv);
			if (drv->tracktiming[0])
				updatetrackspeed (drv, drv->mfmpos);
		}
		if (drv->mfmpos == drv->skipoffset) {
			int skipcnt = disk_jitter;
			while (skipcnt-- > 0) {
				drv->mfmpos += nextbit(drv);
				drv->mfmpos %= drv->tracklen;
				if (drv->mfmpos == drv->indexoffset) {
					if (disk_debug_logging > 2 && drv->indexhack)
						write_log (_T("indexhack cleared\n"));
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

		// MSBSYNC
		if (adkcon & 0x200) {
			if ((word & 0x0001) == 0 && bitoffset == 0) {
				word = 0;
				skipbit = true;
			}
			if ((word & 0x0001) == 0 && bitoffset == 8) {
				word >>= 1;
				skipbit = true;
			}
		}

		if (!skipbit && (bitoffset & 7) == 7) {
			dskbytr_val = word & 0xff;
			dskbytr_val |= 0x8000;
		}

		// WORDSYNC
		if (!(adkcon & 0x200) && word == dsksync) {
			wordsync_detected(false);
		}

		if (!skipbit) {
			bitoffset++;
			bitoffset &= 15;
		}

		floppybits -= drv->trackspeed;
	}
}

static void disk_dma_debugmsg (void)
{
	write_log (_T("LEN=%04X (%d) SYNC=%04X PT=%08X ADKCON=%04X INTREQ=%04X PC=%08X\n"),
		dsklength, dsklength, (adkcon & 0x400) ? dsksync : 0xffff, dskpt, adkcon, intreq, M68K_GETPC);
}

/* this is very unoptimized. DSKBYTR is used very rarely, so it should not matter. */

uae_u16 DSKBYTR (int hpos)
{
	uae_u16 v;

	DISK_update (hpos);
	v = dskbytr_val;
	dskbytr_val &= ~0x8000;
	if (word == dsksync && cycles_in_range (dsksync_cycles)) {
		v |= 0x1000;
		if (disk_debug_logging > 1) {
			dumpdisk(_T("DSKBYTR SYNC"));
		}
	}
	if (dskdmaen != DSKDMA_OFF && dmaen(DMA_DISK) && dmaen(DMA_MASTER))
		v |= 0x4000;
	if (dsklen & 0x4000)
		v |= 0x2000;
	if (disk_debug_logging > 2)
		write_log (_T("DSKBYTR=%04X hpos=%d\n"), v, hpos);
	for (int dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (drv->motoroff)
			continue;
		if (!((selected | disabled) & (1 << dr))) {
			drv->lastdataacesstrack = drv->cyl * 2 + side;
#if REVOLUTION_DEBUG
			if (!drv->track_access_done)
				write_log(_T("DSKBYTR\n"));
#endif
			drv->track_access_done = true;
			if (disk_debug_mode & DISK_DEBUG_PIO) {
				if (disk_debug_track < 0 || disk_debug_track == 2 * drv->cyl + side) {
					disk_dma_debugmsg ();
					write_log (_T("DSKBYTR=%04X\n"), v);
					//activate_debugger ();
					break;
				}
			}
		}
	}
	return v;
}

static void DISK_start (void)
{
	int dr;

	if (disk_debug_logging > 1) {
		dumpdisk(_T("DSKLEN"));
	}

	for (int i = 0; i < 3; i++)
		fifo_inuse[i] = false;
	fifo_filled = 0;

	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (!((selected | disabled) & (1 << dr))) {
			int tr = drv->cyl * 2 + side;
			trackid *ti = drv->trackdata + tr;

			if (drv->dskchange_time == -1) {
				drv->dskchange_time = -2;
				write_log(_T("Accessing state restored non-existing disk '%s'!\n"), drv->newname);
				//if (gui_ask_disk(dr, drv->newname)) {
				//	if (drive_insert(drv, &currprefs, dr, drv->newname, false, false)) {
				//		write_log(_T("Replacement disk '%s' inserted.\n"), drv->newname);
				//		drv->dskready_up_time = 0;
				//		drv->dskchange_time = 0;
				//	}
				//}
			}

			if (dskdmaen == DSKDMA_WRITE) {
				word = 0;
				drv->tracklen = floppy_writemode > 0 ? FLOPPY_WRITE_MAXLEN : FLOPPY_WRITE_LEN * drv->ddhd * 8 * 2;
				drv->trackspeed = get_floppy_speed ();
				drv->skipoffset = -1;
				updatemfmpos (drv);
			}
			/* Ugh.  A nasty hack.  Assume ADF_EXT1 tracks are always read
			from the start.  */
			if (ti->type == TRACK_RAW1) {
				drv->mfmpos = 0;
				bitoffset = 0;
				word = 0;
			}
			if (drv->catweasel) {
				word = 0;
				drive_fill_bigbuf (drv, 1);
			}
		}
		drv->floppybitcounter = 0;
	}

	dma_enable = (adkcon & 0x400) ? 0 : 1;
	if (word == dsksync)
		wordsync_detected(true);
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
				if (disk_debug_logging > 0)
					write_log (_T("delayed insert, drive %d, image '%s'\n"), dr, drv->newname);
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

	// show insert disk in df0: when booting
	if (initial_disk_statusline) {
		initial_disk_statusline = false;
		update_disk_statusline(0);
	}
}

void DISK_update (int tohpos)
{
	int dr;
	int cycles;

	if (disk_hpos < 0) {
		disk_hpos = - disk_hpos;
		return;
	}

	cycles = (tohpos << 8) - disk_hpos;
#if 0
	if (tohpos == 228)
		write_log (_T("x"));
	if (tohpos != maxhpos || cycles / 256 != maxhpos)
		write_log (_T("%d %d %d\n"), tohpos, cycles / 256, disk_hpos / 256);
#endif
	if (cycles <= 0)
		return;
	disk_hpos += cycles;
	if (disk_hpos >= (maxhpos << 8))
		disk_hpos %= 1 << 8;

	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];

		if (drv->motoroff || !drv->tracklen || !drv->trackspeed) {
			continue;
		}
		drv->floppybitcounter += cycles;
		if ((selected | disabled) & (1 << dr)) {
			drv->mfmpos += drv->floppybitcounter / drv->trackspeed;
			drv->mfmpos %= drv->tracklen;
			drv->floppybitcounter %= drv->trackspeed;
			continue;
		}
		if (drv->diskfile) {
			drive_fill_bigbuf(drv, 0);
		}
		drv->mfmpos %= drv->tracklen;
	}
	int didaccess = 0;
	bool done_jitter = false;
	for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		if (drv->motoroff || !drv->trackspeed)
			continue;
		if ((selected | disabled) & (1 << dr))
			continue;
		if (!done_jitter) {
			update_jitter();
			done_jitter = true;
		}
		/* write dma and wordsync enabled: read until wordsync match found */
		if (dskdmaen == DSKDMA_WRITE && dma_enable) {
			disk_doupdate_write(drv->floppybitcounter, drv->trackspeed);
		} else {
			disk_doupdate_read(drv, drv->floppybitcounter);
		}
		
		drv->floppybitcounter %= drv->trackspeed;
		didaccess = 1;
	}
	/* no floppy selected but dma active */
	if (!didaccess) {
		if (dskdmaen == DSKDMA_READ) {
			disk_doupdate_read_nothing(cycles);
		} else if (dskdmaen == DSKDMA_WRITE) {
			disk_doupdate_write(cycles, get_floppy_speed());
		} else {
			//disk_doupdate_read_reallynothing(cycles, true);
		}
	}

	/* instantly finish dma if dsklen==0 and wordsync detected */
	if (dskdmaen != DSKDMA_OFF && dma_enable && dsklength2 == 0 && dsklength == 0)
		disk_dmafinished ();

	if (!done_jitter) {
		update_jitter();
		done_jitter = true;
	}
	disk_doupdate_predict (disk_hpos);
}

void DSKLEN (uae_u16 v, int hpos)
{
	int dr;
	int prevlen = dsklen;
	int prevdatalen = dsklength;
	int noselected = 0;
	int motormask;

	DISK_update (hpos);

	dsklen = v;
	dsklength2 = dsklength = dsklen & 0x3fff;

	if ((v & 0x8000) && (prevlen & 0x8000)) {
		if (dskdmaen == DSKDMA_READ && !(v & 0x4000)) {
			// update only currently active DMA length, don't change DMA state
			write_log(_T("warning: Disk read DMA length rewrite %d -> %d. (%04x) PC=%08x\n"), prevlen & 0x3fff, v & 0x3fff, v, M68K_GETPC);
			return;
		}
		dskdmaen = DSKDMA_READ;
		DISK_start ();
	}
	if (!(v & 0x8000)) {
		if (dskdmaen != DSKDMA_OFF) {
			/* Megalomania and Knightmare does this */
			if (disk_debug_logging > 0 && dskdmaen == DSKDMA_READ)
				write_log (_T("warning: Disk read DMA aborted, %d words left PC=%x\n"), prevdatalen, M68K_GETPC);
			if (dskdmaen == DSKDMA_WRITE) {
				write_log (_T("warning: Disk write DMA aborted, %d words left PC=%x\n"), prevdatalen, M68K_GETPC);
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

	if (dskdmaen == DSKDMA_OFF)
		return;

	if (dsklength == 0 && dma_enable) {
		disk_dmafinished ();
		return;
	}

	if ((v & 0x4000) && (prevlen & 0x4000)) {
		if (dsklength == 0)
			return;
		if (dsklength == 1) {
			disk_dmafinished ();
			return;
		}
		if (dskdmaen == DSKDMA_WRITE) {
			write_log(_T("warning: Disk write DMA length rewrite %d -> %d\n"), prevlen & 0x3fff, v & 0x3fff);
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
#if REVOLUTION_DEBUG
			write_log(_T("DMA\n"));
#endif
		}
	}

	if (((disk_debug_mode & DISK_DEBUG_DMA_READ) && dskdmaen == DSKDMA_READ) ||
		((disk_debug_mode & DISK_DEBUG_DMA_WRITE) && dskdmaen == DSKDMA_WRITE))
	{
		for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
			drive *drv = &floppy[dr];
			if (drv->motoroff)
				continue;
			if (!(selected & (1 << dr))) {
				if (disk_debug_track < 0 || disk_debug_track == 2 * drv->cyl + side) {
					disk_dma_debugmsg ();
					//activate_debugger ();
					break;
				}
			}
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
		if (!amax_enabled) {
			write_log (_T("disk %s DMA started, drvmask=%x motormask=%x PC=%08x\n"),
				dskdmaen == DSKDMA_WRITE ? _T("write") : _T("read"), selected ^ 15, motormask, M68K_GETPC);
		}
		noselected = 1;
	} else {
		if (disk_debug_logging > 0) {
			write_log (_T("disk %s DMA started, drvmask=%x track %d mfmpos %d dmaen=%d PC=%08X\n"),
				dskdmaen == DSKDMA_WRITE ? _T("write") : _T("read"), selected ^ 15,
				floppy[dr].cyl * 2 + side, floppy[dr].mfmpos, dma_enable, M68K_GETPC);
			disk_dma_debugmsg ();
		}
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
		if (drv->filetype != ADF_NORMAL && drv->filetype != ADF_KICK && drv->filetype != ADF_SKICK && drv->filetype != ADF_NORMAL_HEADER)
			break;
	}
	if (dr < MAX_FLOPPY_DRIVES) /* no turbo mode if any selected drive has non-standard ADF */
		return;
	{
		int done = 0;
		for (dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
			drive *drv = &floppy[dr];
			bool floppysupported = (drv->ddhd < 2) || (drv->ddhd > 1 && currprefs.floppyslots[dr].dfxtype == DRV_35_HD);
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

				if ((adkcon & 0x400) && floppysupported) {
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
				// read nothing if not supported and MFMSYNC is on.
				if ((floppysupported) || (!floppysupported && !(adkcon & 0x400))) {
					while (dsklength-- > 0) {
						chipmem_wput_indirect (dskpt, floppysupported ? drv->bigmfmbuf[pos >> 4] : uaerand());
						dskpt += 2;
						pos += 16;
						pos %= drv->tracklen;
					}
				} else {
					pos += uaerand();
					pos %= drv->tracklen;
				}
				drv->mfmpos = pos;
				if (floppysupported)
					INTREQ (0x8000 | 0x1000);
				done = 2;

			} else if (dskdmaen == DSKDMA_WRITE) { /* TURBO write */

				if (floppysupported) {
					for (i = 0; i < dsklength; i++) {
						uae_u16 w = chipmem_wget_indirect (dskpt + i * 2);
						drv->bigmfmbuf[pos >> 4] = w;
#ifdef AMAX
						if (amax_enabled)
							amax_diskwrite (w);
#endif
						pos += 16;
						pos %= drv->tracklen;
					}
					drv->mfmpos = pos;
					drive_write_data (drv);
					done = 2;
				} else {
					pos += uaerand();
					pos %= drv->tracklen;
					drv->mfmpos = pos;
					done = 2;
				}
			}
		}
		if (!done && noselected && amax_enabled) {
			int bits = -1;
			while (dsklength-- > 0) {
				if (dskdmaen == DSKDMA_WRITE) {
					uae_u16 w = chipmem_wget_indirect(dskpt);
#ifdef AMAX
					amax_diskwrite(w);
					if (w) {
						for (int i = 0; i < 16; i++) {
							if (w & (1 << i))
								bits++;
						}
					}
#endif
				} else {
					chipmem_wput_indirect(dskpt, 0);
				}
				dskpt += 2;
			}
			if (bits == 0) {
				// AMAX speedup hack
				done = 1;
			} else {
				INTREQ(0x8000 | 0x1000);
				done = 2;
			}
		}

		if (done) {
			linecounter = done;
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
	return dskdmaen == DSKDMA_WRITE;
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
	for (int dr = 0; dr < MAX_FLOPPY_DRIVES; dr++) {
		drive *drv = &floppy[dr];
		drive_image_free (drv);
	}
}

void DISK_init (void)
{
	for (int dr = MAX_FLOPPY_DRIVES - 1; dr >= 0; dr--) {
		drive *drv = &floppy[dr];
		/* reset all drive types to 3.5 DD */
		drive_settype_id (drv);
		if (!drive_insert (drv, &currprefs, dr, currprefs.floppyslots[dr].df, false, currprefs.floppyslots[dr].forcedwriteprotect))
			disk_eject (dr);
	}
	if (disk_empty (0))
		write_log (_T("No disk in drive 0.\n"));
	//amax_init ();
}

void DISK_reset (void)
{
	if (savestate_state)
		return;

	//floppy[0].catweasel = &cwc.drives[0];
	disk_hpos = 0;
	dskdmaen = 0;
	disabled = 0;
	memset(&disk_info_data, 0, sizeof disk_info_data);
	for (int dr = MAX_FLOPPY_DRIVES - 1; dr >= 0; dr--) {
		reset_drive (dr);
	}
	initial_disk_statusline = true;
	setamax ();
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

using namespace tinyxml2;

static bool abr_loaded;
static tinyxml2::XMLDocument abr_xml[2];
static const TCHAR* abr_files[] = { _T("brainfile.xml"), _T("catlist.xml"), NULL };

static void abrcheck(struct diskinfo *di)
{
	TCHAR path[MAX_DPATH];

	if (!abr_loaded) {
		bool error = false;
		for (int i = 0; abr_files[i] && !error; i++) {
			get_plugin_path(path, sizeof(path) / sizeof(TCHAR), _T("abr"));
			_tcscat(path, abr_files[i]);
			FILE *f = fopen(path, _T("rb"));
			if (f) {
				tinyxml2::XMLError err = abr_xml[i].LoadFile(f);
				if (err != XML_SUCCESS) {
					write_log(_T("failed to parse '%s': %d\n"), path, err);
					error = true;
				}
				fclose(f);
			} else {
				error = true;
			}
		}
		if (!error) {
			abr_loaded = true;
		}
	}
	if (!abr_loaded)
		return;
	tinyxml2::XMLElement *detectedelementcrc32 = NULL;
	tinyxml2::XMLElement *detectedelementrecog = NULL;
	tinyxml2::XMLElement *e = abr_xml[0].FirstChildElement("Bootblocks");
	if (e) {
		e = e->FirstChildElement("Bootblock");
		if (e) {
			do {
				tinyxml2::XMLElement *ercrc = e->FirstChildElement("CRC");
				if (ercrc) {
					const char *n_crc32 = ercrc->GetText();
					if (strlen(n_crc32) == 8) {
						char *endptr;
						uae_u32 crc32 = strtol(n_crc32, &endptr, 16);
						if (di->bootblockcrc32 == crc32) {
							detectedelementcrc32 = e;
						}
					}
				}
				tinyxml2::XMLElement *er = e->FirstChildElement("Recog");
				if (er) {
					const char *tr = er->GetText();
					bool detected = false;
					while (tr) {
						int offset = atoi(tr);
						if (offset < 0 || offset > 1023)
							break;
						tr = strchr(tr, ',');
						if (!tr || !tr[1])
							break;
						tr++;
						int val = atoi(tr);
						if (val < 0 || val > 255)
							break;
						if (di->bootblock[offset] != val)
							break;
						tr = strchr(tr, ',');
						if (!tr) {
							detected = true;
						} else {
							tr++;
						}
					}
					if (detected) {
						detectedelementrecog = e;
					}
				}
				e = e->NextSiblingElement();
			} while (e && !detectedelementcrc32);

			if (detectedelementcrc32 != NULL || detectedelementrecog != NULL) {
				if (detectedelementcrc32) {
					e = detectedelementcrc32;
				} else {
					e = detectedelementrecog;
				}
				tinyxml2::XMLElement *e_name = e->FirstChildElement("Name");
				if (e_name) {
					const char *n_name = e_name->GetText();
					if (n_name) {
						TCHAR *s = au(n_name);
						_tcscpy(di->bootblockinfo, s);
						xfree(s);
					}
				}
				tinyxml2::XMLElement *e_class = e->FirstChildElement("Class");
				if (e_class) {
					const char *t_class = e_class->GetText();
					if (t_class) {
						tinyxml2::XMLElement *ecats = abr_xml[1].FirstChildElement("Categories");
						if (ecats) {
							tinyxml2::XMLElement *ecat = ecats->FirstChildElement("Category");
							if (ecat) {
								do {
									tinyxml2::XMLElement *ecatr = ecat->FirstChildElement("abbrev");
									if (ecatr) {
										const char *catabbr = ecatr->GetText();
										if (!strcmp(catabbr, t_class)) {
											tinyxml2::XMLElement *ecatn = ecat->FirstChildElement("Name");
											if (ecatn) {
												const char *n_catname = ecatn->GetText();
												if (n_catname) {
													TCHAR *s = au(n_catname);
													_tcscpy(di->bootblockclass, s);
													xfree(s);
													break;
												}
											}
										}
									}
									ecat = ecat->NextSiblingElement();
								} while (ecat);
							}
						}
						return;
					}
				}
			}
		}
	}
}

int DISK_examine_image (struct uae_prefs *p, int num, struct diskinfo *di, bool deepcheck)
{
	int drvsec;
	int ret, i;
	drive *drv = &floppy[num];
	uae_u32 dos, crc, crc2;
	int wasdelayed = drv->dskchange_time;
	int sectable[MAX_SECTORS];
	int oldcyl, oldside;
	uae_u32 v = 0;

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
	di->imagecrc32 = zfile_crc32 (drv->diskfile);
	di->unreadable = false;
	decode_buffer (drv->bigmfmbuf, drv->cyl, 11, drv->ddhd, drv->filetype, &drvsec, sectable, 1);
	di->hd = drv->ddhd == 2;
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
	di->bootblockcrc32 = get_crc32(di->bootblock, 1024);
	if (deepcheck) {
		abrcheck(di);
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
	if (pdskbytr)
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
	size = (int)zfile_ftell(drv->diskfile);
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
	setamax ();
#if 0
	if (dskdmaen)
		dumpdisk ();
#endif
}

uae_u8 *restore_disk (int num,uae_u8 *src)
{
	drive *drv;
	int state, dfxtype;
	TCHAR old[MAX_DPATH];
	TCHAR *s;
	int newis;

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
	int dskready_up_time = restore_u16 ();
	int dskready_down_time = restore_u16 ();
	if (restore_u32() & 1) {
		xfree(s);
		s = restore_path_full();
	}
	if (s && s[0])
		write_log(_T("-> '%s'\n"), s);
	_tcscpy(old, currprefs.floppyslots[num].df);
	_tcsncpy(changed_prefs.floppyslots[num].df, s, MAX_DPATH);
	xfree(s);

	newis = changed_prefs.floppyslots[num].df[0] ? 1 : 0;
	if (!(disabled & (1 << num))) {
		if (!newis && old[0]) {
			*currprefs.floppyslots[num].df = *changed_prefs.floppyslots[num].df = 0;
			drv->dskchange = false;
		} else if (newis) {
			drive_insert (floppy + num, &currprefs, num, changed_prefs.floppyslots[num].df, false, false);
			if (drive_empty (floppy + num)) {
				if (newis && zfile_exists(old)) {
					_tcscpy (changed_prefs.floppyslots[num].df, old);
					drive_insert (floppy + num, &currprefs, num, changed_prefs.floppyslots[num].df, false, false);
					if (drive_empty (floppy + num))
						drv->dskchange = true;
				} else {
					drv->dskchange_time = -1;
					_tcscpy(drv->newname, changed_prefs.floppyslots[num].df);
					_tcscpy(currprefs.floppyslots[num].df, drv->newname);
					write_log(_T("Disk image not found, faking inserted disk.\n"));
				}
			}
		}
	}
	drv->mfmpos = mfmpos;
	drv->prevtracklen = drv->tracklen;
	drv->dskready_up_time = dskready_up_time;
	drv->dskready_down_time = dskready_down_time;
	reset_drive_gui (num);
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
		dstbak = dst = xmalloc (uae_u8, 4 + 1 + 1 + 1 + 1 + 4 + 4 + MAX_DPATH + 2 + 2 + 4 + 2 * MAX_DPATH);
	save_u32 (drv->drive_id);	    /* drive type ID */
	save_u8 ((drv->motoroff ? 0 : 1) | ((disabled & (1 << num)) ? 2 : 0) | (drv->idbit ? 4 : 0) | (drv->dskchange ? 8 : 0) | (side ? 16 : 0) | (drv->wrprot ? 32 : 0));
	save_u8 (drv->cyl);				/* cylinder */
	save_u8 (drv->dskready);	    /* dskready */
	save_u8 (drv->drive_id_scnt);   /* id mode position */
	save_u32 (drv->mfmpos);			/* disk position */
	save_u32 (getadfcrc (drv));	    /* CRC of disk image */
	save_path (usepath ? currprefs.floppyslots[num].df : _T(""), SAVESTATE_PATH_FLOPPY);/* image name */
	save_u16 (drv->dskready_up_time);
	save_u16 (drv->dskready_down_time);
	if (usepath) {
		save_u32(1);
		save_path_full(currprefs.floppyslots[num].df, SAVESTATE_PATH_FLOPPY);
	}

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
		size += ((drv->tracklen + 15) * 2) / 8;
		if (drv->tracktiming[0]) {
			m |= 2;
			size *= 2;
		}
	}
	if (!m)
		return NULL;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 2 + 4 + 2 + 4 + 4 + size + 2);

	save_u32 (m);
	save_u16 (drv->floppybitcounter);
	save_u32 (drv->tracklen);
	save_u16 (drv->trackspeed);
	save_u32 (drv->skipoffset);
	save_u32 (drv->indexoffset);
	for (int j = 0; j < (drv->tracklen + 15) / 16; j++) {
		save_u16(drv->bigmfmbuf[j]);
		if (drv->tracktiming[0])
			save_u16(drv->tracktiming[j]);
	}
	save_u16 (drv->revolutions);

	*len = dst - dstbak;
	return dstbak;
}

/* internal floppy controller variables */

uae_u8 *restore_floppy (uae_u8 *src)
{
	word = restore_u16 ();
	bitoffset = restore_u8 ();
	dma_enable = restore_u8 ();
	disk_hpos = restore_u8 () & 0xff;
	dskdmaen = restore_u8 ();
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

uae_u8 *save_floppy (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 100);

	save_u16 (word);			/* shift register */
	save_u8 (bitoffset);		/* dma bit offset */
	save_u8 (dma_enable);		/* disk sync found */
	save_u8 (disk_hpos & 0xff);	/* next bit read position */
	save_u8 (dskdmaen);			/* dma status */
	for (int i = 0; i < 3; i++) {
		save_u16 (fifo[i]);
		save_u8 (fifo_inuse[i]);
	}
	save_u16 (dsklength);
	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */

#define MAX_DISKENTRIES 4
int disk_prevnext_name (TCHAR *imgp, int dir)
{
	TCHAR img[MAX_DPATH], *ext, *p, *p2, *ps, *dst[MAX_DISKENTRIES];
	int num = -1;
	int cnt, i;
	TCHAR imgl[MAX_DPATH];
	int ret, gotone, wrapped;
	TCHAR *old;

	old = my_strdup (imgp);

	struct zfile *zf = zfile_fopen (imgp, _T("rb"), ZFD_NORMAL);
	if (zf) {
		_tcscpy (img, zfile_getname (zf));
		zfile_fclose (zf);
		zf = zfile_fopen (img, _T("rb"), ZFD_NORMAL);
		if (!zf) // oops, no directory support in this archive type
			_tcscpy (img, imgp);
		zfile_fclose (zf);
	} else {
		_tcscpy (img, imgp);
	}

	wrapped = 0;
retry:
	_tcscpy (imgl, img);
	to_lower (imgl, sizeof imgl / sizeof (TCHAR));
	gotone = 0;
	ret = 0;
	ps = imgl;
	cnt = 0;
	dst[cnt] = NULL;
	for (;;) {
		// disk x of y
		p = _tcsstr (ps, _T("(disk "));
		if (p && _istdigit (p[6])) {
			p2 = p - imgl + img;
			num = _tstoi (p + 6);
			dst[cnt++] = p2 + 6;
			if (cnt >= MAX_DISKENTRIES - 1)
				break;
			gotone = 1;
			ps = p + 6;
			continue;
		}
		if (gotone)
			 break;
		p = _tcsstr (ps, _T("disk"));
		if (p && _istdigit (p[4])) {
			p2 = p - imgl + img;
			num = _tstoi (p + 4);
			dst[cnt++] = p2 + 4;
			if (cnt >= MAX_DISKENTRIES - 1)
				break;
			gotone = 1;
			ps = p + 4;
			continue;
		}
		if (gotone)
			 break;
		ext = _tcsrchr (ps, '.');
		if (!ext || ext - ps < 4)
			break;
		TCHAR *ext2 = ext - imgl + img;
		// name_<non numeric character>x.ext
		if (ext[-3] == '_' && !_istdigit (ext[-2]) && _istdigit (ext[-1])) {
			num = _tstoi (ext - 1);
			dst[cnt++] = ext2 - 1;
		// name_x.ext, name-x.ext, name x.ext
		} else if ((ext[-2] == '_' || ext[-2] == '-' || ext[-2] == ' ') && _istdigit (ext[-1])) {
			num = _tstoi (ext - 1);
			dst[cnt++] = ext2 - 1;
		// name_a.ext, name-a.ext, name a .ext
		} else if ((ext[-2] == '_' || ext[-2] == '-' || ext[-2] == ' ') && ext[-1] >= 'a' && ext[-1] <= 'z') {
			num = ext[-1] - 'a' + 1;
			dst[cnt++] = ext2 - 1;
		// nameA.ext
		} else if (ext2[-2] >= 'a' && ext2[-2] <= 'z' && ext2[-1] >= 'A' && ext2[-1] <= 'Z') {
			num = ext[-1] - 'a' + 1;
			dst[cnt++] = ext2 - 1;
		// namex.ext
		} else if (!_istdigit (ext2[-2]) && _istdigit (ext[-1])) {
			num = ext[-1] - '0';
			dst[cnt++] = ext2 - 1;
		}
		break;
	}
	dst[cnt] = NULL;
	if (num <= 0 || num >= 19)
		goto end;
	num += dir;
	if (num > 9)
		goto end;
	if (num == 9)
		num = 1;
	else if (num == 0)
		num = 9;
	for (i = 0; i < cnt; i++) {
		if (!_istdigit (dst[i][0])) {
			int capital = dst[i][0] >= 'A' && dst[i][0] <= 'Z';
			dst[i][0] = (num - 1) + (capital ? 'A' : 'a');
		} else {
			dst[i][0] = num + '0';
		}
	}
	if (zfile_exists (img)) {
		ret = 1;
		goto end;
	}
	if (gotone) { // was (disk x but no match, perhaps there are extra tags..
		TCHAR *old2 = my_strdup (img);
		for (;;) {
			ext = _tcsrchr (img, '.');
			if (!ext)
				break;
			if (ext == img)
				break;
			if (ext[-1] != ']')
				break;
			TCHAR *t = _tcsrchr (img, '[');
			if (!t)
				break;
			t[0] = 0;
			if (zfile_exists (img)) {
				ret = 1;
				goto end;
			}
		}
		_tcscpy (img, old2);
		xfree (old2);
	}
	if (!wrapped) {
		for (i = 0; i < cnt; i++) {
			if (!_istdigit (dst[i][0]))
				dst[i][0] = dst[i][0] >= 'A' && dst[i][0] <= 'Z' ? 'A' : 'a';
			else
				dst[i][0] = '1';
			if (dir < 0)
				dst[i][0] += 8;
		}
		wrapped++;
	}
	if (zfile_exists (img)) {
		ret = -1;
		goto end;
	}
	if (dir < 0 && wrapped < 2)
		goto retry;
	_tcscpy (img, old);

end:
	_tcscpy (imgp, img);
	xfree (old);
	return ret;
}

int disk_prevnext (int drive, int dir)
{
	TCHAR img[MAX_DPATH];

	 _tcscpy (img, currprefs.floppyslots[drive].df);

	if (!img[0])
		return 0;
	disk_prevnext_name (img, dir);
	_tcscpy (changed_prefs.floppyslots[drive].df, img);
	return 1;
}

static int getdebug(void)
{
	return floppy[0].mfmpos;
}

static int get_reserved_id(int num)
{
	for (int i = 0; i < MAX_FLOPPY_DRIVES; i++) {
		if (reserved & (1 << i)) {
			if (num > 0) {
				num--;
				continue;
			}
			return i;
		}
	}
	return -1;
}

void disk_reserved_setinfo(int num, int cyl, int head, int motor)
{
	int i = get_reserved_id(num);
	if (i >= 0) {
		drive *drv = &floppy[i];
		reserved_side = head;
		drv->cyl = cyl;
		drv->state = motor != 0;
		update_drive_gui(i, false);
	}
}

bool disk_reserved_getinfo(int num, struct floppy_reserved *fr)
{
	int idx = get_reserved_id(num);
	if (idx >= 0) {
		drive *drv = &floppy[idx];
		fr->num = idx;
		fr->img = drv->diskfile;
		fr->wrprot = drv->wrprot;
		if (drv->diskfile && !drv->pcdecodedfile && (drv->filetype == ADF_EXT2 || drv->filetype == ADF_FDI || drv->filetype == ADF_IPF || drv->filetype == ADF_SCP)) {
			int cyl = drv->cyl;
			int side2 = side;
			struct zfile *z = zfile_fopen_empty(NULL, zfile_getfilename(drv->diskfile));
			if (z) {
				bool ok = false;
				drv->num_secs = 21; // max possible
				drive_fill_bigbuf(drv, true);
				int secs = drive_write_pcdos(drv, z, 1);
				if (secs >= 8) {
					ok = true;
					drv->num_secs = secs;
					for (int i = 0; i < drv->num_tracks; i++) {
						drv->cyl = i / 2;
						side = i & 1;
						drive_fill_bigbuf(drv, true);
						drive_write_pcdos(drv, z, 0);
					}
				}
				drv->cyl = cyl;
				side = side2;
				if (ok) {
					write_log(_T("Created  internal PC disk image cyl=%d secs=%d size=%d\n"), drv->num_tracks / 2, drv->num_secs, zfile_size(z));
					drv->pcdecodedfile = z;
				} else {
					write_log(_T("Failed to create internal PC disk image\n"));
					zfile_fclose(z);
				}
			}
		}
		if (drv->pcdecodedfile) {
			fr->img = drv->pcdecodedfile;
		}
		fr->cyl = drv->cyl;
		fr->cyls = drv->num_tracks / 2;
		fr->drive_cyls = currprefs.floppyslots[idx].dfxtype == DRV_PC_525_ONLY_40 ? 40 : 80;
		fr->secs = drv->num_secs;
		fr->heads = drv->num_heads;
		fr->disk_changed = drv->dskchange || fr->img == NULL;
		if (currprefs.floppyslots[idx].dfxtype == DRV_PC_35_ONLY_80) {
			if (drv->num_secs > 14)
				fr->rate = FLOPPY_RATE_500K; // 1.2M/1.4M
			else
				fr->rate = FLOPPY_RATE_250K; // 720K
		} else if (currprefs.floppyslots[idx].dfxtype == DRV_PC_525_40_80) {
			if (fr->cyls < 80) {
				if (drv->num_secs < 9)
					fr->rate = FLOPPY_RATE_250K; // 320k in 80 track drive
				else
					fr->rate = FLOPPY_RATE_300K; // 360k in 80 track drive
			} else {
				if (drv->num_secs > 14)
					fr->rate = FLOPPY_RATE_500K; // 1.2M/1.4M
				else
					fr->rate = FLOPPY_RATE_300K; // 720K
			}
		} else {
			if (drv->num_secs < 9)
				fr->rate = FLOPPY_RATE_300K;// 320k in 40 track drive
			else
				fr->rate = FLOPPY_RATE_250K;// 360k in 40 track drive
			// yes, above values are swapped compared to 1.2M drive case
		}
		return true;
	}
	return false;
}

void disk_reserved_reset_disk_change(int num)
{
	int i = get_reserved_id(num);
	if (i >= 0) {
		drive *drv = &floppy[i];
		drv->dskchange = false;
	}
}
