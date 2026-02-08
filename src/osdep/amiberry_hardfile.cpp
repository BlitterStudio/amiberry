#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "filesys.h"
#include "zfile.h"
#include "uae.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cctype>

#ifdef __MACH__
#include <sys/stat.h>
#include <sys/disk.h>
#include <sys/mount.h>
#endif

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
	int mounted;
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
static int num_drives;

static void trim_spaces(char* s)
{
	if (!s)
		return;
	size_t len = strlen(s);
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\n' || s[len - 1] == '\r'))
		s[--len] = 0;
	size_t start = 0;
	while (s[start] == ' ' || s[start] == '\t')
		start++;
	if (start > 0)
		memmove(s, s + start, len - start + 1);
}

static uae_u64 read_uint64_file(const char* path)
{
	FILE* f = fopen(path, "r");
	if (!f)
		return 0;
	unsigned long long v = 0;
	if (fscanf(f, "%llu", &v) != 1)
		v = 0;
	fclose(f);
	return (uae_u64)v;
}

static int read_int_file(const char* path)
{
	FILE* f = fopen(path, "r");
	if (!f)
		return 0;
	int v = 0;
	if (fscanf(f, "%d", &v) != 1)
		v = 0;
	fclose(f);
	return v;
}

static bool read_string_file(const char* path, char* out, size_t outlen)
{
	FILE* f = fopen(path, "r");
	if (!f)
		return false;
	if (!fgets(out, (int)outlen, f)) {
		fclose(f);
		return false;
	}
	fclose(f);
	trim_spaces(out);
	return true;
}

static std::string format_size_bytes(uae_u64 bytes)
{
	char buf[64];
	if (bytes >= (1024ULL * 1024ULL * 1024ULL)) {
		snprintf(buf, sizeof(buf), "%.1fG", (double)bytes / (1024.0 * 1024.0 * 1024.0));
	} else if (bytes >= (1024ULL * 1024ULL)) {
		snprintf(buf, sizeof(buf), "%.1fM", (double)bytes / (1024.0 * 1024.0));
	} else {
		snprintf(buf, sizeof(buf), "%lluK", (unsigned long long)(bytes / 1024ULL));
	}
	return std::string(buf);
}

static bool is_skipped_sys_block(const char* name)
{
	if (!name || !name[0])
		return true;
	if (!strncmp(name, "loop", 4))
		return true;
	if (!strncmp(name, "ram", 3))
		return true;
	if (!strncmp(name, "zram", 4))
		return true;
	return false;
}

static bool is_partition_of(const char* disk, const char* part)
{
	if (!disk || !part)
		return false;
	const size_t disk_len = strlen(disk);
	if (strncmp(part, disk, disk_len) != 0)
		return false;
	const char* rest = part + disk_len;
	if (!*rest)
		return false;
	if (*rest == 'p')
		rest++;
	for (const char* p = rest; *p; ++p) {
		if (*p < '0' || *p > '9')
			return false;
	}
	return true;
}

static void add_drive_entry(const char* display, const char* path, uae_u64 size_bytes, int bytespersector,
	int readonly, int removable, int mounted)
{
	if (num_drives >= MAX_FILESYSTEM_UNITS)
		return;
	struct uae_driveinfo* di = &uae_drives[num_drives++];
	memset(di, 0, sizeof(*di));
	if (display)
		_tcsncpy(di->device_name, display, sizeof(di->device_name) / sizeof(TCHAR) - 1);
	if (path)
		_tcsncpy(di->device_path, path, sizeof(di->device_path) / sizeof(TCHAR) - 1);
	di->size = size_bytes;
	di->bytespersector = bytespersector > 0 ? bytespersector : 512;
	di->readonly = readonly;
	di->removablemedia = removable;
	di->mounted = mounted;
}

