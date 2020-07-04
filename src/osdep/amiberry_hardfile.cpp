#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "filesys.h"
#include "zfile.h"
#include "uae.h"


struct hardfilehandle
{
	int zfile;
	struct zfile *zf;
	FILE *h;
};

struct uae_driveinfo {
	TCHAR vendor_id[128];
	TCHAR product_id[128];
	TCHAR product_rev[128];
	TCHAR product_serial[128];
	TCHAR device_name[2048];
	TCHAR device_path[2048];
	uae_u64 size;
	uae_u64 offset;
	int bytespersector;
	int removablemedia;
	int nomedia;
	int dangerous;
	int readonly;
};

#define HDF_HANDLE_WIN32  1
#define HDF_HANDLE_ZFILE 2
#define HDF_HANDLE_LINUX 3
#undef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE NULL

#define CACHE_SIZE 16384
#define CACHE_FLUSH_TIME 5

/* safety check: only accept drives that:
* - contain RDSK in block 0
* - block 0 is zeroed
*/

int harddrive_dangerous, do_rdbdump;
static struct uae_driveinfo uae_drives[MAX_FILESYSTEM_UNITS];

static void rdbdump(FILE* h, uae_u64 offset, uae_u8* buf, int blocksize)
{
	static int cnt = 1;
	int i, blocks;
	char name[100];
	FILE* f;

	blocks = (buf[132] << 24) | (buf[133] << 16) | (buf[134] << 8) | (buf[135] << 0);
	if (blocks < 0 || blocks > 100000)
		return;
	_stprintf(name, "rdb_dump_%d.rdb", cnt);
	f = uae_tfopen(name, "wb");
	if (!f)
		return;
	for (i = 0; i <= blocks; i++) {
		if (_fseeki64(h, offset, SEEK_SET) != 0)
			break;
		int outlen = fread(buf, 1, blocksize, h);
		if (outlen != blocksize) {
			write_log("rdbdump: warning: read %d bytes (not blocksize %d)\n",
				outlen, blocksize);
		}
		fwrite(buf, 1, blocksize, f);
		offset += blocksize;
	}
	fclose(f);
	cnt++;
}

static int ismounted(FILE* f) {
	int mounted;
	//mounted = 1;
	mounted = 0;
	return mounted;
}

#define CA "Commodore\0Amiga\0"
static int safetycheck(FILE* h, const char* name, uae_u64 offset, uae_u8* buf, int blocksize)
{
	int i, j, blocks = 63, empty = 1;
	long outlen;

	for (j = 0; j < blocks; j++) {
		if (_fseeki64(h, offset, SEEK_SET) != 0) {
			write_log("hd ignored, SetFilePointer failed, error %d\n", errno);
			return 1;
		}
		memset(buf, 0xaa, blocksize);
		outlen = fread(buf, 1, blocksize, h);
		if (outlen != blocksize) {
			write_log("hd ignored, read error %d!\n", errno);
			return 2;
		}
		if (j == 0 && offset > 0)
			return -5;
		if (j == 0 && buf[0] == 0x39 && buf[1] == 0x10 && buf[2] == 0xd3 && buf[3] == 0x12) {
			// ADIDE "CPRM" hidden block..
			if (do_rdbdump)
				rdbdump(h, offset, buf, blocksize);
			write_log("hd accepted (adide rdb detected at block %d)\n", j);
			return -3;
		}
		if (!memcmp(buf, "RDSK", 4) || !memcmp(buf, "DRKS", 4)) {
			if (do_rdbdump)
				rdbdump(h, offset, buf, blocksize);
			write_log("hd accepted (rdb detected at block %d)\n", j);
			return -1;
		}

		if (!memcmp(buf + 2, "CIS@", 4) && !memcmp(buf + 16, CA, strlen(CA))) {
			write_log("hd accepted (PCMCIA RAM)\n");
			return -2;
		}
		if (j == 0) {
			for (i = 0; i < blocksize; i++) {
				if (buf[i])
					empty = 0;
			}
		}
		offset += blocksize;
	}
	if (!empty) {
		int mounted;
		mounted = ismounted(h);
		if (!mounted) {
			write_log("hd accepted, not empty and not mounted in Windows\n");
			return -8;
		}
		if (mounted < 0) {
			write_log("hd ignored, NTFS partitions\n");
			return 0;
		}
		if (harddrive_dangerous == 0x1234dead)
			return -6;
		write_log("hd ignored, not empty and no RDB detected or Windows mounted\n");
		return 0;
	}
	write_log("hd accepted (empty)\n");
	return -9;
}

