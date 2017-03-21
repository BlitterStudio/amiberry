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
#include "custom.h"
#include "newcpu.h"
#include "disk.h"
#include "autoconf.h"
#include "traps.h"
#include "filesys.h"
#include "execlib.h"
#include "native2amiga.h"
#include "gui.h"
#include "uae.h"
#include "scsi.h"
//#include "gayle.h"
#include "execio.h"
#include "zfile.h"

#ifdef WITH_CHD
#include "archivers/chd/chdtypes.h"
#include "archivers/chd/chd.h"
#endif

//#undef DEBUGME
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

#define MAX_ASYNC_REQUESTS 50
#define ASYNC_REQUEST_NONE 0
#define ASYNC_REQUEST_TEMP 1
#define ASYNC_REQUEST_CHANGEINT 10

struct hardfileprivdata {
	volatile uaecptr d_request[MAX_ASYNC_REQUESTS];
	volatile int d_request_type[MAX_ASYNC_REQUESTS];
	volatile uae_u32 d_request_data[MAX_ASYNC_REQUESTS];
	smp_comm_pipe requests;
	int thread_running;
	uae_sem_t sync_sem;
	uaecptr base;
	int changenum;
	uaecptr changeint;
};

#define HFD_VHD_DYNAMIC 3
#define HFD_VHD_FIXED 2
#define HFD_CHD 1

