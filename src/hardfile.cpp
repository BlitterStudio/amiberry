/*
* UAE - The Un*x Amiga Emulator
*
* Hardfile emulation
*
* Copyright 1995 Bernd Schmidt
*           2002 Toni Wilen (scsi emulation, 64-bit support)
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "filesys.h"
#include "execlib.h"
#include "native2amiga.h"
#include "gui.h"
#include "uae.h"
#include "scsi.h"
#include "gayle.h"
#include "execio.h"
#include "zfile.h"
#include "ide.h"
#include "debug.h"
#include "ini.h"
#include "rommgr.h"
#include "zarchive.h"

#ifdef WITH_CHD
#include "archivers/chd/chd.h"
#include "archivers/chd/harddisk.h"
#endif

#define HDF_SUPPORT_NSD 1
#define HDF_SUPPORT_TD64 1
#define HDF_SUPPORT_DS 1
#define HDF_SUPPORT_DS_PARTITION 0

#undef DEBUGME
#define hf_log(fmt, ...)
#define hf_log2(fmt, ...)
#define scsi_log(fmt, ...)
#define hf_log3(fmt, ...)

//#define DEBUGME
#ifdef DEBUGME
#undef hf_log
#define hf_log write_log
#undef hf_log2
#define hf_log2 write_log
#undef hf_log3
#define hf_log3 write_log
#undef scsi_log
#define scsi_log write_log
#endif

extern int log_scsiemu;
int enable_ds_partition_hdf;

#define MAX_ASYNC_REQUESTS 50
#define ASYNC_REQUEST_NONE 0
#define ASYNC_REQUEST_TEMP 1
#define ASYNC_REQUEST_CHANGEINT 10

struct hardfileprivdata {
	uaecptr d_request[MAX_ASYNC_REQUESTS];
	uae_u8 *d_request_iobuf[MAX_ASYNC_REQUESTS];
	int d_request_type[MAX_ASYNC_REQUESTS];
	uae_u32 d_request_data[MAX_ASYNC_REQUESTS];
	smp_comm_pipe requests;
	int thread_running;
	uae_sem_t sync_sem;
	uaecptr base;
	int changenum;
	uaecptr changeint;
	struct scsi_data *sd;
	bool directorydrive;
};

#define HFD_CHD_OTHER 5
#define HFD_CHD_HD 4
#define HFD_VHD_DYNAMIC 3
#define HFD_VHD_FIXED 2

STATIC_INLINE uae_u32 gl (uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

static uae_sem_t change_sem;

static struct hardfileprivdata hardfpd[MAX_FILESYSTEM_UNITS];

static uae_u32 nscmd_cmd;

static void wl (uae_u8 *p, uae_u32 v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v;
}
static void ww (uae_u8 *p, uae_u16 v)
{
	p[0] = v >> 8;
	p[1] = (uae_u8)v;
}
static int rl (uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}


static void getchs2 (struct hardfiledata *hfd, int *cyl, int *cylsec, int *head, int *tracksec)
{
	unsigned int total = (unsigned int)(hfd->virtsize / 1024);
	int heads;
	int sectors = 63;

	if (hfd->ci.physical_geometry) {
		*cyl = hfd->ci.pcyls;
		*tracksec = hfd->ci.psecs;
		*head = hfd->ci.pheads;
		*cylsec = (*head) * (*tracksec);
		return;
	}
	/* do we have RDB values? */
	if (hfd->rdbcylinders) {
		*cyl = hfd->rdbcylinders;
		*tracksec = hfd->rdbsectors;
		*head = hfd->rdbheads;
		*cylsec = hfd->rdbsectors * hfd->rdbheads;
		return;
	}
	/* what about HDF settings? */
	if (hfd->ci.surfaces && hfd->ci.sectors) {
		*head = hfd->ci.surfaces;
		*tracksec = hfd->ci.sectors;
		*cylsec = (*head) * (*tracksec);
		*cyl = (unsigned int)(hfd->virtsize / hfd->ci.blocksize) / ((*tracksec) * (*head));
		if (*cyl == 0)
			*cyl = (unsigned int)hfd->ci.max_lba / ((*tracksec) * (*head));
		return;
	}
	/* no, lets guess something.. */
	if (total <= 504 * 1024)
		heads = 16;
	else if (total <= 1008 * 1024)
		heads = 32;
	else if (total <= 2016 * 1024)
		heads = 64;
	else if (total <= 4032 * 1024)
		heads = 128;
	else
		heads = 255;
	*cyl = (unsigned int)(hfd->virtsize / hfd->ci.blocksize) / (sectors * heads);
	if (*cyl == 0)
		*cyl = (unsigned int)hfd->ci.max_lba / (sectors * heads);
	*cylsec = sectors * heads;
	*tracksec = sectors;
	*head = heads;
}

static void getchsx (struct hardfiledata *hfd, int *cyl, int *cylsec, int *head, int *tracksec)
{
	getchs2 (hfd, cyl, cylsec, head, tracksec);
	hf_log (_T("CHS: %08X-%08X %d %d %d %d %d\n"),
		(uae_u32)(hfd->virtsize >> 32),(uae_u32)hfd->virtsize,
		*cyl, *cylsec, *head, *tracksec);
}

static void getchsgeometry2 (uae_u64 size, int *pcyl, int *phead, int *psectorspertrack, int mode)
{
	int sptt[4];
	int i, spt, head, cyl;
	uae_u64 total = (unsigned int)(size / 512);

	if (mode == 1) {
		// old-style head=1, spt=32 always mode
		head = 1;
		spt = 32;
		cyl = (int)(total / (head * spt));

	} else {

		sptt[0] = 63;
		sptt[1] = 127;
		sptt[2] = 255;
		sptt[3] = -1;

		for (i = 0; sptt[i] >= 0; i++) {
			int maxhead = sptt[i] < 255 ? 16 : 255;
			spt = sptt[i];
			for (head = 4; head <= maxhead; head++) {
				cyl = (int)(total / (head * spt));
				if (size <= 512 * 1024 * 1024) {
					if (cyl <= 1023)
						break;
				} else {
					if (cyl < 16383)
						break;
					if (cyl < 32767 && head >= 5)
						break;
					if (cyl <= 65535)
						break;
				}
				if (maxhead > 16) {
					head *= 2;
					head--;
				}
			}
			if (head <= 16)
				break;
		}

	}
	if (head > 16)
		head--;

	*pcyl = cyl;
	*phead = head;
	*psectorspertrack = spt;
}

void getchsgeometry (uae_u64 size, int *pcyl, int *phead, int *psectorspertrack)
{
	getchsgeometry2 (size, pcyl, phead, psectorspertrack, 0);
}

void getchsgeometry_hdf (struct hardfiledata *hfd, uae_u64 size, int *pcyl, int *phead, int *psectorspertrack)
{
	uae_u8 block[512];
	int i;
	uae_u64 minsize = 512 * 1024 * 1024;

	if (size <= minsize) {
		*phead = 1;
		*psectorspertrack = 32;
	}
	memset (block, 0, sizeof block);
	if (hfd) {
		hdf_read(hfd, block, 0, 512);
		if (block[0] == 'D' && block[1] == 'O' && block[2] == 'S') {
			int mode;
			for (mode = 0; mode < 2; mode++) {
				uae_u32 rootblock;
				uae_u32 chk = 0;
				getchsgeometry2 (size, pcyl, phead, psectorspertrack, mode);
				rootblock = (2 + ((*pcyl) * (*phead) * (*psectorspertrack) - 1)) / 2;
				memset (block, 0, sizeof block);
				hdf_read(hfd, block, (uae_u64)rootblock * 512, 512);
				for (i = 0; i < 512; i += 4)
					chk += (block[i] << 24) | (block[i + 1] << 16) | (block[i + 2] << 8) | (block[i + 3] << 0);
				if (!chk && block[0] == 0 && block[1] == 0 && block[2] == 0 && block[3] == 2 &&
					block[4] == 0 && block[5] == 0 && block[6] == 0 && block[7] == 0 && 
					block[8] == 0 && block[9] == 0 && block[10] == 0 && block[11] == 0 && 
					block[508] == 0 && block[509] == 0 && block[510] == 0 && block[511] == 1) {
						return;
				}
			}
		}
	}
	getchsgeometry2 (size, pcyl, phead, psectorspertrack, size <= minsize ? 1 : 2);
}

// partition hdf default
void gethdfgeometry(uae_u64 size, struct uaedev_config_info *ci)
{
	int head = 1;
	int sectorspertrack = 32;
	if (size >= 1048576000) { // >=1000M
		head = 16;
		while ((size / 512) / ((uae_u64)head * sectorspertrack) >= 32768 && sectorspertrack < 32768) {
			sectorspertrack *= 2;
		}
	}
	ci->surfaces = head;
	ci->sectors = sectorspertrack;
	ci->reserved = 2;
	ci->blocksize = 512;
}

void getchspgeometry (uae_u64 total, int *pcyl, int *phead, int *psectorspertrack, bool idegeometry)
{
	uae_u64 blocks = total / 512;

	if (idegeometry) {
		*phead = 16;
		*psectorspertrack = 63;
		*pcyl = (int)(blocks / ((*psectorspertrack) * (*phead)));
		if (blocks > 16515072) {
			/* >8G, CHS=16383/16/63 */
			*pcyl = 16383;
			*phead = 16;
			*psectorspertrack = 63;
			return;
		}
		return;
	}
	getchsgeometry (total, pcyl, phead, psectorspertrack);
}

static void getchshd (struct hardfiledata *hfd, int *pcyl, int *phead, int *psectorspertrack)
{
	getchspgeometry (hfd->virtsize, pcyl, phead, psectorspertrack, false);
}


static void pl (uae_u8 *p, int off, uae_u32 v)
{
	p += off * 4;
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v >> 0;
}

static void rdb_crc (uae_u8 *p)
{
	uae_u32 sum;
	int i, blocksize;

	sum =0;
	blocksize = rl (p + 1 * 4);
	for (i = 0; i < blocksize; i++)
		sum += rl (p + i * 4);
	sum = 0 - sum;
	pl (p, 2, sum);
}

static uae_u32 get_filesys_version(uae_u8 *fs, int size)
{
	int ver = -1, rev = -1;
	for (int i = 0; i < size - 6; i++) {
		uae_u8 *p = fs + i;
		if (p[0] == 'V' && p[1] == 'E' && p[2] == 'R' && p[3] == ':' && p[4] == ' ') {
			uae_u8 *p2;
			p += 5;
			p2 = p;
			while (*p2 && p2 - fs < size)
				p2++;
			if (p2[0] == 0) {
				while (*p && (ver < 0 || rev < 0)) {
					if (*p == ' ') {
						p++;
						ver = atol((char*)p);
						if (ver < 0)
							ver = 0;
						while (*p) {
							if (*p == ' ')
								break;
							if (*p == '.') {
								p++;
								rev = atol((char*)p);
								if (rev < 0)
									rev = 0;
							}
							else {
								p++;
							}
						}
						break;
					}
					else {
						p++;
					}
				}
			}
			break;
		}
	}
	if (ver < 0)
		return 0xffffffff;
	if (rev < 0)
		rev = 0;
	return (ver << 16) | rev;
}

