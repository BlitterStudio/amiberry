#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>

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
#undef opendir
#undef readdir
#undef closedir
#endif

#ifdef DONT_HAVE_STDIO
#undef fopen
#undef fread
#undef fwrite
#undef fseek
#undef ftell
#undef fclose
#endif

#include "osdep/libretro/libretro_vfs.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GLIBC__)
#define HAVE_FOPENCOOKIE 1
#endif

namespace {
static char* to_fs_path(const TCHAR* path)
{
	if (!path)
		return nullptr;
	return ua_fs(path, -1);
}

static unsigned vfs_mode_from_str(const char* mode)
{
	bool read = false;
	bool write = false;
	if (!mode)
		return RETRO_VFS_FILE_ACCESS_READ;
	if (strchr(mode, 'r'))
		read = true;
	if (strchr(mode, 'w') || strchr(mode, 'a'))
		write = true;
	if (strchr(mode, '+')) {
		read = true;
		write = true;
	}
	unsigned vfs_mode = 0;
	if (read && write)
		vfs_mode = RETRO_VFS_FILE_ACCESS_READ_WRITE;
	else if (write)
		vfs_mode = RETRO_VFS_FILE_ACCESS_WRITE;
	else
		vfs_mode = RETRO_VFS_FILE_ACCESS_READ;
	if (write && !strchr(mode, 'w'))
		vfs_mode |= RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
	return vfs_mode;
}

#if HAVE_FOPENCOOKIE
struct VfsCookie {
	retro_vfs_file_handle* handle;
};

static ssize_t vfs_cookie_read(void* c, char* buf, size_t size)
{
	auto* cookie = static_cast<VfsCookie*>(c);
	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (!vfs || !vfs->read || !cookie || !cookie->handle)
		return -1;
	const int64_t ret = vfs->read(cookie->handle, buf, size);
	if (ret < 0) {
		errno = EIO;
		return -1;
	}
	return ret;
}

static ssize_t vfs_cookie_write(void* c, const char* buf, size_t size)
{
	auto* cookie = static_cast<VfsCookie*>(c);
	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (!vfs || !vfs->write || !cookie || !cookie->handle)
		return -1;
	const int64_t ret = vfs->write(cookie->handle, buf, size);
	if (ret < 0) {
		errno = EIO;
		return -1;
	}
	return ret;
}

static int vfs_cookie_seek(void* c, off64_t* offset, int whence)
{
	auto* cookie = static_cast<VfsCookie*>(c);
	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (!vfs || !vfs->seek || !cookie || !cookie->handle)
		return -1;

	int vfs_whence = RETRO_VFS_SEEK_POSITION_START;
	switch (whence) {
	case SEEK_SET:
		vfs_whence = RETRO_VFS_SEEK_POSITION_START;
		break;
	case SEEK_CUR:
		vfs_whence = RETRO_VFS_SEEK_POSITION_CURRENT;
		break;
	case SEEK_END:
		vfs_whence = RETRO_VFS_SEEK_POSITION_END;
		break;
	default:
		break;
	}
	const int64_t ret = vfs->seek(cookie->handle, *offset, vfs_whence);
	if (ret < 0) {
		errno = ESPIPE;
		return -1;
	}
	*offset = ret;
	return 0;
}

static int vfs_cookie_close(void* c)
{
	auto* cookie = static_cast<VfsCookie*>(c);
	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (cookie && cookie->handle && vfs && vfs->close)
		vfs->close(cookie->handle);
	free(cookie);
	return 0;
}
#endif
} // namespace

extern "C" FILE* stdioemu_fopen(const TCHAR* path, const TCHAR* mode)
{
	const struct retro_vfs_interface* vfs = libretro_get_vfs_interface();
	if (vfs && vfs->open) {
		char* fs_path = to_fs_path(path);
		if (fs_path) {
			const unsigned vfs_mode = vfs_mode_from_str(mode);
			retro_vfs_file_handle* handle = vfs->open(fs_path, vfs_mode, RETRO_VFS_FILE_ACCESS_HINT_NONE);
			free(fs_path);
			if (handle) {
#if HAVE_FOPENCOOKIE
				auto* cookie = static_cast<VfsCookie*>(calloc(1, sizeof(VfsCookie)));
				if (!cookie) {
					if (vfs->close)
						vfs->close(handle);
					errno = ENOMEM;
					return nullptr;
				}
				cookie->handle = handle;
				cookie_io_functions_t io_funcs = {};
				if (mode && strchr(mode, 'r'))
					io_funcs.read = vfs_cookie_read;
				if (mode && (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+')))
					io_funcs.write = vfs_cookie_write;
				io_funcs.seek = vfs_cookie_seek;
				io_funcs.close = vfs_cookie_close;
				FILE* fp = fopencookie(cookie, mode ? mode : "rb", io_funcs);
				if (!fp) {
					if (vfs->close)
						vfs->close(handle);
					free(cookie);
					return nullptr;
				}
				if (mode && strchr(mode, 'a') && vfs->seek)
					vfs->seek(handle, 0, RETRO_VFS_SEEK_POSITION_END);
				return fp;
#else
				if (vfs->close)
					vfs->close(handle);
#endif
			}
		}
	}
	return ::fopen(path, mode);
}

extern "C" int stdioemu_fseek(FILE* stream, int offset, int whence)
{
	return ::fseek(stream, offset, whence);
}

extern "C" int stdioemu_fread(void* buf, int size, int nmemb, FILE* stream)
{
	return static_cast<int>(::fread(buf, size, nmemb, stream));
}

extern "C" int stdioemu_fwrite(const void* buf, int size, int nmemb, FILE* stream)
{
	return static_cast<int>(::fwrite(buf, size, nmemb, stream));
}

extern "C" int stdioemu_ftell(FILE* stream)
{
	return static_cast<int>(::ftell(stream));
}

extern "C" int stdioemu_fclose(FILE* stream)
{
	return ::fclose(stream);
}