STATIC_INLINE uae_u32 gl(uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

static uae_sem_t change_sem;

static struct hardfileprivdata hardfpd[MAX_FILESYSTEM_UNITS];

static uae_u32 nscmd_cmd;

static void wl(uae_u8 *p, int v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v;
}
static void ww(uae_u8 *p, int v)
{
	p[0] = v >> 8;
	p[1] = v;
}
static int rl(uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}


static void getchs2(struct hardfiledata *hfd, int *cyl, int *cylsec, int *head, int *tracksec)
{
	unsigned int total = (unsigned int)(hfd->virtsize / 1024);
	int heads;
	int sectors = 63;

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
	*cylsec = sectors * heads;
	*tracksec = sectors;
	*head = heads;
}

static void getchsx(struct hardfiledata *hfd, int *cyl, int *cylsec, int *head, int *tracksec)
{
	getchs2(hfd, cyl, cylsec, head, tracksec);
	hf_log(_T("CHS: %08X-%08X %d %d %d %d %d\n"),
		(uae_u32)(hfd->virtsize >> 32), (uae_u32)hfd->virtsize,
		*cyl, *cylsec, *head, *tracksec);
}

static void getchsgeometry2(uae_u64 size, int *pcyl, int *phead, int *psectorspertrack, int mode)
{
	int sptt[4];
	int i, spt, head, cyl;
	uae_u64 total = (unsigned int)(size / 512);

	if (mode == 1) {
		// old-style head=1, spt=32 always mode
		head = 1;
		spt = 32;
		cyl = total / (head * spt);

	}
	else {

		sptt[0] = 63;
		sptt[1] = 127;
		sptt[2] = 255;
		sptt[3] = -1;

		for (i = 0; sptt[i] >= 0; i++) {
			spt = sptt[i];
			for (head = 4; head <= 16; head++) {
				cyl = total / (head * spt);
				if (size <= 512 * 1024 * 1024) {
					if (cyl <= 1023)
						break;
				}
				else {
					if (cyl < 16383)
						break;
					if (cyl < 32767 && head >= 5)
						break;
					if (cyl <= 65535)
						break;
				}
			}
			if (head <= 16)
				break;
		}

	}

	*pcyl = cyl;
	*phead = head;
	*psectorspertrack = spt;
}

void getchsgeometry(uae_u64 size, int *pcyl, int *phead, int *psectorspertrack)
{
	getchsgeometry2(size, pcyl, phead, psectorspertrack, 0);
}

void getchsgeometry_hdf(struct hardfiledata *hfd, uae_u64 size, int *pcyl, int *phead, int *psectorspertrack)
{
	uae_u8 block[512];
	int i;
	uae_u64 minsize = 512 * 1024 * 1024;

	if (size <= minsize) {
		*phead = 1;
		*psectorspertrack = 32;
	}
	memset(block, 0, sizeof block);
	if (hfd) {
		hdf_read(hfd, block, 0, 512);
		if (block[0] == 'D' && block[1] == 'O' && block[2] == 'S') {
			int mode;
			for (mode = 0; mode < 2; mode++) {
				uae_u32 rootblock;
				uae_u32 chk = 0;
				getchsgeometry2(size, pcyl, phead, psectorspertrack, mode);
				rootblock = (2 + ((*pcyl) * (*phead) * (*psectorspertrack) - 1)) / 2;
				memset(block, 0, sizeof block);
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
	getchsgeometry2(size, pcyl, phead, psectorspertrack, size <= minsize ? 1 : 2);
}

void getchspgeometry(uae_u64 total, int *pcyl, int *phead, int *psectorspertrack, bool idegeometry)
{
	uae_u64 blocks = total / 512;

	if (blocks > 16515072) {
		/* >8G, CHS=16383/16/63 */
		*pcyl = 16383;
		*phead = 16;
		*psectorspertrack = 63;
		return;
	}
	if (idegeometry) {
		*phead = 16;
		*psectorspertrack = 63;
		*pcyl = blocks / ((*psectorspertrack) * (*phead));
		return;
	}
	getchsgeometry(total, pcyl, phead, psectorspertrack);
}

static void getchshd(struct hardfiledata *hfd, int *pcyl, int *phead, int *psectorspertrack)
{
	getchspgeometry(hfd->virtsize, pcyl, phead, psectorspertrack, false);
}


static void pl(uae_u8 *p, int off, uae_u32 v)
{
	p += off * 4;
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v >> 0;
}

static void rdb_crc(uae_u8 *p)
{
	uae_u32 sum;
	int i, blocksize;

	sum = 0;
	blocksize = rl(p + 1 * 4);
	for (i = 0; i < blocksize; i++)
		sum += rl(p + i * 4);
	sum = -sum;
	pl(p, 2, sum);
}

static void create_virtual_rdb(struct hardfiledata *hfd)
{
	uae_u8 *rdb, *part, *denv;
	int cyl = hfd->ci.surfaces * hfd->ci.sectors;
	int cyls = 262144 / (cyl * 512);
	int size = cyl * cyls * 512;

	rdb = xcalloc(uae_u8, size);
	hfd->virtual_rdb = rdb;
	hfd->virtual_size = size;
	part = rdb + 512;
	pl(rdb, 0, 0x5244534b);
	pl(rdb, 1, 64);
	pl(rdb, 2, 0); // chksum
	pl(rdb, 3, 0); // hostid
	pl(rdb, 4, 512); // blockbytes
	pl(rdb, 5, 0); // flags
	pl(rdb, 6, -1); // badblock
	pl(rdb, 7, 1); // part
	pl(rdb, 8, -1); // fs
	pl(rdb, 9, -1); // driveinit
	pl(rdb, 10, -1); // reserved
	pl(rdb, 11, -1); // reserved
	pl(rdb, 12, -1); // reserved
	pl(rdb, 13, -1); // reserved
	pl(rdb, 14, -1); // reserved
	pl(rdb, 15, -1); // reserved
	pl(rdb, 16, hfd->ci.highcyl);
	pl(rdb, 17, hfd->ci.sectors);
	pl(rdb, 18, hfd->ci.surfaces);
	pl(rdb, 19, hfd->ci.interleave); // interleave
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
	pl(rdb, 33, cyl * cyls); // rdbblockshi
	pl(rdb, 34, cyls); // locyl
	pl(rdb, 35, hfd->ci.highcyl + cyls); // hicyl
	pl(rdb, 36, cyl); // cylblocks
	pl(rdb, 37, 0); // autopark
	pl(rdb, 38, 2); // highrdskblock
	pl(rdb, 39, -1); // res
	ua_copy((char*)rdb + 40 * 4, -1, hfd->vendor_id);
	ua_copy((char*)rdb + 42 * 4, -1, hfd->product_id);
	ua_copy((char*)rdb + 46 * 4, -1, _T("UAE"));
	rdb_crc(rdb);

	pl(part, 0, 0x50415254);
	pl(part, 1, 64);
	pl(part, 2, 0);
	pl(part, 3, 0);
	pl(part, 4, -1);
	pl(part, 5, 1); // bootable
	pl(part, 6, -1);
	pl(part, 7, -1);
	pl(part, 8, 0); // devflags
	part[9 * 4] = _tcslen(hfd->device_name);
	ua_copy((char*)part + 9 * 4 + 1, -1, hfd->device_name);

	denv = part + 128;
	pl(denv, 0, 80);
	pl(denv, 1, 512 / 4);
	pl(denv, 2, 0); // secorg
	pl(denv, 3, hfd->ci.surfaces);
	pl(denv, 4, hfd->ci.blocksize / 512);
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
	rdb_crc(part);

	hfd->virtsize += size;

}

void hdf_hd_close(struct hd_hardfiledata *hfd)
{
	if (!hfd)
		return;
	hdf_close(&hfd->hfd);
}

int hdf_hd_open(struct hd_hardfiledata *hfd)
{
	struct uaedev_config_info *ci = &hfd->hfd.ci;
	if (!hdf_open(&hfd->hfd))
		return 0;
	if (ci->pcyls && ci->pheads && ci->psecs) {
		hfd->cyls = ci->pcyls;
		hfd->heads = ci->pheads;
		hfd->secspertrack = ci->psecs;
	}
	else if (ci->highcyl && ci->surfaces && ci->sectors) {
		hfd->cyls = ci->highcyl;
		hfd->heads = ci->surfaces;
		hfd->secspertrack = ci->sectors;
	}
	else {
		getchshd(&hfd->hfd, &hfd->cyls, &hfd->heads, &hfd->secspertrack);
	}
	hfd->cyls_def = hfd->cyls;
	hfd->secspertrack_def = hfd->secspertrack;
	hfd->heads_def = hfd->heads;
	if (ci->surfaces && ci->sectors) {
		uae_u8 buf[512] = { 0 };
		hdf_read(&hfd->hfd, buf, 0, 512);
		if (buf[0] != 0 && memcmp(buf, _T("RDSK"), 4)) {
			ci->highcyl = (hfd->hfd.virtsize / ci->blocksize) / (ci->sectors * ci->surfaces);
			ci->dostype = rl(buf);
			create_virtual_rdb(&hfd->hfd);
			while (ci->highcyl * ci->surfaces * ci->sectors > hfd->cyls_def * hfd->secspertrack_def * hfd->heads_def) {
				hfd->cyls_def++;
			}
		}
	}
	hfd->size = hfd->hfd.virtsize;
	return 1;
}

static uae_u32 vhd_checksum(uae_u8 *p, int offset)
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

int hdf_open(struct hardfiledata *hfd, const TCHAR *pname)
{
	uae_u8 tmp[512], tmp2[512];
	uae_u32 v;

	if ((!pname || pname[0] == 0) && hfd->ci.rootdir[0] == 0)
		return 0;
	hfd->adide = 0;
	hfd->byteswap = 0;
	hfd->hfd_type = 0;
	if (!pname)
		pname = hfd->ci.rootdir;
#ifdef WITH_CHD
	TCHAR nametmp[MAX_DPATH];
	_tcscpy(nametmp, pname);
	TCHAR *ext = _tcsrchr(nametmp, '.');
	if (ext && !_tcsicmp(ext, _T(".chd"))) {
		struct zfile *zf = zfile_fopen(nametmp, _T("rb"));
		if (zf) {
			int err;
			chd_file *cf = new chd_file();
			err = cf->open(zf, false, NULL);
			if (err != CHDERR_NONE) {
				zfile_fclose(zf);
				goto nonvhd;
			}
			hfd->chd_handle = cf;
			hfd->ci.readonly = true;
			hfd->hfd_type = HFD_CHD;
			hfd->handle_valid = -1;
			hfd->virtsize = cf->logical_bytes();
			goto nonvhd;
		}
	}
#endif
	if (!hdf_open_target(hfd, pname))
		return 0;
	if (hdf_read_target(hfd, tmp, 0, 512) != 512)
		goto nonvhd;
	v = gl(tmp + 8); // features
	if ((v & 3) != 2)
		goto nonvhd;
	v = gl(tmp + 8 + 4); // version
	if ((v >> 16) != 1)
		goto nonvhd;
	hfd->hfd_type = gl(tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4);
	if (hfd->hfd_type != HFD_VHD_FIXED && hfd->hfd_type != HFD_VHD_DYNAMIC)
		goto nonvhd;
	v = gl(tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4);
	if (v == 0)
		goto nonvhd;
	if (vhd_checksum(tmp, 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4) != v)
		goto nonvhd;
	if (hdf_read_target(hfd, tmp2, hfd->physsize - sizeof tmp2, 512) != 512)
		goto end;
	if (memcmp(tmp, tmp2, sizeof tmp))
		goto nonvhd;
	hfd->vhd_footerblock = hfd->physsize - 512;
	hfd->virtsize = (uae_u64)(gl(tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8)) << 32;
	hfd->virtsize |= gl(tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 4);
	if (hfd->hfd_type == HFD_VHD_DYNAMIC) {
		uae_u32 size;
		hfd->vhd_bamoffset = gl(tmp + 8 + 4 + 4 + 4);
		if (hfd->vhd_bamoffset == 0 || hfd->vhd_bamoffset >= hfd->physsize)
			goto end;
		if (hdf_read_target(hfd, tmp, hfd->vhd_bamoffset, 512) != 512)
			goto end;
		v = gl(tmp + 8 + 8 + 8 + 4 + 4 + 4);
		if (vhd_checksum(tmp, 8 + 8 + 8 + 4 + 4 + 4) != v)
			goto end;
		v = gl(tmp + 8 + 8 + 8);
		if ((v >> 16) != 1)
			goto end;
		hfd->vhd_blocksize = gl(tmp + 8 + 8 + 8 + 4 + 4);
		hfd->vhd_bamoffset = gl(tmp + 8 + 8 + 4);
		hfd->vhd_bamsize = (((hfd->virtsize + hfd->vhd_blocksize - 1) / hfd->vhd_blocksize) * 4 + 511) & ~511;
		size = hfd->vhd_bamoffset + hfd->vhd_bamsize;
		hfd->vhd_header = xmalloc(uae_u8, size);
		if (hdf_read_target(hfd, hfd->vhd_header, 0, size) != size)
			goto end;
		hfd->vhd_sectormap = xmalloc(uae_u8, 512);
		hfd->vhd_sectormapblock = -1;
		hfd->vhd_bitmapsize = ((hfd->vhd_blocksize / (8 * 512)) + 511) & ~511;
	}
	write_log(_T("HDF is VHD %s image, virtual size=%lldK\n"),
		hfd->hfd_type == HFD_VHD_FIXED ? _T("fixed") : _T("dynamic"),
		hfd->virtsize / 1024);
	hdf_init_cache(hfd);
	return 1;
nonvhd:
	return 1;
end:
	hdf_close_target(hfd);
	return 0;
}
int hdf_open(struct hardfiledata *hfd)
{
	return hdf_open(hfd, NULL);
}

void hdf_close(struct hardfiledata *hfd)
{
	hdf_flush_cache(hfd);
	hdf_close_target(hfd);
#ifdef WITH_CHD
	if (hfd->chd_handle) {
		chd_file *cf = (chd_file*)hfd->chd_handle;
		cf->close();
		hfd->chd_handle = NULL;
	}
#endif
	hfd->hfd_type = 0;
	xfree(hfd->vhd_header);
	hfd->vhd_header = NULL;
	xfree(hfd->vhd_sectormap);
	hfd->vhd_sectormap = NULL;
}

//int hdf_dup(struct hardfiledata *dhfd, const struct hardfiledata *shfd)
//{
//	return hdf_dup_target(dhfd, shfd);
//}

//extern int get_guid_target(uae_u8 *out);

static uae_u64 vhd_read(struct hardfiledata *hfd, void *v, uae_u64 offset, uae_u64 len)
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
		uae_u32 bamoffset = (offset / hfd->vhd_blocksize) * 4 + hfd->vhd_bamoffset;
		uae_u32 sectoroffset = gl(hfd->vhd_header + bamoffset);
		if (sectoroffset == 0xffffffff) {
			memset(dataptr, 0, 512);
			read += 512;
		}
		else {
			int bitmapoffsetbits;
			int bitmapoffsetbytes;
			uae_u64 sectormapblock;

			bitmapoffsetbits = (offset / 512) % (hfd->vhd_blocksize / 512);
			bitmapoffsetbytes = bitmapoffsetbits / 8;
			sectormapblock = sectoroffset * (uae_u64)512 + (bitmapoffsetbytes & ~511);
			if (hfd->vhd_sectormapblock != sectormapblock) {
				// read sector bitmap
				//write_log (_T("BM %08x\n"), sectormapblock);
				if (hdf_read_target(hfd, hfd->vhd_sectormap, sectormapblock, 512) != 512) {
					write_log(_T("vhd_read: bitmap read error\n"));
					return read;
				}
				hfd->vhd_sectormapblock = sectormapblock;
			}
			// block allocated in bitmap?
			if (hfd->vhd_sectormap[bitmapoffsetbytes & 511] & (1 << (7 - (bitmapoffsetbits & 7)))) {
				// read data block
				uae_u64 block = sectoroffset * (uae_u64)512 + hfd->vhd_bitmapsize + bitmapoffsetbits * 512;
				//write_log (_T("DB %08x\n"), block);
				if (hdf_read_target(hfd, dataptr, block, 512) != 512) {
					write_log(_T("vhd_read: data read error\n"));
					return read;
				}
			}
			else {
				memset(dataptr, 0, 512);
			}
			read += 512;
		}
		len -= 512;
		dataptr += 512;
		offset += 512;
	}
	return read;
}

static int vhd_write_enlarge(struct hardfiledata *hfd, uae_u32 bamoffset)
{
	uae_u8 *buf, *p;
	int len;
	uae_u32 block;
	int v;

	len = hfd->vhd_blocksize + hfd->vhd_bitmapsize + 512;
	buf = xcalloc(uae_u8, len);
	if (!hdf_resize_target(hfd, hfd->physsize + len - 512)) {
		write_log(_T("vhd_enlarge: failure\n"));
		return 0;
	}
	// add footer (same as 512 byte header)
	memcpy(buf + len - 512, hfd->vhd_header, 512);
	v = hdf_write_target(hfd, buf, hfd->vhd_footerblock, len);
	xfree(buf);
	if (v != len) {
		write_log(_T("vhd_enlarge: footer write error\n"));
		return 0;
	}
	// write new offset to BAM
	p = hfd->vhd_header + bamoffset;
	block = hfd->vhd_footerblock / 512;
	p[0] = block >> 24;
	p[1] = block >> 16;
	p[2] = block >> 8;
	p[3] = block >> 0;
	// write to disk
	if (hdf_write_target(hfd, hfd->vhd_header + hfd->vhd_bamoffset, hfd->vhd_bamoffset, hfd->vhd_bamsize) != hfd->vhd_bamsize) {
		write_log(_T("vhd_enlarge: bam write error\n"));
		return 0;
	}
	hfd->vhd_footerblock += len - 512;
	return 1;
}

static uae_u64 vhd_write(struct hardfiledata *hfd, void *v, uae_u64 offset, uae_u64 len)
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
		uae_u32 bamoffset = (offset / hfd->vhd_blocksize) * 4 + hfd->vhd_bamoffset;
		uae_u32 sectoroffset = gl(hfd->vhd_header + bamoffset);
		if (sectoroffset == 0xffffffff) {
			if (!vhd_write_enlarge(hfd, bamoffset))
				return written;
			continue;
		}
		else {
			int bitmapoffsetbits;
			int bitmapoffsetbytes;

			bitmapoffsetbits = (offset / 512) % (hfd->vhd_blocksize / 512);
			bitmapoffsetbytes = bitmapoffsetbits / 8;
			uae_u64 sectormapblock = sectoroffset * (uae_u64)512 + (bitmapoffsetbytes & ~511);
			if (hfd->vhd_sectormapblock != sectormapblock) {
				// read sector bitmap
				if (hdf_read_target(hfd, hfd->vhd_sectormap, sectormapblock, 512) != 512) {
					write_log(_T("vhd_write: bitmap read error\n"));
					return written;
				}
				hfd->vhd_sectormapblock = sectormapblock;
			}
			// write data
			if (hdf_write_target(hfd, dataptr, sectoroffset * (uae_u64)512 + hfd->vhd_bitmapsize + bitmapoffsetbits * 512, 512) != 512) {
				write_log(_T("vhd_write: data write error\n"));
				return written;
			}
			// block already allocated in bitmap?
			if (!(hfd->vhd_sectormap[bitmapoffsetbytes & 511] & (1 << (7 - (bitmapoffsetbits & 7))))) {
				// no, we need to mark it allocated and write the modified bitmap back to the disk
				hfd->vhd_sectormap[bitmapoffsetbytes & 511] |= (1 << (7 - (bitmapoffsetbits & 7)));
				if (hdf_write_target(hfd, hfd->vhd_sectormap, sectormapblock, 512) != 512) {
					write_log(_T("vhd_write: bam write error\n"));
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


int vhd_create(const TCHAR *name, uae_u64 size, uae_u32 dostype)
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
	batsize = (size + blocksize - 1) / blocksize;
	batentrysize = batsize;
	batsize *= 4;
	batsize += 511;
	batsize &= ~511;
	ret = 0;
	b = NULL;
	zf = zfile_fopen(name, _T("wb"), 0);
	if (!zf)
		goto end;
	b = xcalloc(uae_u8, 512 + 1024 + batsize + 512);
	if (zfile_fwrite(b, 512 + 1024 + batsize + 512, 1, zf) != 1)
		goto end;

	memset(&hfd, 0, sizeof hfd);
	hfd.virtsize = hfd.physsize = size;
	hfd.ci.blocksize = 512;
	strcpy((char*)b, "conectix"); // cookie
	b[0x0b] = 2; // features
	b[0x0d] = 1; // version
	b[0x10 + 6] = 2; // data offset
					 // time stamp
	tm = time(NULL) - 946684800;
	b[0x18] = tm >> 24;
	b[0x19] = tm >> 16;
	b[0x1a] = tm >> 8;
	b[0x1b] = tm >> 0;
	strcpy((char*)b + 0x1c, "vpc "); // creator application
	b[0x21] = 5; // creator version
	strcpy((char*)b + 0x24, "Wi2k"); // creator host os
									 // original and current size
	b[0x28] = b[0x30] = size >> 56;
	b[0x29] = b[0x31] = size >> 48;
	b[0x2a] = b[0x32] = size >> 40;
	b[0x2b] = b[0x33] = size >> 32;
	b[0x2c] = b[0x34] = size >> 24;
	b[0x2d] = b[0x35] = size >> 16;
	b[0x2e] = b[0x36] = size >> 8;
	b[0x2f] = b[0x37] = size >> 0;
	getchs2(&hfd, &cyl, &cylsec, &head, &tracksec);
	// cylinders
	b[0x38] = cyl >> 8;
	b[0x39] = cyl;
	// heads
	b[0x3a] = head;
	// sectors per track
	b[0x3b] = tracksec;
	// disk type
	b[0x3c + 3] = HFD_VHD_DYNAMIC;
	//get_guid_target(b + 0x44);
	crc = vhd_checksum(b, -1);
	b[0x40] = crc >> 24;
	b[0x41] = crc >> 16;
	b[0x42] = crc >> 8;
	b[0x43] = crc >> 0;

	// write header
	zfile_fseek(zf, 0, SEEK_SET);
	zfile_fwrite(b, 512, 1, zf);
	// write footer
	zfile_fseek(zf, 512 + 1024 + batsize, SEEK_SET);
	zfile_fwrite(b, 512, 1, zf);

	// dynamic disk header
	memset(b, 0, 1024);
	// cookie
	strcpy((char*)b, "cxsparse");
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
	b[0x1e] = batentrysize >> 8;
	b[0x1f] = batentrysize >> 0;
	b[0x20] = blocksize >> 24;
	b[0x21] = blocksize >> 16;
	b[0x22] = blocksize >> 8;
	b[0x23] = blocksize >> 0;
	crc = vhd_checksum(b, -1);
	b[0x24] = crc >> 24;
	b[0x25] = crc >> 16;
	b[0x26] = crc >> 8;
	b[0x27] = crc >> 0;

	// write dynamic header
	zfile_fseek(zf, 512, SEEK_SET);
	zfile_fwrite(b, 1024, 1, zf);

	// bat
	memset(b, 0, batsize);
	memset(b, 0xff, batentrysize * 4);
	zfile_fwrite(b, batsize, 1, zf);

	zfile_fclose(zf);
	zf = NULL;

	if (dostype) {
		uae_u8 bootblock[512] = { 0 };
		bootblock[0] = dostype >> 24;
		bootblock[1] = dostype >> 16;
		bootblock[2] = dostype >> 8;
		bootblock[3] = dostype >> 0;
		if (hdf_open(&hfd, name)) {
			vhd_write(&hfd, bootblock, 0, 512);
			hdf_close(&hfd);
		}
	}

	ret = 1;

end:
	xfree(b);
	zfile_fclose(zf);
	return ret;
}

static int hdf_read2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	if (hfd->hfd_type == HFD_VHD_DYNAMIC)
		return vhd_read(hfd, buffer, offset, len);
	else if (hfd->hfd_type == HFD_VHD_FIXED)
		return hdf_read_target(hfd, buffer, offset + 512, len);
#ifdef WITH_CHD
	else if (hfd->hfd_type == HFD_CHD) {
		chd_file *cf = (chd_file*)hfd->chd_handle;
		if (cf->read_bytes(offset, buffer, len) == CHDERR_NONE)
			return len;
		return 0;
	}
#endif
	else
		return hdf_read_target(hfd, buffer, offset, len);
}

static int hdf_write2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	if (hfd->hfd_type == HFD_VHD_DYNAMIC)
		return vhd_write(hfd, buffer, offset, len);
	else if (hfd->hfd_type == HFD_VHD_FIXED)
		return hdf_write_target(hfd, buffer, offset + 512, len);
#ifdef WITH_CHD
	else if (hfd->hfd_type == HFD_CHD)
		return 0;
#endif
	else
		return hdf_write_target(hfd, buffer, offset, len);
}

static void adide_decode(void *v, int len)
{
	int i;
	uae_u8 *buffer = (uae_u8*)v;
	for (i = 0; i < len; i += 2) {
		uae_u8 *b = buffer + i;
		uae_u16 w = (b[0] << 8) | (b[1] << 0);
		uae_u16 o = 0;

		if (w & 0x8000)
			o |= 0x0001;
		if (w & 0x0001)
			o |= 0x0002;

		if (w & 0x4000)
			o |= 0x0004;
		if (w & 0x0002)
			o |= 0x0008;

		if (w & 0x2000)
			o |= 0x0010;
		if (w & 0x0004)
			o |= 0x0020;

		if (w & 0x1000)
			o |= 0x0040;
		if (w & 0x0008)
			o |= 0x0080;

		if (w & 0x0800)
			o |= 0x0100;
		if (w & 0x0010)
			o |= 0x0200;

		if (w & 0x0400)
			o |= 0x0400;
		if (w & 0x0020)
			o |= 0x0800;

		if (w & 0x0200)
			o |= 0x1000;
		if (w & 0x0040)
			o |= 0x2000;

		if (w & 0x0100)
			o |= 0x4000;
		if (w & 0x0080)
			o |= 0x8000;

		b[0] = o >> 8;
		b[1] = o >> 0;
	}
}
static void adide_encode(void *v, int len)
{
	int i;
	uae_u8 *buffer = (uae_u8*)v;
	for (i = 0; i < len; i += 2) {
		uae_u8 *b = buffer + i;
		uae_u16 w = (b[0] << 8) | (b[1] << 0);
		uae_u16 o = 0;

		if (w & 0x0001)
			o |= 0x8000;
		if (w & 0x0002)
			o |= 0x0001;

		if (w & 0x0004)
			o |= 0x4000;
		if (w & 0x0008)
			o |= 0x0002;

		if (w & 0x0010)
			o |= 0x2000;
		if (w & 0x0020)
			o |= 0x0004;

		if (w & 0x0040)
			o |= 0x1000;
		if (w & 0x0080)
			o |= 0x0008;

		if (w & 0x0100)
			o |= 0x0800;
		if (w & 0x0200)
			o |= 0x0010;

		if (w & 0x0400)
			o |= 0x0400;
		if (w & 0x0800)
			o |= 0x0020;

		if (w & 0x1000)
			o |= 0x0200;
		if (w & 0x2000)
			o |= 0x0040;

		if (w & 0x4000)
			o |= 0x0100;
		if (w & 0x8000)
			o |= 0x0080;

		b[0] = o >> 8;
		b[1] = o >> 0;
	}
}

static void hdf_byteswap(void *v, int len)
{
	int i;
	uae_u8 *b = (uae_u8*)v;

	for (i = 0; i < len; i += 2) {
		uae_u8 tmp = b[i];
		b[i] = b[i + 1];
		b[i + 1] = tmp;
	}
}

int hdf_read_rdb(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int v;
	v = hdf_read(hfd, buffer, offset, len);
	if (v > 0 && offset < 16 * 512 && !hfd->byteswap && !hfd->adide) {
		uae_u8 *buf = (uae_u8*)buffer;
		bool changed = false;
		if (buf[0] == 0x39 && buf[1] == 0x10 && buf[2] == 0xd3 && buf[3] == 0x12) { // AdIDE encoded "CPRM"
			hfd->adide = 1;
			changed = true;
			write_log(_T("HDF: adide scrambling detected\n"));
		}
		else if (!memcmp(buf, "DRKS", 4)) {
			hfd->byteswap = 1;
			changed = true;
			write_log(_T("HDF: byteswapped RDB detected\n"));
		}
		if (changed)
			v = hdf_read(hfd, buffer, offset, len);
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
	}
	else {
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
	}
	else {
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
	gui_flicker_led(LED_HD, hfd->unitnum, 1);
	return hdf_read(hfd, dataptr, offset, len);
}
static uae_u64 cmd_read(struct hardfiledata *hfd, uaecptr dataptr, uae_u64 offset, uae_u64 len)
{
	addrbank *bank_data = &get_mem_bank(dataptr);
	if (!len || !bank_data || !bank_data->check(dataptr, len))
		return 0;
	return cmd_readx(hfd, bank_data->xlateaddr(dataptr), offset, len);
}
static uae_u64 cmd_writex(struct hardfiledata *hfd, uae_u8 *dataptr, uae_u64 offset, uae_u64 len)
{
	gui_flicker_led(LED_HD, hfd->unitnum, 2);
	return hdf_write(hfd, dataptr, offset, len);
}

static uae_u64 cmd_write(struct hardfiledata *hfd, uaecptr dataptr, uae_u64 offset, uae_u64 len)
{
	addrbank *bank_data = &get_mem_bank(dataptr);
	if (!len || !bank_data || !bank_data->check(dataptr, len))
		return 0;
	return cmd_writex(hfd, bank_data->xlateaddr(dataptr), offset, len);
}

static int checkbounds(struct hardfiledata *hfd, uae_u64 offset, uae_u64 len)
{
	if (offset >= hfd->virtsize)
		return 0;
	if (offset + len > hfd->virtsize)
		return 0;
	return 1;
}

static int nodisk(struct hardfiledata *hfd)
{
	if (hfd->drive_empty)
		return 1;
	return 0;
}

int scsi_hd_emulate(struct hardfiledata *hfd, struct hd_hardfiledata *hdhfd, uae_u8 *cmdbuf, int scsi_cmd_len,
	uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len)
{
	uae_u64 len, offset;
	int lr = 0, ls = 0;
	int scsi_len = -1;
	int status = 0;
	int i, lun;
	char *ss;

	*reply_len = *sense_len = 0;
	lun = cmdbuf[1] >> 5;
	if (cmdbuf[0] != 0x03 && cmdbuf[0] != 0x12 && lun) {
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x25; /* INVALID LUN */
		ls = 0x12;
		goto err;
	}
	switch (cmdbuf[0])
	{
	case 0x00: /* TEST UNIT READY */
		if (nodisk(hfd))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0x03: /* REQUEST SENSE */
		scsi_len = cmdbuf[4] > MAX_SCSI_SENSE ? MAX_SCSI_SENSE : cmdbuf[4];
		memcpy(r, hfd->scsi_sense, scsi_len);
		memset(hfd->scsi_sense, 0, MAX_SCSI_SENSE);
		break;
	case 0x08: /* READ (6) */
		if (nodisk(hfd))
			goto nodisk;
		offset = ((cmdbuf[1] & 31) << 16) | (cmdbuf[2] << 8) | cmdbuf[3];
		offset *= hfd->ci.blocksize;
		len = cmdbuf[4];
		if (!len)
			len = 256;
		len *= hfd->ci.blocksize;
		if (!checkbounds(hfd, offset, len))
			goto outofbounds;
		scsi_len = (uae_u32)cmd_readx(hfd, scsi_data, offset, len);
		break;
	case 0x0a: /* WRITE (6) */
		if (nodisk(hfd))
			goto nodisk;
		if (hfd->ci.readonly || hfd->dangerous)
			goto readprot;
		offset = ((cmdbuf[1] & 31) << 16) | (cmdbuf[2] << 8) | cmdbuf[3];
		offset *= hfd->ci.blocksize;
		len = cmdbuf[4];
		if (!len)
			len = 256;
		len *= hfd->ci.blocksize;
		if (!checkbounds(hfd, offset, len))
			goto outofbounds;
		scsi_len = (uae_u32)cmd_writex(hfd, scsi_data, offset, len);
		break;
	case 0x12: /* INQUIRY */
	{
		if ((cmdbuf[1] & 1) || cmdbuf[2] != 0)
			goto err;
		int alen = (cmdbuf[3] << 8) | cmdbuf[4];
		if (lun != 0) {
			r[0] = 0x7f;
		}
		else {
			r[0] = 0;
			if (hfd->drive_empty) {
				r[1] |= 0x80; // removable..
				r[0] |= 0x20; // not present
			}
		}
		r[2] = 2; /* supports SCSI-2 */
		r[3] = 2; /* response data format */
		r[4] = 32; /* additional length */
		r[7] = 0x20; /* 16 bit bus */
		scsi_len = lr = alen < 36 ? alen : 36;
		if (hdhfd) {
			r[2] = hdhfd->ansi_version;
			r[3] = hdhfd->ansi_version >= 2 ? 2 : 0;
		}
		ss = ua(hfd->vendor_id);
		i = 0; /* vendor id */
		while (i < 8 && ss[i]) {
			r[8 + i] = ss[i];
			i++;
		}
		while (i < 8) {
			r[8 + i] = 32;
			i++;
		}
		xfree(ss);
		ss = ua(hfd->product_id);
		i = 0; /* product id */
		while (i < 16 && ss[i]) {
			r[16 + i] = ss[i];
			i++;
		}
		while (i < 16) {
			r[16 + i] = 32;
			i++;
		}
		xfree(ss);
		ss = ua(hfd->product_rev);
		i = 0; /* product revision */
		while (i < 4 && ss[i]) {
			r[32 + i] = ss[i];
			i++;
		}
		while (i < 4) {
			r[32 + i] = 32;
			i++;
		}
		xfree(ss);
	}
	break;
	case 0x1a: /* MODE SENSE(6) */
	{
		uae_u8 *p;
		int pc = cmdbuf[2] >> 6;
		int pcode = cmdbuf[2] & 0x3f;
		int dbd = cmdbuf[1] & 8;
		int alen = cmdbuf[4];
		int cyl, cylsec, head, tracksec;
		if (nodisk(hfd))
			goto nodisk;
		if (hdhfd) {
			cyl = hdhfd->cyls;
			head = hdhfd->heads;
			tracksec = hdhfd->secspertrack;
			cylsec = 0;
		}
		else {
			getchsx(hfd, &cyl, &cylsec, &head, &tracksec);
		}
		//write_log (_T("MODE SENSE PC=%d CODE=%d DBD=%d\n"), pc, pcode, dbd);
		p = r;
		p[0] = 4 - 1;
		p[1] = 0;
		p[2] = (hfd->ci.readonly || hfd->dangerous) ? 0x80 : 0x00;
		p[3] = 0;
		p += 4;
		if (!dbd) {
			if (alen >= r[0] + 1 + 8) {
				uae_u32 blocks = (uae_u32)(hfd->virtsize / hfd->ci.blocksize);
				p[-1] = 8;
				wl(p + 0, blocks);
				wl(p + 4, hfd->ci.blocksize);
				p += 8;
			}
		}
		if (pcode == 0) {
			if (alen >= r[0] + 1 + r[3] + 4) {
				p[0] = 0;
				p[1] = 3;
				p[2] = 0x20;
				p[3] = 0;
				r[0] += p[1];
			}
		}
		else if (pcode == 3) {
			// format parameters
			if (alen >= r[0] + 1 + r[3] + 24) {
				p[0] = 3;
				p[1] = 24;
				p[3] = 1;
				p[10] = tracksec >> 8;
				p[11] = tracksec;
				p[12] = hfd->ci.blocksize >> 8;
				p[13] = hfd->ci.blocksize;
				p[15] = 1; // interleave
				p[20] = 0x80;
				r[0] += p[1];
			}
		}
		else if (pcode == 4) {
			// rigid drive geometry
			if (alen >= r[0] + 1 + r[3] + 16) {
				p[0] = 4;
				wl(p + 1, cyl);
				p[1] = 24;
				p[5] = head;
				wl(p + 13, cyl);
				ww(p + 20, 5400);
				r[0] += p[1];
			}
		}
		else {
			goto err;
		}
		r[0] += r[3];
		scsi_len = lr = r[0] + 1;
		if (scsi_len > alen)
			scsi_len = alen;
		break;
	}
	break;
	case 0x1d: /* SEND DIAGNOSTICS */
		break;
	case 0x25: /* READ_CAPACITY */
	{
		int pmi = cmdbuf[8] & 1;
		uae_u32 lba = (cmdbuf[2] << 24) | (cmdbuf[3] << 16) | (cmdbuf[4] << 8) | cmdbuf[5];
		uae_u32 blocks;
		int cyl, cylsec, head, tracksec;
		if (nodisk(hfd))
			goto nodisk;
		blocks = (uae_u32)(hfd->virtsize / hfd->ci.blocksize - 1);
		if (hdhfd) {
			cyl = hdhfd->cyls;
			head = hdhfd->heads;
			tracksec = hdhfd->secspertrack;
			cylsec = 0;
		}
		else {
			getchsx(hfd, &cyl, &cylsec, &head, &tracksec);
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
		wl(r, blocks);
		wl(r + 4, hfd->ci.blocksize);
		scsi_len = lr = 8;
	}
	break;
	case 0x28: /* READ (10) */
		if (nodisk(hfd))
			goto nodisk;
		offset = rl(cmdbuf + 2);
		offset *= hfd->ci.blocksize;
		len = rl(cmdbuf + 7 - 2) & 0xffff;
		len *= hfd->ci.blocksize;
		if (!checkbounds(hfd, offset, len))
			goto outofbounds;
		scsi_len = (uae_u32)cmd_readx(hfd, scsi_data, offset, len);
		break;
	case 0x2a: /* WRITE (10) */
		if (nodisk(hfd))
			goto nodisk;
		if (hfd->ci.readonly || hfd->dangerous)
			goto readprot;
		offset = rl(cmdbuf + 2);
		offset *= hfd->ci.blocksize;
		len = rl(cmdbuf + 7 - 2) & 0xffff;
		len *= hfd->ci.blocksize;
		if (!checkbounds(hfd, offset, len))
			goto outofbounds;
		scsi_len = (uae_u32)cmd_writex(hfd, scsi_data, offset, len);
		break;
	case 0x2f: /* VERIFY (10) */
	{
		int bytchk = cmdbuf[1] & 2;
		if (nodisk(hfd))
			goto nodisk;
		if (bytchk) {
			offset = rl(cmdbuf + 2);
			offset *= hfd->ci.blocksize;
			len = rl(cmdbuf + 7 - 2) & 0xffff;
			len *= hfd->ci.blocksize;
			uae_u8 *vb = xmalloc(uae_u8, hfd->ci.blocksize);
			if (checkbounds(hfd, offset, len)) {
				while (len > 0) {
					int readlen = cmd_readx(hfd, vb, offset, hfd->ci.blocksize);
					if (readlen != hfd->ci.blocksize || memcmp(vb, scsi_data, hfd->ci.blocksize)) {
						xfree(vb);
						goto miscompare;
					}
					scsi_data += hfd->ci.blocksize;
					offset += hfd->ci.blocksize;
					len -= hfd->ci.blocksize;
				}
			}
			xfree(vb);
		}
		scsi_len = 0;
	}
	break;
	case 0x35: /* SYNCRONIZE CACHE (10) */
		if (nodisk(hfd))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0xa8: /* READ (12) */
		if (nodisk(hfd))
			goto nodisk;
		offset = rl(cmdbuf + 2);
		offset *= hfd->ci.blocksize;
		len = rl(cmdbuf + 6);
		len *= hfd->ci.blocksize;
		if (!checkbounds(hfd, offset, len))
			goto outofbounds;
		scsi_len = (uae_u32)cmd_readx(hfd, scsi_data, offset, len);
		break;
	case 0xaa: /* WRITE (12) */
		if (nodisk(hfd))
			goto nodisk;
		if (hfd->ci.readonly || hfd->dangerous)
			goto readprot;
		offset = rl(cmdbuf + 2);
		offset *= hfd->ci.blocksize;
		len = rl(cmdbuf + 6);
		len *= hfd->ci.blocksize;
		if (!checkbounds(hfd, offset, len))
			goto outofbounds;
		scsi_len = (uae_u32)cmd_writex(hfd, scsi_data, offset, len);
		break;
	case 0x37: /* READ DEFECT DATA */
		if (nodisk(hfd))
			goto nodisk;
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 0; /* NO SENSE */
		s[12] = 0x1c; /* DEFECT LIST NOT FOUND */
		ls = 0x12;
		break;
	case 0x1b: /* START/STOP UNIT */
		scsi_len = 0;
		break;
	readprot:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 7; /* DATA PROTECT */
		s[12] = 0x27; /* WRITE PROTECTED */
		ls = 0x12;
		break;
	nodisk:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 2; /* NOT READY */
		s[12] = 0x3A; /* MEDIUM NOT PRESENT */
		ls = 0x12;
		break;

	default:
	err:
		write_log(_T("UAEHF: unsupported scsi command 0x%02X LUN=%d\n"), cmdbuf[0], lun);
	errreq:
		lr = -1;
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x24; /* ILLEGAL FIELD IN CDB */
		ls = 0x12;
		break;
	outofbounds:
		lr = -1;
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x21; /* LOGICAL BLOCK OUT OF RANGE */
		ls = 0x12;
		break;
	miscompare:
		lr = -1;
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x1d; /* MISCOMPARE DURING VERIFY OPERATION */
		ls = 0x12;
		break;
	}
	*data_len = scsi_len;
	*reply_len = lr;
	*sense_len = ls;
	if (ls > 0) {
		memset(hfd->scsi_sense, 0, MAX_SCSI_SENSE);
		memcpy(hfd->scsi_sense, s, ls);
	}
	return status;
}

static int handle_scsi(uaecptr request, struct hardfiledata *hfd)
{
	uae_u32 acmd = get_long(request + 40);
	uaecptr scsi_data = get_long(acmd + 0);
	int scsi_len = get_long(acmd + 4);
	uaecptr scsi_cmd = get_long(acmd + 12);
	uae_u16 scsi_cmd_len = get_word(acmd + 16);
	uae_u8 scsi_flags = get_byte(acmd + 20);
	uaecptr scsi_sense = get_long(acmd + 22);
	uae_u16 scsi_sense_len = get_word(acmd + 26);
	uae_u8 cmd = get_byte(scsi_cmd);
	uae_u8 cmdbuf[256];
	int status, ret = 0, reply_len, sense_len;
	uae_u32 i;
	uae_u8 reply[256], sense[256];
	uae_u8 *scsi_data_ptr = NULL;
	addrbank *bank_data = &get_mem_bank(scsi_data);

	if (bank_data  && bank_data->check(scsi_data, scsi_len))
		scsi_data_ptr = bank_data->xlateaddr(scsi_data);
	scsi_sense_len = (scsi_flags & 4) ? 4 : /* SCSIF_OLDAUTOSENSE */
		(scsi_flags & 2) ? scsi_sense_len : /* SCSIF_AUTOSENSE */
		32;
	status = 0;
	memset(reply, 0, sizeof reply);
	reply_len = 0; sense_len = 0;
	scsi_log(_T("hdf scsiemu: cmd=%02X,%d flags=%02X sense=%p,%d data=%p,%d\n"),
		cmd, scsi_cmd_len, scsi_flags, scsi_sense, scsi_sense_len, scsi_data, scsi_len);
	for (i = 0; i < scsi_cmd_len; i++) {
		cmdbuf[i] = get_byte(scsi_cmd + i);
		scsi_log(_T("%02X%c"), get_byte(scsi_cmd + i), i < scsi_cmd_len - 1 ? '.' : ' ');
	}
	scsi_log(_T("\n"));

	status = scsi_hd_emulate(hfd, NULL, cmdbuf, scsi_cmd_len, scsi_data_ptr, &scsi_len, reply, &reply_len, sense, &sense_len);

	put_word(acmd + 18, status != 0 ? 0 : scsi_cmd_len); /* fake scsi_CmdActual */
	put_byte(acmd + 21, status); /* scsi_Status */
	if (reply_len > 0) {
		scsi_log(_T("RD:"));
		i = 0;
		while (i < reply_len) {
			if (i < 24) {
				scsi_log(_T("%02X%c"), reply[i], i < reply_len - 1 ? '.' : ' ');
			}
			put_byte(scsi_data + i, reply[i]);
			i++;
		}
		scsi_log(_T("\n"));
	}
	i = 0;
	if (scsi_sense) {
		while (i < sense_len && i < scsi_sense_len) {
			put_byte(scsi_sense + i, sense[i]);
			i++;
		}
	}
	while (i < scsi_sense_len && scsi_sense) {
		put_byte(scsi_sense + i, 0);
		i++;
	}
	if (scsi_len < 0) {
		put_long(acmd + 8, 0); /* scsi_Actual */
		ret = 20;
	}
	else {
		put_long(acmd + 8, scsi_len); /* scsi_Actual */
	}
	return ret;
}


void hardfile_send_disk_change(struct hardfiledata *hfd, bool insert)
{
	int newstate = insert ? 0 : 1;

	uae_sem_wait(&change_sem);
	hardfpd[hfd->unitnum].changenum++;
	write_log(_T("uaehf.device:%d media status=%d changenum=%d\n"), hfd->unitnum, insert, hardfpd[hfd->unitnum].changenum);
	hfd->drive_empty = newstate;
	int j = 0;
	while (j < MAX_ASYNC_REQUESTS) {
		if (hardfpd[hfd->unitnum].d_request_type[j] == ASYNC_REQUEST_CHANGEINT) {
			uae_Cause(hardfpd[hfd->unitnum].d_request_data[j]);
		}
		j++;
	}
	if (hardfpd[hfd->unitnum].changeint)
		uae_Cause(hardfpd[hfd->unitnum].changeint);
	uae_sem_post(&change_sem);
}

void hardfile_do_disk_change(struct uaedev_config_data *uci, bool insert)
{
	int fsid = uci->configoffset;
	struct hardfiledata *hfd;

	if (uci->ci.controller == HD_CONTROLLER_PCMCIA_SRAM) {
		//gayle_modify_pcmcia_sram_unit(uci->ci.rootdir, uci->ci.readonly, insert); //TODO
		return;
	}
	else if (uci->ci.controller == HD_CONTROLLER_PCMCIA_IDE) {
		//gayle_modify_pcmcia_ide_unit(uci->ci.rootdir, uci->ci.readonly, insert); //TODO
		return;
	}
	hfd = get_hardfile_data(fsid);
	if (!hfd)
		return;
	hardfile_send_disk_change(hfd, insert);
}

static int add_async_request(struct hardfileprivdata *hfpd, uaecptr request, int type, uae_u32 data)
{
	int i;

	i = 0;
	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == request) {
			hfpd->d_request_type[i] = type;
			hfpd->d_request_data[i] = data;
			hf_log(_T("old async request %p (%d) added\n"), request, type);
			return 0;
		}
		i++;
	}
	i = 0;
	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == 0) {
			hfpd->d_request[i] = request;
			hfpd->d_request_type[i] = type;
			hfpd->d_request_data[i] = data;
			hf_log(_T("async request %p (%d) added (total=%d)\n"), request, type, i);
			return 0;
		}
		i++;
	}
	hf_log(_T("async request overflow %p!\n"), request);
	return -1;
}

