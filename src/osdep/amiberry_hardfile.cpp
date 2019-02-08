#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sysdeps.h"

#include "options.h"
#include "filesys.h"
#include "zfile.h"
#include "uae.h"


struct hardfilehandle
{
	int zfile;
	struct zfile *zf;
	FILE *f;
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

#define HDF_HANDLE_FILE  1
#define HDF_HANDLE_ZFILE 2
#define HDF_HANDLE_UNKNOWN 3
#undef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE NULL

#define CACHE_SIZE 16384
#define CACHE_FLUSH_TIME 5

static const TCHAR *hdz[] = { _T("hdz"), _T("zip"), _T("7z"), nullptr };

int hdf_open_target(struct hardfiledata *hfd, const TCHAR *pname)
{
	FILE *f = INVALID_HANDLE_VALUE;
	int i;
	const auto name = my_strdup(pname);
	auto zmode = 0;
	char* ext;

	hfd->flags = 0;
	hfd->drive_empty = 0;
	hdf_close(hfd);
	hfd->cache = (uae_u8*)malloc(CACHE_SIZE);
	hfd->cache_valid = 0;
	hfd->virtual_size = 0;
	hfd->virtual_rdb = nullptr;
	if (!hfd->cache)
	{
		write_log("malloc(%d) failed in hdf_open_target, error %d\n", CACHE_SIZE, errno);
		goto end;
	}
	hfd->handle = xcalloc(struct hardfilehandle, 1);
	hfd->handle->f = INVALID_HANDLE_VALUE;
	write_log(_T("hfd attempting to open: '%s'\n"), name);

	ext = _tcsrchr(name, '.');
	if (ext != nullptr)
	{
		ext++;
		for (i = 0; hdz[i]; i++)
		{
			if (!_tcsicmp(ext, hdz[i]))
				zmode = 1;
		}
	}
	f = fopen(name, (hfd->ci.readonly ? "rb" : "r+b"));
	if (f == INVALID_HANDLE_VALUE && !hfd->ci.readonly)
	{
		f = fopen(name, "rbe");
		if (f != nullptr)
			hfd->ci.readonly = true;
		else
			goto end;
	}
	hfd->handle->f = f;
	i = _tcslen(name) - 1;
	while (i >= 0)
	{
		if ((i > 0 && (name[i - 1] == '/' || name[i - 1] == '\\')) || i == 0)
		{
			_tcscpy(hfd->vendor_id, "UAE");
			_tcsncpy(hfd->product_id, name + i, 15);
			_tcscpy(hfd->product_rev, "0.4");
			break;
		}
		i--;
	}
	if (f != INVALID_HANDLE_VALUE)
	{
		// determine size of hdf file
		int ret;
		off_t low = -1;

		if (low == -1) {
			// assuming regular file; seek to end and ftell
			ret = fseeko(f, 0, SEEK_END);
			if (ret)
				goto end;
			low = ftello(f);
			if (low == -1)
				goto end;
		}

		low &= ~(hfd->ci.blocksize - 1);
		hfd->physsize = hfd->virtsize = low;
		//if (g_debug) {
		//	write_log("set physsize = virtsize = %lld (low)\n",
		//		hfd->virtsize);
		//}
		hfd->handle_valid = HDF_HANDLE_FILE;
		if (hfd->physsize < 64 * 1024 * 1024 && zmode) {
			write_log("HDF '%s' re-opened in zfile-mode\n", name);
			fclose(f);
			hfd->handle->f = INVALID_HANDLE_VALUE;
			hfd->handle->zf = zfile_fopen(name, hfd->ci.readonly ? "rb" : "r+b", ZFD_NORMAL);
			hfd->handle->zfile = 1;
			if (!f)
				goto end;
			zfile_fseek(hfd->handle->zf, 0, SEEK_END);
			hfd->physsize = hfd->virtsize = zfile_ftell(hfd->handle->zf);
			//if (g_debug) {
			//	write_log("set physsize = virtsize = %lld\n",
			//		hfd->virtsize);
			//}
			zfile_fseek(hfd->handle->zf, 0, SEEK_SET);
			hfd->handle_valid = HDF_HANDLE_ZFILE;
		}	
	}
	else
	{
		write_log("HDF '%s' failed to open. error = %d\n", name, errno);
	}
	if (hfd->handle_valid || hfd->drive_empty)
	{
		write_log(_T("HDF '%s' opened, size=%lld mode=%d empty=%d\n"), name, hfd->physsize / 1024, hfd->handle_valid, hfd->drive_empty);
		return 1;
	}

end:
	hdf_close(hfd);
	xfree(name);
	return 0;
}

void hdf_close_target(struct hardfiledata *hfd)
{
	write_log("hdf_close_target\n");
	if (hfd->handle && hfd->handle->f) {
		write_log("closing file handle %p\n", hfd->handle->f);
		fclose(hfd->handle->f);
	}
	xfree(hfd->handle);
	xfree(hfd->emptyname);
	hfd->emptyname = nullptr;
	hfd->handle = nullptr;
	hfd->handle_valid = 0;
	if (hfd->cache)
		free(hfd->cache);
	xfree(hfd->virtual_rdb);
	hfd->virtual_rdb = nullptr;
	hfd->virtual_size = 0;
	hfd->cache = nullptr;
	hfd->cache_valid = 0;
	hfd->drive_empty = 0;
	hfd->dangerous = 0;
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
	if (hfd->handle_valid == HDF_HANDLE_FILE)
	{
		auto ret = fseeko(hfd->handle->f, offset, SEEK_SET);
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

	if (hfd->handle_valid == HDF_HANDLE_FILE)
	{
		ret = fseeko(hfd->handle->f, 0, SEEK_CUR);
		if (ret)
		{
			write_log(_T("hd: poscheck failed. seek failure"));
			target_startup_msg(_T("Internal error"), _T("hd: poscheck failed. seek failure."));
			abort();
		}
		pos = ftello(hfd->handle->f);
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
	if (hfd->handle_valid == HDF_HANDLE_FILE)
		outlen = fread(hfd->cache, 1, CACHE_SIZE, hfd->handle->f);
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
			if (hfd->handle_valid == HDF_HANDLE_FILE)
			{
				ret = fread(hfd->cache, 1, len, hfd->handle->f);
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
	if (hdf_seek(hfd, offset))
		return 0;
	poscheck(hfd, len);
	memcpy(hfd->cache, buffer, len);
	if (hfd->handle_valid == HDF_HANDLE_FILE)
	{
		const auto name = hfd->emptyname == nullptr ? static_cast<char const*>("<unknown>") : hfd->emptyname;
		outlen = fwrite(hfd->cache, 1, len, hfd->handle->f);
		if (offset == 0)
		{
			int tmplen = 512;
			uae_u8 *tmp = (uae_u8*)malloc(tmplen);
			if (tmp)
			{
				memset(tmp, 0xa1, tmplen);
				hdf_seek(hfd, offset);
				int outlen2 = fread(tmp, 1, tmplen, hfd->handle->f);
				if (memcmp(hfd->cache, tmp, tmplen) != 0 || outlen != len)
					gui_message(_T("\"%s\"\n\nblock zero write failed!"), name);
				free(tmp);
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
	uae_u8 *p = (uae_u8*)buffer;

	if (hfd->drive_empty)
		return 0;
	if (offset < hfd->virtual_size)
		return len;
	offset -= hfd->virtual_size;
	while (len > 0)
	{
		auto maxlen = len > CACHE_SIZE ? CACHE_SIZE : len;
		const int ret = hdf_write_2(hfd, p, offset, maxlen);
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
