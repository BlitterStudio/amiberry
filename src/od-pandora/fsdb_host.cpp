#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "fsdb.h"


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