static int release_async_request(struct hardfileprivdata *hfpd, uaecptr request)
{
	int i = 0;

	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == request) {
			int type = hfpd->d_request_type[i];
			hfpd->d_request[i] = 0;
			hfpd->d_request_data[i] = 0;
			hfpd->d_request_type[i] = 0;
			hf_log(_T("async request %p removed\n"), request);
			return type;
		}
		i++;
	}
	hf_log(_T("tried to remove non-existing request %p\n"), request);
	return -1;
}

static void abort_async(struct hardfileprivdata *hfpd, uaecptr request, int errcode, int type)
{
	int i;
	hf_log(_T("aborting async request %p\n"), request);
	i = 0;
	while (i < MAX_ASYNC_REQUESTS) {
		if (hfpd->d_request[i] == request && hfpd->d_request_type[i] == ASYNC_REQUEST_TEMP) {
			/* ASYNC_REQUEST_TEMP = request is processing */
			sleep_millis(1);
			i = 0;
			continue;
		}
		i++;
	}
	i = release_async_request(hfpd, request);
	if (i >= 0) {
		hf_log(_T("asyncronous request=%08X aborted, error=%d\n"), request, errcode);
	}
}

static void *hardfile_thread(void *devs);
static int start_thread(TrapContext *context, int unit)
{
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	if (hfpd->thread_running)
		return 1;
	memset(hfpd, 0, sizeof(struct hardfileprivdata));
	hfpd->base = m68k_areg(regs, 6);
	init_comm_pipe(&hfpd->requests, 100, 1);
	uae_sem_init(&hfpd->sync_sem, 0, 0);
	uae_start_thread(_T("hardfile"), hardfile_thread, hfpd, NULL);
	uae_sem_wait(&hfpd->sync_sem);
	return hfpd->thread_running;
}

