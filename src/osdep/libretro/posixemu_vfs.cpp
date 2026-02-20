#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

#include "sysdeps.h"

#ifdef DONT_HAVE_POSIX
#undef access
#undef open
#undef close
#undef read
#undef write
#undef lseek
#undef stat
#undef mkdir
#undef rmdir
#undef unlink
#undef truncate
#undef rename
#undef chmod
#undef tmpnam
#undef opendir
#undef readdir
#undef closedir
#endif

#include "osdep/libretro/libretro_vfs.h"

namespace {
constexpr int kVfsFdBase = 10000;
constexpr int kVfsFdMax = 512;
static retro_vfs_file_handle* g_vfs_fds[kVfsFdMax] = {};

struct VfsDirWrapper {
	retro_vfs_dir_handle* handle;
	struct dirent entry;
};

static std::vector<VfsDirWrapper*> g_vfs_dirs;

static int vfs_whence(int whence)
{
	switch (whence) {
	case SEEK_SET:
		return RETRO_VFS_SEEK_POSITION_START;
	case SEEK_CUR:
		return RETRO_VFS_SEEK_POSITION_CURRENT;
	case SEEK_END:
		return RETRO_VFS_SEEK_POSITION_END;
	default:
		return RETRO_VFS_SEEK_POSITION_START;
	}
}

static int vfs_alloc_fd(retro_vfs_file_handle* handle)
{
	for (int i = 0; i < kVfsFdMax; i++) {
		if (!g_vfs_fds[i]) {
			g_vfs_fds[i] = handle;
			return kVfsFdBase + i;
		}
	}
	return -1;
}

static retro_vfs_file_handle* vfs_get_fd(int fd)
{
	if (fd < kVfsFdBase)
		return nullptr;
	const int idx = fd - kVfsFdBase;
	if (idx < 0 || idx >= kVfsFdMax)
		return nullptr;
	return g_vfs_fds[idx];
}

static void vfs_release_fd(int fd)
{
	if (fd < kVfsFdBase)
		return;
	const int idx = fd - kVfsFdBase;
	if (idx < 0 || idx >= kVfsFdMax)
		return;
	g_vfs_fds[idx] = nullptr;
}

static VfsDirWrapper* find_vfs_dir(DIR* dirp)
{
	for (auto* wrapper : g_vfs_dirs) {
		if (reinterpret_cast<DIR*>(wrapper) == dirp)
			return wrapper;
	}
	return nullptr;
}

static char* to_fs_path(const TCHAR* path)
{
	if (!path)
		return nullptr;
	return ua_fs(path, -1);
}

static unsigned vfs_mode_from_flags(int flags)
{
	unsigned mode = 0;
	if ((flags & O_RDWR) == O_RDWR)
		mode = RETRO_VFS_FILE_ACCESS_READ_WRITE;
	else if (flags & O_WRONLY)
		mode = RETRO_VFS_FILE_ACCESS_WRITE;
	else
		mode = RETRO_VFS_FILE_ACCESS_READ;

	if ((mode & RETRO_VFS_FILE_ACCESS_WRITE) && !(flags & O_TRUNC))
		mode |= RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;

	return mode;
}
} // namespace

extern "C" int posixemu_access(const TCHAR* path, int)
{
	STAT st;
	return posixemu_stat(path, &st);
}

extern "C" int posixemu_open(const TCHAR* path, int flags, int mode)
{
	if (!path)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->open) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			const unsigned vfs_mode = vfs_mode_from_flags(flags);
			retro_vfs_file_handle* handle = vfs->open(fs_path, vfs_mode, RETRO_VFS_FILE_ACCESS_HINT_NONE);
			free(fs_path);
			if (handle) {
				if ((flags & O_APPEND) && vfs->seek)
					vfs->seek(handle, 0, RETRO_VFS_SEEK_POSITION_END);
				if ((flags & O_TRUNC) && vfs->truncate)
					vfs->truncate(handle, 0);
				const int fd = vfs_alloc_fd(handle);
				if (fd >= 0)
					return fd;
				if (vfs->close)
					vfs->close(handle);
				errno = EMFILE;
				return -1;
			}
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int fd = ::open(fs_path, flags, mode);
	free(fs_path);
	return fd;
}

