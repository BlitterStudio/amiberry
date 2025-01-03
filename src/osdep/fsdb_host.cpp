/*
 * UAE - The Un*x Amiga Emulator
 *
 * Library of functions to make emulated filesystem as independent as
 * possible of the host filesystem's capabilities.
 *
 * Copyright 1997 Mathias Ortmann
 * Copyright 1999 Bernd Schmidt
 * Copyright 2012 Frode Solheim
 */

#include <cstring>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WINDOWS
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

static TCHAR evilchars[NUM_EVILCHARS] = { '%', '\\', '*', '?', '\"', '/', '|', '<', '>' };
static char hex_chars[] = "0123456789abcdef";
#define UAEFSDB_BEGINS _T("__uae___")

static char* aname_to_nname(const char* aname, const int ascii)
{
	const size_t len = strlen(aname);
	unsigned int repl_1 = UINT_MAX;
	unsigned int repl_2 = UINT_MAX;

	TCHAR a = aname[0];
	TCHAR b = (a == '\0' ? a : aname[1]);
	TCHAR c = (b == '\0' ? b : aname[2]);
	TCHAR d = (c == '\0' ? c : aname[3]);

	if (a >= 'a' && a <= 'z') a -= 32;
	if (b >= 'a' && b <= 'z') b -= 32;
	if (c >= 'a' && c <= 'z') c -= 32;

	// reserved dos devices in Windows
	size_t ll = 0;
	if (a == 'A' && b == 'U' && c == 'X') ll = 3; // AUX
	if (a == 'C' && b == 'O' && c == 'N') ll = 3; // CON
	if (a == 'P' && b == 'R' && c == 'N') ll = 3; // PRN
	if (a == 'N' && b == 'U' && c == 'L') ll = 3; // NUL
	if (a == 'L' && b == 'P' && c == 'T' && (d >= '0' && d <= '9')) ll = 4; // LPT#
	if (a == 'C' && b == 'O' && c == 'M' && (d >= '0' && d <= '9')) ll = 4; // COM#
	// AUX.anything, CON.anything etc... are also illegal names in Windows
	if (ll && (len == ll || (len > ll && aname[ll] == '.'))) {
		repl_1 = 2;
	}

	// spaces and periods at the end are a no-no in Windows
	const auto ei = len - 1;
	if (aname[ei] == '.' || aname[ei] == ' ') {
		repl_2 = ei;
	}

	// allocating for worst-case scenario here (max replacements)
	const auto buf = static_cast<char*>(malloc(len * 3 + 1));
	char* p = buf;

	for (unsigned int i = 0; i < len; i++) {
		const char x = aname[i];
		int repl = 0;
		if (i == repl_1) {
			repl = 1;
		}
		else if (i == repl_2) {
			repl = 2;
		}
		else if (x < 32) {
			// these are not allowed on Windows
			repl = 1;
		}
		else if (ascii && x > 127) {
			repl = 1;
		}
		for (const char evilchar : evilchars)
		{
			if (x == evilchar) {
				repl = 1;
				break;
			}
		}
		if (i == len - 1) {
			// last character, we can now check the file ending
			if (len >= 5 && strncasecmp(aname + len - 5, ".uaem", 5) == 0) {
				// we don't allow Amiga files ending with .uaem, so we replace
				// the last character
				repl = 1;
			}
		}
		if (repl) {
			*p++ = '%';
			*p++ = hex_chars[(x & 0xf0) >> 4];
			*p++ = hex_chars[x & 0xf];
		}
		else {
			*p++ = x;
		}
	}
	*p++ = '\0';

	if (ascii) {
		return buf;
	}

	const auto string_buf = string(buf);
	const auto result = iso_8859_1_to_utf8(string_buf);
	free(buf);
	char* char_result = strdup(result.c_str());
	return char_result;
}

/* Return nonzero for any name we can't create on the native filesystem.  */
static int fsdb_name_invalid_2(a_inode* aino, const TCHAR* name, int isDirectory)
{
	const int name_length = _tcslen(name);

	/* the reserved fsdb filename */
	if (_tcscmp(name, FSDB_FILE) == 0)
		return -1;

	if (isDirectory) {
		// '.' and '..' are not valid directory names
		if ((name[0] == '.' && name_length == 1) ||
			(name[0] == '.' && name[1] == '.' && name_length == 2))
			return -1;
	}

	/* these characters are *never* allowed */
	for (const char evilchar : evilchars)
	{
		if (_tcschr(name, evilchar) != nullptr)
			return 1;
	}

	return 0; /* the filename passed all checks, now it should be ok */
}

int fsdb_name_invalid(a_inode* aino, const TCHAR* n)
{
	const int v = fsdb_name_invalid_2(aino, n, 0);
	if (v <= 0)
		return v;
	write_log(_T("FILESYS: '%s' illegal filename\n"), n);
	return v;
}

int fsdb_name_invalid_dir(a_inode* aino, const TCHAR* n)
{
	const int v = fsdb_name_invalid_2(aino, n, 1);
	if (v <= 0)
		return v;
	write_log(_T("FILESYS: '%s' illegal filename\n"), n);
	return v;
}