static int mangleunit(int unit)
{
	if (unit <= 99)
		return unit;
	if (unit == 100)
		return 8;
	if (unit == 110)
		return 9;
	return -1;
}

static uae_u32 REGPARAM2 hardfile_open(TrapContext *context)
{
	uaecptr ioreq = m68k_areg(regs, 1); /* IOReq */
	int unit = mangleunit(m68k_dreg(regs, 0));
	struct hardfileprivdata *hfpd = &hardfpd[unit];
	int err = IOERR_OPENFAIL;

	/* boot device port size == 0!? KS 1.x size = 12???
	* Ignore message size, too many programs do not set it correct
	* int size = get_word (ioreq + 0x12);
	*/
	/* Check unit number */
	if (unit >= 0) {
		struct hardfiledata *hfd = get_hardfile_data(unit);
		if (hfd && (hfd->handle_valid || hfd->drive_empty) && start_thread(context, unit)) {
			put_word(hfpd->base + 32, get_word(hfpd->base + 32) + 1);
			put_long(ioreq + 24, unit); /* io_Unit */
			put_byte(ioreq + 31, 0); /* io_Error */
			put_byte(ioreq + 8, 7); /* ln_type = NT_REPLYMSG */
			hf_log(_T("hardfile_open, unit %d (%d), OK\n"), unit, m68k_dreg(regs, 0));
			return 0;
		}
	}
	if (unit < 1000 || is_hardfile(unit) == FILESYS_VIRTUAL || is_hardfile(unit) == FILESYS_CD)
		err = 50; /* HFERR_NoBoard */
	hf_log(_T("hardfile_open, unit %d (%d), ERR=%d\n"), unit, m68k_dreg(regs, 0), err);
	put_long(ioreq + 20, (uae_u32)err);
	put_byte(ioreq + 31, (uae_u8)err);
	return (uae_u32)err;
}

