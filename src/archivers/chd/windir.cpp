// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, Vas Crabb
//============================================================
//
//  windir.cpp - Windows directory access functions
//
//  Uses MinGW-provided dirent.h for POSIX-compatible
//  directory iteration, same approach as posixdir.cpp.
//
//============================================================

#include "osdcore.h"
#include "osdfile.h"
#include "osdlib.h"
#include "strformat.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>


namespace osd {
	namespace {
		//============================================================
		//  CONSTANTS
		//============================================================

		constexpr char PATHSEPCH = '\\';


		using sdl_dirent = struct dirent;
		using sdl_stat = struct stat;
#define sdl_readdir readdir
#define sdl_stat_fn stat


		//============================================================
		//  TYPE DEFINITIONS
		//============================================================

		class win_directory : public osd::directory
		{
		public:
			win_directory();
			virtual ~win_directory() override;

			virtual const entry* read() override;

			bool open_impl(std::string const& dirname);

		private:
			typedef std::unique_ptr<DIR, int (*)(DIR*)>    dir_ptr;

			entry       m_entry;
			sdl_dirent* m_data;
			dir_ptr     m_fd;
			std::string m_path;
		};


		//============================================================
		//  win_directory::win_directory
		//============================================================

		win_directory::win_directory()
			: m_entry()
			, m_data(nullptr)
			, m_fd(nullptr, &::closedir)
			, m_path()
		{
		}


		//============================================================
		//  win_directory::~win_directory
		//============================================================

		win_directory::~win_directory()
		{
		}


		//============================================================
		//  win_directory::read
		//============================================================

		const osd::directory::entry* win_directory::read()
		{
			m_data = sdl_readdir(m_fd.get());
			if (!m_data)
				return nullptr;

			m_entry.name = m_data->d_name;

			sdl_stat st;
			bool stat_err(0 > sdl_stat_fn(util::string_format("%s%c%s", m_path, PATHSEPCH, m_data->d_name).c_str(), &st));

			// No DT_xxx on Windows/MinGW, use stat result
			if (stat_err)
				m_entry.type = entry::entry_type::OTHER;
			else if (S_ISDIR(st.st_mode))
				m_entry.type = entry::entry_type::DIR;
			else
				m_entry.type = entry::entry_type::FILE;

			m_entry.size = stat_err ? 0 : std::uint64_t(std::make_unsigned_t<decltype(st.st_size)>(st.st_size));
			m_entry.last_modified = std::chrono::system_clock::from_time_t(st.st_mtime);
			return &m_entry;
		}


		//============================================================
		//  win_directory::open_impl
		//============================================================

		bool win_directory::open_impl(std::string const& dirname)
		{
			assert(!m_fd);

			osd_subst_env(m_path, dirname);
			m_fd.reset(::opendir(m_path.c_str()));
			return bool(m_fd);
		}

	} // anonymous namespace


	//============================================================
	//  osd::directory::open
	//============================================================

	directory::ptr directory::open(std::string const& dirname)
	{
		std::unique_ptr<win_directory> dir;
		try { dir.reset(new win_directory); }
		catch (...) { return nullptr; }

		if (!dir->open_impl(dirname))
			return nullptr;

		return ptr(std::move(dir));
	}

} // namespace osd


//============================================================
//  osd_subst_env
//============================================================

void osd_subst_env(std::string& dst, std::string const& src)
{
	std::string result, var;
	auto start = src.begin();

	// a leading tilde expands as $HOME (or %USERPROFILE% on Windows)
	if ((src.end() != start) && ('~' == *start))
	{
		char const* home = std::getenv("HOME");
		if (!home)
			home = std::getenv("USERPROFILE");
		if (home)
		{
			++start;
			if ((src.end() == start) || (osd::PATHSEPCH == *start) || ('/' == *start))
				result.append(home);
			else
				result.push_back('~');
		}
	}

	while (src.end() != start)
	{
		// find $ or % marking start of environment variable or end of string
		auto it = start;
		while ((src.end() != it) && ('$' != *it) && ('%' != *it)) ++it;
		if (start != it) result.append(start, it);
		start = it;

		if (src.end() != start)
		{
			if ('$' == *start)
			{
				// Unix-style $VAR or ${VAR}
				start = ++it;
				if ((src.end() != start) && ('{' == *start))
				{
					start = ++it;
					for (++it; (src.end() != it) && ('}' != *it); ++it) {}
					if (src.end() == it)
					{
						result.append("${").append(start, it);
						start = it;
					}
					else
					{
						var.assign(start, it);
						start = ++it;
						const char* const exp = std::getenv(var.c_str());
						if (exp)
							result.append(exp);
						else
							fprintf(stderr, "Warning: osd_subst_env variable %s not found.\n", var.c_str());
					}
				}
				else if ((src.end() != start) && (('_' == *start) || std::isalnum(*start)))
				{
					for (++it; (src.end() != it) && (('_' == *it) || std::isalnum(*it)); ++it) {}
					var.assign(start, it);
					start = it;
					const char* const exp = std::getenv(var.c_str());
					if (exp)
						result.append(exp);
					else
						fprintf(stderr, "Warning: osd_subst_env variable %s not found.\n", var.c_str());
				}
				else
				{
					result.push_back('$');
				}
			}
			else
			{
				// Windows-style %VAR%
				start = ++it;
				while ((src.end() != it) && ('%' != *it)) ++it;
				if (src.end() == it)
				{
					result.push_back('%');
					result.append(start, it);
					start = it;
				}
				else
				{
					var.assign(start, it);
					start = ++it;
					const char* const exp = std::getenv(var.c_str());
					if (exp)
						result.append(exp);
					else
						fprintf(stderr, "Warning: osd_subst_env variable %s not found.\n", var.c_str());
				}
			}
		}
	}

	dst = std::move(result);
}
