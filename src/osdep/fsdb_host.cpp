/*
 * UAE - The Un*x Amiga Emulator
 *
 * Library of functions to make emulated filesystem as independent as
 * possible of the host filesystem's capabilities.
 *
 * Copyright 1997 Mathias Ortmann
 * Copyright 1999 Bernd Schmidt
 * Copyright 2012 Frode Solheim
 * Copyright 2025 Dimitris Panokostas
 */

#include <algorithm>
#include <cstring>
#include <climits>
#include <memory>
#include <chrono>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif


#include "sysconfig.h"
#include "sysdeps.h"

#include "fsdb.h"
#include "zfile.h"
#include "fsdb_host.h"
#include "uae.h"

enum
{
	NUM_EVILCHARS = 9
};

static char evilchars[NUM_EVILCHARS] = { '%', '\\', '*', '?', '\"', '/', '|', '<', '>' };
static char hex_chars[] = "0123456789abcdef";
#define UAEFSDB_BEGINS _T("__uae___")

static bool is_hex_digit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static unsigned char char_to_hex(char c)
{
	if (c >= '0' && c <= '9')
		return static_cast<unsigned char>(c - '0');
	if (c >= 'a' && c <= 'f')
		return static_cast<unsigned char>(10 + c - 'a');
	if (c >= 'A' && c <= 'F')
		return static_cast<unsigned char>(10 + c - 'A');
	return 0;
}

char* aname_to_nname(const char* aname, const int ascii)
{
    // Input validation
    if (!aname) {
        write_log(_T("aname_to_nname: null input filename\n"));
        return nullptr;
    }

    try {
        const size_t len = strlen(aname);
        if (len == 0) {
            return strdup("");
        }

        // Check for Windows reserved device names
        bool need_escape_name = false;
        size_t reserved_prefix_len = 0;

        // Safer handling of first 4 characters with null-termination checks
        const char first_chars[5] = {
            aname[0],
            len > 1 ? aname[1] : '\0',
            len > 2 ? aname[2] : '\0',
            len > 3 ? aname[3] : '\0',
            '\0'
        };

        // Convert first chars to uppercase for device name comparison
        char ucase[5];
        for (int i = 0; i < 4; i++) {
            ucase[i] = (first_chars[i] >= 'a' && first_chars[i] <= 'z')
                ? first_chars[i] - 32
                : first_chars[i];
        }
        ucase[4] = '\0';

        // Check for reserved Windows device names
        if ((memcmp(ucase, "AUX", 3) == 0 && (len == 3 || (len > 3 && aname[3] == '.'))) ||
            (memcmp(ucase, "CON", 3) == 0 && (len == 3 || (len > 3 && aname[3] == '.'))) ||
            (memcmp(ucase, "PRN", 3) == 0 && (len == 3 || (len > 3 && aname[3] == '.'))) ||
            (memcmp(ucase, "NUL", 3) == 0 && (len == 3 || (len > 3 && aname[3] == '.')))) {
            need_escape_name = true;
            reserved_prefix_len = 2;  // Escape the 3rd character (X in "AUX")
        }
        else if (((memcmp(ucase, "LPT", 3) == 0) || (memcmp(ucase, "COM", 3) == 0)) &&
                 len > 3 && ucase[3] >= '0' && ucase[3] <= '9' &&
                 (len == 4 || (len > 4 && aname[4] == '.'))) {
            need_escape_name = true;
            reserved_prefix_len = 2;  // Escape the 3rd character (T in "LPT" or M in "COM")
        }

        // Check for trailing spaces or periods (illegal in Windows)
        bool has_trailing_illegal = (len > 0 && (aname[len-1] == '.' || aname[len-1] == ' '));

        // Allocate sufficient buffer for worst-case scenario (all chars encoded as %XX)
        std::unique_ptr<char[]> buffer(new char[len * 3 + 1]);
        char* p = buffer.get();

        // Process each character
        for (size_t i = 0; i < len; i++) {
            const char c = aname[i];
            bool needs_encoding = false;

            // Determine if this character needs encoding
            if ((i == reserved_prefix_len && need_escape_name) ||
                (i == len - 1 && has_trailing_illegal) ||
                (c < 32) || // Control characters
                (ascii && c > 127) || // Non-ASCII in ASCII mode
                std::find(evilchars, evilchars + NUM_EVILCHARS, c) != evilchars + NUM_EVILCHARS) {
                needs_encoding = true;
            }

            // Check for illegal .uaem extension
            if (i == len - 1 && len >= 5 &&
                strncasecmp(aname + len - 5, ".uaem", 5) == 0) {
                needs_encoding = true;
            }

            if (needs_encoding) {
                *p++ = '%';
                *p++ = hex_chars[(c & 0xf0) >> 4];
                *p++ = hex_chars[c & 0xf];
            } else {
                *p++ = c;
            }
        }
        *p = '\0';

        // If ASCII mode, return the buffer directly
        if (ascii) {
            char* result = strdup(buffer.get());
            return result;
        }

        // Convert to UTF-8
        const auto utf8_result = iso_8859_1_to_utf8(std::string_view(buffer.get()));
        return strdup(utf8_result.c_str());
    }
    catch (const std::exception& e) {
        write_log(_T("aname_to_nname: exception processing '%s': %s\n"),
                 aname, e.what());
        return strdup("");  // Return empty string rather than null to prevent crashes
    }
}