static uae_u32 REGPARAM2 hardfile_close(TrapContext *context)
{
	uaecptr request = m68k_areg(regs, 1); /* IOReq */
	int unit = mangleunit(get_long(request + 24));
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	if (!hfpd)
		return 0;
	put_word(hfpd->base + 32, get_word(hfpd->base + 32) - 1);
	if (get_word(hfpd->base + 32) == 0)
		write_comm_pipe_u32(&hfpd->requests, 0, 1);
	return 0;
}

static uae_u32 REGPARAM2 hardfile_expunge(TrapContext *context)
{
	return 0; /* Simply ignore this one... */
}

static void outofbounds(int cmd, uae_u64 offset, uae_u64 len, uae_u64 max)
{
	write_log(_T("UAEHF: cmd %d: out of bounds, %08X-%08X + %08X-%08X > %08X-%08X\n"), cmd,
		(uae_u32)(offset >> 32), (uae_u32)offset, (uae_u32)(len >> 32), (uae_u32)len,
		(uae_u32)(max >> 32), (uae_u32)max);
}
static void unaligned(int cmd, uae_u64 offset, uae_u64 len, int blocksize)
{
	write_log(_T("UAEHF: cmd %d: unaligned access, %08X-%08X, %08X-%08X, %08X\n"), cmd,
		(uae_u32)(offset >> 32), (uae_u32)offset, (uae_u32)(len >> 32), (uae_u32)len,
		blocksize);
}