static int isharddrive(const TCHAR* name)
{
	int i;

	for (i = 0; i < hdf_getnumharddrives(); i++) {
		if (!_tcscmp(uae_drives[i].device_name, name))
			return i;
	}
	return -1;
}

static const TCHAR *hdz[] = { _T("hdz"), _T("zip"), _T("7z"), nullptr };

int hdf_open_target(struct hardfiledata *hfd, const TCHAR *pname)
{
	FILE *h = INVALID_HANDLE_VALUE;
	int i;
	struct uae_driveinfo* udi;
	char* name = strdup(pname);
	
	hfd->flags = 0;
	hfd->drive_empty = 0;
	hdf_close(hfd);
	hfd->cache = (uae_u8*)malloc(CACHE_SIZE);
	hfd->cache_valid = 0;
	hfd->virtual_size = 0;
	hfd->virtual_rdb = nullptr;
	if (!hfd->cache)
	{
		write_log("VirtualAlloc(%d) failed in hdf_open_target, error %d\n", CACHE_SIZE, errno);
		goto end;
	}
	hfd->handle = xcalloc(struct hardfilehandle, 1);
	hfd->handle->h = INVALID_HANDLE_VALUE;
	write_log(_T("hfd attempting to open: '%s'\n"), name);
	if (_tcslen(name) > 4 && !_tcsncmp(name, "HD_", 3)) {
		hdf_init_target();
		i = isharddrive(name);
		if (i >= 0) {
			udi = &uae_drives[i];
			hfd->flags = HFD_FLAGS_REALDRIVE;
			if (udi->nomedia)
				hfd->drive_empty = -1;
			if (udi->readonly)
				hfd->ci.readonly = 1;
			h = uae_tfopen(udi->device_path, hfd->ci.readonly ? "rb" : "r+b");
			hfd->handle->h = h;
			if (h == INVALID_HANDLE_VALUE)
				goto end;
			_tcsncpy(hfd->vendor_id, udi->vendor_id, 8);
			_tcsncpy(hfd->product_id, udi->product_id, 16);
			_tcsncpy(hfd->product_rev, udi->product_rev, 4);
			hfd->offset = udi->offset;
			hfd->physsize = hfd->virtsize = udi->size;
			hfd->ci.blocksize = udi->bytespersector;
			if (hfd->offset == 0 && !hfd->drive_empty) {
				int sf = safetycheck(hfd->handle->h, udi->device_path, 0, hfd->cache, hfd->ci.blocksize);
				if (sf > 0)
					goto end;
				if (sf == 0 && !hfd->ci.readonly && harddrive_dangerous != 0x1234dead) {
					write_log("'%s' forced read-only, safetycheck enabled\n", udi->device_path);
					hfd->dangerous = 1;
					// clear GENERIC_WRITE
					fclose(h);
					h = uae_tfopen(udi->device_path, "r+b");
					hfd->handle->h = h;
					if (h == INVALID_HANDLE_VALUE)
						goto end;
				}
			}
			hfd->handle_valid = HDF_HANDLE_LINUX;
			hfd->emptyname = strdup(name);
		}
		else {
			hfd->flags = HFD_FLAGS_REALDRIVE;
			hfd->drive_empty = -1;
			hfd->emptyname = strdup(name);
		}
	}
	else {
		int zmode = 0;
		char* ext = _tcsrchr(name, '.');
		if (ext != NULL) {
			ext++;
			for (i = 0; hdz[i]; i++) {
				if (!_tcsicmp(ext, hdz[i]))
					zmode = 1;
			}
		}
		h = uae_tfopen(name, hfd->ci.readonly ? "rb" : "r+b");
		if (h == INVALID_HANDLE_VALUE)
			goto end;
		hfd->handle->h = h;
		i = _tcslen(name) - 1;
		while (i >= 0) {
			if ((i > 0 && (name[i - 1] == '/' || name[i - 1] == '\\')) || i == 0) {
				_tcscpy(hfd->vendor_id, "UAE");
				_tcsncpy(hfd->product_id, name + i, 15);
				_tcscpy(hfd->product_rev, "0.3");
				break;
			}
			i--;
		}
		if (h != INVALID_HANDLE_VALUE) {
			// determine size of hdf file
			int ret;
			off_t low = -1;

#if defined(MACOSX) || defined(OPENBSD)
			// check type of file
			struct stat st;
			ret = stat(name, &st);
			if (ret) {
				write_log("osx: can't stat '%s'\n", name);
				goto end;
			}
			// block devices need special handling on BSD and OSX
			if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
				int fh = fileno(h);
#if defined(MACOSX)
				uint32_t block_size;
				uint64_t block_count;
				// get number of blocks
				ret = ioctl(fh, DKIOCGETBLOCKCOUNT, &block_count);
				if (ret) {
					write_log("osx: can't get block count of '%s' (%d)\n",
						name, fh);
					goto end;
				}
				// get block size
				ret = ioctl(fh, DKIOCGETBLOCKSIZE, &block_size);
				if (ret) {
					write_log("osx: can't get block size of '%s' (%d)\n",
						name, fh);
					goto end;
				}
				write_log("osx: found raw device: block_size=%u "
					"block_count=%llu\n", block_size, block_count);
				low = block_size * block_count;
#elif defined(OPENBSD)
				struct disklabel label;
				if (ioctl(fh, DIOCGDINFO, &label) < 0) {
					write_log("openbsd: can't get disklabel of '%s' (%d)\n", name, fh);
					goto end;
				}
				write_log("openbsd: bytes per sector: %u\n", label.d_secsize);
				write_log("openbsd: sectors per unit: %u\n", label.d_secperunit);
				low = label.d_secsize * label.d_secperunit;
				write_log("openbsd: total bytes: %llu\n", low);
#endif
			}
#endif // OPENBSD || MACOSX

			if (low == -1) {
				// assuming regular file; seek to end and ftell
				ret = _fseeki64(h, 0, SEEK_END);
				if (ret)
					goto end;
				low = _ftelli64(h);
				if (low == -1)
					goto end;
			}

			low &= ~(hfd->ci.blocksize - 1);
			hfd->physsize = hfd->virtsize = low;
			hfd->handle_valid = HDF_HANDLE_LINUX;
			if (hfd->physsize < 64 * 1024 * 1024 && zmode) {
				write_log("HDF '%s' re-opened in zfile-mode\n", name);
				fclose(h);
				hfd->handle->h = INVALID_HANDLE_VALUE;
				hfd->handle->zf = zfile_fopen(name, hfd->ci.readonly ? "rb" : "r+b", ZFD_NORMAL);
				hfd->handle->zfile = 1;
				if (!h)
					goto end;
				zfile_fseek(hfd->handle->zf, 0, SEEK_END);
				hfd->physsize = hfd->virtsize = zfile_ftell(hfd->handle->zf);
				zfile_fseek(hfd->handle->zf, 0, SEEK_SET);
				hfd->handle_valid = HDF_HANDLE_ZFILE;
			}
		}
		else {
			write_log("HDF '%s' failed to open. error = %d\n", name, errno);
		}
	}
	if (hfd->handle_valid || hfd->drive_empty) {
		write_log("HDF '%s' opened, size=%dK mode=%d empty=%d\n",
			name, (int)(hfd->physsize / 1024), hfd->handle_valid, hfd->drive_empty);
		return 1;
	}