// hardware block size is always 256 or 512
// filesystem block size can be 256, 512 or larger
static void create_virtual_rdb (struct hardfiledata *hfd)
{
	uae_u8 *rdb, *part, *denv, *fs;
	int fsblocksize = hfd->ci.blocksize;
	int hardblocksize = fsblocksize >= 512 ? 512 : 256;
	int cyl = hfd->ci.surfaces * hfd->ci.sectors;
	int cyls = (262144 + (cyl * fsblocksize) - 1) / (cyl * fsblocksize);
	int size = cyl * cyls * fsblocksize;
	int idx = 0;
	uae_u8 *filesys = NULL;
	int filesyslen = 0;
	uae_u32 fsver = 0;

	write_log(_T("Creating virtual RDB (RDB size=%d, %d blocks). H=%d S=%d HBS=%d FSBS=%d)\n"),
		size, size / hardblocksize, hfd->ci.surfaces, hfd->ci.sectors, hardblocksize, fsblocksize);

	if (hfd->ci.filesys[0]) {
		struct zfile *f = NULL;
		TCHAR fspath[MAX_DPATH];
		cfgfile_resolve_path_out_load(hfd->ci.filesys, fspath, MAX_DPATH, PATH_HDF);
		filesys = zfile_load_file(fspath, &filesyslen);
		if (filesys) {
			fsver = get_filesys_version(filesys, filesyslen);
			if (fsver == 0xffffffff)
				fsver = (99 << 16) | 99;
			if (filesyslen & 3) {
				xfree(filesys);
				filesys = NULL;
				filesyslen = 0;
			}
		}
	}

	int filesysblocks = (filesyslen + hardblocksize - 5 * 4 - 1) / (hardblocksize - 5 * 4);

	rdb = xcalloc (uae_u8, size);
	hfd->virtual_rdb = rdb;
	hfd->virtual_size = size;

	pl(rdb, 0, 0x5244534b); // "RDSK"
	pl(rdb, 1, 256 / 4);
	pl(rdb, 2, 0); // chksum
	pl(rdb, 3, 7); // hostid
	pl(rdb, 4, hardblocksize); // blockbytes
	pl(rdb, 5, 0); // flags
	pl(rdb, 6, -1); // badblock
	pl(rdb, 7, idx + 1); // part
	pl(rdb, 8, filesys ? idx + 2 : -1); // fs
	pl(rdb, 9, -1); // driveinit
	pl(rdb, 10, -1); // reserved
	pl(rdb, 11, -1); // reserved
	pl(rdb, 12, -1); // reserved
	pl(rdb, 13, -1); // reserved
	pl(rdb, 14, -1); // reserved
	pl(rdb, 15, -1); // reserved
	pl(rdb, 16, hfd->ci.highcyl + cyls - 1);
	pl(rdb, 17, hfd->ci.sectors);
	pl(rdb, 18, hfd->ci.surfaces * fsblocksize / hardblocksize);
	pl(rdb, 19, hfd->ci.interleave);
	pl(rdb, 20, 0); // park
	pl(rdb, 21, -1); // res
	pl(rdb, 22, -1); // res
	pl(rdb, 23, -1); // res
	pl(rdb, 24, 0); // writeprecomp
	pl(rdb, 25, 0); // reducedwrite
	pl(rdb, 26, 0); // steprate
	pl(rdb, 27, -1); // res
	pl(rdb, 28, -1); // res
	pl(rdb, 29, -1); // res
	pl(rdb, 30, -1); // res
	pl(rdb, 31, -1); // res
	pl(rdb, 32, 0); // rdbblockslo
	pl(rdb, 33, cyl * cyls  * fsblocksize / hardblocksize - 1); // rdbblockshi
	pl(rdb, 34, cyls); // locyl
	pl(rdb, 35, hfd->ci.highcyl + cyls - 1); // hicyl
	pl(rdb, 36, cyl  * fsblocksize / hardblocksize); // cylblocks
	pl(rdb, 37, 0); // autopark
	pl(rdb, 38, (1 + 1 + (filesysblocks ? 2 + filesysblocks : 0) - 1)); // highrdskblock
	pl(rdb, 39, -1); // res
	ua_copy ((char*)rdb + 40 * 4, 8, hfd->vendor_id);
	ua_copy ((char*)rdb + 42 * 4, 16, hfd->product_id);
	ua_copy ((char*)rdb + 46 * 4, 4, _T("UAE"));
	rdb_crc (rdb);
	idx++;

	part = rdb + hardblocksize * idx;
	pl(part, 0, 0x50415254); // "PART"
	pl(part, 1, 256 / 4);
	pl(part, 2, 0); // chksum
	pl(part, 3, 7); // hostid
	pl(part, 4, -1); // next
	pl(part, 5, hfd->ci.bootpri < -128 ? 2 : hfd->ci.bootpri == -128 ? 0 : 1); // bootable/nomount
	pl(part, 6, -1);
	pl(part, 7, -1);
	pl(part, 8, 0); // devflags
	part[9 * 4] = (uae_u8)_tcslen(hfd->ci.devname);
	ua_copy ((char*)part + 9 * 4 + 1, 30, hfd->ci.devname);
	denv = part + 128;
	pl(denv, 0, 16);
	pl(denv, 1, fsblocksize / 4);
	pl(denv, 2, 0); // secorg
	pl(denv, 3, hfd->ci.surfaces);
	pl(denv, 4, 1);
	pl(denv, 5, hfd->ci.sectors);
	pl(denv, 6, hfd->ci.reserved);
	pl(denv, 7, 0); // prealloc
	pl(denv, 8, hfd->ci.interleave); // interleave
	pl(denv, 9, cyls); // lowcyl
	pl(denv, 10, hfd->ci.highcyl + cyls - 1);
	pl(denv, 11, hfd->ci.buffers);
	pl(denv, 12, hfd->ci.bufmemtype);
	pl(denv, 13, hfd->ci.maxtransfer);
	pl(denv, 14, hfd->ci.mask);
	pl(denv, 15, hfd->ci.bootpri);
	pl(denv, 16, hfd->ci.dostype);
	rdb_crc (part);
	idx++;

	if (filesys) {
		fs = rdb + hardblocksize * idx;
		pl(fs, 0, 0x46534844); // "FSHD"
		pl(fs, 1, 256 / 4);
		pl(fs, 2, 0); // chksum
		pl(fs, 3, 7); // hostid
		pl(fs, 4, -1); // next
		pl(fs, 5, 0); // flags
		pl(fs, 8, hfd->ci.dostype);
		pl(fs, 9, fsver); // version
		pl(fs, 10, 0x100 | 0x80 | 0x20 | 0x10); // patchflags: seglist + globvec + pri + stack
		pl(fs, 15, hfd->ci.stacksize); // stack
		pl(fs, 16, hfd->ci.priority); // priority
		pl(fs, 18, idx + 1); // first lseg
		pl(fs, 19, -1); // globvec
		rdb_crc(fs);
		idx++;

		int offset = 0;
		for (;;) {
			uae_u8 *lseg = rdb + hardblocksize * idx;
			int lsegdatasize = hardblocksize - 5 * 4;
			if (lseg + hardblocksize > rdb + size)
				break;
			pl(lseg, 0, 0x4c534547); // "LSEG"
			pl(lseg, 1, hardblocksize / 4);
			pl(lseg, 2, 0); // chksum
			pl(lseg, 3, 7); // hostid
			int v = filesyslen - offset;
			if (v <= lsegdatasize) {
				memcpy(lseg + 5 * 4, filesys + offset, v);
				pl(lseg, 4, -1);
				pl(lseg, 1, 5 + v / 4);
				rdb_crc(lseg);
				break;
			}
			memcpy(lseg + 5 * 4, filesys + offset, lsegdatasize);
			offset += lsegdatasize;
			idx++;
			pl(lseg, 4, idx); // next
			rdb_crc(lseg);
		}
		xfree(filesys);
	}

	hfd->virtsize += size;
}

void hdf_hd_close (struct hd_hardfiledata *hfd)
{
	if (!hfd)
		return;
	hdf_close (&hfd->hfd);
}

int hdf_hd_open (struct hd_hardfiledata *hfd)
{
	struct uaedev_config_info *ci = &hfd->hfd.ci;
	if (hdf_open (&hfd->hfd) <= 0)
		return 0;
	hfd->hfd.unitnum = ci->uae_unitnum;
	if (ci->physical_geometry) {
		hfd->cyls = ci->pcyls;
		hfd->heads = ci->pheads;
		hfd->secspertrack = ci->psecs;
	} else if (ci->highcyl && ci->surfaces && ci->sectors) {
		hfd->cyls = ci->highcyl;
		hfd->heads = ci->surfaces;
		hfd->secspertrack = ci->sectors;
	} else {
		getchshd (&hfd->hfd, &hfd->cyls, &hfd->heads, &hfd->secspertrack);
	}
	hfd->cyls_def = hfd->cyls;
	hfd->secspertrack_def = hfd->secspertrack;
	hfd->heads_def = hfd->heads;
	if (ci->surfaces && ci->sectors) {
		uae_u8 buf[512] = { 0 };
		hdf_read (&hfd->hfd, buf, 0, 512);
		if (buf[0] != 0 && memcmp (buf, _T("RDSK"), 4)) {
			ci->highcyl = (int)((hfd->hfd.virtsize / ci->blocksize) / (ci->sectors * ci->surfaces));
			ci->dostype = rl (buf);
			create_virtual_rdb (&hfd->hfd);
			while (ci->highcyl * ci->surfaces * ci->sectors > hfd->cyls_def * hfd->secspertrack_def * hfd->heads_def) {
				hfd->cyls_def++;
			}
		}
	}
	hfd->size = hfd->hfd.virtsize;
	return 1;
}

static uae_u32 vhd_checksum (uae_u8 *p, int offset)
{
	int i;
	uae_u32 sum;

	sum = 0;
	for (i = 0; i < 512; i++) {
		if (offset >= 0 && i >= offset && i < offset + 4)
			continue;
		sum += p[i];
	}
	return ~sum;
}

static int hdf_write2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
static int hdf_read2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);

static void hdf_init_cache(struct hardfiledata *hfd)
{
}
static void hdf_flush_cache(struct hardfiledata *hdf)
{
}

static int hdf_cache_read(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	return hdf_read2(hfd, buffer, offset, len);
}

static int hdf_cache_write(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	return hdf_write2(hfd, buffer, offset, len);
}

int hdf_open (struct hardfiledata *hfd, const TCHAR *pname)
{
	int ret;
	uae_u8 tmp[512], tmp2[512];
	uae_u32 v;
	TCHAR filepath[MAX_DPATH];

	if ((!pname || pname[0] == 0) && hfd->ci.rootdir[0] == 0)
		return 0;
	hfd->adide = 0;
	hfd->byteswap = 0;
	hfd->hfd_type = 0;
	hfd->virtual_size = 0;
	hfd->virtual_rdb = NULL;
	if (!pname)
		pname = hfd->ci.rootdir;
	cfgfile_resolve_path_out_load(pname, filepath, MAX_DPATH, PATH_HDF);
#ifdef WITH_CHD
	TCHAR nametmp[MAX_DPATH];
	_tcscpy (nametmp, filepath);
	TCHAR *ext = _tcsrchr (nametmp, '.');
	if (ext && !_tcsicmp (ext, _T(".chd"))) {
		bool chd_readonly = false;
		struct zfile *zf = NULL;
		if (!hfd->ci.readonly)
			zf = zfile_fopen (nametmp, _T("rb+"));
		if (!zf) {
			chd_readonly = true;
			zf = zfile_fopen (nametmp, _T("rb"));
		}
		if (zf) {
			std::error_condition err;
			hard_disk_file *chdf;
			chd_file *cf = new chd_file();
			if (!chd_readonly)
				err = cf->open(zf->name, true, NULL);
			if (err == chd_file::error::FILE_NOT_WRITEABLE) {
				chd_readonly = true;
				err = cf->open(zf->name, false, NULL);
			}
			if (err != std::error_condition()) {
				zfile_fclose (zf);
				delete cf;
				goto end;
			}
			chdf = hard_disk_open(cf);
			if (!chdf) {
				hfd->ci.readonly = true;
				hfd->hfd_type = HFD_CHD_OTHER;
				hfd->chd_handle = cf;
			} else {
				hfd->hfd_type = HFD_CHD_HD;
				hfd->chd_handle = chdf;
			}
			if (chd_readonly)
				hfd->ci.readonly = true;
			hfd->virtsize = cf->logical_bytes();
			hfd->handle_valid = -1;
			write_log(_T("CHD '%s' mounted as %s, %s.\n"), filepath, chdf ? _T("HD") : _T("OTHER"), hfd->ci.readonly ? _T("read only") : _T("read/write"));
			return 1;
		}
	}
#endif
	ret = hdf_open_target (hfd, filepath);
	if (ret <= 0)
		return ret;
	if (hdf_read_target (hfd, tmp, 0, 512) != 512)
		goto nonvhd;
	v = gl (tmp + 8); // features
	if ((v & 3) != 2)
		goto nonvhd;
	v = gl (tmp + 8 + 4); // version
	if ((v >> 16) != 1)
		goto nonvhd;
	hfd->hfd_type = gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4);
	if (hfd->hfd_type != HFD_VHD_FIXED && hfd->hfd_type != HFD_VHD_DYNAMIC)
		goto nonvhd;
	v = gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4);
	if (v == 0)
		goto nonvhd;
	if (vhd_checksum (tmp, 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4) != v)
		goto nonvhd;
	if (hdf_read_target (hfd, tmp2, hfd->physsize - sizeof tmp2, 512) != 512)
		goto end;
	if (memcmp (tmp, tmp2, sizeof tmp))
		goto nonvhd;
	hfd->vhd_footerblock = hfd->physsize - 512;
	hfd->virtsize = (uae_u64)(gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 +4 + 4 + 8)) << 32;
	hfd->virtsize |= gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 +4 + 4 + 8 + 4);
	if (hfd->hfd_type == HFD_VHD_DYNAMIC) {
		uae_u32 size;
		hfd->vhd_bamoffset = gl (tmp + 8 + 4 + 4 + 4);
		if (hfd->vhd_bamoffset == 0 || hfd->vhd_bamoffset >= hfd->physsize)
			goto end;
		if (hdf_read_target (hfd, tmp, hfd->vhd_bamoffset, 512) != 512)
			goto end;
		v = gl (tmp + 8 + 8 + 8 + 4 + 4 + 4);
		if (vhd_checksum (tmp, 8 + 8 + 8 + 4 + 4 + 4) != v)
			goto end;
		v = gl (tmp + 8 + 8 + 8);
		if ((v >> 16) != 1)
			goto end;
		hfd->vhd_blocksize = gl (tmp + 8 + 8 + 8 + 4 + 4);
		hfd->vhd_bamoffset = gl (tmp + 8 + 8 + 4);
		hfd->vhd_bamsize = (((hfd->virtsize + hfd->vhd_blocksize - 1) / hfd->vhd_blocksize) * 4 + 511) & ~511;
		size = hfd->vhd_bamoffset + hfd->vhd_bamsize;
		hfd->vhd_header = xmalloc (uae_u8, size);
		if (hdf_read_target (hfd, hfd->vhd_header, 0, size) != size)
			goto end;
		hfd->vhd_sectormap = xmalloc (uae_u8, 512);
		hfd->vhd_sectormapblock = -1;
		hfd->vhd_bitmapsize = ((hfd->vhd_blocksize / (8 * 512)) + 511) & ~511;
	}
	write_log (_T("HDF is VHD %s image, virtual size=%lldK (%llx %lld)\n"),
		hfd->hfd_type == HFD_VHD_FIXED ? _T("fixed") : _T("dynamic"),
		hfd->virtsize / 1024, hfd->virtsize, hfd->virtsize);
	hdf_init_cache (hfd);
	return 1;
nonvhd:
	hfd->hfd_type = 0;
	return 1;
end:
	hdf_close_target (hfd);
	return 0;
}
int hdf_open (struct hardfiledata *hfd)
{
	int v = hdf_open (hfd, NULL);
	if (!v)
		return v;
	get_hd_geometry(&hfd->ci);
	hfd->geometry = ini_load(hfd->ci.geometry, true);
	return v;
}

void hdf_close (struct hardfiledata *hfd)
{
	hdf_flush_cache (hfd);
	hdf_close_target (hfd);
#ifdef WITH_CHD
	if (hfd->hfd_type == HFD_CHD_OTHER) {
		chd_file *cf = (chd_file*)hfd->chd_handle;
		cf->close();
		delete cf;
	} else if (hfd->hfd_type == HFD_CHD_HD) {
		hard_disk_file *chdf = (hard_disk_file*)hfd->chd_handle;
		chd_file *cf = hard_disk_get_chd(chdf);
		hard_disk_close(chdf);
		cf->close();
		delete cf;
	}
	xfree(hfd->virtual_rdb);
	hfd->virtual_rdb = 0;
	hfd->virtual_size = 0;
	ini_free(hfd->geometry);
	hfd->geometry = NULL;
	hfd->chd_handle = NULL;
#endif
	hfd->hfd_type = 0;
	xfree (hfd->vhd_header);
	hfd->vhd_header = NULL;
	xfree (hfd->vhd_sectormap);
	hfd->vhd_sectormap = NULL;
}

