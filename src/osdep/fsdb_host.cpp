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
static TCHAR evilchars[NUM_EVILCHARS] = { '%', '\\', '*', '?', '\"', '/', '|',
        '<', '>' };

enum uaem_write_flags_t {
    WF_NEVER = (1 << 0),
    WF_ALWAYS = (1 << 1),
    WF_EXISTS = (1 << 2),
    WF_NOTE = (1 << 3),
    WF_TIME = (1 << 4),
    WF_HOLD = (1 << 5), // FIXME HOLD?
    WF_SCRIPT = (1 << 6),
    WF_PURE = (1 << 7),
    WF_ARCHIVE = (1 << 8),
    WF_READ = (1 << 9),
    WF_WRITE = (1 << 10),
    WF_EXECUTE = (1 << 11),
    WF_DELETE = (1 << 12),
};

static char g_fsdb_file_path_buffer[PATH_MAX + 1] = { };
static unsigned int g_uaem_flags = WF_ALWAYS;

//Begin: Time functions from FS-UAE
struct tm* fs_localtime_r(const time_t* timep, struct tm* result)
{
#ifdef HAVE_LOCALTIME_R
    return localtime_r(timep, result);
#else
    /* On Windows, localtime is thread-safe due to using thread-local
     * storage, using mutexes for other platforms (and hopefully no-one
     * else calls localtime simultaneously...) */

    struct tm* tm = localtime(timep);
    if (tm == NULL) {
        write_log("WARNING: localtime - invalid time_t (%d)\n", *timep);
    }
    else {
        *result = *tm;
    }

    if (tm == NULL) {
        return NULL;
    }
    return result;
#endif
}

struct tm* fs_gmtime_r(const time_t* timep, struct tm* result)
{
#ifdef HAVE_GMTIME_R
    return gmtime_r(timep, result);
#else
    /* on Windows, gmtime is thread-safe due to using thread-local
     * storage, using mutexes for other platforms (and hopefully no-one
     * else calls gmtime simultaneously...) */

    struct tm* tm = gmtime(timep);
    if (tm == NULL) {
        write_log("WARNING: gmtime - invalid time_t (%d)\n", *timep);
    }
    else {
        *result = *tm;
    }

    if (tm == NULL) {
        return NULL;
    }
    return result;
#endif
}

time_t fs_timegm(struct tm* tm)
{
    time_t ret;
    /* Code adapted from the man page of timegm */
    char* tz;

    tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz) {
        setenv("TZ", tz, 1);
    }
    else {
        unsetenv("TZ");
    }
    tzset();
    return ret;
}

int fs_get_local_time_offset(time_t time)
{
    time_t t = time;
    struct tm lt;
    void* result1 = fs_localtime_r(&t, &lt);
    struct tm gt;
    void* result2 = fs_gmtime_r(&t, &gt);
    gt.tm_isdst = lt.tm_isdst;
    if (result1 == NULL || result2 == NULL) {
        return 0;
    }
    return t - mktime(&gt);
}
//End: Time functions from FS-UAE