end:
	hdf_close(hfd);
	xfree(name);
	return 0;
}

void hdf_close_target(struct hardfiledata* hfd) {
	write_log("hdf_close_target\n");
	if (hfd->handle && hfd->handle->h) {
		write_log("closing file handle %p\n", hfd->handle->h);
		fclose(hfd->handle->h);
	}
	//freehandle (hfd->handle);
	xfree(hfd->handle);
	xfree(hfd->emptyname);
	hfd->emptyname = NULL;
	hfd->handle = NULL;
	hfd->handle_valid = 0;
	if (hfd->cache)
		xfree(hfd->cache);
	xfree(hfd->virtual_rdb);
	hfd->virtual_rdb = 0;
	hfd->virtual_size = 0;
	hfd->cache = 0;
	hfd->cache_valid = 0;
	hfd->drive_empty = 0;
	hfd->dangerous = 0;
}

int hdf_dup_target(struct hardfiledata* dhfd, const struct hardfiledata* shfd)
{
	if (!shfd->handle_valid)
		return 0;

	return 0;
}

static int hdf_seek(struct hardfiledata *hfd, uae_u64 offset)
{
	if (hfd->handle_valid == 0)
	{
		target_startup_msg(_T("Internal error"), _T("hd: hdf handle is not valid."));
		abort();
	}
	if (offset >= hfd->physsize - hfd->virtual_size)
	{
		write_log(_T("hd: tried to seek out of bounds! (%I64X >= %I64X - %I64X)\n"), offset, hfd->physsize, hfd->virtual_size);
		target_startup_msg(_T("Internal error"), _T("hd: tried to seek out of bounds."));
		abort();
	}
	offset += hfd->offset;
	if (offset & (hfd->ci.blocksize - 1))
	{
		write_log(_T("hd: poscheck failed, offset=%I64X not aligned to blocksize=%d! (%I64X & %04X = %04X)\n"),
			offset, hfd->ci.blocksize, offset, hfd->ci.blocksize, offset & (hfd->ci.blocksize - 1));
		target_startup_msg(_T("Internal error"), _T("hd: poscheck failed."));
		abort();
	}
	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	{
		auto ret = _fseeki64(hfd->handle->h, offset, SEEK_SET);
		if (ret != 0)
		{
			write_log("hdf_seek failed\n");
			return -1;
		}
	}
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
	{
		zfile_fseek(hfd->handle->zf, long(offset), SEEK_SET);
	}
	return 0;
}

