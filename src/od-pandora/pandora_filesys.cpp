#include <sys/timeb.h>
#include <fcntl.h>
#include <unistd.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "zfile.h"


int my_setcurrentdir (const TCHAR *curdir, TCHAR *oldcur)
{
    int ret = 0;
    if (oldcur)
        getcwd(oldcur, MAX_DPATH);
    if (curdir)
        chdir (curdir);
    return ret;
}


int my_mkdir (const char*name)
{
    return mkdir(name, 0777);
}


int my_rmdir (const char*name)
{
    return rmdir(name);
}


int my_unlink (const char* name)
{
    return unlink(name);
}


int my_rename (const char* oldname, const char* newname)
{
    return rename(oldname, newname);
}


struct my_opendir_s
{
    void *h;
};


struct my_opendir_s *my_opendir (const char* name)
{
    struct my_opendir_s *mod;

    mod = xmalloc (struct my_opendir_s, 1);
    if (!mod)
        return NULL;
    mod->h = opendir(name);
    if (mod->h == NULL)
    {
        xfree (mod);
        return NULL;
    }
    return mod;
}


void my_closedir (struct my_opendir_s *mod)
{
    if (mod)
        closedir((DIR *) mod->h);
    xfree (mod);
}


int my_readdir (struct my_opendir_s *mod, char* name)
{
    struct dirent *de;

    if (!mod)
        return 0;

    de = readdir((DIR *) mod->h);
    if(de == 0)
        return 0;
    strncpy(name, de->d_name, MAX_PATH);
    return 1;
}


struct my_openfile_s
{
    void *h;
};


void my_close (struct my_openfile_s *mos)
{
    if(mos)
        close((int) mos->h);
    xfree (mos);
}


uae_s64 int my_lseek (struct my_openfile_s *mos, uae_s64 int offset, int pos)
{
    return lseek((int) mos->h, offset, pos);
}


uae_s64 int my_fsize (struct my_openfile_s *mos)
{
    uae_s64 pos = lseek((int) mos->h, 0, SEEK_CUR);
    uae_s64 size = lseek((int) mos->h, 0, SEEK_END);
    lseek((int) mos->h, pos, SEEK_SET);
    return size;
}


unsigned int my_read (struct my_openfile_s *mos, void *b, unsigned int size)
{
    return read((int) mos->h, b, size);
}


unsigned int my_write (struct my_openfile_s *mos, void *b, unsigned int size)
{
    return write((int) mos->h, b, size);
}


int my_existsfile (const char *name)
{
    struct stat st;
    if (lstat (name, &st) == -1)
    {
        return 0;
    }
    else
    {
        if (!S_ISDIR(st.st_mode))
            return 1;
    }
    return 0;
}


int my_existsdir(const char *name)
{
    struct stat st;

    if(lstat(name, &st) == -1)
    {
        return 0;
    }
    else
    {
        if (S_ISDIR(st.st_mode))
            return 1;
    }
    return 0;
}


struct my_openfile_s *my_open (const TCHAR *name, int flags)
{
    struct my_openfile_s *mos;

    mos = xmalloc (struct my_openfile_s, 1);
    if (!mos)
        return NULL;
    mos->h = (void *) open(name, flags);
    if (!mos->h)
    {
        xfree (mos);
        mos = NULL;
    }
    return mos;
}


int my_truncate (const TCHAR *name, uae_u64 len)
{
    return truncate(name, len);
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


FILE *my_opentext (const TCHAR *name)
{
    return fopen (name, "r");
}

/* Returns 1 if an actual volume-name was found, 2 if no volume-name (so uses some defaults) */
int target_get_volume_name(struct uaedev_mount_info *mtinf, const char *volumepath, char *volumename, int size, bool inserted, bool fullcheck)
{
    sprintf(volumename, "DH_%c", volumepath[0]);
    return 2;
}