int hdf_dup (struct hardfiledata *dhfd, const struct hardfiledata *shfd)
{
	return hdf_dup_target (dhfd, shfd);
}

static uae_u64 vhd_read (struct hardfiledata *hfd, void *v, uae_u64 offset, uae_u64 len)
{
	uae_u64 read;
	uae_u8 *dataptr = (uae_u8*)v;

	//write_log (_T("%08x %08x\n"), (uae_u32)offset, (uae_u32)len);
	read = 0;
	if (offset & 511)
		return read;
	if (len & 511)
		return read;
	while (len > 0) {
		uae_u32 bamoffset = (uae_u32)((offset / hfd->vhd_blocksize) * 4 + hfd->vhd_bamoffset);
		uae_u32 sectoroffset = gl (hfd->vhd_header + bamoffset);
		if (sectoroffset == 0xffffffff) {
			memset (dataptr, 0, 512);
			read += 512;
		} else {
			int bitmapoffsetbits;
			int bitmapoffsetbytes;
			uae_u64 sectormapblock;

			bitmapoffsetbits = (offset / 512) % (hfd->vhd_blocksize / 512);
			bitmapoffsetbytes = bitmapoffsetbits / 8;
			sectormapblock = sectoroffset * (uae_u64)512 + (bitmapoffsetbytes & ~511);
			if (hfd->vhd_sectormapblock != sectormapblock) {
				// read sector bitmap
				//write_log (_T("BM %08x\n"), sectormapblock);
				if (hdf_read_target (hfd, hfd->vhd_sectormap, sectormapblock, 512) != 512) {
					write_log (_T("vhd_read: bitmap read error\n"));
					return read;
				}
				hfd->vhd_sectormapblock = sectormapblock;
			}
			// block allocated in bitmap?
			if (hfd->vhd_sectormap[bitmapoffsetbytes & 511] & (1 << (7 - (bitmapoffsetbits & 7)))) {
				// read data block
				uae_u64 block = sectoroffset * (uae_u64)512 + hfd->vhd_bitmapsize + bitmapoffsetbits * 512;
				//write_log (_T("DB %08x\n"), block);
				if (hdf_read_target (hfd, dataptr, block, 512) != 512) {
					write_log (_T("vhd_read: data read error\n"));
					return read;
				}
			} else {
				memset (dataptr, 0, 512);
			}
			read += 512;
		}
		len -= 512;
		dataptr += 512;
		offset += 512;
	}
	return read;
}

static int vhd_write_enlarge (struct hardfiledata *hfd, uae_u32 bamoffset)
{
	uae_u8 *buf, *p;
	int len;
	uae_u32 block;
	int v;

	len = hfd->vhd_blocksize + hfd->vhd_bitmapsize + 512;
	buf = xcalloc (uae_u8, len);
	if (!hdf_resize_target (hfd, hfd->physsize + len - 512)) {
		write_log (_T("vhd_enlarge: failure\n"));
		xfree(buf);
		return 0;
	}
	// add footer (same as 512 byte header)
	memcpy (buf + len - 512, hfd->vhd_header, 512);
	v = hdf_write_target (hfd, buf, hfd->vhd_footerblock, len);
	xfree (buf);
	if (v != len) {
		write_log (_T("vhd_enlarge: footer write error\n"));
		return 0;
	}
	// write new offset to BAM
	p = hfd->vhd_header + bamoffset;
	block = (uae_u32)(hfd->vhd_footerblock / 512);
	p[0] = block >> 24;
	p[1] = block >> 16;
	p[2] = block >>  8;
	p[3] = block >>  0;
	// write to disk
	if (hdf_write_target (hfd, hfd->vhd_header + hfd->vhd_bamoffset, hfd->vhd_bamoffset, hfd->vhd_bamsize) != hfd->vhd_bamsize) {
		write_log (_T("vhd_enlarge: bam write error\n"));
		return 0;
	}
	hfd->vhd_footerblock += len - 512;
	return 1;
}

static uae_u64 vhd_write (struct hardfiledata *hfd, void *v, uae_u64 offset, uae_u64 len)
{
	uae_u64 written;
	uae_u8 *dataptr = (uae_u8*)v;

	//write_log (_T("%08x %08x\n"), (uae_u32)offset, (uae_u32)len);
	written = 0;
	if (offset & 511)
		return written;
	if (len & 511)
		return written;
	while (len > 0) {
		uae_u32 bamoffset = (uae_u32)((offset / hfd->vhd_blocksize) * 4 + hfd->vhd_bamoffset);
		uae_u32 sectoroffset = gl (hfd->vhd_header + bamoffset);
		if (sectoroffset == 0xffffffff) {
			if (!vhd_write_enlarge (hfd, bamoffset))
				return written;
			continue;
		} else {
			int bitmapoffsetbits;
			int bitmapoffsetbytes;

			bitmapoffsetbits = (offset / 512) % (hfd->vhd_blocksize / 512);
			bitmapoffsetbytes = bitmapoffsetbits / 8;
			uae_u64 sectormapblock = sectoroffset * (uae_u64)512 + (bitmapoffsetbytes & ~511);
			if (hfd->vhd_sectormapblock != sectormapblock) {
				// read sector bitmap
				if (hdf_read_target (hfd, hfd->vhd_sectormap, sectormapblock, 512) != 512) {
					write_log (_T("vhd_write: bitmap read error\n"));
					return written;
				}
				hfd->vhd_sectormapblock = sectormapblock;
			}
			// write data
			if (hdf_write_target (hfd, dataptr, sectoroffset * (uae_u64)512 + hfd->vhd_bitmapsize + bitmapoffsetbits * 512, 512) != 512) {
				write_log (_T("vhd_write: data write error\n"));
				return written;
			}
			// block already allocated in bitmap?
			if (!(hfd->vhd_sectormap[bitmapoffsetbytes & 511] & (1 << (7 - (bitmapoffsetbits & 7))))) {
				// no, we need to mark it allocated and write the modified bitmap back to the disk
				hfd->vhd_sectormap[bitmapoffsetbytes & 511] |= (1 << (7 - (bitmapoffsetbits & 7)));
				if (hdf_write_target (hfd, hfd->vhd_sectormap, sectormapblock, 512) != 512) {
					write_log (_T("vhd_write: bam write error\n"));
					return written;
				}
			}
			written += 512;
		}
		len -= 512;
		dataptr += 512;
		offset += 512;
	}
	return written;
}


int vhd_create (const TCHAR *name, uae_u64 size, uae_u32 dostype)
{
	struct hardfiledata hfd;
	struct zfile *zf;
	uae_u8 *b;
	int cyl, cylsec, head, tracksec;
	uae_u32 crc, blocksize, batsize, batentrysize;
	int ret, i;
	time_t tm;

	if (size >= (uae_u64)10 * 1024 * 1024 * 1024)
		blocksize = 2 * 1024 * 1024;
	else
		blocksize = 512 * 1024;
	batsize = (uae_u32)((size + blocksize - 1) / blocksize);
	batentrysize = batsize;
	batsize *= 4;
	batsize += 511;
	batsize &= ~511;
	ret = 0;
	b = NULL;
	zf = zfile_fopen (name, _T("wb"), 0);
	if (!zf)
		goto end;
	b = xcalloc (uae_u8, 512 + 1024 + batsize + 512);
	if (zfile_fwrite (b, 512 + 1024 + batsize + 512, 1, zf) != 1)
		goto end;

	memset (&hfd, 0, sizeof hfd);
	hfd.virtsize = hfd.physsize = size;
	hfd.ci.blocksize = 512;
	strcpy ((char*)b, "conectix"); // cookie
	b[0x0b] = 2; // features
	b[0x0d] = 1; // version
	b[0x10 + 6] = 2; // data offset
	// time stamp
	tm = time (NULL) - 946684800;
	b[0x18] = (uae_u8)(tm >> 24);
	b[0x19] = (uae_u8)(tm >> 16);
	b[0x1a] = (uae_u8)(tm >>  8);
	b[0x1b] = (uae_u8)(tm >>  0);
	strcpy ((char*)b + 0x1c, "vpc "); // creator application
	b[0x21] = 5; // creator version
	strcpy ((char*)b + 0x24, "Wi2k"); // creator host os
	// original and current size
	b[0x28] = b[0x30] = (uae_u8)(size >> 56);
	b[0x29] = b[0x31] = (uae_u8)(size >> 48);
	b[0x2a] = b[0x32] = (uae_u8)(size >> 40);
	b[0x2b] = b[0x33] = (uae_u8)(size >> 32);
	b[0x2c] = b[0x34] = (uae_u8)(size >> 24);
	b[0x2d] = b[0x35] = (uae_u8)(size >> 16);
	b[0x2e] = b[0x36] = (uae_u8)(size >>  8);
	b[0x2f] = b[0x37] = (uae_u8)(size >>  0);
	getchs2 (&hfd, &cyl, &cylsec, &head, &tracksec);
	// cylinders
	b[0x38] = cyl >> 8;
	b[0x39] = cyl;
	// heads
	b[0x3a] = head;
	// sectors per track
	b[0x3b] = tracksec;
	// disk type
	b[0x3c + 3] = HFD_VHD_DYNAMIC;
	get_guid_target (b + 0x44);
	crc = vhd_checksum (b, -1);
	b[0x40] = crc >> 24;
	b[0x41] = crc >> 16;
	b[0x42] = crc >>  8;
	b[0x43] = crc >>  0;

	// write header
	zfile_fseek (zf, 0, SEEK_SET);
	zfile_fwrite (b, 512, 1, zf);
	// write footer
	zfile_fseek (zf, 512 + 1024 + batsize, SEEK_SET);
	zfile_fwrite (b, 512, 1, zf);

	// dynamic disk header
	memset (b, 0, 1024);
	// cookie
	strcpy ((char*)b, "cxsparse");
	// data offset
	for (i = 0; i < 8; i++)
		b[0x08 + i] = 0xff;
	// table offset (bat)
	b[0x10 + 6] = 0x06;
	// version
	b[0x19] = 1;
	// max table entries
	b[0x1c] = batentrysize >> 24;
	b[0x1d] = batentrysize >> 16;
	b[0x1e] = batentrysize >>  8;
	b[0x1f] = batentrysize >>  0;
	b[0x20] = blocksize >> 24;
	b[0x21] = blocksize >> 16;
	b[0x22] = blocksize >>  8;
	b[0x23] = blocksize >>  0;
	crc = vhd_checksum (b, -1);
	b[0x24] = crc >> 24;
	b[0x25] = crc >> 16;
	b[0x26] = crc >>  8;
	b[0x27] = crc >>  0;

	// write dynamic header
	zfile_fseek (zf, 512, SEEK_SET);
	zfile_fwrite (b, 1024, 1, zf);

	// bat
	memset (b, 0, batsize);
	memset (b, 0xff, batentrysize * 4);
	zfile_fwrite (b, batsize, 1, zf);

	zfile_fclose (zf);
	zf = NULL;

	if (dostype) {
		uae_u8 bootblock[512] = { 0 };
		bootblock[0] = dostype >> 24;
		bootblock[1] = dostype >> 16;
		bootblock[2] = dostype >>  8;
		bootblock[3] = dostype >>  0;
		if (hdf_open (&hfd, name) > 0) {
			vhd_write (&hfd, bootblock, 0, 512);
			hdf_close (&hfd);
		}
	}

	ret = 1;

end:
	xfree (b);
	zfile_fclose (zf);
	return ret;
}

static int hdf_read2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int ret = 0, extra = 0;

	if (offset < hfd->virtual_size) {
		uae_s64 len2 = offset + len <= hfd->virtual_size ? len : hfd->virtual_size - offset;
		if (len > INT_MAX) {
			return 0;
		}
		if (!hfd->virtual_rdb)
			return 0;
		memcpy(buffer, hfd->virtual_rdb + offset, (size_t)len2);
		len -= (int)len2;
		if (len <= 0)
			return (int)len2;
		offset += len2;
		buffer = (uae_u8*)buffer + len2;
		extra = (int)len2;
	}
	offset -= hfd->virtual_size;

	if (hfd->hfd_type == HFD_VHD_DYNAMIC)
		ret = (int)vhd_read (hfd, buffer, offset, len);
	else if (hfd->hfd_type == HFD_VHD_FIXED)
		ret = hdf_read_target (hfd, buffer, offset + 512, len);
#ifdef WITH_CHD
	else if (hfd->hfd_type == HFD_CHD_OTHER) {
		chd_file *cf = (chd_file*)hfd->chd_handle;
		if (cf->read_bytes(offset, buffer, len) == std::error_condition())
			ret = len;
		else
			return 0;
	} else if (hfd->hfd_type == HFD_CHD_HD) {
		hard_disk_file *chdf = (hard_disk_file*)hfd->chd_handle;
		hard_disk_info *chdi = hard_disk_get_info(chdf);
		chd_file *cf = hard_disk_get_chd(chdf);
		uae_u8 *buf = (uae_u8*)buffer;
		int got = 0;
		offset /= chdi->sectorbytes;
		while (len > 0) {
			if (cf->read_units(offset, buf) != std::error_condition())
				break;
			got += chdi->sectorbytes;
			buf += chdi->sectorbytes;
			len -= chdi->sectorbytes;
			offset++;
		}
		ret = got;
	}
#endif
	else
		ret = hdf_read_target (hfd, buffer, offset, len);

	if (ret <= 0)
		return ret;
	ret += extra;
	return ret;
}

