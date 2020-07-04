#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "sysdeps.h"
#include "fsdb.h"
#include "zfile.h"
#include <sys/types.h>
#include <sys/stat.h>

int dos_errno(void)
{
	int e = errno;

	switch (e) {
	case ENOMEM:	return ERROR_NO_FREE_STORE;
	case EEXIST:	return ERROR_OBJECT_EXISTS;
	case EACCES:	return ERROR_WRITE_PROTECTED;
	case ENOENT:	return ERROR_OBJECT_NOT_AROUND;
	case ENOTDIR:	return ERROR_OBJECT_WRONG_TYPE;
	case ENOSPC:	return ERROR_DISK_IS_FULL;
	case EBUSY:       	return ERROR_OBJECT_IN_USE;
	case EISDIR:	return ERROR_OBJECT_WRONG_TYPE;
#if defined(ETXTBSY)
	case ETXTBSY:	return ERROR_OBJECT_IN_USE;
#endif
#if defined(EROFS)
	case EROFS:       	return ERROR_DISK_WRITE_PROTECTED;
#endif
#if defined(ENOTEMPTY)
#if ENOTEMPTY != EEXIST
	case ENOTEMPTY:	return ERROR_DIRECTORY_NOT_EMPTY;
#endif
#endif

	default:
		write_log(("Unimplemented error %s\n", strerror(e)));
		return ERROR_NOT_IMPLEMENTED;
	}
}

/* return supported combination */
int fsdb_mode_supported(const a_inode* aino)
{
	int mask = aino->amigaos_mode;
	return mask;
}

bool my_createshortcut(const char* source, const char* target, const char* description)
{
	return false;
}

bool my_resolvesoftlink(TCHAR* linkfile, int size, bool linkonly)
{
	return false;
}

void my_canonicalize_path(const TCHAR* path, TCHAR* out, int size)
{
	_tcsncpy(out, path, size);
	out[size - 1] = 0;
	return;
}

int my_issamevolume(const TCHAR* path1, const TCHAR* path2, TCHAR* path)
{
	TCHAR p1[MAX_DPATH];
	TCHAR p2[MAX_DPATH];
	unsigned int len, cnt;

	my_canonicalize_path(path1, p1, sizeof p1 / sizeof(TCHAR));
	my_canonicalize_path(path2, p2, sizeof p2 / sizeof(TCHAR));
	len = _tcslen(p1);
	if (len > _tcslen(p2))
		len = _tcslen(p2);
	if (_tcsnicmp(p1, p2, len))
		return 0;
	_tcscpy(path, p2 + len);
	cnt = 0;
	for (unsigned int i = 0; i < _tcslen(path); i++) {
		if (path[i] == '\\' || path[i] == '/') {
			path[i] = '/';
			cnt++;
		}
	}
	write_log(_T("'%s' (%s) matched with '%s' (%s), extra = '%s'\n"), path1, p1, path2, p2, path);
	return cnt;
}

bool my_stat(const TCHAR *name, struct mystat *statbuf)
{
	struct stat64 st;

	if (stat64(name, &st) == -1) {
		write_log("my_stat: stat on file %s failed\n", name);
		return false;
	}

	statbuf->size = st.st_size;
	statbuf->mode = 0;
	if (st.st_mode & S_IRUSR) {
		statbuf->mode |= FILEFLAG_READ;
	}
	if (st.st_mode & S_IWUSR) {
		statbuf->mode |= FILEFLAG_WRITE;
	}
	statbuf->mtime.tv_sec = st.st_mtime;
	statbuf->mtime.tv_usec = 0;

	return true;
}


bool my_chmod(const TCHAR *name, uae_u32 mode)
{
	// Note: only used to set or clear write protect on disk file

	// get current state
	struct stat64 st;
	if (stat64(name, &st) == -1) {
		write_log("my_chmod: stat on file %s failed\n", name);
		return false;
	}

	if (mode & FILEFLAG_WRITE)
		// set write permission
		st.st_mode |= S_IWUSR;
	else
		// clear write permission
		st.st_mode &= ~(S_IWUSR);
	chmod(name, st.st_mode);

	stat64(name, &st);
	int newmode = 0;
	if (st.st_mode & S_IRUSR) {
		newmode |= FILEFLAG_READ;
	}
	if (st.st_mode & S_IWUSR) {
		newmode |= FILEFLAG_WRITE;
	}

	return (mode == newmode);
}


bool my_utime(const TCHAR *name, struct mytimeval *tv)
{
	struct mytimeval mtv;
	struct timeval times[2];

	if (tv == NULL) {
		struct timeval time;
		struct timezone tz;

		gettimeofday(&time, &tz);
		mtv.tv_sec = time.tv_sec + tz.tz_minuteswest;
		mtv.tv_usec = time.tv_usec;
  } else {
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

// Get local time in secs, starting from 01.01.1970
uae_u32 getlocaltime(void)
{
	return time(NULL); // ToDo: convert UTC to local time...
}
