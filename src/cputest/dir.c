#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include "msc_dirent.h"
#else
#include <dirent.h>
#endif

#ifdef AMIGA

// Uses dos.library directly because -mcrt=nix13 directory routines
// are not reliable at least when using FAT95

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

struct DosLibrary *DOSBase;

struct DirInfo
{
	BPTR lock;
	struct FileInfoBlock fib;
};

#else

static char tmpbuffer[256];
static int isdir(const char *dirpath, const char *name)
{
	struct stat buf;

	snprintf(tmpbuffer, sizeof(tmpbuffer), "%s%s", dirpath, name);
	return stat(tmpbuffer, &buf) == 0 && S_ISDIR(buf.st_mode);
}

#endif

void *x_opendir(char *path)
{
#ifdef AMIGA
	struct DirInfo *di = AllocMem(sizeof(struct DirInfo), MEMF_CLEAR | MEMF_PUBLIC);
	if (!di) {
		return NULL;
	}
	di->lock = Lock(path, SHARED_LOCK);
	if (!di->lock) {
		FreeMem(di, sizeof(struct DirInfo));
		return NULL;
	}
	if (!Examine(di->lock, &di->fib)) {
		FreeMem(di, sizeof(struct DirInfo));
		return NULL;
	}
	return (void*)di;
#else
	return opendir(path);
#endif
}

void x_closedir(void *v)
{
#ifdef AMIGA
	struct DirInfo *di = (struct DirInfo*)v;
	if (di) {
		UnLock(di->lock);
		FreeMem(di, sizeof(struct DirInfo));
	}
#else
	closedir((DIR*)v);
#endif
}

int x_readdir(char *path, void *v, char *s)
{
#ifdef AMIGA
	struct DirInfo *di = (struct DirInfo*)v;
	if (!ExNext(di->lock, &di->fib)) {
		return 0;
	}
	strcpy(s, di->fib.fib_FileName);
	return di->fib.fib_DirEntryType < 0 ? 1 : - 1;
#else
	struct dirent *d = readdir((DIR*)v);
	if (!d)
		return 0;
	strcpy(s, d->d_name);
	return isdir(path, s) ? -1 : 1;
#endif
}