#ifdef __cplusplus
extern "C" {
#endif

#define PARSE_WF(flag, name) \
    else if (*c == flag) { \
        write_log("- " #name "\n"); \
        g_uaem_flags |= name; \
    }

    void uae_set_uaem_write_flags_from_string(const char* flags)
    {
        assert(flags != NULL);
        write_log("uaem_write_flags = %s\n", flags);
        g_uaem_flags = 0;
        for (const char* c = flags; *c; c++) {
            if (*c == '0') {
                /* usually used by itself to disable all flags (by not
                 * specifying any other) */
            }
            PARSE_WF('1', WF_ALWAYS)
                PARSE_WF('n', WF_NOTE)
                PARSE_WF('t', WF_TIME)
                PARSE_WF('h', WF_HOLD)
                PARSE_WF('s', WF_SCRIPT)
                PARSE_WF('p', WF_PURE)
                PARSE_WF('a', WF_ARCHIVE)
                PARSE_WF('r', WF_READ)
                PARSE_WF('w', WF_WRITE)
                PARSE_WF('e', WF_EXECUTE)
                PARSE_WF('d', WF_DELETE)
            else if (*c == ' ') {
                /* ignore space */
            }
            else {
                write_log("- unknown flag '%c'\n", *c);
            }
        }
    }

#ifdef __cplusplus
}
#endif

static char* fsdb_file_path(const char* nname)
{
    strncpy(g_fsdb_file_path_buffer, nname, PATH_MAX);
    return g_fsdb_file_path_buffer;
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

static uae_u32 filesys_parse_mask(uae_u32 mask)
{
    return mask ^ 0xf;
}

int fsdb_exists(const TCHAR* nname)
{
    return fs_path_exists(nname);
}

#define PERM_OFFSET 0
#define PERM_LEN 8
//#define DAYS_LEN 5
//#define MINS_LEN 4
//#define TICKS_LEN 4

#define COMMENT_OFFSET 25

//#define TIME_OFFSET PERM_LEN + 1
//#define TIME_LEN (1 + DAYS_LEN + 1 + MINS_LEN + 1 + TICKS_LEN + 1)
#define TIME_LEN 22

void fsdb_init_file_info(fsdb_file_info* info)
{
    info->days = 0;
    info->mins = 0;
    info->ticks = 0;
    info->comment = NULL;
    info->mode = A_FIBF_READ | A_FIBF_WRITE | A_FIBF_EXECUTE | A_FIBF_DELETE;

    if (uae_deterministic_mode()) {
        // leave time at 0, 0, 0
    }
    else {
        struct timeval time{};
        struct timezone tz{};
        gettimeofday(&time, &tz);
        struct mytimeval mtv{};
        mtv.tv_sec = time.tv_sec + tz.tz_minuteswest;
        mtv.tv_usec = time.tv_usec;
        timeval_to_amiga(&mtv, &info->days, &info->mins, &info->ticks, 50);
    }
}

static FILE* fsdb_open_meta_file(const char* meta_file, const char* mode)
{
    FILE* f = fopen(meta_file, mode);
    return f;
}

/* return supported combination */
int fsdb_mode_supported(const a_inode* aino)
{
	int mask = aino->amigaos_mode;
	return mask;
}

static char hex_chars[] = "0123456789abcdef";

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

    std::string str;
    str.assign(buf);
    const auto result = iso_8859_1_to_utf8(str);
    xfree(buf);

    return (char*)result.c_str();
}

static unsigned char char_to_hex(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    }
    return 0;
}

int same_aname(const char* an1, const char* an2)
{
    // FIXME: latin 1 chars?
    // FIXME: compare with latin1 table in charset/filesys_host/fsdb_host
    return strcasecmp(an1, an2) == 0;
}

static char* nname_to_aname(const char* nname, int noconvert)
{
    char* cresult;
    int len = strlen(nname);
    if (noconvert) {
        cresult = my_strdup(nname);
    }
    else {
        cresult = Utf8ToLatin1String((char*)nname);
    }

    if (!cresult) {
        write_log("[WARNING] nname_to_aname %s => Failed\n", nname);
        return NULL;
    }

    char* result = my_strdup(cresult);
    unsigned char* p = (unsigned char*)result;
    for (int i = 0; i < len; i++) {
        unsigned char c = cresult[i];
        if (c == '%' && i < len - 2) {
            *p++ = (char_to_hex(cresult[i + 1]) << 4) |
                char_to_hex(cresult[i + 2]);
            i += 2;
        }
        else {
            *p++ = c;
        }
    }
    *p++ = '\0';
    free(cresult);
    return result;
}

/* Return nonzero if we can represent the amigaos_mode of AINO within the
 * native FS.  Return zero if that is not possible.
 */
int fsdb_mode_representable_p(const a_inode* aino, int amigaos_mode)
{
    return 1;
}

TCHAR* fsdb_create_unique_nname(a_inode* base, const TCHAR* suggestion)
{
    char* nname = aname_to_nname(suggestion, 0);
    TCHAR* p = build_nname(base->nname, nname);
    free(nname);
    return p;
}

/* Return 1 if the nname is a special host-only name which must be translated
 * to aname using fsdb.
 */