static void scan_harddrives_linux()
{
	DIR* d = opendir("/sys/block");
	if (!d)
		return;

	char vendor[128] = {};
	char model[128] = {};

	// Track mounted devices
	std::vector<std::string> mounted;
	FILE* mf = fopen("/proc/self/mounts", "r");
	if (!mf)
		mf = fopen("/proc/mounts", "r");
	if (mf) {
		char dev[256] = {};
		char mountpt[256] = {};
		char fstype[64] = {};
		while (fscanf(mf, "%255s %255s %63s %*s %*d %*d\n", dev, mountpt, fstype) == 3) {
			if (strncmp(dev, "/dev/", 5) == 0)
				mounted.emplace_back(dev);
		}
		fclose(mf);
	}

	std::map<std::string, bool> parent_mounted;

	struct dirent* ent;
	while ((ent = readdir(d)) != nullptr) {
		if (ent->d_name[0] == '.')
			continue;
		if (is_skipped_sys_block(ent->d_name))
			continue;

		const std::string disk(ent->d_name);
		const std::string disk_path = "/dev/" + disk;
		struct stat st{};
		if (stat(disk_path.c_str(), &st) != 0)
			continue;

		std::string sys_base = "/sys/block/" + disk;
		std::string size_path = sys_base + "/size";
		std::string ro_path = sys_base + "/ro";
		std::string rem_path = sys_base + "/removable";
		std::string hwsec_path = sys_base + "/queue/hw_sector_size";
		std::string logsec_path = sys_base + "/queue/logical_block_size";
		std::string vendor_path = sys_base + "/device/vendor";
		std::string model_path = sys_base + "/device/model";

		uae_u64 sectors = read_uint64_file(size_path.c_str());
		int readonly = read_int_file(ro_path.c_str());
		int removable = read_int_file(rem_path.c_str());
		int bytespersector = (int)read_uint64_file(hwsec_path.c_str());
		if (bytespersector <= 0)
			bytespersector = (int)read_uint64_file(logsec_path.c_str());
		if (bytespersector <= 0)
			bytespersector = 512;
		uae_u64 size_bytes = sectors * 512ULL;

		vendor[0] = model[0] = 0;
		read_string_file(vendor_path.c_str(), vendor, sizeof(vendor));
		read_string_file(model_path.c_str(), model, sizeof(model));

		std::string label;
		if (vendor[0] || model[0]) {
			label = std::string(vendor) + (vendor[0] && model[0] ? " " : "") + std::string(model);
		} else {
			label = disk;
		}

		bool mounted_self = std::find(mounted.begin(), mounted.end(), disk_path) != mounted.end();
		std::string display = label + " (" + format_size_bytes(size_bytes) + ") — " + disk_path;
		add_drive_entry(display.c_str(), disk_path.c_str(), size_bytes, bytespersector, readonly, removable, mounted_self);

		if (mounted_self)
			parent_mounted[disk] = true;

		// Partitions
		DIR* pd = opendir(sys_base.c_str());
		if (!pd)
			continue;
		struct dirent* pent;
		while ((pent = readdir(pd)) != nullptr) {
			if (pent->d_name[0] == '.')
				continue;
			if (!is_partition_of(disk.c_str(), pent->d_name))
				continue;

			const std::string part(pent->d_name);
			const std::string part_path = "/dev/" + part;
			struct stat pst{};
			if (stat(part_path.c_str(), &pst) != 0)
				continue;

			std::string p_sys = sys_base + "/" + part;
			uae_u64 p_sectors = read_uint64_file((p_sys + "/size").c_str());
			int p_readonly = read_int_file((p_sys + "/ro").c_str());
			uae_u64 p_size_bytes = p_sectors * 512ULL;
			bool p_mounted = std::find(mounted.begin(), mounted.end(), part_path) != mounted.end();
			if (p_mounted)
				parent_mounted[disk] = true;
			std::string p_display = part + " (" + format_size_bytes(p_size_bytes) + ") — " + part_path;
			add_drive_entry(p_display.c_str(), part_path.c_str(), p_size_bytes, bytespersector, p_readonly, removable, p_mounted);
		}
		closedir(pd);
	}
	closedir(d);

	// Update mounted flag for parent disks if any partition is mounted
	for (int i = 0; i < num_drives; ++i) {
		const char* path = ua(uae_drives[i].device_path);
		if (!path)
			continue;
		std::string spath(path);
		xfree((void*)path);
		if (spath.rfind("/dev/", 0) != 0)
			continue;
		std::string name = spath.substr(5);
		auto it = parent_mounted.find(name);
		if (it != parent_mounted.end() && it->second) {
			uae_drives[i].mounted = 1;
		}
	}
}

#ifdef __MACH__
static bool is_disk_name(const char* name)
{
	if (!name)
		return false;
	if (strncmp(name, "disk", 4) != 0)
		return false;
	const char* p = name + 4;
	if (!isdigit((unsigned char)*p))
		return false;
	while (isdigit((unsigned char)*p))
		p++;
	if (*p == 0)
		return true;
	if (*p != 's')
		return false;
	p++;
	if (!isdigit((unsigned char)*p))
		return false;
	while (isdigit((unsigned char)*p))
		p++;
	return *p == 0;
}

static std::string disk_parent(const std::string& name)
{
	const auto s = name.find('s');
	if (s == std::string::npos)
		return name;
	return name.substr(0, s);
}

