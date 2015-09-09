#include <sys/timeb.h>
#include <fcntl.h>
#include <unistd.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "zfile.h"


int my_existsfile (const char *name)
{
	struct stat st;
	if (lstat (name, &st) == -1) {
		return 0;
	} else {
		if (!S_ISDIR(st.st_mode))
			return 1;
	}
        return 0;
}


int my_existsdir(const char *name)
{
  struct stat st;

  if(lstat(name, &st) == -1) {
    return 0;
  } else {
    if (S_ISDIR(st.st_mode))
      return 1;
  }
  return 0;
}


int my_getvolumeinfo (const char *root)
{
  struct stat st;
  int ret = 0;

  if (lstat (root, &st) == -1)
    return -1;
  if (!S_ISDIR(st.st_mode))
    return -1;
  return ret;
}


void *my_opendir (const char* name)
{
  return opendir(name);
}


void my_closedir (void* dir)
{
  closedir((DIR *) dir);
}


int my_readdir (void* dir, char* name)
{
  struct dirent *de;
  
  if (!dir)
	  return 0;
	
	de = readdir((DIR *) dir);
	if(de == 0)
	  return 0;
	strncpy(name, de->d_name, MAX_PATH);
	return 1;
}


int my_rmdir (const char*name)
{
  return rmdir(name);
}


int my_mkdir (const char*name)
{
  return mkdir(name, 0777);
}


int my_unlink (const char* name)
{
  return unlink(name);
}


int my_rename (const char* oldname, const char* newname)
{
  return rename(oldname, newname);
}


void *my_open (const char* name, int flags)
{
  return (void *) open(name, flags);
}


void my_close (void* f)
{
  close((int) f);
}


unsigned int my_lseek (void* f, unsigned int offset, int pos)
{
  return lseek((int) f, offset, pos);
}


unsigned int my_read (void* f, void* b, unsigned int len)
{
  return read((int) f, b, len);
}


unsigned int my_write (void* f, void* b, unsigned int len)
{
  return write((int) f, b, len);
}


int my_truncate (const char *name, long int len)
{
  return truncate(name, len);
}


/* Returns 1 if an actual volume-name was found, 2 if no volume-name (so uses some defaults) */
int target_get_volume_name(struct uaedev_mount_info *mtinf, const char *volumepath, char *volumename, int size, int inserted, int fullcheck)
{
	sprintf(volumename, "DH_%c", volumepath[0]);
  return 2;
}