int fsdb_exists(const TCHAR* nname)
{
	return fs_path_exists(nname);
}

bool my_utime(const char* name, const struct mytimeval* tv)
{
	struct mytimeval mtv {};
	struct timeval times[2];

	if (tv == nullptr) {
		struct timeval time {};
		struct timezone tz {};

		if (gettimeofday(&time, &tz) == -1)
			return false;

		mtv.tv_sec = time.tv_sec + tz.tz_minuteswest;
		mtv.tv_usec = time.tv_usec;
	}
	else {
		mtv.tv_sec = tv->tv_sec;
		mtv.tv_usec = tv->tv_usec;
	}

	times[0].tv_sec = mtv.tv_sec;
	times[0].tv_usec = mtv.tv_usec;
	times[1].tv_sec = mtv.tv_sec;
	times[1].tv_usec = mtv.tv_usec;
	if (utimes(name, times) == 0)
		return true;

	return false;
}

/* return supported combination */
int fsdb_mode_supported(const a_inode* aino)
{
	const int mask = aino->amigaos_mode;
	return mask;
}

/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
int fsdb_fill_file_attrs(a_inode* base, a_inode* aino)
{
	struct stat statbuf {};
	/* This really shouldn't happen...  */
	const auto output = iso_8859_1_to_utf8(string(aino->nname));

	if (stat(output.c_str(), &statbuf) == -1)
		return 0;
	aino->dir = S_ISDIR(statbuf.st_mode) ? 1 : 0;
	aino->amigaos_mode = ((S_IXUSR & statbuf.st_mode ? 0 : A_FIBF_EXECUTE)
		| (S_IWUSR & statbuf.st_mode ? 0 : A_FIBF_WRITE)
		| (S_IRUSR & statbuf.st_mode ? 0 : A_FIBF_READ));

#if defined(AMIBERRY)
	// Always give execute & read permission
	// Temporary do this for raspberry...
	aino->amigaos_mode &= ~A_FIBF_EXECUTE;
	aino->amigaos_mode &= ~A_FIBF_READ;
#endif
	return 1;
}

int fsdb_set_file_attrs(a_inode* aino)
{
	struct stat statbuf {};
	const uae_u32 mask = aino->amigaos_mode;

	const auto input = string(aino->nname);
	const auto output = iso_8859_1_to_utf8(input);

	if (stat(output.c_str(), &statbuf) == -1)
		return ERROR_OBJECT_NOT_AROUND;

	int mode = statbuf.st_mode;

	mode = (mask & A_FIBF_READ) ? (mode & ~S_IRUSR) : (mode | S_IRUSR);
	mode = (mask & A_FIBF_WRITE) ? (mode & ~S_IWUSR) : (mode | S_IWUSR);
	mode = (mask & A_FIBF_EXECUTE) ? (mode & ~S_IXUSR) : (mode | S_IXUSR);

	if (chmod(output.c_str(), mode) != 0)
		return ERROR_OBJECT_NOT_AROUND;

	aino->dirty = 1;
	return 0;
}

int same_aname(const char* an1, const char* an2)
{
	// FIXME: latin 1 chars?
	// FIXME: compare with latin1 table in charset/filesys_host/fsdb_host
	return strcasecmp(an1, an2) == 0;
}
/* Return nonzero if we can represent the amigaos_mode of AINO within the
 * native FS.  Return zero if that is not possible.  */
int fsdb_mode_representable_p(const a_inode* aino, int amigaos_mode)
{
	// XOR operation with 15 to flip the last 4 bits of amigaos_mode
	const int flipped_amigaos_mode = amigaos_mode ^ 15;

	// Check if the script bit is set in the flipped mode
	if (flipped_amigaos_mode & A_FIBF_SCRIPT) /* script */
		return 0;

	// Check if the last 4 bits of the flipped mode are all set (xxxxRWED == OK)
	if ((flipped_amigaos_mode & 15) == 15)
		return 1;

	// Check if the execute bit is not set in the flipped mode
	if (!(flipped_amigaos_mode & A_FIBF_EXECUTE)) /* not executable */
		return 0;

	// Check if the read bit is not set in the flipped mode
	if (!(flipped_amigaos_mode & A_FIBF_READ)) /* not readable */
		return 0;

	// Check if only the read and execute bits are set in the flipped mode (----RxEx == ReadOnly)
	if ((flipped_amigaos_mode & 15) == (A_FIBF_READ | A_FIBF_EXECUTE))
		return 1;

	return 0;
}

TCHAR* fsdb_create_unique_nname(const a_inode* base, const TCHAR* suggestion)
{
	char* nname = aname_to_nname(suggestion, 0);
	TCHAR* p = build_nname(base->nname, nname);
	free(nname);
	return p;
}

// Get local time in secs, starting from 01.01.1970
uae_u32 getlocaltime()
{
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    return mktime(timeinfo);
}