char* nname_to_aname(const char* nname, int noconvert)
{
	if (!nname) {
		return nullptr;
	}

	const char* input = nname;
	std::string latin1_str;
	if (!noconvert) {
		if (!utf8_to_latin1_string(std::string_view(nname), latin1_str)) {
			return my_strdup(nname);
		}
		input = latin1_str.c_str();
	}

	const size_t len = strlen(input);
	char* result = xmalloc(char, len + 1);
	char* out = result;

	for (size_t i = 0; i < len; ++i) {
		if (input[i] == '%' && i + 2 < len && is_hex_digit(input[i + 1]) && is_hex_digit(input[i + 2])) {
			*out++ = static_cast<char>((char_to_hex(input[i + 1]) << 4) | char_to_hex(input[i + 2]));
			i += 2;
		} else {
			*out++ = input[i];
		}
	}
	*out = '\0';
	return result;
}

void fsdb_init_file_info(fsdb_file_info* info)
{
	if (!info) {
		return;
	}
	info->type = 0;
	info->mode = A_FIBF_READ | A_FIBF_WRITE | A_FIBF_EXECUTE | A_FIBF_DELETE;
	info->days = 0;
	info->mins = 0;
	info->ticks = 0;
	info->comment = nullptr;
}

int fsdb_read_uaem(const char* nname, fsdb_file_info* info)
{
	if (!nname || !info) {
		return ERROR_OBJECT_NOT_AROUND;
	}

	const std::string uaem_path = std::string(nname) + ".uaem";
	const auto uaem_path_utf8 = iso_8859_1_to_utf8(std::string_view(uaem_path));
	FILE* file = fopen(uaem_path_utf8.c_str(), "rb");
	if (!file) {
		return ERROR_OBJECT_NOT_AROUND;
	}

	int result = ERROR_BAD_NUMBER;
	do {
		if (fseek(file, 0, SEEK_END) != 0) {
			break;
		}
		const long size = ftell(file);
		if (size < 0) {
			break;
		}
		if (fseek(file, 0, SEEK_SET) != 0) {
			break;
		}

		std::vector<char> buffer(static_cast<size_t>(size) + 1);
		if (size > 0 && fread(buffer.data(), 1, static_cast<size_t>(size), file) != static_cast<size_t>(size)) {
			break;
		}
		buffer[static_cast<size_t>(size)] = '\0';

		const char* p = buffer.data();
		size_t remaining = static_cast<size_t>(size);
		if (remaining >= 3 && static_cast<unsigned char>(p[0]) == 0xEF
			&& static_cast<unsigned char>(p[1]) == 0xBB
			&& static_cast<unsigned char>(p[2]) == 0xBF) {
			p += 3;
			remaining -= 3;
		}

		if (remaining < 8) {
			break;
		}

		fsdb_init_file_info(info);
		info->mode = 0;
		static const int bits[8] = {
			A_FIBF_HIDDEN,
			A_FIBF_SCRIPT,
			A_FIBF_PURE,
			A_FIBF_ARCHIVE,
			A_FIBF_READ,
			A_FIBF_WRITE,
			A_FIBF_EXECUTE,
			A_FIBF_DELETE
		};
		static const char chars[8] = { 'h', 's', 'p', 'a', 'r', 'w', 'e', 'd' };
		for (int i = 0; i < 8; ++i) {
			if (p[i] == chars[i]) {
				info->mode |= bits[i];
			}
		}
		p += 8;
		remaining -= 8;

		while (remaining > 0 && *p == ' ') {
			++p;
			--remaining;
		}

		int year, month, day, hour, min, sec, centisec;
		if (remaining < 22 || sscanf(p, "%4d-%2d-%2d %2d:%2d:%2d.%2d", &year, &month, &day, &hour, &min, &sec, &centisec) != 7) {
			break;
		}

		struct tm tmv {};
		tmv.tm_year = year - 1900;
		tmv.tm_mon = month - 1;
		tmv.tm_mday = day;
		tmv.tm_hour = hour;
		tmv.tm_min = min;
		tmv.tm_sec = sec;
#ifdef _WIN32
		time_t utc = _mkgmtime(&tmv);
#else
		time_t utc = timegm(&tmv);
#endif
		if (utc == static_cast<time_t>(-1)) {
			break;
		}

		struct mytimeval tv {};
		tv.tv_sec = static_cast<int>(utc);
		tv.tv_usec = centisec * 10000;
		timeval_to_amiga(&tv, &info->days, &info->mins, &info->ticks, 50);

		p += 22;
		remaining -= 22;
		while (remaining > 0 && *p == ' ') {
			++p;
			--remaining;
		}

		if (remaining > 0 && *p != '\r' && *p != '\n') {
			const char* start = p;
			while (remaining > 0 && *p != '\r' && *p != '\n') {
				++p;
				--remaining;
			}
			std::string comment(start, static_cast<size_t>(p - start));
			if (!comment.empty()) {
				info->comment = nname_to_aname(comment.c_str(), 1);
			}
		}

		result = 0;
	} while (0);

	fclose(file);
	return result;
}