int custom_fsdb_used_as_nname(a_inode* base, const TCHAR* nname)
{
    return 1;
}

int fs_stat(const char* path, struct fs_stat* buf)
{
    struct stat st {};
    int result = stat(path, &st);
    if (result == 0) {
        buf->atime = st.st_atime;
        buf->mtime = st.st_mtime;
        buf->ctime = st.st_ctime;
        buf->size = st.st_size;
        buf->mode = st.st_mode;
    }
    return result;
}

static int fsdb_get_file_info(const char* nname, fsdb_file_info* info)
{
    int error = 0;
    info->comment = NULL;
    if (!fs_path_exists(nname)) {
        info->type = 0;
        return ERROR_OBJECT_NOT_AROUND;
    }

    info->type = my_existsdir(nname) ? 2 : 1;
    info->mode = 0;

    int read_perm = 0;
    int read_time = 0;

    auto* meta_file = (TCHAR*)malloc(MAX_DPATH);
    strcpy(meta_file, nname);
    strcat(meta_file, ".uaem");

    FILE* f = fsdb_open_meta_file(meta_file, "rb");
    int file_size = 0;
    if (f == NULL) {
        if (fs_path_exists(meta_file)) {
            error = host_errno_to_dos_errno(errno);
            write_log("WARNING: fsdb_get_file_info - could not open "
                "meta file for reading\n");
        }
    }
    else {
        fseek(f, 0, SEEK_END);
        file_size = ftell(f);
        fseek(f, 0, SEEK_SET);
    }

    char* data = (char*)malloc(file_size + 1);
    data[file_size] = '\0';
    char* p = data;
    char* end = data + file_size;
    if (end - data > 0) {
        /* file must be open, or (end - data) would be zero */
        int count = fread(data, 1, file_size, f);
        if (count != file_size) {
            write_log("WARNING: could not read permissions "
                "from %s (%d)\n", meta_file, errno);
            error = host_errno_to_dos_errno(errno);
            end = data;
        }
    }
    if (f != NULL) {
        fclose(f);
    }

    if ((end - p) >= 3) {
        if ((uint8_t)p[0] == 0xef && (uint8_t)p[1] == 0xbb && (uint8_t)p[2] == 0xbf) {
            p += 3;
        }
    }

    if ((end - p) >= 8) {
        info->mode |= *(p++) == 'h' ? A_FIBF_HIDDEN : 0;
        info->mode |= *(p++) == 's' ? A_FIBF_SCRIPT : 0;
        info->mode |= *(p++) == 'p' ? A_FIBF_PURE : 0;
        info->mode |= *(p++) == 'a' ? A_FIBF_ARCHIVE : 0;
        info->mode |= *(p++) == 'r' ? A_FIBF_READ : 0;
        info->mode |= *(p++) == 'w' ? A_FIBF_WRITE : 0;
        info->mode |= *(p++) == 'e' ? A_FIBF_EXECUTE : 0;
        info->mode |= *(p++) == 'd' ? A_FIBF_DELETE : 0;
        read_perm = 1;
    }

    p++;

    if ((end - p) >= TIME_LEN) {
        //p[TIME_LEN - 1] = '\0';
        struct tm tm;
        tm.tm_isdst = -1;

        // haven't got strptime on Windows...

        p[4] = '\0';
        tm.tm_year = atoi(p) - 1900;
        p += 5;
        p[2] = '\0';
        tm.tm_mon = atoi(p) - 1;
        p += 3;
        p[2] = '\0';
        tm.tm_mday = atoi(p);
        p += 3;

        p[2] = '\0';
        tm.tm_hour = atoi(p);
        p += 3;
        p[2] = '\0';
        tm.tm_min = atoi(p);
        p += 3;
        p[2] = '\0';
        tm.tm_sec = atoi(p);
        p += 3;

        p[2] = '\0';
        int sub = atoi(p);
        p += 3;

        struct mytimeval mtv{};
        mtv.tv_sec = fs_timegm(&tm);
        if (mtv.tv_sec == -1) {
            // an error occured while parsing time - invalid time
            write_log("- warning, error reading time from metadata\n");
        }
        else {
            mtv.tv_usec = sub * 10000;
            timeval_to_amiga(&mtv, &info->days, &info->mins, &info->ticks, 50);
            read_time = 1;
        }
    }

    if ((end - p) > 0) {
        ptrdiff_t len = end - p;
        info->comment = (char*)malloc(len + 1);
        if (info->comment) {
            char* o = info->comment;
            while (p < end && *p != '\r' && *p != '\n') {
                *o++ = *p++;
            }
            *o = '\0';
        }
    }

    xfree(data);
    xfree(meta_file);

    if (!read_perm) {
        info->mode |= A_FIBF_READ;
        info->mode |= A_FIBF_WRITE;
        info->mode |= A_FIBF_EXECUTE;
        info->mode |= A_FIBF_DELETE;
        if (!uae_deterministic_mode()) {
            // FIXME: remove WRITE and DELETE if file is not writable
        }
    }

    if (!read_time) {
        if (uae_deterministic_mode()) {
            // this is not a very good solution. For instance, WB 1.3 does
            // not update .info files correctly when the file date/time is
            // constant.
            info->days = 0;
            info->mins = 0;
            info->ticks = 0;
        }
        else {
            struct fs_stat buf{};
            if (fs_stat(nname, &buf) != 0) {
            }

            mytimeval mtv{};
            mtv.tv_sec = buf.mtime + fs_get_local_time_offset(buf.mtime);
            mtv.tv_usec = buf.mtime_nsec / 1000;
            timeval_to_amiga(&mtv, &info->days, &info->mins, &info->ticks, 50);
        }
    }
    return error;
}