static void poscheck(struct hardfiledata *hfd, int len)
{
	int ret;
	uae_u64 pos = 0;

	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	{
		ret = _fseeki64(hfd->handle->h, 0, SEEK_CUR);
		if (ret)
		{
			write_log(_T("hd: poscheck failed. seek failure"));
			target_startup_msg(_T("Internal error"), _T("hd: poscheck failed. seek failure."));
			abort();
		}
		pos = _ftelli64(hfd->handle->h);
	}
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
	{
		pos = zfile_ftell(hfd->handle->zf);
	}
	if (len < 0)
	{
		write_log(_T("hd: poscheck failed, negative length! (%d)"), len);
		target_startup_msg(_T("Internal error"), _T("hd: poscheck failed, negative length."));
		abort();
	}
	if (pos < hfd->offset)
	{
		write_log(_T("hd: poscheck failed, offset out of bounds! (%I64d < %I64d)"), pos, hfd->offset);
		target_startup_msg(_T("Internal error"), _T("hd: hd: poscheck failed, offset out of bounds."));
		abort();
	}
	if (pos >= hfd->offset + hfd->physsize - hfd->virtual_size || pos >= hfd->offset + hfd->physsize + len - hfd->virtual_size)
	{
		write_log(_T("hd: poscheck failed, offset out of bounds! (%I64d >= %I64d, LEN=%d)"), pos, hfd->offset + hfd->physsize, len);
		target_startup_msg(_T("Internal error"), _T("hd: hd: poscheck failed, offset out of bounds."));
		abort();
	}
	if (pos & (hfd->ci.blocksize - 1))
	{
		write_log(_T("hd: poscheck failed, offset not aligned to blocksize! (%I64X & %04X = %04X\n"), pos, hfd->ci.blocksize, pos & hfd->ci.blocksize);
		target_startup_msg(_T("Internal error"), _T("hd: poscheck failed, offset not aligned to blocksize."));
		abort();
	}
}

static int isincache(struct hardfiledata *hfd, uae_u64 offset, int len)
{
	if (!hfd->cache_valid)
		return -1;
	if (offset >= hfd->cache_offset && offset + len <= hfd->cache_offset + CACHE_SIZE)
		return int(offset - hfd->cache_offset);
	return -1;
}

static int hdf_read_2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	auto outlen = 0;

	if (offset == 0)
		hfd->cache_valid = 0;
	auto coffset = isincache(hfd, offset, len);
	if (coffset >= 0)
	{
		memcpy(buffer, hfd->cache + coffset, len);
		return len;
	}
	hfd->cache_offset = offset;
	if (offset + CACHE_SIZE > hfd->offset + (hfd->physsize - hfd->virtual_size))
		hfd->cache_offset = hfd->offset + (hfd->physsize - hfd->virtual_size) - CACHE_SIZE;
	hdf_seek(hfd, hfd->cache_offset);
	poscheck(hfd, CACHE_SIZE);
	if (hfd->handle_valid == HDF_HANDLE_LINUX)
		outlen = fread(hfd->cache, 1, CACHE_SIZE, hfd->handle->h);
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
		outlen = zfile_fread(hfd->cache, 1, CACHE_SIZE, hfd->handle->zf);
	hfd->cache_valid = 0;
	if (outlen != CACHE_SIZE)
		return 0;
	hfd->cache_valid = 1;
	coffset = isincache(hfd, offset, len);
	if (coffset >= 0)
	{
		memcpy(buffer, hfd->cache + coffset, len);
		return len;
	}
	write_log(_T("hdf_read: cache bug! offset=%I64d len=%d\n"), offset, len);
	hfd->cache_valid = 0;
	return 0;
}