int fsdb_write_uaem(const char* nname, const fsdb_file_info* info)
{
	if (!nname || !info) {
		return ERROR_OBJECT_NOT_AROUND;
	}

	const int default_mode = A_FIBF_READ | A_FIBF_WRITE | A_FIBF_EXECUTE | A_FIBF_DELETE;
	const bool has_comment = info->comment && info->comment[0] != '\0';
	const bool need_uaem = info->mode != static_cast<uint32_t>(default_mode) || has_comment;

	const std::string uaem_path = std::string(nname) + ".uaem";
	const auto uaem_path_utf8 = iso_8859_1_to_utf8(std::string_view(uaem_path));

	if (!need_uaem) {
		remove(uaem_path_utf8.c_str());
		return 0;
	}

	FILE* file = fopen(uaem_path_utf8.c_str(), "wb");
	if (!file) {
		return ERROR_OBJECT_NOT_AROUND;
	}

	static const int bits[8] = {
		A_FIBF_HIDDEN,
		A_FIBF_SCRIPT,
		A_FIBF_PURE,
		A_FIBF_ARCHIVE,
		A_FIBF_READ,
		A_FIBF_WRITE,
		A_FIBF_EXECUTE,
		A_FIBF_DELETE
	};
	static const char chars[8] = { 'h', 's', 'p', 'a', 'r', 'w', 'e', 'd' };
	char prot[9] = {};
	for (int i = 0; i < 8; ++i) {
		prot[i] = (info->mode & bits[i]) ? chars[i] : '-';
	}
	prot[8] = '\0';

	struct mytimeval tv {};
	amiga_to_timeval(&tv, info->days, info->mins, info->ticks, 50);
	time_t t = static_cast<time_t>(tv.tv_sec);
	struct tm tmv {};
#ifdef _WIN32
	gmtime_s(&tmv, &t);
#else
	gmtime_r(&t, &tmv);
#endif
	const int centisec = tv.tv_usec / 10000;

	if (fprintf(file, "%s %04d-%02d-%02d %02d:%02d:%02d.%02d",
		prot,
		tmv.tm_year + 1900,
		tmv.tm_mon + 1,
		tmv.tm_mday,
		tmv.tm_hour,
		tmv.tm_min,
		tmv.tm_sec,
		centisec) < 0) {
		fclose(file);
		return ERROR_OBJECT_NOT_AROUND;
	}

	if (has_comment) {
		char* encoded_comment = aname_to_nname(info->comment, 1);
		if (!encoded_comment) {
			fclose(file);
			return ERROR_NO_FREE_STORE;
		}
		if (fprintf(file, " %s", encoded_comment) < 0) {
			free(encoded_comment);
			fclose(file);
			return ERROR_OBJECT_NOT_AROUND;
		}
		free(encoded_comment);
	}

	fputc('\n', file);
	fclose(file);
	return 0;
}