static int hdf_write2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	if (len > INT_MAX)
		return 0;
	int ret = 0, extra = 0;
	// writes to virtual RDB are ignored
	if (offset < hfd->virtual_size) {
		uae_s64 len2 = offset + len <= hfd->virtual_size ? len : hfd->virtual_size - offset;
		if (len2 > INT_MAX) {
			return 0;
		}
		len -= (int)len2;
		if (len <= 0)
			return (int)len2;
		offset += len2;
		buffer = (uae_u8*)buffer + len2;
		extra = (int)len2;
	}
	offset -= hfd->virtual_size;

	if (hfd->hfd_type == HFD_VHD_DYNAMIC)
		ret = (int)vhd_write(hfd, buffer, offset, len);
	else if (hfd->hfd_type == HFD_VHD_FIXED)
		ret = hdf_write_target(hfd, buffer, offset + 512, len);
#ifdef WITH_CHD
	else if (hfd->hfd_type == HFD_CHD_OTHER)
		return 0;
	else if (hfd->hfd_type == HFD_CHD_HD) {
		if (hfd->ci.readonly)
			return 0;
		hard_disk_file *chdf = (hard_disk_file*)hfd->chd_handle;
		hard_disk_info *chdi = hard_disk_get_info(chdf);
		chd_file *cf = hard_disk_get_chd(chdf);
		uae_u8 *buf = (uae_u8*)buffer;
		int got = 0;
		offset /= chdi->sectorbytes;
		while (len > 0) {
			if (cf->write_units(offset, buf) != std::error_condition())
				break;
			got += chdi->sectorbytes;
			buf += chdi->sectorbytes;
			len -= chdi->sectorbytes;
			offset++;
		}
		ret = got;
	}
#endif
	else
		ret = hdf_write_target(hfd, buffer, offset, len);

	if (ret <= 0)
		return ret;
	ret += extra;
	return ret;
}

static void adide_decode (void *v, int len)
{
	int i;
	uae_u8 *buffer = (uae_u8*)v;
	for (i = 0; i < len; i += 2) {
		uae_u8 *b =  buffer + i;
		uae_u16 w = (b[0] << 8) | (b[1] << 0);
		uae_u16 o = adide_decode_word(w);
		b[0] = o >> 8;
		b[1] = o >> 0;
	}
}
static void adide_encode (void *v, int len)
{
	int i;
	uae_u8 *buffer = (uae_u8*)v;
	for (i = 0; i < len; i += 2) {
		uae_u8 *b =  buffer + i;
		uae_u16 w = (b[0] << 8) | (b[1] << 0);
		uae_u16 o = adide_encode_word(w);
		b[0] = o >> 8;
		b[1] = o >> 0;
	}
}

static void hdf_byteswap (void *v, int len)
{
	int i;
	uae_u8 *b = (uae_u8*)v;

	for (i = 0; i < len; i += 2) {
		uae_u8 tmp = b[i];
		b[i] = b[i + 1];
		b[i + 1] = tmp;
	}
}

int hdf_read_rdb (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int v;
	v = hdf_read (hfd, buffer, offset, len);
	if (v > 0 && offset < 16 * 512 && !hfd->byteswap && !hfd->adide)  {
		uae_u8 *buf = (uae_u8*)buffer;
		bool changed = false;
		if (buf[0] == 0x39 && buf[1] == 0x10 && buf[2] == 0xd3 && buf[3] == 0x12) { // AdIDE encoded "CPRM"
			hfd->adide = 1;
			changed = true;
			write_log (_T("HDF: adide scrambling detected\n"));
		} else if (!memcmp (buf, "DRKS", 4)) {
			hfd->byteswap = 1;
			changed = true;
			write_log (_T("HDF: byteswapped RDB detected\n"));
		}
		if (changed)
			v = hdf_read (hfd, buffer, offset, len);
	}
	return v;
}

int hdf_read(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int v;

	hf_log3(_T("cmd_read: %p %04x-%08x (%d) %08x (%d)\n"),
		buffer, (uae_u32)(offset >> 32), (uae_u32)offset, (uae_u32)(offset / hfd->ci.blocksize), (uae_u32)len, (uae_u32)(len / hfd->ci.blocksize));

	if (!hfd->adide) {
		v = hdf_cache_read(hfd, buffer, offset, len);
	} else {
		offset += 512;
		v = hdf_cache_read(hfd, buffer, offset, len);
		adide_decode(buffer, len);
	}
	if (hfd->byteswap)
		hdf_byteswap(buffer, len);
	return v;
}

int hdf_write(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int v;

	hf_log3(_T("cmd_write: %p %04x-%08x (%d) %08x (%d)\n"),
		buffer, (uae_u32)(offset >> 32), (uae_u32)offset, (uae_u32)(offset / hfd->ci.blocksize), (uae_u32)len, (uae_u32)(len / hfd->ci.blocksize));

	if (hfd->byteswap)
		hdf_byteswap(buffer, len);
	if (!hfd->adide) {
		v = hdf_cache_write(hfd, buffer, offset, len);
	} else {
		offset += 512;
		adide_encode(buffer, len);
		v = hdf_cache_write(hfd, buffer, offset, len);
		adide_decode(buffer, len);
	}
	if (hfd->byteswap)
		hdf_byteswap(buffer, len);
	return v;
}

static uae_u64 cmd_readx(struct hardfiledata *hfd, uae_u8 *dataptr, uae_u64 offset, uae_u64 len)
{
	if (!len || len > INT_MAX)
		return 0;
	m68k_cancel_idle();
	gui_flicker_led(LED_HD, hfd->unitnum, 1);
	return hdf_read(hfd, dataptr, offset, (int)len);
}
static uae_u64 cmd_read(TrapContext *ctx, struct hardfiledata *hfd, uaecptr dataptr, uae_u64 offset, uae_u64 len)
{
	if (!len || len > INT_MAX)
		return 0;
	if (!ctx && real_address_allowed()) {
		addrbank *bank_data = &get_mem_bank (dataptr);
		if (!bank_data)
			return 0;
		if (bank_data->check(dataptr, (uae_u32)len)) {
			uae_u8 *buffer = bank_data->xlateaddr(dataptr);
			return cmd_readx(hfd, buffer, offset, (uae_u32)len);
		}
	}
	int total = 0;
	while (len > 0) {
		uae_u8 buf[RTAREA_TRAP_DATA_EXTRA_SIZE];
		int max = RTAREA_TRAP_DATA_EXTRA_SIZE & ~511;
		int size = (int)(len > max ? max : len);
		if (cmd_readx(hfd, buf, offset, size) != size)
			break;
		trap_put_bytes(ctx, buf, dataptr, size);
		offset += size;
		dataptr += size;
		len -= size;
		total += size;
	}
	return total;
}
static uae_u64 cmd_writex(struct hardfiledata *hfd, uae_u8 *dataptr, uae_u64 offset, uae_u64 len)
{
	if (!len || len > INT_MAX)
		return 0;
	m68k_cancel_idle();
	gui_flicker_led(LED_HD, hfd->unitnum, 2);
	return hdf_write(hfd, dataptr, offset, (uae_u32)len);
}

static uae_u64 cmd_write(TrapContext *ctx, struct hardfiledata *hfd, uaecptr dataptr, uae_u64 offset, uae_u64 len)
{
	if (!len || len > INT_MAX)
		return 0;
	if (!ctx && real_address_allowed()) {
		addrbank *bank_data = &get_mem_bank (dataptr);
		if (!bank_data)
			return 0;
		if (bank_data->check(dataptr, (uae_u32)len)) {
			uae_u8 *buffer = bank_data->xlateaddr(dataptr);
			return cmd_writex(hfd, buffer, offset, len);
		}
	}
	int total = 0;
	while (len > 0) {
		uae_u8 buf[RTAREA_TRAP_DATA_EXTRA_SIZE];
		int max = RTAREA_TRAP_DATA_EXTRA_SIZE & ~511;
		int size = (int)(len > max ? max : len);
		trap_get_bytes(ctx, buf, dataptr, size);
		if (cmd_writex(hfd, buf, offset, size) != size)
			break;
		offset += size;
		dataptr += size;
		len -= size;
		total += size;
	}
	return total;
}

static int checkbounds (struct hardfiledata *hfd, uae_u64 offset, uae_u64 len, int mode)
{
	uae_u64 max = hfd->virtsize;
	if (offset >= max || offset + len > max || (offset > 0xffffffff && (uae_s64)offset < 0)) {
		write_log (_T("UAEHF SCSI: out of bounds, %08X-%08X + %08X-%08X > %08X-%08X\n"),
			(uae_u32)(offset >> 32),(uae_u32)offset,(uae_u32)(len >> 32),(uae_u32)len,
			(uae_u32)(max >> 32),(uae_u32)max);
		return -1;
	}
	if (hfd->ci.max_lba) {
		max = hfd->ci.max_lba * hfd->ci.blocksize;
		if (offset >= max) {
			write_log (_T("UAEHF SCSI: forced last lba out of bounds, %08X-%08X + %08X-%08X > %08X-%08X\n"),
				(uae_u32)(offset >> 32),(uae_u32)offset,(uae_u32)(len >> 32),(uae_u32)len,
				(uae_u32)(max >> 32),(uae_u32)max);
			return -1;
		}
	}
	if ((mode == 1 || mode == 2) && hfd->ci.badblock_num) {
		offset /= hfd->ci.blocksize;
		len /= hfd->ci.blocksize;
		for (int i = 0; i < hfd->ci.badblock_num; i++) {
			struct uaedev_badblock *bb = &hfd->ci.badblocks[i];
			if (offset + len >= bb->first && offset < bb->last)
				return 1;
		}
	}
	return 0;
}

static bool is_writeprotected(struct hardfiledata *hfd)
{
	return hfd->ci.readonly || hfd->dangerous || currprefs.harddrive_read_only;
}

static int nodisk (struct hardfiledata *hfd)
{
	if (hfd->drive_empty)
		return 1;
	return 0;
}

static void setdrivestring(const TCHAR *s, uae_u8 *d, int start, int length)
{
	int i = 0;
	uae_char *ss = ua(s);
	while (i < length && ss[i]) {
		d[start + i] = ss[i];
		i++;
	}
	while (i > 0) {
		uae_char c = d[start + i - 1];
		if (c != '_')
			break;
		i--;
	}
	while (i < length) {
		d[start + i] = 32;
		i++;
	}
	xfree (ss);
}

static const uae_u8 sasi_commands[] =
{
	0x00, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x12,
	0xe0, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xff
};
static const uae_u8 sasi_commands2[] =
{
	0x12,
	0xff
};

static uae_u64 get_scsi_6_offset(struct hardfiledata *hfd, struct hd_hardfiledata *hdhfd, uae_u8 *cmdbuf, uae_u64 *lba)
{
	bool chs = hfd->ci.unit_feature_level == HD_LEVEL_SASI_CHS;
	uae_u64 offset;
	if (chs) {
		int cyl, cylsec, head, tracksec;
		if (hdhfd) {
			cyl = hdhfd->cyls;
			head = hdhfd->heads;
			tracksec = hdhfd->secspertrack;
			cylsec = 0;
		} else {
			getchsx(hfd, &cyl, &cylsec, &head, &tracksec);
		}
		int d_head = cmdbuf[1] & 31;
		int d_cyl = cmdbuf[3] | ((cmdbuf[2] >> 6) << 8) | ((cmdbuf[1] >> 7) << 10);
		int d_sec = cmdbuf[2] & 63;

		*lba = ((cmdbuf[1] & (0x1f | 0x80 | 0x40)) << 16) | (cmdbuf[2] << 8) || cmdbuf[3];
		if (d_cyl >= cyl || d_head >= head || d_sec >= tracksec)
			return ~0;
		offset = d_cyl * head * tracksec + d_head * tracksec + d_sec;
	} else {
		offset = ((cmdbuf[1] & 31) << 16) | (cmdbuf[2] << 8) | cmdbuf[3];
	}
	return offset;
}