int hdf_read_target(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int got = 0;
	uae_u8 *p = (uae_u8*)buffer;

	if (hfd->drive_empty)
		return 0;

	if (offset < hfd->virtual_size)
	{
		uae_u64 len2 = offset + len <= hfd->virtual_size ? len : hfd->virtual_size - offset;
		if (!hfd->virtual_rdb)
			return 0;
		memcpy(buffer, hfd->virtual_rdb + offset, len2);
		len -= len2;
		if (len <= 0)
			return len2;
	}
	offset -= hfd->virtual_size;

	while (len > 0)
	{
		int maxlen;
		int ret = 0;
		if (hfd->physsize < CACHE_SIZE)
		{
			hfd->cache_valid = 0;
			if (hdf_seek(hfd, offset))
				return got;
			if (hfd->physsize)
				poscheck(hfd, len);
			if (hfd->handle_valid == HDF_HANDLE_LINUX)
			{
				ret = fread(hfd->cache, 1, len, hfd->handle->h);
				memcpy(buffer, hfd->cache, ret);
			}
			else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
			{
				ret = zfile_fread(buffer, 1, len, hfd->handle->zf);
			}
			maxlen = len;
		}
		else
		{
			maxlen = len > CACHE_SIZE ? CACHE_SIZE : len;
			ret = hdf_read_2(hfd, p, offset, maxlen);
		}
		got += ret;
		if (ret != maxlen)
			return got;
		offset += maxlen;
		p += maxlen;
		len -= maxlen;
	}
	return got;
}

static int hdf_write_2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	auto outlen = 0;

	if (hfd->ci.readonly)
		return 0;
	if (hfd->dangerous)
		return 0;
	if (len == 0)
		return 0;

	hfd->cache_valid = 0;
	hdf_seek(hfd, offset);
	poscheck(hfd, len);
	memcpy(hfd->cache, buffer, len);
	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	{
		outlen = fwrite(hfd->cache, 1, len, hfd->handle->h);
		const auto* const name = hfd->emptyname == nullptr ? _T("<unknown>") : hfd->emptyname;
		if (offset == 0)
		{
			const auto tmplen = 512;
			const auto tmp = (uae_u8*)xmalloc(uae_u8, tmplen);
			if (tmp)
			{
				memset(tmp, 0xa1, tmplen);
				hdf_seek(hfd, offset);
				int outlen2 = fread(tmp, 1, tmplen, hfd->handle->h);
				if (memcmp(hfd->cache, tmp, tmplen) != 0 || outlen != len)
					gui_message(_T("\"%s\"\n\nblock zero write failed!"), name);
				xfree(tmp);
			}
		}
	}
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
		outlen = zfile_fwrite(hfd->cache, 1, len, hfd->handle->zf);
	return outlen;
}

int hdf_write_target(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	auto got = 0;
	auto* p = (uae_u8*)buffer;

	if (hfd->drive_empty)
		return 0;
	if (offset < hfd->virtual_size)
		return len;
	offset -= hfd->virtual_size;
	while (len > 0)
	{
		const auto maxlen = len > CACHE_SIZE ? CACHE_SIZE : len;
		const auto ret = hdf_write_2(hfd, p, offset, maxlen);
		if (ret < 0)
			return ret;
		got += ret;
		if (ret != maxlen)
			return got;
		offset += maxlen;
		p += maxlen;
		len -= maxlen;
	}
	return got;
}

int hdf_resize_target(struct hardfiledata* hfd, uae_u64 newsize)
{
	if (newsize < hfd->physsize) {
		write_log("hdf_resize_target: truncation not implemented\n");
		return 0;
	}
	if (newsize == hfd->physsize) {
		return 1;
	}
	/* Now, newsize must be larger than hfd->physsize, we seek to newsize - 1
	 * and write a single 0 byte to make the file exactly newsize bytes big. */
	if (_fseeki64(hfd->handle->h, newsize - 1, SEEK_SET) != 0) {
		write_log("hdf_resize_target: fseek failed errno %d\n", errno);
		return 0;
	}
	if (fwrite("", 1, 1, hfd->handle->h) != 1) {
		write_log("hdf_resize_target: failed to write byte at position "
			"%lld errno %d\n", newsize - 1, errno);
		return 0;
	}
	write_log("hdf_resize_target: %lld -> %lld\n", hfd->physsize, newsize);
	hfd->physsize = newsize;
	return 1;
}

static int num_drives;

static int hdf_init2(int force)
{
	static int done;

	if (done && !force)
		return num_drives;
	done = 1;
	num_drives = 0;
	return num_drives;
}

int hdf_init_target(void) {
	return hdf_init2(0);
}

int hdf_getnumharddrives(void)
{
	return num_drives;
}