int fs_set_file_time(const char* path, struct timeval* t)
{
    struct timeval tv[2];
    tv[0].tv_sec = t->tv_sec;
    tv[1].tv_sec = t->tv_sec;
    tv[0].tv_usec = t->tv_usec;
    tv[1].tv_usec = t->tv_usec;
    return utimes(path, tv);
}

int fsdb_set_file_info(const char* nname, fsdb_file_info* info)
{
    FILE* f = NULL;
    int error = 0;
    if (!fs_path_exists(nname)) {
        error = ERROR_OBJECT_NOT_AROUND;
    }

    int need_metadata_file = 0;

    if (info->comment != NULL) {
        if ((g_uaem_flags & WF_NOTE)) {
            need_metadata_file |= WF_NOTE;
        }
    }

    if (info->mode != (A_FIBF_READ | A_FIBF_WRITE | A_FIBF_EXECUTE | \
        A_FIBF_DELETE)) {
        /* check for bits set */
        if ((g_uaem_flags & WF_HOLD) && (info->mode & A_FIBF_HIDDEN)) {
            need_metadata_file |= WF_HOLD;
        }
        if ((g_uaem_flags & WF_SCRIPT) && (info->mode & A_FIBF_SCRIPT)) {
            need_metadata_file |= WF_SCRIPT;
        }
        if ((g_uaem_flags & WF_PURE) && (info->mode & A_FIBF_PURE)) {
            need_metadata_file |= WF_PURE;
        }
        if ((g_uaem_flags & WF_ARCHIVE) && (info->mode & A_FIBF_ARCHIVE)) {
            need_metadata_file |= WF_ARCHIVE;
        }
        /* check for bits NOT set */
        if ((g_uaem_flags & WF_READ) && !(info->mode & A_FIBF_READ)) {
            need_metadata_file |= WF_READ;
        }
        if ((g_uaem_flags & WF_WRITE) && !(info->mode & A_FIBF_WRITE)) {
            need_metadata_file |= WF_WRITE;
        }
        if ((g_uaem_flags & WF_EXECUTE) && !(info->mode & A_FIBF_EXECUTE)) {
            need_metadata_file |= WF_EXECUTE;
        }
        if ((g_uaem_flags & WF_DELETE) && !(info->mode & A_FIBF_DELETE)) {
            need_metadata_file |= WF_DELETE;
        }
    }

    struct mytimeval mtv{};
    amiga_to_timeval(&mtv, info->days, info->mins, info->ticks, 50);
    mtv.tv_sec -= fs_get_local_time_offset(mtv.tv_sec);

    struct timeval tv{};
    tv.tv_sec = mtv.tv_sec;
    tv.tv_usec = mtv.tv_usec;
    if (fs_set_file_time(nname, &tv) != 0) {
        error = errno;
    }
    else {
        struct fs_stat buf{};
        if (fs_stat(nname, &buf) == 0) {
            if (buf.mtime == mtv.tv_sec &&
                buf.mtime_nsec == mtv.tv_usec * 1000) {
            }
        }
    }

    if ((g_uaem_flags & WF_ALWAYS)) {
        need_metadata_file |= WF_ALWAYS;
    }

    auto* meta_file = (TCHAR*)malloc(MAX_DPATH);
    strcpy(meta_file, nname);
    strcat(meta_file, ".uaem");
	
    if (fs_path_exists(meta_file)) {
        need_metadata_file |= WF_EXISTS;
    }
    if (!error && need_metadata_file) {
        write_log("need_metadata_file %d", need_metadata_file);
        if ((need_metadata_file & WF_ALWAYS)) {
            write_log(" WF_ALWAYS");
        }
        if ((need_metadata_file & WF_EXISTS)) {
            write_log(" WF_EXISTS");
        }
        if ((need_metadata_file & WF_NOTE)) {
            write_log(" WF_NOTE");
        }
        if ((need_metadata_file & WF_TIME)) {
            write_log(" WF_TIME");
        }
        if ((need_metadata_file & WF_HOLD)) {
            write_log(" WF_HOLD");
        }
        if ((need_metadata_file & WF_SCRIPT)) {
            write_log(" WF_SCRIPT");
        }
        if ((need_metadata_file & WF_PURE)) {
            write_log(" WF_PURE");
        }
        if ((need_metadata_file & WF_ARCHIVE)) {
            write_log(" WF_ARCHIVE");
        }
        if ((need_metadata_file & WF_READ)) {
            write_log(" WF_READ");
        }
        if ((need_metadata_file & WF_WRITE)) {
            write_log(" WF_WRITE");
        }
        if ((need_metadata_file & WF_EXECUTE)) {
            write_log(" WF_EXECUTE");
        }
        if ((need_metadata_file & WF_DELETE)) {
            write_log(" WF_DELETE");
        }
        write_log("\n");
        f = fsdb_open_meta_file(meta_file, "wb");
        if (f == NULL) {
            error = host_errno_to_dos_errno(errno);
            write_log("WARNING: fsdb_set_file_info - could not open "
                "meta file for writing\n");
        }
    }
    xfree(meta_file);

    if (!error && f != NULL) {
        char astr[] = "--------";
        if (info->mode & A_FIBF_HIDDEN) astr[0] = 'h';
        if (info->mode & A_FIBF_SCRIPT) astr[1] = 's';
        if (info->mode & A_FIBF_PURE) astr[2] = 'p';
        if (info->mode & A_FIBF_ARCHIVE) astr[3] = 'a';
        if (info->mode & A_FIBF_READ) astr[4] = 'r';
        if (info->mode & A_FIBF_WRITE) astr[5] = 'w';
        if (info->mode & A_FIBF_EXECUTE) astr[6] = 'e';
        if (info->mode & A_FIBF_DELETE) astr[7] = 'd';
        write_log("- writing mode %s\n", astr);

        if (fwrite(astr, 8, 1, f) != 1) {
            error = host_errno_to_dos_errno(errno);
        }
    }

    if (!error && f != NULL) {
        struct mytimeval mtv{};
        amiga_to_timeval(&mtv, info->days, info->mins, info->ticks, 50);

        // FIXME: reentrant?
        time_t secs = mtv.tv_sec;
        struct tm* gt = gmtime(&secs);

        if (fprintf(f, " %04d-%02d-%02d %02d:%02d:%02d.%02d ",
            1900 + gt->tm_year, 1 + gt->tm_mon, gt->tm_mday,
            gt->tm_hour, gt->tm_min, gt->tm_sec,
            mtv.tv_usec / 10000) < 0) {
            error = host_errno_to_dos_errno(errno);
        }
    }

    if (!error && f != NULL) {
        if (info->comment) {
            write_log("- writing comment %s\n", info->comment);
            int len = strlen(info->comment);
            if (len && fwrite(info->comment, len, 1, f) != 1) {
                error = host_errno_to_dos_errno(errno);
            }
        }
    }

    if (!error && f != NULL) {
        fprintf(f, "\n");
    }
    if (f != NULL) {
        fclose(f);
    }
    if (info->comment) {
        xfree(info->comment);
    }
    return error;
}