int scsi_hd_emulate (struct hardfiledata *hfd, struct hd_hardfiledata *hdhfd, uae_u8 *cmdbuf, int scsi_cmd_len,
	uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len)
{
	TrapContext *ctx = NULL;
	if (cmdbuf == NULL)
		return 0;

	uae_u64 len, offset;
	int lr = 0, ls = 0;
	int chkerr;
	int scsi_len = -1;
	int status = 0;
	int lun;
	uae_u8 cmd;
	bool sasi = hfd->ci.unit_feature_level >= HD_LEVEL_SASI && hfd->ci.unit_feature_level <= HD_LEVEL_SASI_ENHANCED;
	bool sasie = hfd->ci.unit_feature_level == HD_LEVEL_SASI_ENHANCED;
	bool omti = hfd->ci.unit_feature_level == HD_LEVEL_SASI_CHS;
	uae_u8 sasi_sense = 0;
	uae_u64 current_lba = ~0;

	cmd = cmdbuf[0];

	if (log_scsiemu) {
		write_log (_T("SCSIEMU HD %d: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X CMDLEN=%d DATA=%p\n"), hfd->unitnum,
			cmdbuf[0], cmdbuf[1], cmdbuf[2], cmdbuf[3], cmdbuf[4], cmdbuf[5], cmdbuf[6], 
			cmdbuf[7], cmdbuf[8], cmdbuf[9], cmdbuf[10], cmdbuf[11],
			scsi_cmd_len, scsi_data);
	}

	/* REQUEST SENSE */
	if (cmd == 0x03) {
		return 0;
	}

	*reply_len = *sense_len = 0;
	lun = cmdbuf[1] >> 5;
	if (sasi || omti) {
		lun = lun & 1;
		if (lun)
			goto nodisk;
	}
	if (cmd != 0x03 && cmd != 0x12 && lun) {
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x25; /* INVALID LUN */
		ls = 0x12;
		write_log (_T("UAEHF: CMD=%02X LUN=%d ignored\n"), cmdbuf[0], lun);
		goto scsi_done;
	}

	if (sasi || omti) {
		int i;
		for (i = 0; sasi_commands[i] != 0xff; i++) {
			if (sasi_commands[i] == cmdbuf[0])
				break;
		}
		if (sasi_commands[i] == 0xff) {
			if (sasie) {
				for (i = 0; sasi_commands2[i] != 0xff; i++) {
					if (sasi_commands2[i] == cmdbuf[0])
						break;
				}
				if (sasi_commands2[i] == 0xff)
					goto errreq;
			} else {
				goto errreq;
			}
		}
		switch (cmdbuf[0])
		{
			case 0x05: /* READ VERIFY */
			if (nodisk(hfd))
				goto nodisk;
			offset = get_scsi_6_offset(hfd, hdhfd, cmdbuf, &current_lba);
			if (offset == ~0) {
				chkerr = 1;
				goto checkfail;
			}
			current_lba = offset;
			offset *= hfd->ci.blocksize;
			chkerr = checkbounds(hfd, offset, hfd->ci.blocksize, 1);
			if (chkerr) {
				current_lba = offset;
				goto checkfail;
			}
			scsi_len = 0;
			goto scsi_done;
			case 0x0c: /* INITIALIZE DRIVE CHARACTERISTICS */
			scsi_len = 8;
			write_log(_T("INITIALIZE DRIVE CHARACTERISTICS: "));
			write_log(_T("Heads: %d Cyls: %d Secs: %d\n"),
				(scsi_data[1] >> 4) | ((scsi_data[0] & 0xc0) << 4),
				((scsi_data[1] & 15) << 8) | (scsi_data[2]),
				scsi_data[5]);
			for (int i = 0; i < 8; i++) {
				write_log(_T("%02X "), scsi_data[i]);
			}
			write_log(_T("\n"));
			goto scsi_done;
			case 0x11: // ASSIGN ALTERNATE TRACK
			if (nodisk(hfd))
				goto nodisk;
			// do nothing, swallow data
			scsi_len = 4;
			goto scsi_done;
			case 0x12: /* INQUIRY */
			{
				int cyl, cylsec, head, tracksec;
				int alen = cmdbuf[4];
				if (nodisk(hfd))
					goto nodisk;
				if (hdhfd) {
					cyl = hdhfd->cyls;
					head = hdhfd->heads;
					tracksec = hdhfd->secspertrack;
					cylsec = 0;
				} else {
					getchsx(hfd, &cyl, &cylsec, &head, &tracksec);
				}
				r[0] = 0;
				r[1] = 11;
				r[9] = cyl >> 8;
				r[10] = cyl;
				r[11] = head;
				scsi_len = lr = alen > 12 ? 12 : alen;
				goto scsi_done;
			}
			break;
		}
	}

	switch (cmdbuf[0])
	{
	case 0x12: /* INQUIRY */
		{
			if ((cmdbuf[1] & 1) || cmdbuf[2] != 0)
				goto err;
			int alen = (cmdbuf[3] << 8) | cmdbuf[4];
			if (lun != 0) {
				r[0] = 0x7f;
			} else {
				r[0] = 0;
			}
			r[2] = 2; /* supports SCSI-2 */
			r[3] = 2; /* response data format */
			r[4] = 32; /* additional length */
			r[7] = 0;
			lr = alen < 36 ? alen : 36;
			if (hdhfd) {
				r[2] = hdhfd->ansi_version;
				r[3] = hdhfd->ansi_version >= 2 ? 2 : 0;
			}
			setdrivestring(hfd->vendor_id, r, 8, 8);
			setdrivestring(hfd->product_id, r, 16, 16);
			setdrivestring(hfd->product_rev, r, 32, 4);
			uae_u8 *rr;
			if (ini_getdata(hfd->geometry, _T("INQUIRY"), _T("00"), &rr, &lr)) {
				if (lr > alen)
					lr = alen;
				memcpy(r, rr, lr);
				xfree(rr);
			}
			if (lun == 0 && hfd->drive_empty) {
				r[0] |= 0x20; // not present
				r[1] |= 0x80; // removable..
			}
			scsi_len = lr;
		}
		goto scsi_done;
	case 0x1b: /* START/STOP UNIT */
		scsi_len = 0;
		hfd->unit_stopped = (cmdbuf[4] & 1) == 0;
		goto scsi_done;
	}

	if (hfd->unit_stopped) {
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 2; /* NOT READY */
		s[12] = 4; /* not ready */
		s[13] = 2; /* need initialise command */
		ls = 0x12;
		goto scsi_done;
	}

	switch (cmdbuf[0])
	{
	case 0x00: /* TEST UNIT READY */
		if (nodisk (hfd))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0x01: /* REZERO UNIT */
		if (nodisk (hfd))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0x04: /* FORMAT UNIT */
		// do nothing
		if (nodisk (hfd))
			goto nodisk;
		if (is_writeprotected(hfd))
			goto readprot;
		scsi_len = 0;
		break;
	case 0x05: /* VERIFY TRACK */
		// do nothing
		if (nodisk (hfd))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0x06: /* FORMAT TRACK */
	case 0x07: /* FORMAT BAD TRACK */
		if (nodisk (hfd))
			goto nodisk;
		if (is_writeprotected(hfd))
			goto readprot;
		// do nothing
		if (cmdbuf[5] & 0x40) {
			// data from sector buffer
		} else {
			// data is static 0x6c
		}
		scsi_len = 0;
		break;
	case 0x09: /* READ VERIFY */
		if (nodisk(hfd))
			goto nodisk;
		offset = get_scsi_6_offset(hfd, hdhfd, cmdbuf, &current_lba);
		if (offset == ~0) {
			chkerr = 1;
			goto checkfail;
		}
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, hfd->ci.blocksize, 1);
		if (chkerr) {
			current_lba = offset;
			goto checkfail;
		}
		scsi_len = 0;
		break;
	case 0x0b: /* SEEK (6) */
		if (nodisk (hfd))
			goto nodisk;
		offset = get_scsi_6_offset(hfd, hdhfd, cmdbuf, &current_lba);
		if (offset == ~0) {
			chkerr = 1;
			goto checkfail;
		}
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, hfd->ci.blocksize, 3);
		if (chkerr)
			goto checkfail;
		scsi_len = 0;
		break;
	case 0x08: /* READ (6) */
		if (nodisk (hfd))
			goto nodisk;
		offset = get_scsi_6_offset(hfd, hdhfd, cmdbuf, &current_lba);
		if (offset == ~0) {
			chkerr = 1;
			goto checkfail;
		}
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		len = cmdbuf[4];
		if (!len)
			len = 256;
		len *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, len, 1);
		if (chkerr)
			goto checkfail;
		scsi_len = (uae_u32)cmd_readx(hfd, scsi_data, offset, len);
		break;
	case 0x0e: /* READ SECTOR BUFFER */
		len = hfd->ci.blocksize;
		scsi_len = (int)len;
		memset(scsi_data, 0, (size_t)len);
		if (len > sizeof(hfd->sector_buffer))
			len = sizeof(hfd->sector_buffer);
		memcpy(scsi_data, hfd->sector_buffer, (size_t)len);
		break;
	case 0x0f: /* WRITE SECTOR BUFFER */
		len = hfd->ci.blocksize;
		scsi_len = (int)len;
		if (len > sizeof(hfd->sector_buffer))
			len = sizeof(hfd->sector_buffer);
		memcpy(hfd->sector_buffer, scsi_data, (size_t)len);
		break;
	case 0x0a: /* WRITE (6) */
		if (nodisk (hfd))
			goto nodisk;
		if (is_writeprotected(hfd))
			goto readprot;
		offset = get_scsi_6_offset(hfd, hdhfd, cmdbuf, &current_lba);
		if (offset == ~0) {
			chkerr = 1;
			goto checkfail;
		}
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		len = cmdbuf[4];
		if (!len)
			len = 256;
		len *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, len, 2);
		if (chkerr)
			goto checkfail;
		scsi_len = (uae_u32)cmd_writex(hfd, scsi_data, offset, len);
		break;
	case 0x55: // MODE SELECT(10)
	case 0x15: // MODE SELECT(6)
		{
			bool select10 = cmdbuf[0] == 0x55;
			bool sp = (cmdbuf[1] & 1) != 0;
			bool pf = (cmdbuf[1] & 0x10) != 0;
			int plen;
			uae_u8 bd[8];

			if (nodisk (hfd))
				goto nodisk;
			// we don't support save pages
			if (pf)
				goto errreq;

			// assume error first
			status = 2; /* CHECK CONDITION */
			s[0] = 0x70;
			s[2] = 5; /* ILLEGAL REQUEST */
			s[12] = 0x26; /*  INVALID FIELD IN PARAMETER LIST */
			ls = 0x12;

			memset(bd, 0, sizeof bd);
			uae_u8 *p = scsi_data;
			if (select10) {
				plen = (cmdbuf[7] << 8) | cmdbuf[8];
				memcpy(bd, p, plen > 8 ? 8 : plen);
				p += plen > 8 ? 8 : plen;
			} else {
				plen = cmdbuf[4];
				if (plen > 0)
					bd[0] = scsi_data[0];
				if (plen > 1)
					bd[2] = scsi_data[1]; // medium type
				if (plen > 2)
					bd[3] = scsi_data[2];
				if (plen > 3)
					bd[7] = scsi_data[3]; // bd len
				p += plen > 4 ? 4 : plen;
			}
			if (bd[0] != 0 || bd[1] != 0 || bd[2] != 0 || bd[3] != 0 || bd[4] != 0 || bd[5] != 0)
				goto scsi_done;
			int bdlen = (bd[6] << 8) | bd[7];
			if (bdlen != 0 && bdlen != 8)
				goto scsi_done;
			if (bdlen) {
				int block = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | (p[7] << 0);
				if (block != 512)
					goto scsi_done;
				p += bdlen;
			}
			for (;;) {
				int rem = (int)(plen - (p - scsi_data));
				if (!rem)
					break;
				if (rem < 2)
					goto scsi_done;
				uae_u8 pc = *p++;
				if (pc >= 0x40)
					goto scsi_done;
				uae_u8 pagelen = *p++;
				rem -= 2;
				if (!pagelen || pagelen > rem)
					goto scsi_done;
				if (pc != 0 && pc != 1 && pc != 3 && pc != 4)
					goto scsi_done;
				if (pc == 0)
					break;
				p += pagelen;
			}
			status = 0;
			ls = 0;
			scsi_len = 0;
			break;
		}
	case 0x5a: // MODE SENSE(10)
	case 0x1a: /* MODE SENSE(6) */
		{
			uae_u8 *p;
			bool pcodeloop = false;
			bool sense10 = cmdbuf[0] == 0x5a;
			int pc = cmdbuf[2] >> 6;
			int pcode = cmdbuf[2] & 0x3f;
			int dbd = cmdbuf[1] & 8;
			int cyl, head, tracksec;
			int totalsize, bdsize, alen;

			if (nodisk (hfd))
				goto nodisk;
			if (hdhfd) {
				cyl = hdhfd->cyls;
				head = hdhfd->heads;
				tracksec = hdhfd->secspertrack;
			} else {
				int cylsec;
				getchsx (hfd, &cyl, &cylsec, &head, &tracksec);
			}
			//write_log (_T("MODE SENSE PC=%d CODE=%d DBD=%d\n"), pc, pcode, dbd);
			p = r;

			if (sense10) {
				totalsize = 8 - 2;
				alen = (cmdbuf[7] << 8) | cmdbuf[8];
				p[2] = 0;
				p[3] = is_writeprotected(hfd) ? 0x80 : 0x00;
				p[4] = 0;
				p[5] = 0;
				p[6] = 0;
				p[7] = 0;
				p += 8;
			} else {
				totalsize = 4 - 1;
				alen = cmdbuf[4];
				p[1] = 0;
				p[2] = is_writeprotected(hfd) ? 0x80 : 0x00;
				p[3] = 0;
				p += 4;
			}

			bdsize = 0;
			if (!dbd) {
				uae_u32 blocks = (uae_u32)(hfd->virtsize / hfd->ci.blocksize);
				wl(p + 0, blocks >= 0x00ffffff ? 0x00ffffff : blocks);
				wl(p + 4, hfd->ci.blocksize);
				bdsize = 8;
				p += bdsize;
			}

			if (pcode == 0x3f) {
				pcode = 1; // page = 0 must be last
				pcodeloop = true;
			}
			for (;;) {
				int psize = 0;
				if (pcode == 0) {
					p[0] = 0;
					p[1] = 0;
					p[2] = 0x20;
					p[3] = 0;
					psize = 4;
				} else if (pcode == 1) {
					// error recovery page
					p[0] = 1;
					p[1] = 0x0a;
					psize = p[1] + 2;
					// return defaults (0)
				} else if (pcode == 3) {
					// format parameters
					p[0] = 3;
					p[1] = 22;
					p[3] = 1;
					p[10] = tracksec >> 8;
					p[11] = tracksec;
					p[12] = hfd->ci.blocksize >> 8;
					p[13] = hfd->ci.blocksize;
					p[15] = 1; // interleave
					p[20] = 0x80;
					psize = p[1] + 2;
				} else if (pcode == 4) {
					// rigid drive geometry
					p[0] = 4;
					wl(p + 1, cyl);
					p[1] = 22;
					p[5] = head;
					wl(p + 13, cyl);
					ww(p + 20, 5400);
					psize = p[1] + 2;
				} else {
					if (!pcodeloop)
						goto err;
				}
				totalsize += psize;
				p += psize;
				if (!pcodeloop)
					break;
				if (pcode == 0)
					break;
				pcode++;
				if (pcode == 0x3f)
					pcode = 0;
			}

			if (sense10) {
				totalsize += bdsize;
				r[6] = bdsize >> 8;
				r[7] = bdsize & 0xff;
				r[0] = totalsize >> 8;
				r[1] = totalsize & 0xff;
			} else {
				totalsize += bdsize;
				r[3] = (uae_u8)bdsize;
				r[0] = (uae_u8)totalsize;
			}

			scsi_len = lr = totalsize + 1;
			if (scsi_len > alen)
				scsi_len = alen;
			if (lr > alen)
				lr = alen;
			break;
		}
		break;
	case 0x1d: /* SEND DIAGNOSTICS */
		break;
	case 0x25: /* READ CAPACITY */
		{
			int pmi = cmdbuf[8] & 1;
			uae_u64 lba = (cmdbuf[2] << 24) | (cmdbuf[3] << 16) | (cmdbuf[4] << 8) | cmdbuf[5];
			uae_u64 blocks;
			int cyl, head, tracksec;
			if (nodisk (hfd))
				goto nodisk;
			blocks = hfd->virtsize / hfd->ci.blocksize;
			if (hfd->ci.max_lba)
				blocks = hfd->ci.max_lba;
			if (hdhfd) {
				cyl = hdhfd->cyls;
				head = hdhfd->heads;
				tracksec = hdhfd->secspertrack;
			} else {
				int cylsec;
				getchsx (hfd, &cyl, &cylsec, &head, &tracksec);
			}
			if (pmi == 0 && lba != 0)
				goto errreq;
			if (pmi) {
				lba += tracksec * head;
				lba /= tracksec * head;
				lba *= tracksec * head;
				if (lba > blocks)
					lba = blocks;
				blocks = lba;
			}
			wl (r, (uae_u32)(blocks <= 0x100000000 ? blocks - 1 : 0xffffffff));
			wl (r + 4, hfd->ci.blocksize);
			scsi_len = lr = 8;
		}
		break;
	case 0x2b: /* SEEK (10) */
		if (nodisk (hfd))
			goto nodisk;
		offset = rl (cmdbuf + 2);
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, hfd->ci.blocksize, 3);
		if (chkerr)
			goto checkfail;
		scsi_len = 0;
		break;
	case 0x28: /* READ (10) */
		if (nodisk (hfd))
			goto nodisk;
		offset = rl (cmdbuf + 2);
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		len = rl (cmdbuf + 7 - 2) & 0xffff;
		len *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, len, 1);
		if (chkerr)
			goto checkfail;
		scsi_len = (uae_u32)cmd_readx(hfd, scsi_data, offset, len);
		break;
	case 0x2a: /* WRITE (10) */
		if (nodisk (hfd))
			goto nodisk;
		if (is_writeprotected(hfd))
			goto readprot;
		offset = rl (cmdbuf + 2);
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		len = rl (cmdbuf + 7 - 2) & 0xffff;
		len *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, len, 2);
		if (chkerr)
			goto checkfail;
		scsi_len = (uae_u32)cmd_writex(hfd, scsi_data, offset, len);
		break;
	case 0x2f: /* VERIFY (10) */
		{
			int bytchk = cmdbuf[1] & 2;
			if (nodisk (hfd))
				goto nodisk;
			if (bytchk) {
				offset = rl (cmdbuf + 2);
				current_lba = offset;
				offset *= hfd->ci.blocksize;
				len = rl (cmdbuf + 7 - 2) & 0xffff;
				len *= hfd->ci.blocksize;
				chkerr = checkbounds (hfd, offset, len, 1);
				if (chkerr)
					goto checkfail;
				uae_u8 *vb = xmalloc(uae_u8, hfd->ci.blocksize);
				while (len > 0) {
					uae_u64 readlen = cmd_readx (hfd, vb, offset, hfd->ci.blocksize);
					if (readlen != hfd->ci.blocksize || memcmp (vb, scsi_data, hfd->ci.blocksize)) {
						xfree (vb);
						goto miscompare;
					}
					scsi_data += hfd->ci.blocksize;
					offset += hfd->ci.blocksize;
					len -= hfd->ci.blocksize;
				}
				xfree (vb);
			}
			scsi_len = 0;
		}
		break;
	case 0x35: /* SYNCRONIZE CACHE (10) */
		if (nodisk (hfd))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0xa8: /* READ (12) */
		if (nodisk (hfd))
			goto nodisk;
		offset = rl (cmdbuf + 2);
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		len = rl (cmdbuf + 6);
		len *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, len, 1);
		if (chkerr)
			goto checkfail;
		scsi_len = (uae_u32)cmd_readx(hfd, scsi_data, offset, len);
		break;
	case 0xaa: /* WRITE (12) */
		if (nodisk (hfd))
			goto nodisk;
		if (is_writeprotected(hfd))
			goto readprot;
		offset = rl (cmdbuf + 2);
		current_lba = offset;
		offset *= hfd->ci.blocksize;
		len = rl (cmdbuf + 6);
		len *= hfd->ci.blocksize;
		chkerr = checkbounds(hfd, offset, len, 2);
		if (chkerr)
			goto checkfail;
		scsi_len = (uae_u32)cmd_writex (hfd, scsi_data, offset, len);
		break;
	case 0x37: /* READ DEFECT DATA */
		if (nodisk (hfd))
			goto nodisk;
		scsi_len = lr = 4;
		r[0] = 0;
		r[1] = cmdbuf[1] & 0x1f;
		r[2] = 0;
		r[3] = 0;
