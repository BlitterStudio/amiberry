// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, Vas Crabb
//============================================================
//
//  winfile.cpp - Windows file access functions
//
//  Based on posixfile.cpp, adapted for Windows/MinGW.
//  MinGW provides POSIX-compatible open/read/write/close,
//  so most code is shared with the POSIX version.
//
//============================================================

#define _FILE_OFFSET_BITS 64

// MAME headers
#include "winfile.h"
#include "osdcore.h"
#include "unicode.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

#include <fcntl.h>
#include <climits>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>

// MinGW provides unistd.h with open/read/write/close/lseek/ftruncate/unlink
#include <unistd.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


namespace {

	//============================================================
	//  CONSTANTS
	//============================================================

	constexpr char PATHSEPCH = '\\';
	constexpr char INVPATHSEPCH = '/';


	class win_osd_file : public osd_file
	{
	public:
		win_osd_file(win_osd_file const&) = delete;
		win_osd_file(win_osd_file&&) = delete;
		win_osd_file& operator=(win_osd_file const&) = delete;
		win_osd_file& operator=(win_osd_file&&) = delete;

		win_osd_file(int fd) noexcept : m_fd(fd)
		{
			assert(m_fd >= 0);
		}

		virtual ~win_osd_file() override
		{
			::close(m_fd);
		}

		virtual std::error_condition read(void* buffer, std::uint64_t offset, std::uint32_t count, std::uint32_t& actual) noexcept override
		{
			if (lseek(m_fd, off_t(std::make_unsigned_t<off_t>(offset)), SEEK_SET) < 0)
				return std::error_condition(errno, std::generic_category());
			ssize_t const result = ::read(m_fd, buffer, size_t(count));

			if (result < 0)
				return std::error_condition(errno, std::generic_category());

			actual = std::uint32_t(std::size_t(result));
			return std::error_condition();
		}

		virtual std::error_condition write(void const* buffer, std::uint64_t offset, std::uint32_t count, std::uint32_t& actual) noexcept override
		{
			if (lseek(m_fd, off_t(std::make_unsigned_t<off_t>(offset)), SEEK_SET) < 0)
				return std::error_condition(errno, std::generic_category());
			ssize_t const result = ::write(m_fd, buffer, size_t(count));

			if (result < 0)
				return std::error_condition(errno, std::generic_category());

			actual = std::uint32_t(std::size_t(result));
			return std::error_condition();
		}

		virtual std::error_condition truncate(std::uint64_t offset) noexcept override
		{
			int const result = ::ftruncate(m_fd, off_t(std::make_unsigned_t<off_t>(offset)));
			if (result < 0)
				return std::error_condition(errno, std::generic_category());

			return std::error_condition();
		}

		virtual std::error_condition flush() noexcept override
		{
			// no user-space buffering on low-level I/O
			return std::error_condition();
		}

	private:
		int m_fd;
	};


	//============================================================
	//  is_path_separator
	//============================================================

	bool is_path_separator(char c) noexcept
	{
		return (c == PATHSEPCH) || (c == INVPATHSEPCH);
	}


	//============================================================
	//  create_path_recursive
	//============================================================

	std::error_condition create_path_recursive(std::string_view path) noexcept
	{
		// if there's still a separator, and it's not the root, nuke it and recurse
		auto const sep = path.rfind(PATHSEPCH);
		if ((sep != std::string_view::npos) && (sep > 0) && (path[sep - 1] != PATHSEPCH))
		{
			// also check for drive letter (e.g. C:\)
			if (!((sep == 2) && (path[1] == ':')))
			{
				std::error_condition err = create_path_recursive(path.substr(0, sep));
				if (err)
					return err;
			}
		}

		// need a NUL-terminated version of the subpath
		std::string p;
		try { p = path; }
		catch (...) { return std::errc::not_enough_memory; }

		// if the path already exists, we're done
		struct stat st;
		if (!::stat(p.c_str(), &st))
			return std::error_condition();

		// create the path
		if (_mkdir(p.c_str()) < 0)
			return std::error_condition(errno, std::generic_category());
		else
			return std::error_condition();
	}

} // anonymous namespace


//============================================================
//  osd_file::open
//============================================================

std::error_condition osd_file::open(std::string const& path, std::uint32_t openflags, ptr& file, std::uint64_t& filesize) noexcept
{
	std::string dst;
	if (win_check_socket_path(path))
		return win_open_socket(path, openflags, file, filesize);
	else if (win_check_ptty_path(path))
		return win_open_ptty(openflags, file, filesize, dst);

	// select the file open modes
	int access;
	if (openflags & OPEN_FLAG_WRITE)
	{
		access = (openflags & OPEN_FLAG_READ) ? O_RDWR : O_WRONLY;
		access |= (openflags & OPEN_FLAG_CREATE) ? (O_CREAT | O_TRUNC) : 0;
	}
	else if (openflags & OPEN_FLAG_READ)
	{
		access = O_RDONLY;
	}
	else
	{
		return std::errc::invalid_argument;
	}
	access |= O_BINARY;

	// convert the path into something compatible
	dst = path;
	for (auto it = dst.begin(); it != dst.end(); ++it)
		*it = (INVPATHSEPCH == *it) ? PATHSEPCH : *it;
	try { osd_subst_env(dst, dst); }
	catch (...) { return std::errc::not_enough_memory; }

	// attempt to open the file
	int fd = ::open(dst.c_str(), access, 0666);

	if (fd < 0)
	{
		// save the error from the first attempt to open the file
		std::error_condition openerr(errno, std::generic_category());

		// create the path if necessary
		if ((openflags & OPEN_FLAG_CREATE) && (openflags & OPEN_FLAG_CREATE_PATHS))
		{
			auto const pathsep = dst.rfind(PATHSEPCH);
			if (pathsep != std::string::npos)
			{
				// create the path up to the file
				std::error_condition const createrr = create_path_recursive(dst.substr(0, pathsep));

				// attempt to reopen the file
				if (!createrr)
				{
					fd = ::open(dst.c_str(), access, 0666);
				}
				if (fd < 0)
				{
					openerr.assign(errno, std::generic_category());
				}
			}
		}

		// if we still failed, clean up and free
		if (fd < 0)
		{
			return openerr;
		}
	}

	// get the file size
	struct stat st;
	if (::fstat(fd, &st) < 0)
	{
		std::error_condition staterr(errno, std::generic_category());
		::close(fd);
		return staterr;
	}

	osd_file::ptr result(new (std::nothrow) win_osd_file(fd));
	if (!result)
	{
		::close(fd);
		return std::errc::not_enough_memory;
	}
	file = std::move(result);
	filesize = std::uint64_t(std::make_unsigned_t<decltype(st.st_size)>(st.st_size));
	return std::error_condition();
}