extern "C" int posixemu_close(int fd)
{
	retro_vfs_file_handle* handle = vfs_get_fd(fd);
	if (handle) {
		const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
		int ret = 0;
		if (vfs && vfs->close)
			ret = vfs->close(handle);
		vfs_release_fd(fd);
		return ret;
	}
	return ::close(fd);
}

extern "C" int posixemu_read(int fd, void* buf, int len)
{
	retro_vfs_file_handle* handle = vfs_get_fd(fd);
	if (handle) {
		const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
		if (vfs && vfs->read) {
			const int64_t ret = vfs->read(handle, buf, static_cast<uint64_t>(len));
			return ret < 0 ? -1 : static_cast<int>(ret);
		}
		errno = EBADF;
		return -1;
	}
	return static_cast<int>(::read(fd, buf, len));
}

extern "C" int posixemu_write(int fd, const void* buf, int len)
{
	retro_vfs_file_handle* handle = vfs_get_fd(fd);
	if (handle) {
		const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
		if (vfs && vfs->write) {
			const int64_t ret = vfs->write(handle, buf, static_cast<uint64_t>(len));
			return ret < 0 ? -1 : static_cast<int>(ret);
		}
		errno = EBADF;
		return -1;
	}
	return static_cast<int>(::write(fd, buf, len));
}

extern "C" off_t posixemu_seek(int fd, off_t offset, int whence)
{
	retro_vfs_file_handle* handle = vfs_get_fd(fd);
	if (handle) {
		const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
		if (vfs && vfs->seek) {
			const int64_t ret = vfs->seek(handle, offset, vfs_whence(whence));
			return ret < 0 ? static_cast<off_t>(-1) : static_cast<off_t>(ret);
		}
		errno = ESPIPE;
		return static_cast<off_t>(-1);
	}
	return ::lseek(fd, offset, whence);
}

extern "C" int posixemu_stat(const TCHAR* path, STAT* st)
{
	if (!path || !st)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->stat) {
		int32_t size = -1;
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			const int flags = vfs->stat(fs_path, &size);
			free(fs_path);
			if (flags & RETRO_VFS_STAT_IS_VALID) {
				memset(st, 0, sizeof(*st));
				if (size >= 0)
					st->st_size = size;
				if (flags & RETRO_VFS_STAT_IS_DIRECTORY)
					st->st_mode = S_IFDIR;
				else
					st->st_mode = S_IFREG;
				return 0;
			}
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int ret = ::stat(fs_path, st);
	free(fs_path);
	return ret;
}

extern "C" int posixemu_mkdir(const TCHAR* path, int mode)
{
	if (!path)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->mkdir) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			const int ret = vfs->mkdir(fs_path);
			free(fs_path);
			return ret;
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
#ifdef _WIN32
	const int ret = ::mkdir(fs_path);
#else
	const int ret = ::mkdir(fs_path, mode);
#endif
	free(fs_path);
	return ret;
}

extern "C" int posixemu_rmdir(const TCHAR* path)
{
	if (!path)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->remove) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			const int ret = vfs->remove(fs_path);
			free(fs_path);
			return ret;
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int ret = ::rmdir(fs_path);
	free(fs_path);
	return ret;
}

extern "C" int posixemu_unlink(const TCHAR* path)
{
	if (!path)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->remove) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			const int ret = vfs->remove(fs_path);
			free(fs_path);
			return ret;
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int ret = ::unlink(fs_path);
	free(fs_path);
	return ret;
}

extern "C" int posixemu_truncate(const TCHAR* path, long int length)
{
	if (!path)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->truncate && vfs->open && vfs->close) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			retro_vfs_file_handle* handle = vfs->open(fs_path,
				RETRO_VFS_FILE_ACCESS_READ_WRITE | RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING,
				RETRO_VFS_FILE_ACCESS_HINT_NONE);
			free(fs_path);
			if (handle) {
				const int ret = vfs->truncate(handle, length);
				vfs->close(handle);
				return ret;
			}
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int ret = ::truncate(fs_path, length);
	free(fs_path);
	return ret;
}

