#pragma once

#include <filesystem>
#include <sys/stat.h>

#include "sysdeps.h"

namespace libretro_fs {
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
#else
static inline bool exists(const std::filesystem::path& path)
{
	return std::filesystem::exists(path);
}

static inline bool remove(const std::filesystem::path& path)
{
	return std::filesystem::remove(path);
}

static inline bool create_directories(const std::filesystem::path& path)
{
	return std::filesystem::create_directories(path);
}
#endif
} // namespace libretro_fs
