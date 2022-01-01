
#include "sysconfig.h"
#include "sysdeps.h"

#include "zfile.h"
#include "fsdb.h"
#include "threaddep/thread.h"

#include "corefile.h"
#include "osdfile.h"

int core_fseek(util::core_file *file, signed long long offset, int whence)
{
	return zfile_fseek(file, offset, whence);
}
std::error_condition core_fopen(std::string_view filename, std::uint32_t openflags, util::core_file::ptr& file)
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
void core_fclose(util::core_file *file)
{
	zfile_fclose(file);
}
unsigned int core_fread(util::core_file *file, void *buffer, unsigned int length)
{
	return zfile_fread(buffer, 1, length, file);
}
unsigned int core_fwrite(util::core_file *file, const void *buffer, unsigned int length)
{
	return zfile_fwrite(buffer, 1, length, file);
}
signed long long core_ftell(util::core_file *file)
{
	return zfile_ftell(file);
}


std::error_condition osd_rmfile(const TCHAR *filename)
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