static uae_u32 hardfile_do_io(struct hardfiledata *hfd, struct hardfileprivdata *hfpd, uaecptr request)
{
	uae_u32 dataptr, offset, actual = 0, cmd;
	uae_u64 offset64;
	int unit = get_long(request + 24);
	uae_u32 error = 0, len;
	int async = 0;
	int bmask = hfd->ci.blocksize - 1;

	cmd = get_word(request + 28); /* io_Command */
	dataptr = get_long(request + 40);
	switch (cmd)
	{
	case CMD_READ:
		if (nodisk(hfd))
			goto no_disk;
		offset = get_long(request + 44);
		len = get_long(request + 36); /* io_Length */
		if (offset & bmask) {
			unaligned(cmd, offset, len, hfd->ci.blocksize);
			goto bad_command;
		}
		if (len & bmask) {
			unaligned(cmd, offset, len, hfd->ci.blocksize);
			goto bad_len;
		}
		if (len + offset > hfd->virtsize) {
			outofbounds(cmd, offset, len, hfd->virtsize);
			goto bad_len;
		}
		actual = (uae_u32)cmd_read(hfd, dataptr, offset, len);
		break;

	case TD_READ64:
	case NSCMD_TD_READ64:
		if (nodisk(hfd))
			goto no_disk;
		offset64 = get_long(request + 44) | ((uae_u64)get_long(request + 32) << 32);
		len = get_long(request + 36); /* io_Length */
		if (offset64 & bmask) {
			unaligned(cmd, offset64, len, hfd->ci.blocksize);
			goto bad_command;
		}
		if (len & bmask) {
			unaligned(cmd, offset64, len, hfd->ci.blocksize);
			goto bad_len;
		}
		if (len + offset64 > hfd->virtsize) {
			outofbounds(cmd, offset64, len, hfd->virtsize);
			goto bad_len;
		}
		actual = (uae_u32)cmd_read(hfd, dataptr, offset64, len);
		break;

	case CMD_WRITE:
	case CMD_FORMAT: /* Format */
		if (nodisk(hfd))
			goto no_disk;
		if (hfd->ci.readonly || hfd->dangerous) {
			error = 28; /* write protect */
		}
		else {
			offset = get_long(request + 44);
			len = get_long(request + 36); /* io_Length */
			if (offset & bmask) {
				unaligned(cmd, offset, len, hfd->ci.blocksize);
				goto bad_command;
			}
			if (len & bmask) {
				unaligned(cmd, offset, len, hfd->ci.blocksize);
				goto bad_len;
			}
			if (len + offset > hfd->virtsize) {
				outofbounds(cmd, offset, len, hfd->virtsize);
				goto bad_len;
			}
			actual = (uae_u32)cmd_write(hfd, dataptr, offset, len);
		}
		break;

	case TD_WRITE64:
	case TD_FORMAT64:
	case NSCMD_TD_WRITE64:
	case NSCMD_TD_FORMAT64:
		if (nodisk(hfd))
			goto no_disk;
		if (hfd->ci.readonly || hfd->dangerous) {
			error = 28; /* write protect */
		}
		else {
			offset64 = get_long(request + 44) | ((uae_u64)get_long(request + 32) << 32);
			len = get_long(request + 36); /* io_Length */
			if (offset64 & bmask) {
				unaligned(cmd, offset64, len, hfd->ci.blocksize);
				goto bad_command;
			}
			if (len & bmask) {
				unaligned(cmd, offset64, len, hfd->ci.blocksize);
				goto bad_len;
			}
			if (len + offset64 > hfd->virtsize) {
				outofbounds(cmd, offset64, len, hfd->virtsize);
				goto bad_len;
			}
			actual = (uae_u32)cmd_write(hfd, dataptr, offset64, len);
		}
		break;

	case NSCMD_DEVICEQUERY:
		put_long(dataptr + 0, 0);
		put_long(dataptr + 4, 16); /* size */
		put_word(dataptr + 8, NSDEVTYPE_TRACKDISK);
		put_word(dataptr + 10, 0);
		put_long(dataptr + 12, nscmd_cmd);
		actual = 16;
		break;

	case CMD_GETDRIVETYPE:
		actual = DRIVE_NEWSTYLE;
		break;

	case CMD_GETNUMTRACKS:
	{
		int cyl, cylsec, head, tracksec;
		getchsx(hfd, &cyl, &cylsec, &head, &tracksec);
		actual = cyl * head;
		break;
	}

	case CMD_GETGEOMETRY:
	{
		int cyl, cylsec, head, tracksec;
		uae_u64 size;
		getchsx(hfd, &cyl, &cylsec, &head, &tracksec);
		put_long(dataptr + 0, hfd->ci.blocksize);
		size = hfd->virtsize / hfd->ci.blocksize;
		if (size > 0x00ffffffff)
			size = 0xffffffff;
		put_long(dataptr + 4, (uae_u32)size);
		put_long(dataptr + 8, cyl);
		put_long(dataptr + 12, cylsec);
		put_long(dataptr + 16, head);
		put_long(dataptr + 20, tracksec);
		put_long(dataptr + 24, 0); /* bufmemtype */
		put_byte(dataptr + 28, 0); /* type = DG_DIRECT_ACCESS */
		put_byte(dataptr + 29, 0); /* flags */
	}
	break;

	case CMD_PROTSTATUS:
		if (hfd->ci.readonly || hfd->dangerous)
			actual = -1;
		else
			actual = 0;
		break;

	case CMD_CHANGESTATE:
		actual = hfd->drive_empty ? 1 : 0;
		break;

		/* Some commands that just do nothing and return zero */
	case CMD_UPDATE:
	case CMD_CLEAR:
	case CMD_MOTOR:
	case CMD_SEEK:
	case TD_SEEK64:
	case NSCMD_TD_SEEK64:
		break;

	case CMD_REMOVE:
		hfpd->changeint = get_long(request + 40);
		break;

	case CMD_CHANGENUM:
		actual = hfpd->changenum;
		break;

	case CMD_ADDCHANGEINT:
		error = add_async_request(hfpd, request, ASYNC_REQUEST_CHANGEINT, get_long(request + 40));
		if (!error)
			async = 1;
		break;
	case CMD_REMCHANGEINT:
		release_async_request(hfpd, request);
		break;

	case HD_SCSICMD: /* SCSI */
		if (!hfd->ci.sectors && !hfd->ci.surfaces && !hfd->ci.reserved) {
			error = handle_scsi(request, hfd);
		}
		else { /* we don't want users trashing their "partition" hardfiles with hdtoolbox */
			error = IOERR_NOCMD;
			write_log(_T("UAEHF: HD_SCSICMD tried on regular HDF, unit %d\n"), unit);
		}
		break;

	case CD_EJECT:
		if (hfd->ci.sectors && hfd->ci.surfaces) {
			int len = get_long(request + 36);
			if (len) {
				if (hfd->drive_empty) {
					hardfile_media_change(hfd, NULL, true, false);
				}
				else {
					hardfile_media_change(hfd, NULL, false, false);
				}
			}
			else {
				if (hfd->drive_empty) {
					hardfile_media_change(hfd, NULL, true, false);
				}
			}
		}
		else {
			error = IOERR_NOCMD;
		}
		break;

	bad_command:
		error = IOERR_BADADDRESS;
		break;
	bad_len:
		error = IOERR_BADLENGTH;
		break;
	no_disk:
		error = 29; /* no disk */
		break;

	default:
		/* Command not understood. */
		error = IOERR_NOCMD;
		break;
	}
	put_long(request + 32, actual);
	put_byte(request + 31, error);

	hf_log2(_T("hf: unit=%d, request=%p, cmd=%d offset=%u len=%d, actual=%d error%=%d\n"), unit, request,
		get_word(request + 28), get_long(request + 44), get_long(request + 36), actual, error);

	return async;
}

