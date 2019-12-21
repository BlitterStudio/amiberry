#ifndef AMIBERRY_ANDROID_AMIBERRY_FILESYS_HPP
#define AMIBERRY_ANDROID_AMIBERRY_FILESYS_HPP

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>

#include "sysdeps.h"
#include "options.h"

struct my_opendir_s
{
    void* h;
};


struct my_openfile_s
{
    int h;
};

struct my_opendir_s* my_opendir(const char* name);
int my_setcurrentdir(const TCHAR* curdir, TCHAR* oldcur);
int my_mkdir(const char* name);
int my_rmdir(const char* name);
int my_unlink(const char* name);
int my_rename(const char* oldname, const char* newname);
void my_closedir(struct my_opendir_s* mod);
int my_readdir(struct my_opendir_s* mod, char* name);
void my_close(struct my_openfile_s* mos);
uae_s64 my_lseek(struct my_openfile_s* mos,const uae_s64 offset, const int pos);
uae_s64 my_fsize(struct my_openfile_s* mos);
unsigned int my_read(struct my_openfile_s* mos, void* b, unsigned int size);
unsigned int my_write(struct my_openfile_s* mos, void* b, unsigned int size);
int my_existsfile(const char* name);
int my_existsdir(const char* name);
struct my_openfile_s* my_open(const TCHAR* name, const int flags);
int my_truncate(const TCHAR* name, uae_u64 len);
int my_getvolumeinfo(const char* root);
FILE* my_opentext(const TCHAR* name);
bool my_issamepath(const TCHAR* path1, const TCHAR* path2);
const TCHAR* my_getfilepart(const TCHAR* filename);

/* Returns 1 if an actual volume-name was found, 2 if no volume-name (so uses some defaults) */
int target_get_volume_name(struct uaedev_mount_info* mtinf, struct uaedev_config_info* ci, bool inserted,
                           bool fullcheck, int cnt);

#endif //AMIBERRY_ANDROID_AMIBERRY_FILESYS_HPP