void fsdb_get_file_time(a_inode* aino, int* days, int* mins, int* ticks)
{
	fsdb_file_info info;
	fsdb_get_file_info(aino->nname, &info);
	if (info.type) {
		*days = info.days;
		*mins = info.mins;
		*ticks = info.ticks;
		if (info.comment) {
			free(info.comment);
		}
	}
	else {
		// FIXME: file does not exist
		*days = 0;
		*mins = 0;
		*ticks = 0;
	}
}

bool my_utime(const char* name, struct mytimeval* tv)
{
    int days = 0;
    int mins = 0;
    int ticks = 0;

    if (tv == NULL) {
        struct timeval time{};
        struct timezone tz{};
        gettimeofday(&time, &tz);
        struct mytimeval mtv{};
        mtv.tv_sec = time.tv_sec + tz.tz_minuteswest;
        mtv.tv_usec = time.tv_usec;
        timeval_to_amiga(&mtv, &days, &mins, &ticks, 50);
    }
    else {
        struct mytimeval mtv2{};
        mtv2.tv_sec = tv->tv_sec;
        mtv2.tv_usec = tv->tv_usec;
        timeval_to_amiga(&mtv2, &days, &mins, &ticks, 50);
    }

    if (!fs_path_exists(name)) {
        write_log("WARNING: fsdb_set_file_time file \"%s\" does not exist\n",
            name);
        my_errno = ERROR_OBJECT_NOT_AROUND;
        return 0;
    }

    fsdb_file_info info;
    fsdb_get_file_info(name, &info);
    info.days = days;
    info.mins = mins;
    info.ticks = ticks;
    my_errno = fsdb_set_file_info(name, &info);
    return my_errno == 0;
}