static uae_u32 REGPARAM2 hardfile_abortio(TrapContext *context)
{
	uae_u32 request = m68k_areg(regs, 1);
	int unit = mangleunit(get_long(request + 24));
	struct hardfiledata *hfd = get_hardfile_data(unit);
	struct hardfileprivdata *hfpd = &hardfpd[unit];

	hf_log2(_T("uaehf.device abortio "));
	start_thread(context, unit);
	if (!hfd || !hfpd || !hfpd->thread_running) {
		put_byte(request + 31, 32);
		hf_log2(_T("error\n"));
		return get_byte(request + 31);
	}
	put_byte(request + 31, -2);
	hf_log2(_T("unit=%d, request=%08X\n"), unit, request);
	abort_async(hfpd, request, -2, 0);
	return 0;
}

static int hardfile_can_quick(uae_u32 command)
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

static int hardfile_canquick(struct hardfiledata *hfd, uaecptr request)
{
	uae_u32 command = get_word(request + 28);
	return hardfile_can_quick(command);
}

static uae_u32 REGPARAM2 hardfile_beginio(TrapContext *context)
{
	uae_u32 request = m68k_areg(regs, 1);
	uae_u8 flags = get_byte(request + 30);
	int cmd = get_word(request + 28);
	int unit = mangleunit(get_long(request + 24));
	struct hardfiledata *hfd = get_hardfile_data(unit);
	struct hardfileprivdata *hfpd = &hardfpd[unit];
	int canquick;

	put_byte(request + 8, NT_MESSAGE);
	start_thread(context, unit);
	if (!hfd || !hfpd || !hfpd->thread_running) {
		put_byte(request + 31, 32);
		return get_byte(request + 31);
	}
	put_byte(request + 31, 0);
	canquick = hardfile_canquick(hfd, request);
	if (((flags & 1) && canquick) || (canquick < 0)) {
		hf_log(_T("hf quickio unit=%d request=%p cmd=%d\n"), unit, request, cmd);
		if (hardfile_do_io(hfd, hfpd, request)) {
			hf_log2(_T("uaehf.device cmd %d bug with IO_QUICK\n"), cmd);
		}
		if (!(flags & 1))
			uae_ReplyMsg(request);
		return get_byte(request + 31);
	}
	else {
		hf_log2(_T("hf asyncio unit=%d request=%p cmd=%d\n"), unit, request, cmd);
		add_async_request(hfpd, request, ASYNC_REQUEST_TEMP, 0);
		put_byte(request + 30, get_byte(request + 30) & ~1);
		write_comm_pipe_u32(&hfpd->requests, request, 1);
		return 0;
	}
}

