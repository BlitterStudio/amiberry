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

#ifdef __MACH__
#  define off64_t off_t
#  define fopen64 fopen
#  define fseeko64 fseeko
#  define ftello64 ftello
#endif

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
	char name[100];

	int blocks = (buf[132] << 24) | (buf[133] << 16) | (buf[134] << 8) | (buf[135] << 0);
	if (blocks < 0 || blocks > 100000)
		return;
	_sntprintf(name, sizeof name, "rdb_dump_%d.rdb", cnt);
	auto f = uae_tfopen(name, "wb");
	if (!f)
		return;
	for (int i = 0; i <= blocks; i++) {
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
	//mounted = 1;
	int mounted = 0;
	return mounted;
}

#define CA "Commodore\0Amiga\0"
static int safetycheck(FILE* h, const char* name, uae_u64 offset, uae_u8* buf, int blocksize)
{
	int blocks = 63, empty = 1;

	for (int j = 0; j < blocks; j++) {
		if (_fseeki64(h, offset, SEEK_SET) != 0) {
			write_log("hd ignored, SetFilePointer failed, error %d\n", errno);
			return 1;
		}
		memset(buf, 0xaa, blocksize);
		auto outlen = fread(buf, 1, blocksize, h);
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
			for (int i = 0; i < blocksize; i++) {
				if (buf[i])
					empty = 0;
			}
		}
		offset += blocksize;
	}
	if (!empty) {
		int mounted = ismounted(h);
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
	for (int i = 0; i < hdf_getnumharddrives(); i++) {
		if (!_tcscmp(uae_drives[i].device_name, name))
			return i;
	}
	return -1;
}

static const TCHAR *hdz[] = { _T("hdz"), _T("zip"), _T("7z"), nullptr };

