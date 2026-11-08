#pragma once

// Host file access for the directory filesystem. Paths and names are UTF-8
// on every host (iso_8859_1_to_utf8() turns Amiga names into host paths).
//
// Windows reads char paths in the ANSI code page, so the standalone Windows
// build converts them to UTF-16 and calls the wide functions. POSIX hosts
// take UTF-8 as it is, and the libretro core's posixemu layer (sysdeps.h)
// already expects UTF-8, so both keep the narrow calls.
//
// Include after sysdeps.h (STAT).

#include <cstdio>
#include <dirent.h>
#include <filesystem>
#include <string>
#include <string_view>
#ifdef _WIN32
#include <sys/utime.h>
#endif

// The UTF-8 host path for a directory filesystem name: a registered mount
// root stays as it is, the rest is converted from ISO-8859-1.
std::string nname_to_host(std::string_view nname);
// The other way round; false when a name below the root is not Latin-1.
bool host_to_nname(std::string_view host, std::string& nname);

std::filesystem::path host_path(const std::string& utf8);
std::string host_path_utf8(const std::filesystem::path& path);

int host_stat(const char* path, STAT* st);
int host_open(const char* path, int flags, int mode);
FILE* host_fopen(const char* path, const char* mode);
int host_rename(const char* from, const char* to);
int host_remove(const char* path);
int host_mkdir(const char* path);
int host_rmdir(const char* path);
int host_chmod(const char* path, int mode);
#ifdef _WIN32
int host_utime(const char* path, struct _utimbuf* times);
#endif

// Directory listing with UTF-8 entry names. On standalone Windows it reads
// through _wreaddir(): the narrow readdir() hands out ANSI code page names.
class host_dir {
public:
	host_dir() = default;
	~host_dir() { close(); }
	host_dir(const host_dir&) = delete;
	host_dir& operator=(const host_dir&) = delete;

	bool open(const char* path);
	// Returns closedir()'s result; 0 when nothing was open.
	int close();
	// The next entry name, "." and ".." included, or nullptr at the end.
	// The pointer stays valid until the next call.
	const char* next();

private:
#if defined(_WIN32) && !defined(LIBRETRO)
	_WDIR* dir_ = nullptr;
	std::string name_;
#else
	DIR* dir_ = nullptr;
#endif
};