/* Return nonzero for any name we can't create on the native filesystem.  */
static int fsdb_name_invalid_2(a_inode* aino, const TCHAR* name, int isDirectory)
{
    // Input validation
    if (!name) {
        write_log(_T("fsdb_name_invalid_2: null filename\n"));
        return -1;
    }

    try {
        const int name_length = _tcslen(name);

        // Check for empty name
        if (name_length == 0) {
            write_log(_T("fsdb_name_invalid_2: empty filename\n"));
            return -1;
        }

        // Check for reserved filename
        if (_tcscmp(name, FSDB_FILE) == 0) {
            write_log(_T("fsdb_name_invalid_2: reserved filename '%s'\n"), FSDB_FILE);
            return -1;
        }

        if (isDirectory) {
            if ((name[0] == '.' && name_length == 1) ||
                (name[0] == '.' && name[1] == '.' && name_length == 2)) {
                return -1;
            }
        }

        // Check for maximum path length (pre-emptive check)
        if (name_length > 255) {
            write_log(_T("fsdb_name_invalid_2: name too long (%d chars)\n"), name_length);
            return 1;
        }

        // Optimize character checking with a single loop
        for (int i = 0; i < name_length; i++) {
            const TCHAR c = name[i];

            // Quick check for evil characters using binary search or direct comparison
            for (const char evilchar : evilchars) {
                if (c == evilchar) {
                    write_log(_T("fsdb_name_invalid_2: name contains invalid character '%c'\n"), c);
                    return 1;
                }
            }
        }

        // All checks passed
        return 0;
    }
    catch (const std::exception& e) {
        write_log(_T("fsdb_name_invalid_2: exception while checking '%s': %s\n"),
                 name, e.what());
        return -1;
    }
}

int fsdb_name_invalid(a_inode* aino, const TCHAR* n)
{
	if (!aino || !n) {
		write_log(_T("fsdb_name_invalid: null aino or filename\n"));
		return -1;
	}

	const int v = fsdb_name_invalid_2(aino, n, 0);

	// If name is invalid, log with specific reason
	if (v > 0) {
		write_log(_T("FILESYS: '%s' illegal filename\n"), n);
	}

	return v;
}

int fsdb_name_invalid_dir(a_inode* aino, const TCHAR* n)
{
	// Delegate to the implementation function specifically for directories (isDirectory=1)
	const int v = fsdb_name_invalid_2(aino, n, 1);

	// If name is invalid, log with specific reason
	if (v > 0) {
		write_log(_T("FILESYS: '%s' illegal directory name\n"), n);
	}

	return v;
}

int fsdb_exists(const TCHAR* nname)
{
	return fs_path_exists(nname);
}