#if 0
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 0; /* NO SENSE */
		s[12] = 0x1c; /* DEFECT LIST NOT FOUND */
		ls = 0x12;
#endif
		break;
	case 0xe0: /* RAM DIAGNOSTICS */
	case 0xe3: /* DRIVE DIAGNOSTIC */
	case 0xe4: /* CONTROLLER INTERNAL DIAGNOSTICS */
		scsi_len = 0;
		break;
readprot:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 7; /* DATA PROTECT */
		s[12] = 0x27; /* WRITE PROTECTED */
		ls = 0x12;
		sasi_sense = 0x03; // write fault
		break;
nodisk:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 2; /* NOT READY */
		s[12] = 0x3A; /* MEDIUM NOT PRESENT */
		ls = 0x12;
		sasi_sense = 0x04; // drive not ready
		break;

	default:
err:
		write_log (_T("UAEHF: unsupported scsi command 0x%02X LUN=%d\n"), cmdbuf[0], lun);
errreq:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x24; /* ILLEGAL FIELD IN CDB */
		ls = 0x12;
		sasi_sense = 0x22; // invalid parameter
		break;
checkfail:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70 | ((current_lba != ~0) ? 0x80 : 0x00);
		if (chkerr < 0) {
			s[2] = 5; /* ILLEGAL REQUEST */
			s[12] = 0x21; /* LOGICAL BLOCK OUT OF RANGE */
			sasi_sense = 0x21; // illegal disk address
		} else {
			s[2] = 3; /* MEDIUM ERROR */
			if (chkerr == 1) {
				s[12] = 0x11; /* Unrecovered Read Error */
				sasi_sense = 0x11; // uncorrectable data error
			}
			if (chkerr == 2) {
				s[12] = 0x0c; /* Write Error */
				sasi_sense = 0x03; // write fault
			}
		}
		ls = 0x12;
		break;
miscompare:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70 | ((current_lba != ~0) ? 0x80 : 0x00);
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x1d; /* MISCOMPARE DURING VERIFY OPERATION */
		ls = 0x12;
		sasi_sense = 0x11; // uncorrectable data error
		break;
	}
scsi_done:

	if (ls > 7)
		s[7] = ls - 8;

	if (log_scsiemu)
		write_log (_T("-> DATAOUT=%d ST=%d SENSELEN=%d REPLYLEN=%d\n"), scsi_len, status, ls, lr);

	*data_len = scsi_len;
	*reply_len = lr;
	if (lr > 0 && lr < 512) {
		if (log_scsiemu) {
			write_log (_T("REPLY: "));
			for (int i = 0; i < lr && i < 40; i++)
				write_log (_T("%02X."), r[i]);
			write_log (_T("\n"));	
		}
	}
	if (ls > 0) {
		if (omti || sasi) {
			if (sasi_sense != 0) {
				bool islba = (s[0] & 0x80) != 0;
				ls = 4;
				s[0] = sasi_sense | (islba ? 0x80 : 0x00);
				s[1] = (lun & 1) << 5;
				s[2] = 0;
				s[3] = 0;
				if (islba) {
					s[1] |= (current_lba >> 16) & 31;
					s[2] = (current_lba >> 8) & 255;
					s[3] = (current_lba >> 0) & 255;
				}
			}
			if (log_scsiemu && ls) {
				write_log(_T("-> SENSE STATUS:\n"));
				for (int i = 0; i < ls; i++)
					write_log(_T("%02X."), s[i]);
				write_log(_T("\n"));
			}
		} else {
			if (s[0] & 0x80) {
				s[3] = (current_lba >> 24) & 255;
				s[4] = (current_lba >> 16) & 255;
				s[5] = (current_lba >>  8) & 255;
				s[6] = (current_lba >>  0) & 255;
			}
			if (log_scsiemu && ls) {
				write_log(_T("-> SENSE STATUS: KEY=%d ASC=%02X ASCQ=%02X\n"), s[2], s[12], s[13]);
				for (int i = 0; i < ls; i++)
					write_log(_T("%02X."), s[i]);
				write_log(_T("\n"));
			}
		}
		memset (hfd->scsi_sense, 0, MAX_SCSI_SENSE);
		memcpy (hfd->scsi_sense, s, ls);
	}
	*sense_len = ls;
	return status;
}

static int handle_scsi (TrapContext *ctx, uae_u8 *iobuf, uaecptr request, struct hardfiledata *hfd, struct scsi_data *sd, bool safeonly)
{
	int ret = 0;

	uae_u32 scsicmdaddr = get_long_host(iobuf + 40);

	uae_u8 scsicmd[30];
	trap_get_bytes(ctx, scsicmd, scsicmdaddr, sizeof  scsicmd);

	uaecptr scsi_data = get_long_host(scsicmd + 0);
	int scsi_len = get_long_host(scsicmd + 4);
	uaecptr scsi_cmd = get_long_host(scsicmd + 12);
	uae_u16 scsi_cmd_len = get_word_host(scsicmd + 16);
	uae_u8 scsi_flags = get_byte_host(scsicmd + 20);
	uaecptr scsi_sense = get_long_host(scsicmd + 22);
	uae_u16 scsi_sense_len = get_word_host(scsicmd + 26);
	uae_u8 cmd = trap_get_byte(ctx, scsi_cmd);

	scsi_sense_len  = (scsi_flags & 4) ? 4 : /* SCSIF_OLDAUTOSENSE */
		(scsi_flags & 2) ? scsi_sense_len : /* SCSIF_AUTOSENSE */
		32;
	scsi_log (_T("hdf scsiemu: cmd=%02X,%d flags=%02X sense=%p,%d data=%p,%d\n"),
		cmd, scsi_cmd_len, scsi_flags, scsi_sense, scsi_sense_len, scsi_data, scsi_len);

	sd->cmd_len = scsi_cmd_len;
	sd->data_len = scsi_len;

	trap_get_bytes(ctx, sd->cmd, scsi_cmd, sd->cmd_len);
	for (int i = 0; i < sd->cmd_len; i++) {
		scsi_log (_T("%02X%c"), sd->cmd[i], i < sd->cmd_len - 1 ? '.' : ' ');
	}
	scsi_log (_T("\n"));

	if (safeonly && !scsi_cmd_is_safe(sd->cmd[0])) {
		sd->reply_len = 0;
		sd->data_len = 0;
		sd->status = 2;
		sd->sense_len = 18;
		sd->sense[0] = 0x70;
		sd->sense[2] = 5; /* ILLEGAL REQUEST */
		sd->sense[12] = 0x30; /* INCOMPATIBLE MEDIUM INSERTED */
	} else {
		scsi_emulate_analyze(sd);
		scsi_start_transfer(sd);
		if (sd->direction > 0) {
			trap_get_bytes(ctx, sd->buffer, scsi_data, sd->data_len);
			scsi_emulate_cmd(sd);
		} else {
			scsi_emulate_cmd(sd);
			if (sd->direction < 0)
				trap_put_bytes(ctx, sd->buffer, scsi_data, sd->data_len);
		}
	}

	put_word_host(scsicmd + 18, sd->status != 0 ? 0 : sd->cmd_len); /* fake scsi_CmdActual */
	put_byte_host(scsicmd + 21, sd->status); /* scsi_Status */
	if (sd->reply_len > 0) {
		trap_put_bytes(ctx, sd->reply, scsi_data, sd->reply_len);
		scsi_log (_T("RD:"));
		int i = 0;
		while (i < sd->reply_len && i < 24) {
			scsi_log (_T("%02X%c"), sd->reply[i], i < sd->reply_len - 1 ? '.' : ' ');
			i++;
		}
		scsi_log (_T("\n"));
	}
	if (scsi_sense) {
		int slen = sd->sense_len < scsi_sense_len ? sd->sense_len : scsi_sense_len;
		trap_put_bytes(ctx, sd->sense, scsi_sense, slen);
		if (scsi_sense_len > sd->sense_len) {
			trap_set_bytes(ctx, scsi_sense + sd->sense_len, 0, scsi_sense_len - sd->sense_len);
		}
		put_word_host(scsicmd + 28, slen); /* scsi_SenseActual */
	} else {
		put_word_host(scsicmd + 28, 0);
	}
	if (sd->data_len < 0) {
		put_long_host(scsicmd + 8, 0); /* scsi_Actual */
		ret = 20;
	} else {
		put_long_host(scsicmd + 8, sd->data_len); /* scsi_Actual */
	}
	
	trap_put_bytes(ctx, scsicmd, scsicmdaddr, sizeof scsicmd);
	return ret;
}


void hardfile_send_disk_change (struct hardfiledata *hfd, bool insert)
{
	int newstate = insert ? 0 : 1;

	uae_sem_wait (&change_sem);
	hardfpd[hfd->unitnum].changenum++;
	write_log (_T("uaehf.device:%d media status=%d changenum=%d\n"), hfd->unitnum, insert, hardfpd[hfd->unitnum].changenum);
	hfd->drive_empty = newstate;
	int j = 0;
	while (j < MAX_ASYNC_REQUESTS) {
		if (hardfpd[hfd->unitnum].d_request_type[j] == ASYNC_REQUEST_CHANGEINT) {
			uae_Cause (hardfpd[hfd->unitnum].d_request_data[j]);
		}
		j++;
	}
	if (hardfpd[hfd->unitnum].changeint)
		uae_Cause (hardfpd[hfd->unitnum].changeint);
	uae_sem_post (&change_sem);
}

void hardfile_do_disk_change (struct uaedev_config_data *uci, bool insert)
{
	int fsid = uci->configoffset;
	struct hardfiledata *hfd;

	const struct expansionromtype *ert = get_unit_expansion_rom(uci->ci.controller_type);
	if (ert && (ert->deviceflags & EXPANSIONTYPE_PCMCIA)) {
		pcmcia_reinsert(&currprefs);
		return;
	}

	hfd = get_hardfile_data (fsid);
	if (!hfd)
		return;
	hardfile_send_disk_change (hfd, insert);
}

