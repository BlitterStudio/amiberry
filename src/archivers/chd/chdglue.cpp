
#include "sysconfig.h"
#include "sysdeps.h"

#include "zfile.h"
#include "fsdb.h"
#include "threaddep/thread.h"

#include "chdtypes.h"
#include "corefile.h"

int core_fseek(core_file *file, INT64 offset, int whence)
{
	return zfile_fseek(file, offset, whence);
}
file_error core_fopen(const TCHAR *filename, UINT32 openflags, core_file **file)
{
	const TCHAR *mode;

	if (openflags & OPEN_FLAG_CREATE)
		mode = _T("wb");
	else if (openflags & OPEN_FLAG_WRITE)
		mode = _T("rb+");
	else
		mode = _T("rb");
	struct zfile *z = zfile_fopen(filename, mode);
	*file = z;
	return z == NULL ? FILERR_NOT_FOUND : FILERR_NONE;
}
void core_fclose(core_file *file)
{
	zfile_fclose(file);
}
UINT32 core_fread(core_file *file, void *buffer, UINT32 length)
{
	return zfile_fread(buffer, 1, length, file);
}
UINT32 core_fwrite(core_file *file, const void *buffer, UINT32 length)
{
	return zfile_fwrite(buffer, 1, length, file);
}
UINT64 core_ftell(core_file *file)
{
	return zfile_ftell(file);
}


file_error osd_rmfile(const TCHAR *filename)
{
	my_rmdir(filename);
	return FILERR_NONE;
}

void osd_break_into_debugger(const char *message)
{
	write_log(message);
}

void *osd_malloc(size_t size)
{
	return malloc(size);
}
void *osd_malloc_array(size_t size)
{
	return malloc(size);
}
void osd_free(void *ptr)
{
	free(ptr);
}
osd_lock *osd_lock_alloc(void)
{
	uae_sem_t t = NULL;
	uae_sem_init(&t, 0, 1);
	return (osd_lock*)t;
}
void osd_lock_free(osd_lock *lock)
{
	uae_sem_destroy((uae_sem_t*)&lock);
}
void osd_lock_release(osd_lock *lock)
{
	uae_sem_post((uae_sem_t*)&lock);
}
void osd_lock_acquire(osd_lock *lock)
{
	uae_sem_wait((uae_sem_t*)&lock);
}