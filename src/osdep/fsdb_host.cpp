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

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"

#include "uae/uae.h"
#include "fsdb.h"
#include "zfile.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef WINDOWS
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif
#include <string.h>
#include <limits.h>

#include "fsdb_host.h"

#include "uae.h"

#define NUM_EVILCHARS 9
static TCHAR evilchars[NUM_EVILCHARS] = { '%', '\\', '*', '?', '\"', '/', '|', '<', '>' };
static char hex_chars[] = "0123456789abcdef";
#define UAEFSDB_BEGINS _T("__uae___")

static char* aname_to_nname(const char* aname, int ascii)
{
    size_t len = strlen(aname);
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
    // AUX.anything, CON.anything etc.. are also illegal names in Windows
    if (ll && (len == ll || (len > ll && aname[ll] == '.'))) {
        repl_1 = 2;
    }

    // spaces and periods at the end are a no-no in Windows
    int ei = len - 1;
    if (aname[ei] == '.' || aname[ei] == ' ') {
        repl_2 = ei;
    }

    // allocating for worst-case scenario here (max replacements)
    char* buf = (char*)malloc(len * 3 + 1);
    char* p = buf;

    int repl, j;
    unsigned char x;
    for (unsigned int i = 0; i < len; i++) {
        x = (unsigned char)aname[i];
        repl = 0;
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
        for (j = 0; j < NUM_EVILCHARS; j++) {
            if (x == evilchars[j]) {
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

    auto string_buf = string(buf);
    auto result = iso_8859_1_to_utf8(string_buf);
    free(buf);
    char* char_result = strdup(result.c_str());
    return char_result;
}

/* Return nonzero for any name we can't create on the native filesystem.  */
static int fsdb_name_invalid_2(a_inode* aino, const TCHAR* n, int dir)
{
    int i;
    int l = _tcslen(n);

    /* the reserved fsdb filename */
    if (_tcscmp(n, FSDB_FILE) == 0)
        return -1;

    if (dir) {
        if (n[0] == '.' && l == 1)
            return -1;
        if (n[0] == '.' && n[1] == '.' && l == 2)
            return -1;
    }

    /* these characters are *never* allowed */
    for (i = 0; i < NUM_EVILCHARS; i++) {
        if (_tcschr(n, evilchars[i]) != 0)
            return 1;
    }

    return 0; /* the filename passed all checks, now it should be ok */
}

int fsdb_name_invalid(a_inode* aino, const TCHAR* n)
{
    int v = fsdb_name_invalid_2(aino, n, 0);
    if (v <= 0)
        return v;
    write_log(_T("FILESYS: '%s' illegal filename\n"), n);
    return v;
}

int fsdb_name_invalid_dir(a_inode* aino, const TCHAR* n)
{
    int v = fsdb_name_invalid_2(aino, n, 1);
    if (v <= 0)
        return v;
    write_log(_T("FILESYS: '%s' illegal filename\n"), n);
    return v;
}

int fsdb_exists(const TCHAR* nname)
{
    return fs_path_exists(nname);
}

bool my_utime(const char* name, struct mytimeval* tv)
{
    struct mytimeval mtv{};
    struct timeval times[2];

    if (tv == NULL) {
        struct timeval time{};
        struct timezone tz{};

        gettimeofday(&time, &tz);
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
    int mask = aino->amigaos_mode;
    return mask;
}

/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
int fsdb_fill_file_attrs(a_inode* base, a_inode* aino)
{
    struct stat statbuf{};
    /* This really shouldn't happen...  */
    auto input = string(aino->nname);
    auto output = iso_8859_1_to_utf8(input);
	
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
    struct stat statbuf{};
    int mode;
    uae_u32 mask = aino->amigaos_mode;

    auto input = string(aino->nname);
    auto output = iso_8859_1_to_utf8(input);
	
    if (stat(output.c_str(), &statbuf) == -1)
        return ERROR_OBJECT_NOT_AROUND;

    mode = statbuf.st_mode;

    if (mask & A_FIBF_READ)
        mode &= ~S_IRUSR;
    else
        mode |= S_IRUSR;

    if (mask & A_FIBF_WRITE)
        mode &= ~S_IWUSR;
    else
        mode |= S_IWUSR;

    if (mask & A_FIBF_EXECUTE)
        mode &= ~S_IXUSR;
    else
        mode |= S_IXUSR;

    chmod(output.c_str(), mode);

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
    int mask = amigaos_mode ^ 15;

    if (0 && aino->dir)
        return amigaos_mode == 0;

    if (mask & A_FIBF_SCRIPT) /* script */
        return 0;
    if ((mask & 15) == 15) /* xxxxRWED == OK */
        return 1;
    if (!(mask & A_FIBF_EXECUTE)) /* not executable */
        return 0;
    if (!(mask & A_FIBF_READ)) /* not readable */
        return 0;
    if ((mask & 15) == (A_FIBF_READ | A_FIBF_EXECUTE)) /* ----RxEx == ReadOnly */
        return 1;
    return 0;
}

TCHAR* fsdb_create_unique_nname(a_inode* base, const TCHAR* suggestion)
{
    char* nname = aname_to_nname(suggestion, 0);
    TCHAR* p = build_nname(base->nname, nname);
    free(nname);
    return p;
}

// Get local time in secs, starting from 01.01.1970
uae_u32 getlocaltime(void)
{
	return time(NULL); // ToDo: convert UTC to local time...
}