int hdf_open_target(struct hardfiledata *hfd, const TCHAR *pname)
{
	FILE *h = nullptr;
	int i;
	char* name = my_strdup(pname);
	int zmode = 0;
	
	hfd->flags = 0;
	hfd->drive_empty = 0;
	hdf_close(hfd);
	hfd->cache = (uae_u8*)xmalloc(uae_u8, CACHE_SIZE);
	hfd->cache_valid = 0;
	hfd->virtual_size = 0;
	hfd->virtual_rdb = nullptr;
	if (!hfd->cache)
	{
		write_log("VirtualAlloc(%d) failed in hdf_open_target, error %d\n", CACHE_SIZE, errno);
		goto end;
	}
	hfd->handle = xcalloc(struct hardfilehandle, 1);
	hfd->handle->h = nullptr;
	write_log(_T("hfd attempting to open: '%s'\n"), name);

	char* ext;
	ext = _tcsrchr(name, '.');
	if (ext != nullptr) {
		ext++;
		for (i = 0; hdz[i]; i++) {
			if (!_tcsicmp(ext, hdz[i]))
				zmode = 1;
		}
	}
	h = uae_tfopen(name, hfd->ci.readonly ? "rb" : "r+b");
	if (h == nullptr && !hfd->ci.readonly) {
		h = uae_tfopen(name, "rb");
		if (h != nullptr)
			hfd->ci.readonly = true;
	}
	hfd->handle->h = h;
	i = _tcslen(name) - 1;
	while (i >= 0) {
		if ((i > 0 && (name[i - 1] == '/' || name[i - 1] == '\\')) || i == 0) {
			_tcsncpy(hfd->product_id, name + i, 15);
			break;
		}
		i--;
	}
	_tcscpy(hfd->vendor_id, _T("UAE"));
	_tcscpy(hfd->product_rev, _T("0.4"));
	if (h != nullptr) {
		uae_s64 pos = _ftelli64(h);
		_fseeki64(h, 0, SEEK_END);
		uae_s64 size = _ftelli64(h);
		_fseeki64(h, pos, SEEK_SET);

		size &= ~(hfd->ci.blocksize - 1);
		hfd->physsize = hfd->virtsize = size;
		if (hfd->physsize < hfd->ci.blocksize || hfd->physsize == 0) {
			write_log(_T("HDF '%s' is too small\n"), name);
			goto end;
		}
		hfd->handle_valid = HDF_HANDLE_LINUX;
		if (hfd->physsize < 64 * 1024 * 1024 && zmode) {
			write_log("HDF '%s' re-opened in zfile-mode\n", name);
			fclose(h);
			hfd->handle->h = nullptr;
			hfd->handle->zf = zfile_fopen(name, _T("rb"), ZFD_NORMAL);
			hfd->handle->zfile = 1;
			if (!hfd->handle->zf)
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

	if (hfd->handle_valid || hfd->drive_empty) {
		write_log("HDF '%s' opened, size=%dK mode=%d empty=%d\n",
			name, static_cast<int>(hfd->physsize / 1024), hfd->handle_valid, hfd->drive_empty);
		return 1;
	}
end:
	hdf_close(hfd);
	xfree(name);
	return 0;
}

static void freehandle(struct hardfilehandle* h)
{
	if (!h)
		return;
	if (!h->zfile && h->h != nullptr)
		fclose(h->h);
	if (h->zfile && h->zf)
		zfile_fclose(h->zf);
	h->zf = nullptr;
	h->h = nullptr;
	h->zfile = 0;
}

void hdf_close_target(struct hardfiledata* hfd) {
	write_log("hdf_close_target\n");
	freehandle (hfd->handle);
	xfree(hfd->handle);
	xfree(hfd->emptyname);
	hfd->emptyname = nullptr;
	hfd->handle = nullptr;
	hfd->handle_valid = 0;
	if (hfd->cache)
		xfree(hfd->cache);
	xfree(hfd->virtual_rdb);
	hfd->virtual_rdb = nullptr;
	hfd->virtual_size = 0;
	hfd->cache = nullptr;
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

static int hdf_seek(const struct hardfiledata *hfd, uae_u64 offset)
{
	if (hfd->handle_valid == 0)
	{
		gui_message(_T("hd: hdf handle is not valid. bug."));
		abort();
	}
	if (hfd->physsize) {
		if (offset >= hfd->physsize - hfd->virtual_size)
		{
			if (hfd->virtual_rdb)
				return -1;
			write_log(_T("hd: tried to seek out of bounds! (%I64X >= %I64X - %I64X)\n"), offset, hfd->physsize, hfd->virtual_size);
			gui_message(_T("hd: tried to seek out of bounds!"));
			abort();
		}
		offset += hfd->offset;
		if (offset & (hfd->ci.blocksize - 1))
		{
			write_log(_T("hd: poscheck failed, offset=%I64X not aligned to blocksize=%d! (%I64X & %04X = %04X)\n"),
				offset, hfd->ci.blocksize, offset, hfd->ci.blocksize, offset & (hfd->ci.blocksize - 1));
			gui_message(_T("hd: poscheck failed, offset not aligned to blocksize!"));
			abort();
		}
	}
	
	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	{
		const auto ret = _fseeki64(hfd->handle->h, offset, SEEK_SET);
		if (ret != 0)
		{
			write_log("hdf_seek failed\n");
			return -1;
		}
	}
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
	{
		zfile_fseek(hfd->handle->zf, offset, SEEK_SET);
	}
	return 0;
}

static void poscheck(const struct hardfiledata *hfd, int len)
{
	uae_u64 pos = -1;

	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	{
		const int ret = _fseeki64(hfd->handle->h, 0, SEEK_CUR);
		if (ret)
		{
			write_log(_T("hd: poscheck failed. seek failure"));
			gui_message(_T("Internal error"), _T("hd: poscheck failed. seek failure."));
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

static int isincache(const struct hardfiledata *hfd, uae_u64 offset, int len)
{
	if (!hfd->cache_valid)
		return -1;
	if (offset >= hfd->cache_offset && offset + len <= hfd->cache_offset + CACHE_SIZE)
		return static_cast<int>(offset - hfd->cache_offset);
	return -1;
}

static int hdf_read_2(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	size_t outlen = 0;

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
	if (hdf_seek(hfd, hfd->cache_offset))
		return 0;
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
	auto p = static_cast<uae_u8*>(buffer);

	if (hfd->drive_empty)
		return 0;

	while (len > 0)
	{
		int maxlen;
		size_t ret = 0;
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

static int hdf_write_2(struct hardfiledata *hfd, const void *buffer, uae_u64 offset, int len)
{
	size_t outlen = 0;

	if (hfd->ci.readonly)
		return 0;
	if (hfd->dangerous)
		return 0;
	if (len == 0)
		return 0;

	hfd->cache_valid = 0;
	if (hdf_seek(hfd, offset))
		return 0;
	poscheck(hfd, len);
	memcpy(hfd->cache, buffer, len);
	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	{
		outlen = fwrite(hfd->cache, 1, len, hfd->handle->h);
		const auto* const name = hfd->emptyname == nullptr ? _T("<unknown>") : hfd->emptyname;
		if (offset == 0)
		{
			constexpr auto tmplen = 512;
			auto* const tmp = (uae_u8*)xmalloc(uae_u8, tmplen);
			if (tmp)
			{
				int cmplen = tmplen > len ? len : tmplen;
				memset(tmp, 0xa1, tmplen);
				hdf_seek(hfd, offset);
				auto outlen2 = fread(tmp, 1, tmplen, hfd->handle->h);
				if (memcmp(hfd->cache, tmp, cmplen) != 0 || outlen != len)
					gui_message(_T("\"%s\"\n\nblock zero write failed!"), name);
				xfree(tmp);
			}
		}
	}
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
		outlen = zfile_fwrite(hfd->cache, 1, len, hfd->handle->zf);
	return static_cast<int>(outlen);
}

int hdf_write_target(struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	auto got = 0;
	auto* p = static_cast<uae_u8*>(buffer);

	if (hfd->drive_empty || hfd->physsize == 0)
		return 0;

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

int get_guid_target(uae_u8* out)
{
	return 0;
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

int hdf_init_target()
{
	return hdf_init2(0);
}

int hdf_getnumharddrives()
{
	return num_drives;
}