static int add_async_request (struct hardfileprivdata *hfpd, uae_u8 *iobuf, uaecptr request, int type, uae_u32 data)
{
	int i;

	i = 0;
	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == request) {
			hfpd->d_request_type[i] = type;
			hfpd->d_request_data[i] = data;
			hf_log (_T("old async request %p (%d) added\n"), request, type);
			return 0;
		}
		i++;
	}
	i = 0;
	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == 0) {
			hfpd->d_request[i] = request;
			hfpd->d_request_iobuf[i] = iobuf;
			hfpd->d_request_type[i] = type;
			hfpd->d_request_data[i] = data;
			hf_log (_T("async request %p (%d) added (total=%d)\n"), request, type, i);
			return 0;
		}
		i++;
	}
	hf_log (_T("async request overflow %p!\n"), request);
	return -1;
}

static int release_async_request(struct hardfileprivdata *hfpd, uaecptr request)
{
	int i = 0;

	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == request) {
			int type = hfpd->d_request_type[i];
			hfpd->d_request[i] = 0;
			xfree(hfpd->d_request_iobuf[i]);
			hfpd->d_request_iobuf[i] = 0;
			hfpd->d_request_data[i] = 0;
			hfpd->d_request_type[i] = 0;
			hf_log (_T("async request %p removed\n"), request);
			return type;
		}
		i++;
	}
	hf_log (_T("tried to remove non-existing request %p\n"), request);
	return -1;
}

static void abort_async (struct hardfileprivdata *hfpd, uaecptr request, int errcode, int type)
{
	int i;
	hf_log (_T("aborting async request %p\n"), request);
	i = 0;
	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == request && hfpd->d_request_type[i] == ASYNC_REQUEST_TEMP) {
			/* ASYNC_REQUEST_TEMP = request is processing */
			sleep_millis (1);
			i = 0;
			continue;
		}
		i++;
	}
	i = release_async_request (hfpd, request);
	if (i >= 0) {
		hf_log (_T("asyncronous request=%08X aborted, error=%d\n"), request, errcode);
	}
}

static int hardfile_thread (void *devs);
static int start_thread (TrapContext *ctx, int unit)
{
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	if (hfpd->thread_running)
		return 1;
	memset (hfpd, 0, sizeof (struct hardfileprivdata));
	hfpd->base = trap_get_areg(ctx, 6);
	init_comm_pipe (&hfpd->requests, 300, 3);
	uae_sem_init (&hfpd->sync_sem, 0, 0);
	uae_start_thread (_T("hardfile"), hardfile_thread, hfpd, NULL);
	uae_sem_wait (&hfpd->sync_sem);
	return hfpd->thread_running;
}

static int mangleunit (int unit)
{
	if (unit <= 99)
		return unit;
	if (unit == 100)
		return 8;
	if (unit == 110)
		return 9;
	return -1;
}

static uae_u32 REGPARAM2 hardfile_open (TrapContext *ctx)
{
	uaecptr ioreq = trap_get_areg (ctx, 1); /* IOReq */
	int unit = mangleunit (trap_get_dreg(ctx, 0));
	int err = IOERR_OPENFAIL;

	/* boot device port size == 0!? KS 1.x size = 12???
	 * Ignore message size, too many programs do not set it correct
	 * int size = get_word (ioreq + 0x12);
	 */
	/* Check unit number */
	if (unit >= 0 && unit < MAX_FILESYSTEM_UNITS) {
		struct hardfileprivdata *hfpd = &hardfpd[unit];
		struct hardfiledata *hfd = get_hardfile_data_controller(unit);
		if (hfd) {
			if (hfd->ci.type == UAEDEV_DIR) {
				if (start_thread(ctx, unit)) {
					hfpd->directorydrive = true;
					trap_put_word(ctx, hfpd->base + 32, trap_get_word(ctx, hfpd->base + 32) + 1);
					trap_put_long(ctx, ioreq + 24, unit); /* io_Unit */
					trap_put_byte(ctx, ioreq + 31, 0); /* io_Error */
					trap_put_byte(ctx, ioreq + 8, 7); /* ln_type = NT_REPLYMSG */
					if (!hfpd->sd)
						hfpd->sd = scsi_alloc_generic(hfd, UAEDEV_DIR, unit);
					hf_log(_T("virtual hardfile_open, unit %d (%d), OK\n"), unit, trap_get_dreg(ctx, 0));
					return 0;
				}
			} else {
				if ((hfd->handle_valid || hfd->drive_empty) && start_thread(ctx, unit)) {
					hfpd->directorydrive = false;
					trap_put_word(ctx, hfpd->base + 32, trap_get_word(ctx, hfpd->base + 32) + 1);
					trap_put_long(ctx, ioreq + 24, unit); /* io_Unit */
					trap_put_byte(ctx, ioreq + 31, 0); /* io_Error */
					trap_put_byte(ctx, ioreq + 8, 7); /* ln_type = NT_REPLYMSG */
					if (!hfpd->sd)
						hfpd->sd = scsi_alloc_generic(hfd, UAEDEV_HDF, unit);
					hf_log(_T("hardfile_open, unit %d (%d), OK\n"), unit, trap_get_dreg(ctx, 0));
					return 0;
				}
			}
		}
	}
	if (unit < 1000)
		err = 50; /* HFERR_NoBoard */
	hf_log (_T("hardfile_open, unit %d (%d), ERR=%d\n"), unit, trap_get_dreg(ctx, 0), err);
	trap_put_long(ctx, ioreq + 20, (uae_u32)err);
	trap_put_byte(ctx, ioreq + 31, (uae_u8)err);
	return (uae_u32)err;
}

static uae_u32 REGPARAM2 hardfile_close (TrapContext *ctx)
{
	uaecptr request = trap_get_areg (ctx, 1); /* IOReq */
	int unit = mangleunit (trap_get_long(ctx, request + 24));
	if (unit < 0 || unit >= MAX_FILESYSTEM_UNITS) {
		return 0;
	}
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	if (!hfpd)
		return 0;
	trap_put_word(ctx, hfpd->base + 32, trap_get_word(ctx, hfpd->base + 32) - 1);
	if (trap_get_word(ctx, hfpd->base + 32) == 0) {
		scsi_free(hfpd->sd);
		hfpd->sd = NULL;
		write_comm_pipe_pvoid(&hfpd->requests, NULL, 0);
		write_comm_pipe_pvoid(&hfpd->requests, NULL, 0);
		write_comm_pipe_u32(&hfpd->requests, 0, 1);
	}
	return 0;
}

static uae_u32 REGPARAM2 hardfile_expunge (TrapContext *context)
{
	return 0; /* Simply ignore this one... */
}

static void outofbounds (int cmd, uae_u64 offset, uae_u64 len, uae_u64 max)
{
	write_log (_T("UAEHF: cmd %d: out of bounds, %08X-%08X + %08X-%08X > %08X-%08X\n"), cmd,
		(uae_u32)(offset >> 32),(uae_u32)offset,(uae_u32)(len >> 32),(uae_u32)len,
		(uae_u32)(max >> 32),(uae_u32)max);
}
static void unaligned (int cmd, uae_u64 offset, uae_u64 len, int blocksize)
{
	write_log (_T("UAEHF: cmd %d: unaligned access, %08X-%08X, %08X-%08X, %08X\n"), cmd,
		(uae_u32)(offset >> 32),(uae_u32)offset,(uae_u32)(len >> 32),(uae_u32)len,
		blocksize);
}

static bool isbadblock(struct hardfiledata *hfd, uae_u64 offset, uae_u64 len)
{
	if (!hfd->ci.badblock_num)
		return false;
	offset /= hfd->ci.blocksize;
	len /= hfd->ci.blocksize;
	for (int i = 0; i < hfd->ci.badblock_num; i++) {
		struct uaedev_badblock *bb = &hfd->ci.badblocks[i];
		if (offset + len >= bb->first && offset < bb->last) {
			return true;
		}
	}
	return false;
}

static bool vdisk(struct hardfileprivdata *hfdp)
{
	return hfdp->directorydrive;
}

static uae_u32 hardfile_do_io (TrapContext *ctx, struct hardfiledata *hfd, struct hardfileprivdata *hfpd, uae_u8 *iobuf, uaecptr request)
{
	uae_u32 dataptr, offset, actual = 0, cmd;
	uae_u64 offset64;
	int unit;
	uae_u32 error = 0, len;
	int async = 0;
	int bmask = hfd->ci.blocksize - 1;

	unit = get_long_host(iobuf + 24);
	cmd = get_word_host(iobuf + 28); /* io_Command */
	dataptr = get_long_host(iobuf + 40);
	switch (cmd)
	{
	case CMD_READ:
		if (nodisk (hfd))
			goto no_disk;
		if (vdisk(hfpd))
			goto v_disk;
		offset = get_long_host(iobuf + 44);
		len = get_long_host(iobuf + 36); /* io_Length */
		if (offset & bmask) {
			unaligned (cmd, offset, len, hfd->ci.blocksize);
			goto bad_command;
		}
		if (len & bmask) {
			unaligned (cmd, offset, len, hfd->ci.blocksize);
			goto bad_len;
		}
		if (len + offset > hfd->virtsize) {
			outofbounds(cmd, offset, len, hfd->virtsize);
			goto bad_len;
		}
		if (isbadblock(hfd, offset, len)) {
			goto bad_block;
		}
		actual = (uae_u32)cmd_read(ctx, hfd, dataptr, offset, len);
		break;

#if HDF_SUPPORT_TD64
	case TD_READ64:
#endif
#if HDF_SUPPORT_NSD
	case NSCMD_TD_READ64:
#endif
#if defined(HDF_SUPPORT_NSD) || defined(HDF_SUPPORT_TD64)
		if (nodisk (hfd))
			goto no_disk;
		if (vdisk(hfpd))
			goto v_disk;
		offset64 = get_long_host(iobuf + 44) | ((uae_u64)get_long_host(iobuf + 32) << 32);
		len = get_long_host(iobuf + 36); /* io_Length */
		if (offset64 & bmask) {
			unaligned (cmd, offset64, len, hfd->ci.blocksize);
			goto bad_command;
		}
		if (len & bmask) {
			unaligned (cmd, offset64, len, hfd->ci.blocksize);
			goto bad_len;
		}
		if (len + offset64 > hfd->virtsize || (uae_s64)offset64 < 0) {
			outofbounds (cmd, offset64, len, hfd->virtsize);
			goto bad_len;
		}
		if (isbadblock(hfd, offset64, len)) {
			goto bad_block;
		}
		actual = (uae_u32)cmd_read(ctx, hfd, dataptr, offset64, len);
		break;
#endif

	case CMD_WRITE:
	case CMD_FORMAT: /* Format */
		if (nodisk (hfd))
			goto no_disk;
		if (vdisk(hfpd))
			goto v_disk;
		if (is_writeprotected(hfd)) {
			error = 28; /* write protect */
		} else {
			offset = get_long_host(iobuf + 44);
			len = get_long_host(iobuf + 36); /* io_Length */
			if (offset & bmask) {
				unaligned (cmd, offset, len, hfd->ci.blocksize);
				goto bad_command;
			}
			if (len & bmask) {
				unaligned (cmd, offset, len, hfd->ci.blocksize);
				goto bad_len;
			}
			if (len + offset > hfd->virtsize) {
				outofbounds (cmd, offset, len, hfd->virtsize);
				goto bad_len;
			}
			if (isbadblock(hfd, offset, len)) {
				goto bad_block;
			}
			actual = (uae_u32)cmd_write(ctx, hfd, dataptr, offset, len);
		}
		break;

#if HDF_SUPPORT_TD64
	case TD_WRITE64:
	case TD_FORMAT64:
#endif
#if HDF_SUPPORT_NSD
	case NSCMD_TD_WRITE64:
	case NSCMD_TD_FORMAT64:
#endif
#if defined(HDF_SUPPORT_NSD) || defined(HDF_SUPPORT_TD64)
		if (nodisk (hfd))
			goto no_disk;
		if (vdisk(hfpd))
			goto v_disk;
		if (is_writeprotected(hfd)) {
			error = 28; /* write protect */
		} else {
			offset64 = get_long_host(iobuf + 44) | ((uae_u64)get_long_host(iobuf + 32) << 32);
			len = get_long_host(iobuf + 36); /* io_Length */
			if (offset64 & bmask) {
				unaligned (cmd, offset64, len, hfd->ci.blocksize);
				goto bad_command;
			}
			if (len & bmask) {
				unaligned (cmd, offset64, len, hfd->ci.blocksize);
				goto bad_len;
			}
			if (len + offset64 > hfd->virtsize || (uae_s64)offset64 < 0) {
				outofbounds (cmd, offset64, len, hfd->virtsize);
				goto bad_len;
			}
			if (isbadblock(hfd, offset64, len)) {
				goto bad_block;
			}
			actual = (uae_u32)cmd_write(ctx, hfd, dataptr, offset64, len);
		}
		break;
#endif

#if HDF_SUPPORT_NSD
	case NSCMD_DEVICEQUERY:
		if (vdisk(hfpd))
			goto v_disk;
		trap_put_long(ctx, dataptr + 0, 0);
		trap_put_long(ctx, dataptr + 4, 16); /* size */
		trap_put_word(ctx, dataptr + 8, NSDEVTYPE_TRACKDISK);
		trap_put_word(ctx, dataptr + 10, 0);
		trap_put_long(ctx, dataptr + 12, nscmd_cmd);
		actual = 16;
		break;
#endif

	case CMD_GETDRIVETYPE:
		if (vdisk(hfpd))
			goto v_disk;
#if HDF_SUPPORT_NSD
		actual = DRIVE_NEWSTYLE;
		break;
#else
		goto no_cmd;
#endif

	case CMD_GETNUMTRACKS:
		{
			int cyl, cylsec, head, tracksec;
			getchsx (hfd, &cyl, &cylsec, &head, &tracksec);
			actual = cyl * head;
			break;
		}

	case CMD_GETGEOMETRY:
		{
			int cyl, cylsec, head, tracksec;
			uae_u64 size;
			getchsx (hfd, &cyl, &cylsec, &head, &tracksec);
			trap_put_long(ctx, dataptr + 0, hfd->ci.blocksize);
			size = hfd->virtsize / hfd->ci.blocksize;
			if (!size)
				size = hfd->ci.max_lba;
			if (size > 0x00ffffffff)
				size = 0xffffffff;
			trap_put_long(ctx, dataptr + 4, (uae_u32)size);
			trap_put_long(ctx, dataptr + 8, cyl);
			trap_put_long(ctx, dataptr + 12, cylsec);
			trap_put_long(ctx, dataptr + 16, head);
			trap_put_long(ctx, dataptr + 20, tracksec);
			trap_put_long(ctx, dataptr + 24, 0); /* bufmemtype */
			trap_put_byte(ctx, dataptr + 28, 0); /* type = DG_DIRECT_ACCESS */
			trap_put_byte(ctx, dataptr + 29, 0); /* flags */
		}
		break;

	case CMD_PROTSTATUS:
		if (is_writeprotected(hfd))
			actual = -1;
		else
			actual = 0;
		break;

	case CMD_CHANGESTATE:
		actual = hfd->drive_empty ? 1 :0;
		break;

		/* Some commands that just do nothing and return zero */
	case CMD_UPDATE:
	case CMD_CLEAR:
	case CMD_MOTOR:
	case CMD_SEEK:
		break;

#if HDF_SUPPORT_TD64
	case TD_SEEK64:
		if (vdisk(hfpd))
			goto v_disk;
		break;
#endif
#ifdef HDF_SUPPORT_NSD
	case NSCMD_TD_SEEK64:
		if (vdisk(hfpd))
			goto v_disk;
		break;
#endif

	case CMD_REMOVE:
		hfpd->changeint = get_long (request + 40);
		break;

	case CMD_CHANGENUM:
		actual = hfpd->changenum;
		break;

	case CMD_ADDCHANGEINT:
		if (vdisk(hfpd))
			goto v_disk;
		error = add_async_request (hfpd, iobuf, request, ASYNC_REQUEST_CHANGEINT, get_long_host(iobuf + 40));
		if (!error)
			async = 1;
		break;
	case CMD_REMCHANGEINT:
		if (vdisk(hfpd))
			goto v_disk;
		release_async_request (hfpd, request);
		break;

#if HDF_SUPPORT_DS
	case HD_SCSICMD: /* SCSI */
		if (vdisk(hfpd)) {
			error = handle_scsi(ctx, iobuf, request, hfd, hfpd->sd, true);
		} else if (HDF_SUPPORT_DS_PARTITION || enable_ds_partition_hdf || (!hfd->ci.sectors && !hfd->ci.surfaces && !hfd->ci.reserved)) {
			error = handle_scsi(ctx, iobuf, request, hfd, hfpd->sd, false);
		} else { /* we don't want users trashing their "partition" hardfiles with hdtoolbox */
			error = handle_scsi(ctx, iobuf, request, hfd, hfpd->sd, true);
		}
		actual = 30; // sizeof(struct SCSICmd)
		break;
#endif

	case CD_EJECT:
		if (vdisk(hfpd))
			goto v_disk;
		if (hfd->ci.sectors && hfd->ci.surfaces) {
			int len = get_long_host(iobuf + 36);
			if (len) {
				if (hfd->drive_empty) {
					hardfile_media_change (hfd, NULL, true, false);
				} else {
					hardfile_media_change (hfd, NULL, false, false);
				}
			} else {
				if (hfd->drive_empty) {
					hardfile_media_change (hfd, NULL, true, false);
				}
			}
		} else {
			error = IOERR_NOCMD;
		}
		break;

bad_block:
		error = 45; // HFERR_BadStatus
		break;
bad_command:
		error = IOERR_BADADDRESS;
		break;
bad_len:
		error = IOERR_BADLENGTH;
		break;
v_disk:
		error = IOERR_BadDriveType;
		break;
no_disk:
		error = 29; /* no disk */
		break;

	default:
		/* Command not understood. */
		error = IOERR_NOCMD;
		break;
	}
	put_long_host(iobuf + 32, actual);
	put_byte_host(iobuf + 31, error);

	hf_log2 (_T("hf: unit=%d, request=%p, cmd=%d offset=%u len=%d, actual=%d error%=%d\n"), unit, request,
		get_word_host(iobuf + 28), get_long_host(iobuf + 44), get_long_host(iobuf + 36), actual, error);

	return async;
}