bool my_utime(const char* name, const struct mytimeval* tv)
{
	if (!name) {
		return false;
	}

	struct mytimeval mtv;

	bool ok = false;
	try {
		if (tv == nullptr) {
			time_t rawtime;
			time(&rawtime);

			struct tm* local_tm = localtime(&rawtime);
			if (!local_tm) {
				return false;
			}

			time_t local_time = mktime(local_tm);
			if (local_time == -1) {
				return false;
			}

			mtv.tv_sec = local_time;
			mtv.tv_usec = 0;
#ifndef _WIN32
			struct timeval current_time {};
			if (gettimeofday(&current_time, nullptr) != -1)
				mtv.tv_usec = current_time.tv_usec;
#endif
		}
		else {
			mtv = *tv;
		}

#ifdef _WIN32
		struct _utimbuf utb;
		utb.actime = mtv.tv_sec;
		utb.modtime = mtv.tv_sec;
		ok = _utime(name, &utb) == 0;
#else
		struct timeval times[2];
		times[0] = times[1] = { mtv.tv_sec, mtv.tv_usec };
		ok = utimes(name, times) == 0;
#endif
	}
	catch (...) {
		return false;
	}

	if (!ok) {
		return false;
	}

	fsdb_file_info info;
	fsdb_init_file_info(&info);
	fsdb_read_uaem(name, &info);
	timeval_to_amiga(&mtv, &info.days, &info.mins, &info.ticks, 50);
	const int uaem_err = fsdb_write_uaem(name, &info);
	if (info.comment) {
		xfree(info.comment);
	}
	return uaem_err == 0;
}

/* return supported combination */
int fsdb_mode_supported(const a_inode* aino)
{
	// Input validation
	if (!aino) {
		write_log(_T("fsdb_mode_supported: null aino provided\n"));
		return 0;
	}

	const int mask = aino->amigaos_mode;
	int supported_mask = 0;

	// Basic Amiga file protection bits are: HSPARWED
	// Check each protection bit to determine which ones are supported

	// Read/Write/Execute bits are always supported on host filesystems
	supported_mask |= (mask & (A_FIBF_READ | A_FIBF_WRITE | A_FIBF_EXECUTE));

	// Delete bit is usually represented by write permission on the directory
	supported_mask |= (mask & A_FIBF_DELETE);

	// Archive bit can be tracked
	supported_mask |= (mask & A_FIBF_ARCHIVE);

	// Pure bit isn't typically supported by host filesystems
	// Script bit can be partially emulated with execute permission
	if (mask & A_FIBF_SCRIPT) {
		supported_mask |= A_FIBF_SCRIPT;
	}

	// Hold and SUID bits aren't typically supported in mappings

	return supported_mask;
}

/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
int fsdb_fill_file_attrs(a_inode* base, a_inode* aino)
{
	// Input validation
	if (!aino || !aino->nname) {
		write_log(_T("fsdb_fill_file_attrs: null aino or nname provided\n"));
		return 0;
	}

	try {
		// Convert filename to UTF-8 for filesystem operations
		const auto path_utf8 = iso_8859_1_to_utf8(std::string_view(aino->nname));

		// Get file information
		struct stat statbuf {};
		if (stat(path_utf8.c_str(), &statbuf) == -1) {
			const int error = errno;
			write_log(_T("fsdb_fill_file_attrs: stat failed for '%s': %s\n"),
					  aino->nname, strerror(error));
			return 0;
		}

		// Set file type (directory or file)
		aino->dir = S_ISDIR(statbuf.st_mode) ? 1 : 0;

		// Map UNIX permissions to Amiga protection bits
		// Note: In Amiga, protection bits are inverted (set = denied)
		aino->amigaos_mode = ((S_IXUSR & statbuf.st_mode ? 0 : A_FIBF_EXECUTE)
			| (S_IWUSR & statbuf.st_mode ? 0 : A_FIBF_WRITE)
			| (S_IRUSR & statbuf.st_mode ? 0 : A_FIBF_READ));

#if defined(AMIBERRY)
		// Always give execute & read permission
		aino->amigaos_mode &= ~A_FIBF_EXECUTE;
		aino->amigaos_mode &= ~A_FIBF_READ;
#endif

		fsdb_file_info info;
		fsdb_init_file_info(&info);
		if (fsdb_read_uaem(aino->nname, &info) == 0) {
			aino->amigaos_mode = info.mode ^ 0xf;
			if (info.comment) {
				if (aino->comment) {
					xfree(aino->comment);
				}
				aino->comment = info.comment;
				info.comment = nullptr;
			}
		}
		if (info.comment) {
			xfree(info.comment);
		}

		return 1;
	}
	catch (const std::exception& e) {
		write_log(_T("fsdb_fill_file_attrs: exception processing '%s': %s\n"),
				  aino->nname,
				  e.what());
		return 0;
	}
}