static void *hardfile_thread(void *devs)
{
	struct hardfileprivdata *hfpd = (struct hardfileprivdata*)devs;

	uae_set_thread_priority(NULL, 1);
	hfpd->thread_running = 1;
	uae_sem_post(&hfpd->sync_sem);
	for (;;) {
		uaecptr request = (uaecptr)read_comm_pipe_u32_blocking(&hfpd->requests);
		uae_sem_wait(&change_sem);
		if (!request) {
			hfpd->thread_running = 0;
			uae_sem_post(&hfpd->sync_sem);
			uae_sem_post(&change_sem);
			return 0;
		}
		else if (hardfile_do_io(get_hardfile_data(hfpd - &hardfpd[0]), hfpd, request) == 0) {
			put_byte(request + 30, get_byte(request + 30) & ~1);
			release_async_request(hfpd, request);
			uae_ReplyMsg(request);
		}
		else {
			hf_log2(_T("async request %08X\n"), request);
		}
		uae_sem_post(&change_sem);
	}
}

void hardfile_reset(void)
{
	int i, j;
	struct hardfileprivdata *hfpd;

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		hfpd = &hardfpd[i];
		if (hfpd->base && valid_address(hfpd->base, 36) && get_word(hfpd->base + 32) > 0) {
			for (j = 0; j < MAX_ASYNC_REQUESTS; j++) {
				uaecptr request;
				if ((request = hfpd->d_request[i]))
					abort_async(hfpd, request, 0, 0);
			}
		}
		memset(hfpd, 0, sizeof(struct hardfileprivdata));
	}
}

void hardfile_install(void)
{
	uae_u32 functable, datatable;
	uae_u32 initcode, openfunc, closefunc, expungefunc;
	uae_u32 beginiofunc, abortiofunc;

	uae_sem_init(&change_sem, 0, 1);

	ROM_hardfile_resname = ds(_T("uaehf.device"));
	ROM_hardfile_resid = ds(_T("UAE hardfile.device 0.3"));

	nscmd_cmd = here();
	dw(NSCMD_DEVICEQUERY);
	dw(CMD_RESET);
	dw(CMD_READ);
	dw(CMD_WRITE);
	dw(CMD_UPDATE);
	dw(CMD_CLEAR);
	dw(CMD_START);
	dw(CMD_STOP);
	dw(CMD_FLUSH);
	dw(CMD_MOTOR);
	dw(CMD_SEEK);
	dw(CMD_FORMAT);
	dw(CMD_REMOVE);
	dw(CMD_CHANGENUM);
	dw(CMD_CHANGESTATE);
	dw(CMD_PROTSTATUS);
	dw(CMD_GETDRIVETYPE);
	dw(CMD_GETGEOMETRY);
	dw(CMD_ADDCHANGEINT);
	dw(CMD_REMCHANGEINT);
	dw(HD_SCSICMD);
	dw(NSCMD_TD_READ64);
	dw(NSCMD_TD_WRITE64);
	dw(NSCMD_TD_SEEK64);
	dw(NSCMD_TD_FORMAT64);
	dw(0);

	/* initcode */
#if 0
	initcode = here();
	calltrap(deftrap(hardfile_init)); dw(RTS);
#else
	initcode = filesys_initcode;
#endif
	/* Open */
	openfunc = here();
	calltrap(deftrap(hardfile_open)); dw(RTS);

	/* Close */
	closefunc = here();
	calltrap(deftrap(hardfile_close)); dw(RTS);

	/* Expunge */
	expungefunc = here();
	calltrap(deftrap(hardfile_expunge)); dw(RTS);

	/* BeginIO */
	beginiofunc = here();
	calltrap(deftrap(hardfile_beginio));
	dw(RTS);

	/* AbortIO */
	abortiofunc = here();
	calltrap(deftrap(hardfile_abortio)); dw(RTS);

	/* FuncTable */
	functable = here();
	dl(openfunc); /* Open */
	dl(closefunc); /* Close */
	dl(expungefunc); /* Expunge */
	dl(EXPANSION_nullfunc); /* Null */
	dl(beginiofunc); /* BeginIO */
	dl(abortiofunc); /* AbortIO */
	dl(0xFFFFFFFFul); /* end of table */

					  /* DataTable */
	datatable = here();
	dw(0xE000); /* INITBYTE */
	dw(0x0008); /* LN_TYPE */
	dw(0x0300); /* NT_DEVICE */
	dw(0xC000); /* INITLONG */
	dw(0x000A); /* LN_NAME */
	dl(ROM_hardfile_resname);
	dw(0xE000); /* INITBYTE */
	dw(0x000E); /* LIB_FLAGS */
	dw(0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
	dw(0xD000); /* INITWORD */
	dw(0x0014); /* LIB_VERSION */
	dw(0x0004); /* 0.4 */
	dw(0xD000);
	dw(0x0016); /* LIB_REVISION */
	dw(0x0000);
	dw(0xC000);
	dw(0x0018); /* LIB_IDSTRING */
	dl(ROM_hardfile_resid);
	dw(0x0000); /* end of table */

	ROM_hardfile_init = here();
	dl(0x00000100); /* ??? */
	dl(functable);
	dl(datatable);
	dl(initcode);
}