extern unsigned char g_latin1_lower_table[256];

static void lower_latin1(char* s)
{
    unsigned char* u = (unsigned char*)s;
    while (*u) {
        *u = g_latin1_lower_table[*u];
        u++;
    }
}

int fsdb_fill_file_attrs(a_inode* base, a_inode* aino)
{
    struct stat statbuf{};
    if (stat(aino->nname, &statbuf) == -1)
        return 0;
    fsdb_file_info info;
    fsdb_get_file_info(aino->nname, &info);
    //if (!info.type) {
    //    // file does not exist
    //    return 0;
    //}
    //aino->dir = info.type == 2;
    //aino->amigaos_mode = filesys_parse_mask(info.mode);
	
    aino->dir = S_ISDIR(statbuf.st_mode) ? 1 : 0;
    aino->amigaos_mode = ((S_IXUSR & statbuf.st_mode ? 0 : A_FIBF_EXECUTE)
        | (S_IWUSR & statbuf.st_mode ? 0 : A_FIBF_WRITE)
        | (S_IRUSR & statbuf.st_mode ? 0 : A_FIBF_READ));
#if defined(AMIBERRY)
    // Always give execute & read permission
    aino->amigaos_mode &= ~A_FIBF_EXECUTE;
    aino->amigaos_mode &= ~A_FIBF_READ;
#endif
    if (info.comment) {
        aino->comment = nname_to_aname(info.comment, 1);
        free(info.comment);
    }
    else {
        aino->comment = NULL;
    }
    return 1;
}

int fsdb_set_file_attrs(a_inode* aino)
{
    if (!fs_path_exists(aino->nname)) {
        write_log("WARNING: fsdb_set_file_attrs file \"%s\" does not exist\n",
            aino->nname);
        return ERROR_OBJECT_NOT_AROUND;
    }
    fsdb_file_info info;
    fsdb_get_file_info(aino->nname, &info);
    info.mode = filesys_parse_mask(aino->amigaos_mode);
    if (info.comment) {
        free(info.comment);
        info.comment = NULL;
    }
    if (aino->comment && aino->comment[0]) {
        info.comment = aname_to_nname(aino->comment, 1);
    }
    return fsdb_set_file_info(aino->nname, &info);
}