static uae_u32 REGPARAM2 hardfile_abortio (TrapContext *ctx)
{
	uae_u32 request = trap_get_areg (ctx, 1);
	int unit = mangleunit (trap_get_long(ctx, request + 24));
	struct hardfiledata *hfd = get_hardfile_data_controller(unit);
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	hf_log2 (_T("uaehf.device abortio "));
	start_thread (ctx, unit);
	if (!hfd || !hfpd || !hfpd->thread_running) {
		trap_put_byte(ctx, request + 31, 32);
		hf_log2 (_T("error\n"));
		return trap_get_byte(ctx, request + 31);
	}
	trap_put_byte(ctx, request + 31, -2);
	hf_log2 (_T("unit=%d, request=%08X\n"),  unit, request);
	abort_async(hfpd, request, -2, 0);
	return 0;
}

static int hardfile_can_quick (uae_u32 command)
{
	switch (command)
	{
	case CMD_REMCHANGEINT:
		return -1;
	case CMD_RESET:
	case CMD_STOP:
	case CMD_START:
	case CMD_CHANGESTATE:
	case CMD_PROTSTATUS:
	case CMD_MOTOR:
	case CMD_GETDRIVETYPE:
	case CMD_GETGEOMETRY:
	case CMD_GETNUMTRACKS:
	case NSCMD_DEVICEQUERY:
		return 1;
	}
	return 0;
}

static int hardfile_canquick (TrapContext *ctx, struct hardfiledata *hfd, uae_u8 *iobuf)
{
	uae_u32 command = get_word_host(iobuf + 28);
	return hardfile_can_quick (command);
}

static uae_u32 REGPARAM2 hardfile_beginio (TrapContext *ctx)
{
	int canquick;
	uae_u32 request = trap_get_areg(ctx, 1);

	uae_u8 *iobuf = xmalloc(uae_u8, 48);

	trap_get_bytes(ctx, iobuf, request, 48);

	uae_u8 flags = get_byte_host(iobuf + 30);
	int cmd = get_word_host(iobuf + 28);
	int unit = mangleunit(get_long_host(iobuf + 24));

#if 0
	if (cmd == CMD_GETGEOMETRY)
		activate_debugger();
#endif

	struct hardfiledata *hfd = get_hardfile_data_controller(unit);
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	put_byte_host(iobuf + 8, NT_MESSAGE);
	start_thread(ctx, unit);
	if (!hfd || !hfpd || !hfpd->thread_running) {
		put_byte_host(iobuf + 31, 32);
		uae_u8 v = get_byte_host(iobuf + 31);
		trap_put_bytes(ctx, iobuf + 8, request + 8, 48 - 8);
		xfree(iobuf);
		return v;
	}
	put_byte_host(iobuf + 31, 0);
	canquick = hardfile_canquick(ctx, hfd, iobuf);
	if (((flags & 1) && canquick) || (canquick < 0)) {
		hf_log (_T("hf quickio unit=%d request=%p cmd=%d\n"), unit, request, cmd);
		if (hardfile_do_io(ctx, hfd, hfpd, iobuf, request)) {
			hf_log2 (_T("uaehf.device cmd %d bug with IO_QUICK\n"), cmd);
		}
		uae_u8 v = get_byte_host(iobuf + 31);
		trap_put_bytes(ctx, iobuf + 8, request + 8, 48 - 8);
		xfree(iobuf);
		if (!(flags & 1))
			uae_ReplyMsg(request);
		return v;
	} else {
		hf_log2 (_T("hf asyncio unit=%d request=%p cmd=%d\n"), unit, request, cmd);
		add_async_request(hfpd, iobuf, request, ASYNC_REQUEST_TEMP, 0);
		put_byte_host(iobuf + 30, get_byte_host(iobuf + 30) & ~1);
		trap_put_bytes(ctx, iobuf + 8, request + 8, 48 - 8);
		trap_set_background(ctx);
		write_comm_pipe_pvoid(&hfpd->requests, ctx, 0);
		write_comm_pipe_pvoid(&hfpd->requests, iobuf, 0);
		write_comm_pipe_u32(&hfpd->requests, request, 1);
		return 0;
	}
}

static int hardfile_thread (void *devs)
{
	struct hardfileprivdata *hfpd = (struct hardfileprivdata*)devs;

	uae_set_thread_priority (NULL, 1);
	hfpd->thread_running = 1;
	uae_sem_post (&hfpd->sync_sem);
	for (;;) {
		TrapContext *ctx = (TrapContext*)read_comm_pipe_pvoid_blocking(&hfpd->requests);
		uae_u8  *iobuf = (uae_u8*)read_comm_pipe_pvoid_blocking(&hfpd->requests);
		uaecptr request = (uaecptr)read_comm_pipe_u32_blocking (&hfpd->requests);
		uae_sem_wait (&change_sem);
		if (!request) {
			hfpd->thread_running = 0;
			uae_sem_post (&hfpd->sync_sem);
			uae_sem_post (&change_sem);
			return 0;
		} else if (hardfile_do_io(ctx, get_hardfile_data_controller((int)(hfpd - &hardfpd[0])), hfpd, iobuf, request) == 0) {
			put_byte_host(iobuf + 30, get_byte_host(iobuf + 30) & ~1);
			trap_put_bytes(ctx, iobuf + 8, request + 8, 48 - 8);
			release_async_request(hfpd, request);
			uae_ReplyMsg(request);
		} else {
			hf_log2 (_T("async request %08X\n"), request);
			trap_put_bytes(ctx, iobuf + 8, request + 8, 48 - 8);
		}
		trap_background_set_complete(ctx);
		uae_sem_post (&change_sem);
	}
}

void hardfile_reset (void)
{
	int i, j;
	struct hardfileprivdata *hfpd;

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		hfpd = &hardfpd[i];
		if (hfpd->base && valid_address (hfpd->base, 36) && get_word (hfpd->base + 32) > 0) {
			for (j = 0; j < MAX_ASYNC_REQUESTS; j++) {
				uaecptr request;
				if ((request = hfpd->d_request[j]))
					abort_async (hfpd, request, 0, 0);
			}
		}
		memset (hfpd, 0, sizeof (struct hardfileprivdata));
	}
}

void hardfile_install (void)
{
	uae_u32 functable, datatable;
	uae_u32 initcode, openfunc, closefunc, expungefunc;
	uae_u32 beginiofunc, abortiofunc;

	uae_sem_init (&change_sem, 0, 1);

	ROM_hardfile_resname = ds (currprefs.uaescsidevmode == 1 ? _T("scsi.device") : _T("uaehf.device"));
	ROM_hardfile_resid = ds (_T("UAE hardfile.device 0.6"));

	nscmd_cmd = here ();
	dw (NSCMD_DEVICEQUERY);
	dw (CMD_RESET);
	dw (CMD_READ);
	dw (CMD_WRITE);
	dw (CMD_UPDATE);
	dw (CMD_CLEAR);
	dw (CMD_START);
	dw (CMD_STOP);
	dw (CMD_FLUSH);
	dw (CMD_MOTOR);
	dw (CMD_SEEK);
	dw (CMD_FORMAT);
	dw (CMD_REMOVE);
	dw (CMD_CHANGENUM);
	dw (CMD_CHANGESTATE);
	dw (CMD_PROTSTATUS);
	dw (CMD_GETDRIVETYPE);
	dw (CMD_GETGEOMETRY);
	dw (CMD_ADDCHANGEINT);
	dw (CMD_REMCHANGEINT);
	dw (HD_SCSICMD);
	dw (NSCMD_TD_READ64);
	dw (NSCMD_TD_WRITE64);
	dw (NSCMD_TD_SEEK64);
	dw (NSCMD_TD_FORMAT64);
	dw (0);

	/* initcode */
	initcode = filesys_initcode;

	/* Open */
	openfunc = here ();
	calltrap (deftrap (hardfile_open)); dw (RTS);

	/* Close */
	closefunc = here ();
	calltrap (deftrap (hardfile_close)); dw (RTS);

	/* Expunge */
	expungefunc = here ();
	calltrap (deftrap (hardfile_expunge)); dw (RTS);

	/* BeginIO */
	beginiofunc = here ();
	calltrap (deftrap (hardfile_beginio));
	dw (RTS);

	/* AbortIO */
	abortiofunc = here ();
	calltrap (deftrap (hardfile_abortio)); dw (RTS);

	/* FuncTable */
	functable = here ();
	dl (openfunc); /* Open */
	dl (closefunc); /* Close */
	dl (expungefunc); /* Expunge */
	dl (EXPANSION_nullfunc); /* Null */
	dl (beginiofunc); /* BeginIO */
	dl (abortiofunc); /* AbortIO */
	dl (0xFFFFFFFFul); /* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000); /* INITBYTE */
	dw (0x0008); /* LN_TYPE */
	dw (0x0300); /* NT_DEVICE */
	dw (0xC000); /* INITLONG */
	dw (0x000A); /* LN_NAME */
	dl (ROM_hardfile_resname);
	dw (0xE000); /* INITBYTE */
	dw (0x000E); /* LIB_FLAGS */
	dw (0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
	dw (0xD000); /* INITWORD */
	dw (0x0014); /* LIB_VERSION */
	dw (0x0032); /* 50 */
	dw (0xD000);
	dw (0x0016); /* LIB_REVISION */
	dw (0x0001);
	dw (0xC000);
	dw (0x0018); /* LIB_IDSTRING */
	dl (ROM_hardfile_resid);
	dw (0x0000); /* end of table */

	ROM_hardfile_init = here ();
	dl (0x00000100); /* ??? */
	dl (functable);
	dl (datatable);
	filesys_initcode_ptr = here();
	filesys_initcode_real = initcode;
	dl (initcode);
}
