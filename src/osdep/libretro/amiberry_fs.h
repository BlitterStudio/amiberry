#pragma once

#include <filesystem>
#include <sys/stat.h>

#include "sysdeps.h"

#if !defined(LIBRETRO)
#include <unistd.h>
#include <fcntl.h>
#endif

namespace amiberry_fs {
#if defined(LIBRETRO)
static inline bool exists(const std::filesystem::path& path)
{
	STAT st{};
	return posixemu_stat(path.c_str(), &st) == 0;
}

static inline bool is_regular_file(const std::filesystem::path& path)
{
	STAT st{};
	if (posixemu_stat(path.c_str(), &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
}

static inline bool is_directory(const std::filesystem::path& path)
{
	STAT st{};
	if (posixemu_stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

static inline bool remove(const std::filesystem::path& path)
{
	STAT st{};
	if (posixemu_stat(path.c_str(), &st) != 0)
		return false;
	if (S_ISDIR(st.st_mode))
		return posixemu_rmdir(path.c_str()) == 0;
	return posixemu_unlink(path.c_str()) == 0;
}

static inline bool create_directories(const std::filesystem::path& path)
{
	if (path.empty())
		return false;
	std::filesystem::path current;
	for (const auto& part : path) {
		current /= part;
		if (current.empty())
			continue;
		STAT st{};
		if (posixemu_stat(current.c_str(), &st) == 0) {
			if (!S_ISDIR(st.st_mode))
				return false;
			continue;
		}
		if (posixemu_mkdir(current.c_str(), 0755) != 0)
			return false;
	}
	return true;
}

static inline int io_rename(const char* oldpath, const char* newpath)
{
	return posixemu_rename(oldpath, newpath);
}

static inline int io_open(const char* path, int flags, int mode)
{
	return posixemu_open(path, flags, mode);
}

static inline int io_close(int fd)
{
	return posixemu_close(fd);
}

static inline int io_read(int fd, void* buf, int count)
{
	return posixemu_read(fd, buf, count);
}

static inline int io_write(int fd, const void* buf, int count)
{
	return posixemu_write(fd, buf, count);
}

static inline int io_lseek(int fd, int offset, int whence)
{
	return posixemu_seek(fd, offset, whence);
}
#else
static inline bool exists(const std::filesystem::path& path)
{
	return std::filesystem::exists(path);
}

static inline bool is_regular_file(const std::filesystem::path& path)
{
	return std::filesystem::is_regular_file(path);
}

static inline bool is_directory(const std::filesystem::path& path)
{
	return std::filesystem::is_directory(path);
}

static inline bool remove(const std::filesystem::path& path)
{
	return std::filesystem::remove(path);
}

static inline bool create_directories(const std::filesystem::path& path)
{
	return std::filesystem::create_directories(path);
}

static inline int io_rename(const char* oldpath, const char* newpath)
{
	return ::rename(oldpath, newpath);
}

static inline int io_open(const char* path, int flags, int mode)
{
	return ::open(path, flags, mode);
}

static inline int io_close(int fd)
{
	return ::close(fd);
}

static inline int io_read(int fd, void* buf, int count)
{
	return static_cast<int>(::read(fd, buf, static_cast<size_t>(count)));
}

static inline int io_write(int fd, const void* buf, int count)
{
	return static_cast<int>(::write(fd, buf, static_cast<size_t>(count)));
}

static inline int io_lseek(int fd, int offset, int whence)
{
	return static_cast<int>(::lseek(fd, static_cast<off_t>(offset), whence));
}
#endif
} // namespace amiberry_fs