int fsdb_set_file_attrs(a_inode* aino)
{
    // Input validation
    if (!aino || !aino->nname) {
        write_log(_T("fsdb_set_file_attrs: null aino or nname\n"));
        return ERROR_OBJECT_NOT_AROUND;
    }

    try {
        // Convert path to UTF-8
        const auto path_utf8 = iso_8859_1_to_utf8(std::string_view(aino->nname));

        // Get file attributes
        struct stat statbuf {};
        if (stat(path_utf8.c_str(), &statbuf) == -1) {
            const int err = errno;
            write_log(_T("fsdb_set_file_attrs: stat failed for '%s': %s\n"),
                     aino->nname, strerror(err));
            return host_errno_to_dos_errno(err);
        }

        // Calculate new mode based on Amiga flags
        // Note: In Amiga, protection bits are inverted (set = denied)
        const uae_u32 mask = aino->amigaos_mode;
        int mode = statbuf.st_mode;

        // Update user permissions
        mode = (mask & A_FIBF_READ) ? (mode & ~S_IRUSR) : (mode | S_IRUSR);
        mode = (mask & A_FIBF_WRITE) ? (mode & ~S_IWUSR) : (mode | S_IWUSR);
        mode = (mask & A_FIBF_EXECUTE) ? (mode & ~S_IXUSR) : (mode | S_IXUSR);

        // Add group/other read permissions if user can read
        if (mode & S_IRUSR) {
            mode |= (S_IRGRP | S_IROTH);
        }

        // Add group/other execute permissions if user can execute
        if (mode & S_IXUSR) {
            mode |= (S_IXGRP | S_IXOTH);
        }

		// Apply new permissions
		if (chmod(path_utf8.c_str(), mode) != 0) {
			const int err = errno;
			write_log(_T("fsdb_set_file_attrs: chmod failed for '%s': %s\n"),
					 aino->nname, strerror(err));
			return host_errno_to_dos_errno(err);
		}

		fsdb_file_info info;
		fsdb_init_file_info(&info);
		fsdb_read_uaem(aino->nname, &info);
		info.mode = aino->amigaos_mode ^ 0xf;
		const int uaem_err = fsdb_write_uaem(aino->nname, &info);
		if (info.comment) {
			xfree(info.comment);
		}
		if (uaem_err != 0) {
			return uaem_err;
		}

		aino->dirty = 1;
		return 0;
    }
    catch (const std::exception& e) {
        write_log(_T("fsdb_set_file_attrs: exception for '%s': %s\n"),
                 aino->nname, e.what());
        return ERROR_OBJECT_NOT_AROUND;
    }
}

static void find_nname_case(const char* dir_path, char** name)
{
	if (!dir_path || !name || !*name) {
		return;
	}

	const auto dir_path_utf8 = iso_8859_1_to_utf8(std::string_view(dir_path));
	DIR* dir = opendir(dir_path_utf8.c_str());
	if (!dir) {
		return;
	}

	std::string search_latin1;
	const char* search_name = *name;
	if (utf8_to_latin1_string(std::string_view(*name), search_latin1)) {
		search_name = search_latin1.c_str();
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		std::string entry_latin1;
		const char* entry_name = entry->d_name;
		if (utf8_to_latin1_string(std::string_view(entry->d_name), entry_latin1)) {
			entry_name = entry_latin1.c_str();
		}
		if (strcasecmp(entry_name, search_name) == 0) {
			free(*name);
			*name = strdup(entry->d_name);
			break;
		}
	}

	closedir(dir);
}