static void scan_harddrives_macos()
{
	DIR* d = opendir("/dev");
	if (!d)
		return;

	std::vector<std::string> mounted;
	struct statfs* mntbuf = nullptr;
	int mntcount = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (int i = 0; i < mntcount; ++i) {
		if (mntbuf[i].f_mntfromname[0])
			mounted.emplace_back(mntbuf[i].f_mntfromname);
	}

	std::map<std::string, bool> parent_mounted;

	struct dirent* ent;
	while ((ent = readdir(d)) != nullptr) {
		if (!is_disk_name(ent->d_name))
			continue;
		std::string name(ent->d_name);
		std::string path = "/dev/" + name;

		struct stat st{};
		if (stat(path.c_str(), &st) != 0)
			continue;

		int fd = open(path.c_str(), O_RDONLY);
		if (fd < 0)
			continue;
		uint32_t block_size = 0;
		uint64_t block_count = 0;
		if (ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) != 0 || ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) != 0) {
			close(fd);
			continue;
		}
		close(fd);

		uae_u64 size_bytes = (uae_u64)block_size * (uae_u64)block_count;
		bool is_mounted = std::find(mounted.begin(), mounted.end(), path) != mounted.end();
		if (is_mounted)
			parent_mounted[disk_parent(name)] = true;

		std::string display = name + " (" + format_size_bytes(size_bytes) + ") — " + path;
		add_drive_entry(display.c_str(), path.c_str(), size_bytes, (int)block_size, 0, 0, is_mounted);
	}
	closedir(d);

	for (int i = 0; i < num_drives; ++i) {
		const char* path = ua(uae_drives[i].device_path);
		if (!path)
			continue;
		std::string spath(path);
		xfree((void*)path);
		if (spath.rfind("/dev/", 0) != 0)
			continue;
		std::string name = spath.substr(5);
		auto it = parent_mounted.find(disk_parent(name));
		if (it != parent_mounted.end() && it->second) {
			uae_drives[i].mounted = 1;
		}
	}
}
#endif

static int scan_harddrives(bool force)
{
	static int done;
	if (done && !force)
		return num_drives;
	done = 1;
	num_drives = 0;
	memset(uae_drives, 0, sizeof(uae_drives));

#ifdef __MACH__
	scan_harddrives_macos();
#else
	scan_harddrives_linux();
#endif

	return num_drives;
}

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
			write_log("NTFS partitions\n");
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
	struct stat st{};
	
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
	if (stat(name, &st) == 0) {
		if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode))
			hfd->flags |= 1;
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
		uae_s64 size;
#ifdef __MACH__
	// check type of file
	int ret = stat(name,&st);
	if(ret) {
		write_log ("osx: can't stat '%s'\n", name);
		goto end;
	}
		// block devices need special handling on osx
		if(S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
			uint32_t block_size;
			uint64_t block_count;
			int fh = fileno(h);
			// get number of blocks
			ret = ioctl(fh, DKIOCGETBLOCKCOUNT, &block_count);
			if(ret) {
				write_log ("osx: can't get block count of '%s'(%d)\n", name, fh);
				goto end;
			}
			// get block size
			ret = ioctl(fh, DKIOCGETBLOCKSIZE, &block_size);
			if(ret) {
				write_log ("osx: can't get block size of '%s'(%d)\n", name, fh);
				goto end;
			}
			write_log("osx: found raw device: block_size=%u block_count=%lu\n",
					  block_size, block_count);
			size = block_size * block_count;
		} else {
#endif
			uae_s64 pos = _ftelli64(h);
			_fseeki64(h, 0, SEEK_END);
			size = _ftelli64(h);
			_fseeki64(h, pos, SEEK_SET);
#ifdef __MACH__
		}
#endif
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
		if (offset >= hfd->physsize) {
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
	if (offset + CACHE_SIZE > hfd->offset + hfd->physsize) {
		hfd->cache_offset = hfd->offset + hfd->physsize - CACHE_SIZE;
	}
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

static int hdf_init2(int force)
{
	return scan_harddrives(force != 0);
}

int hdf_init_target()
{
	return hdf_init2(1);
}

int hdf_getnumharddrives()
{
	return num_drives;
}

TCHAR *hdf_getnameharddrive (int index, int flags, int *sectorsize, int *dangerousdrive, uae_u32 *outflags)
{
	if (index >= num_drives || index < 0)
		return NULL;
	if (dangerousdrive)
		*dangerousdrive = uae_drives[index].dangerous;
	if (sectorsize)
		*sectorsize = uae_drives[index].bytespersector ? uae_drives[index].bytespersector : 512;
	if (outflags)
		*outflags = (uae_drives[index].mounted ? HDF_DRIVEFLAG_MOUNTED : 0) |
			(uae_drives[index].readonly ? HDF_DRIVEFLAG_READONLY : 0) |
			(uae_drives[index].removablemedia ? HDF_DRIVEFLAG_REMOVABLE : 0);
	return my_strdup(uae_drives[index].device_name);
}

TCHAR *hdf_getpathharddrive(int index)
{
	if (index >= num_drives || index < 0)
		return NULL;
	return my_strdup(uae_drives[index].device_path);
}