extern "C" int posixemu_rename(const TCHAR* old_path, const TCHAR* new_path)
{
	if (!old_path || !new_path)
		return -1;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->rename) {
		char* fs_old = to_fs_path(old_path);
		char* fs_new = to_fs_path(new_path);
		if (fs_old && fs_new) {
			const int ret = vfs->rename(fs_old, fs_new);
			free(fs_old);
			free(fs_new);
			return ret;
		}
		free(fs_old);
		free(fs_new);
	}

	char* fs_old = to_fs_path(old_path);
	char* fs_new = to_fs_path(new_path);
	if (!fs_old || !fs_new) {
		free(fs_old);
		free(fs_new);
		return -1;
	}
	const int ret = ::rename(fs_old, fs_new);
	free(fs_old);
	free(fs_new);
	return ret;
}

extern "C" int posixemu_chmod(const TCHAR* path, int mode)
{
	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int ret = ::chmod(fs_path, mode);
	free(fs_path);
	return ret;
}

extern "C" void posixemu_tmpnam(TCHAR* out)
{
	if (!out)
		return;
	char* tmp = ::tmpnam(nullptr);
	if (!tmp) {
		out[0] = 0;
		return;
	}
	strncpy(out, tmp, 255);
	out[255] = 0;
}

extern "C" int posixemu_utime(const TCHAR* path, struct utimbuf* buf)
{
	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return -1;
	const int ret = ::utime(fs_path, buf);
	free(fs_path);
	return ret;
}

extern "C" DIR* posixemu_opendir(const TCHAR* path)
{
	if (!path)
		return nullptr;

	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->opendir && vfs->readdir && vfs->dirent_get_name && vfs->closedir) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			retro_vfs_dir_handle* handle = vfs->opendir(fs_path, false);
			free(fs_path);
			if (handle) {
				auto* wrapper = new VfsDirWrapper{};
				wrapper->handle = handle;
				memset(&wrapper->entry, 0, sizeof(wrapper->entry));
				g_vfs_dirs.push_back(wrapper);
				return reinterpret_cast<DIR*>(wrapper);
			}
		}
	}

	char* fs_path = to_fs_path(path);
	if (!fs_path)
		return nullptr;
	DIR* dirp = ::opendir(fs_path);
	free(fs_path);
	return dirp;
}

extern "C" struct dirent* posixemu_readdir(DIR* dirp)
{
	VfsDirWrapper* wrapper = find_vfs_dir(dirp);
	if (wrapper) {
		const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
		if (!vfs || !vfs->readdir || !vfs->dirent_get_name)
			return nullptr;
		if (!vfs->readdir(wrapper->handle))
			return nullptr;
		const char* name = vfs->dirent_get_name(wrapper->handle);
		if (!name)
			return nullptr;
		strncpy(wrapper->entry.d_name, name, sizeof(wrapper->entry.d_name) - 1);
		wrapper->entry.d_name[sizeof(wrapper->entry.d_name) - 1] = '\0';
#ifdef _DIRENT_HAVE_D_TYPE
		if (vfs->dirent_is_dir && vfs->dirent_is_dir(wrapper->handle))
			wrapper->entry.d_type = DT_DIR;
		else
			wrapper->entry.d_type = DT_REG;
#endif
		return &wrapper->entry;
	}
	return ::readdir(dirp);
}

extern "C" int posixemu_closedir(DIR* dirp)
{
	VfsDirWrapper* wrapper = find_vfs_dir(dirp);
	if (wrapper) {
		const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
		int ret = 0;
		if (vfs && vfs->closedir)
			ret = vfs->closedir(wrapper->handle);
		auto it = std::find(g_vfs_dirs.begin(), g_vfs_dirs.end(), wrapper);
		if (it != g_vfs_dirs.end())
			g_vfs_dirs.erase(it);
		delete wrapper;
		return ret;
	}
	return ::closedir(dirp);
}