a_inode* custom_fsdb_lookup_aino_aname(a_inode* base, const TCHAR* aname)
{
	if (!base || !base->nname || !aname) {
		return nullptr;
	}

	char* encoded = aname_to_nname(aname, 0);
	if (!encoded) {
		return nullptr;
	}

	find_nname_case(base->nname, &encoded);
	TCHAR* full_nname = build_nname(base->nname, encoded);
	const auto full_nname_utf8 = iso_8859_1_to_utf8(std::string_view(full_nname));
	struct stat statbuf {};
	if (stat(full_nname_utf8.c_str(), &statbuf) != 0) {
		free(encoded);
		xfree(full_nname);
		return nullptr;
	}

	a_inode* aino = xcalloc(a_inode, 1);
	if (!aino) {
		free(encoded);
		xfree(full_nname);
		return nullptr;
	}

	aino->aname = nname_to_aname(encoded, 0);
	aino->nname = full_nname;
	aino->dir = S_ISDIR(statbuf.st_mode) ? 1 : 0;
	aino->amigaos_mode = ((S_IXUSR & statbuf.st_mode ? 0 : A_FIBF_EXECUTE)
		| (S_IWUSR & statbuf.st_mode ? 0 : A_FIBF_WRITE)
		| (S_IRUSR & statbuf.st_mode ? 0 : A_FIBF_READ));
#if defined(AMIBERRY)
	aino->amigaos_mode &= ~A_FIBF_EXECUTE;
	aino->amigaos_mode &= ~A_FIBF_READ;
#endif
	aino->comment = nullptr;
	aino->has_dbentry = 0;
	aino->dirty = 0;
	aino->db_offset = 0;

	fsdb_file_info info;
	fsdb_init_file_info(&info);
	if (fsdb_read_uaem(full_nname, &info) == 0) {
		aino->amigaos_mode = info.mode ^ 0xf;
		if (info.comment) {
			aino->comment = info.comment;
			info.comment = nullptr;
		}
	}
	if (info.comment) {
		xfree(info.comment);
	}

	free(encoded);
	return aino;
}

a_inode* custom_fsdb_lookup_aino_nname(a_inode* base, const TCHAR* nname)
{
	if (!base || !base->nname || !nname) {
		return nullptr;
	}

	TCHAR* full_nname = build_nname(base->nname, nname);
	const auto full_nname_utf8 = iso_8859_1_to_utf8(std::string_view(full_nname));
	struct stat statbuf {};
	if (stat(full_nname_utf8.c_str(), &statbuf) != 0) {
		xfree(full_nname);
		return nullptr;
	}

	a_inode* aino = xcalloc(a_inode, 1);
	if (!aino) {
		xfree(full_nname);
		return nullptr;
	}

	aino->aname = nname_to_aname(nname, 0);
	aino->nname = full_nname;
	aino->dir = S_ISDIR(statbuf.st_mode) ? 1 : 0;
	aino->amigaos_mode = ((S_IXUSR & statbuf.st_mode ? 0 : A_FIBF_EXECUTE)
		| (S_IWUSR & statbuf.st_mode ? 0 : A_FIBF_WRITE)
		| (S_IRUSR & statbuf.st_mode ? 0 : A_FIBF_READ));
#if defined(AMIBERRY)
	aino->amigaos_mode &= ~A_FIBF_EXECUTE;
	aino->amigaos_mode &= ~A_FIBF_READ;
#endif
	aino->comment = nullptr;
	aino->has_dbentry = 0;
	aino->dirty = 0;
	aino->db_offset = 0;

	fsdb_file_info info;
	fsdb_init_file_info(&info);
	if (fsdb_read_uaem(full_nname, &info) == 0) {
		aino->amigaos_mode = info.mode ^ 0xf;
		if (info.comment) {
			aino->comment = info.comment;
			info.comment = nullptr;
		}
	}
	if (info.comment) {
		xfree(info.comment);
	}

	return aino;
}