static void find_nname_case(const char* dir_path, char** name)
{
    // FIXME: should cache these results...!!
    auto* dir = opendir(dir_path);
    if (dir == NULL) {
        write_log("open dir %s failed\n", *name);
        return;
    }

    char* cmp_name = Utf8ToLatin1String(*name);
    if (cmp_name == NULL) {
        write_log("WARNING: could not convert to latin1: %s", *name);
        closedir(dir);
        return;
    }
    lower_latin1(cmp_name);

    struct dirent64* entry;
    while ((entry = readdir64(dir)) != NULL) {
        char* result = entry->d_name;
        char* cmp_result = Utf8ToLatin1String(result);
        if (cmp_result == NULL) {
            // file name could not be represented as ISO-8859-1, so it
            // will be ignored
            write_log("cannot convert name \"%s\" to ISO-8859-1 - ignoring\n",
                result);
            continue;
        }
        lower_latin1(cmp_result);

        if (strcmp(cmp_name, cmp_result) == 0) {
            // FIXME: memory leak, free name first?
            *name = my_strdup(result);
            break;
        }
        free(cmp_result);
    }
    closedir(dir);
    xfree(cmp_name);
}

a_inode* custom_fsdb_lookup_aino_aname(a_inode* base, const TCHAR* aname)
{
    char* nname = aname_to_nname(aname, 0);
    find_nname_case(base->nname, &nname);
    char* full_nname = build_nname(base->nname, (TCHAR*)nname);
    fsdb_file_info info;
    fsdb_get_file_info(full_nname, &info);
    if (!info.type) {
        if (info.comment) {
            free(info.comment);
            info.comment = NULL;
        }
        free(full_nname);
        free(nname);
        return NULL;
    }
    a_inode* aino = xcalloc(a_inode, 1);

    aino->aname = nname_to_aname(nname, 0);
    free(nname);
    aino->nname = full_nname;
    if (info.comment) {
        aino->comment = nname_to_aname(info.comment, 1);
        free(info.comment);
    }
    else {
        aino->comment = NULL;
    }
    aino->amigaos_mode = filesys_parse_mask(info.mode);
    aino->dir = info.type == 2;
    aino->has_dbentry = 0;
    aino->dirty = 0;
    aino->db_offset = 0;
    return aino;
}

a_inode* custom_fsdb_lookup_aino_nname(a_inode* base, const TCHAR* nname)
{
    //STUB("base=? nname=\"%s\"", nname);

    char* full_nname = build_nname(base->nname, (TCHAR*)nname);
    fsdb_file_info info;
    fsdb_get_file_info(full_nname, &info);
    if (!info.type) {
        if (info.comment) {
            free(info.comment);
            info.comment = NULL;
        }
        free(full_nname);
        return NULL;
    }
    a_inode* aino = xcalloc(a_inode, 1);
    aino->aname = nname_to_aname(nname, 0);
    aino->nname = full_nname;
    if (info.comment) {
        aino->comment = nname_to_aname(info.comment, 1);
        free(info.comment);
    }
    else {
        aino->comment = NULL;
    }
    aino->amigaos_mode = filesys_parse_mask(info.mode);
    aino->dir = info.type == 2;
    aino->has_dbentry = 0;
    aino->dirty = 0;
    aino->db_offset = 0;
    return aino;
}

unsigned char g_latin1_lower_table[256] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
        47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
        97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94,
        95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
        124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
        138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
        152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165,
        166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
        180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 224, 225,
        226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 215, 248, 249, 250, 251, 252, 253,
        254, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
        236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
        250, 251, 252, 253, 254, 255, };

// Get local time in secs, starting from 01.01.1970
uae_u32 getlocaltime(void)
{
	return time(NULL); // ToDo: convert UTC to local time...
}