//============================================================
//  osd_file::openpty
//============================================================

std::error_condition osd_file::openpty(ptr& file, std::string& name) noexcept
{
	// PTY not supported on Windows
	return std::errc::not_supported;
}


//============================================================
//  osd_file::remove
//============================================================

std::error_condition osd_file::remove(std::string const& filename) noexcept
{
	if (::unlink(filename.c_str()) < -1)
		return std::error_condition(errno, std::generic_category());
	else
		return std::error_condition();
}


//============================================================
//  osd_get_physical_drive_geometry
//============================================================

bool osd_get_physical_drive_geometry(const char* filename, uint32_t* cylinders, uint32_t* heads, uint32_t* sectors, uint32_t* bps) noexcept
{
	return false; // not supported
}


//============================================================
//  osd_stat
//============================================================

std::unique_ptr<osd::directory::entry> osd_stat(const std::string& path)
{
	struct stat st;
	int const err = ::stat(path.c_str(), &st);
	if (err < 0) return nullptr;

	// create an osd_directory_entry; be sure to make sure that the caller can
	// free all resources by just freeing the resulting osd_directory_entry
	osd::directory::entry* result;
	try { result = reinterpret_cast<osd::directory::entry*>(::operator new(sizeof(*result) + path.length() + 1)); }
	catch (...) { return nullptr; }
	new (result) osd::directory::entry;

	std::strcpy(reinterpret_cast<char*>(result) + sizeof(*result), path.c_str());
	result->name = reinterpret_cast<char*>(result) + sizeof(*result);
	result->type = S_ISDIR(st.st_mode) ? osd::directory::entry::entry_type::DIR : osd::directory::entry::entry_type::FILE;
	result->size = std::uint64_t(std::make_unsigned_t<decltype(st.st_size)>(st.st_size));
	result->last_modified = std::chrono::system_clock::from_time_t(st.st_mtime);

	return std::unique_ptr<osd::directory::entry>(result);
}


//============================================================
//  osd_get_full_path
//============================================================

std::error_condition osd_get_full_path(std::string& dst, std::string const& path) noexcept
{
	try
	{
		std::vector<char> path_buffer(MAX_PATH);
		if (::_fullpath(&path_buffer[0], path.c_str(), MAX_PATH))
		{
			dst = &path_buffer[0];
			return std::error_condition();
		}
		else
		{
			return std::errc::io_error;
		}
	}
	catch (...)
	{
		return std::errc::not_enough_memory;
	}
}


//============================================================
//  osd_is_absolute_path
//============================================================

bool osd_is_absolute_path(std::string const& path) noexcept
{
	if (!path.empty() && is_path_separator(path[0]))
		return true;
	else if ((path.length() > 1) && (path[1] == ':'))
		return true;
	else
		return false;
}


//============================================================
//  osd_get_volume_name
//============================================================

std::string osd_get_volume_name(int idx)
{
	// Return drive letters
	DWORD drives = GetLogicalDrives();
	int count = 0;
	for (int i = 0; i < 26; i++)
	{
		if (drives & (1 << i))
		{
			if (count == idx)
			{
				char drive[4] = { char('A' + i), ':', '\\', '\0' };
				return std::string(drive);
			}
			count++;
		}
	}
	return std::string();
}


//============================================================
//  osd_get_volume_names
//============================================================

std::vector<std::string> osd_get_volume_names()
{
	std::vector<std::string> result;
	DWORD drives = GetLogicalDrives();
	for (int i = 0; i < 26; i++)
	{
		if (drives & (1 << i))
		{
			char drive[4] = { char('A' + i), ':', '\\', '\0' };
			result.emplace_back(drive);
		}
	}
	return result;
}


//============================================================
//  osd_is_valid_filename_char
//============================================================

bool osd_is_valid_filename_char(char32_t uchar) noexcept
{
	return osd_is_valid_filepath_char(uchar)
		&& uchar != PATHSEPCH
		&& uchar != INVPATHSEPCH
		&& uchar != ':';
}


//============================================================
//  osd_is_valid_filepath_char
//============================================================

bool osd_is_valid_filepath_char(char32_t uchar) noexcept
{
	return uchar >= 0x20
		&& !(uchar >= '\x7F' && uchar <= '\x9F')
		&& uchar != '<'
		&& uchar != '>'
		&& uchar != '\"'
		&& uchar != '|'
		&& uchar != '?'
		&& uchar != '*'
		&& uchar_isvalid(uchar);
}