int custom_fsdb_used_as_nname(a_inode* base, const TCHAR* nname)
{
	return 1;
}

int same_aname(const char* an1, const char* an2)
{
    // Input validation
    if (!an1 || !an2) {
        write_log(_T("same_aname: null input string(s)\n"));
        return 0;  // Not equal if either is null
    }

    try {
        // Fast path for identity check
        if (an1 == an2) {
            return 1;  // Same pointer = same string
        }

        // Fast path for ASCII-only strings
        bool ascii_only = true;
        const unsigned char* s1 = reinterpret_cast<const unsigned char*>(an1);
        const unsigned char* s2 = reinterpret_cast<const unsigned char*>(an2);

        // Check lengths first for early exit
        if (strlen(an1) != strlen(an2)) {
            return 0;  // Different lengths can't be equal
        }

        // Compare characters with proper Latin-1 case folding
        while (*s1 && *s2) {
            unsigned char c1 = *s1++;
            unsigned char c2 = *s2++;

            // Check for Latin-1 characters
            if (c1 > 127 || c2 > 127) {
                ascii_only = false;

                // Convert Latin-1 uppercase to lowercase (Å→å, Ä→ä, Ö→ö, etc.)
                if (c1 >= 192 && c1 <= 214) c1 += 32;  // A-O with diacritics
                else if (c1 >= 216 && c1 <= 222) c1 += 32;  // U-Y with diacritics

                if (c2 >= 192 && c2 <= 214) c2 += 32;
                else if (c2 >= 216 && c2 <= 222) c2 += 32;
            } else {
                // ASCII character - simple tolower
                if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
                if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
            }

            if (c1 != c2) {
                return 0;  // Characters differ
            }
        }

        // If we only saw ASCII, use the optimized function for final verification
        if (ascii_only) {
            return strcasecmp(an1, an2) == 0;
        }

        // If we reach here with non-ASCII content, our manual comparison was complete
        return 1;
    }
    catch (const std::exception& e) {
        write_log(_T("same_aname: exception comparing strings: %s\n"), e.what());
        return strcasecmp(an1, an2) == 0;  // Fall back to standard comparison
    }
}

/* Return nonzero if we can represent the amigaos_mode of AINO within the
 * native FS.  Return zero if that is not possible.  */
int fsdb_mode_representable_p(const a_inode* aino, int amigaos_mode)
{
	// Input validation
	if (!aino) {
		write_log(_T("fsdb_mode_representable_p: null aino provided\n"));
		return 0;
	}

	// Amiga protection bits are inverted: bit set = protection on (denied)
	// We need to focus on the lower 4 bits: RWED (Read/Write/Execute/Delete)
	// XOR with 15 (binary 1111) inverts these bits for easier comparison
	const int flipped_mode = amigaos_mode ^ 15;
	const int rwed_bits = flipped_mode & 15;  // Extract just RWED bits

	// A script bit (bit 6) is not representable in most host filesystems
	if (flipped_mode & A_FIBF_SCRIPT)
		return 0;

	// Full access (----rwed) is always representable
	if (rwed_bits == 15)
		return 1;

	// Execute bit must be set for any representable mode
	if (!(flipped_mode & A_FIBF_EXECUTE))
		return 0;

	// Read bit must be set for any representable mode
	if (!(flipped_mode & A_FIBF_READ))
		return 0;

	// Read-only mode (----r-e-) is representable
	if (rwed_bits == (A_FIBF_READ | A_FIBF_EXECUTE))
		return 1;

	// Any other combination is not representable on host filesystem
	return 0;
}

TCHAR* fsdb_create_unique_nname(const a_inode* base, const TCHAR* suggestion)
{
	unique_ptr<char, decltype(&free)> nname(aname_to_nname(suggestion, 0), free);
	TCHAR* p = build_nname(base->nname, nname.get());
	return p;
}

// Get local time in secs, starting from 01.01.1970
uae_u32 getlocaltime()
{
	using namespace std::chrono;
	const auto now = system_clock::now();
	return static_cast<uae_u32>(duration_cast<seconds>(now.time_since_epoch()).count());
}
