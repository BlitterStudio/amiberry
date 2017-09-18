 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder
  */

#ifndef UAE_ZFILE_H
#define UAE_ZFILE_H

#include "uae/types.h"

struct zfile;
struct zvolume;
struct zdirectory;

#define FS_DIRECTORY 0
#define FS_ARCHIVE 1
#define FS_CDFS 2

struct fs_dirhandle
{
	int fstype;
	union {
		struct zdirectory *zd;
		struct my_opendir_s *od;
	};
};
struct fs_filehandle
{
	int fstype;
	union {
		struct zfile *zf;
		struct my_openfile_s *of;
	};
};

typedef int (*zfile_callback)(struct zfile*, void*);

extern struct zfile *zfile_fopen (const TCHAR *, const TCHAR *, int mask);
extern struct zfile *zfile_fopen (const TCHAR *, const TCHAR *);
extern struct zfile *zfile_fopen (const TCHAR *, const TCHAR *, int mask, int index);
extern struct zfile *zfile_fopen_empty (struct zfile*, const TCHAR *name, uae_u64 size);
extern struct zfile *zfile_fopen_empty (struct zfile*, const TCHAR *name);
extern struct zfile *zfile_fopen_data (const TCHAR *name, uae_u64 size, const uae_u8 *data);
extern struct zfile *zfile_fopen_load_zfile (struct zfile *f);
extern uae_u8 *zfile_load_data (const TCHAR *name, const uae_u8 *data,int datalen, int *outlen);
extern uae_u8 *zfile_load_file(const TCHAR *name, int *outlen);
extern struct zfile *zfile_fopen_parent (struct zfile*, const TCHAR*, uae_u64 offset, uae_u64 size);

extern int zfile_exists (const TCHAR *name);
extern void zfile_fclose (struct zfile *);
extern uae_s64 zfile_fseek (struct zfile *z, uae_s64 offset, int mode);
extern uae_s64 zfile_ftell (struct zfile *z);
extern uae_s64 zfile_size (struct zfile *z);
extern size_t zfile_fread (void *b, size_t l1, size_t l2, struct zfile *z);
extern size_t zfile_fwrite  (const void *b, size_t l1, size_t l2, struct zfile *z);
extern TCHAR *zfile_fgets (TCHAR *s, int size, struct zfile *z);
extern char *zfile_fgetsa (char *s, int size, struct zfile *z);
extern size_t zfile_fputs (struct zfile *z, const TCHAR *s);
extern int zfile_getc (struct zfile *z);
extern int zfile_putc (int c, struct zfile *z);
extern int zfile_ferror (struct zfile *z);
extern uae_u8 *zfile_getdata (struct zfile *z, uae_s64 offset, int len);
extern void zfile_exit (void);
extern int execute_command (TCHAR *);
extern int zfile_iscompressed (struct zfile *z);
extern int zfile_zcompress (struct zfile *dst, void *src, int size);
extern int zfile_zuncompress (void *dst, int dstsize, struct zfile *src, int srcsize);
extern int zfile_gettype (struct zfile *z);
extern int zfile_zopen (const TCHAR *name, zfile_callback zc, void *user);
extern TCHAR *zfile_getname (struct zfile *f);
extern TCHAR *zfile_getfilename (struct zfile *f);
extern uae_u32 zfile_crc32 (struct zfile *f);
extern struct zfile *zfile_dup (struct zfile *f);
extern struct zfile *zfile_gunzip (struct zfile *z);
extern int zfile_is_diskimage (const TCHAR *name);
extern int iszip (struct zfile *z);
extern int zfile_convertimage (const TCHAR *src, const TCHAR *dst);
extern struct zfile *zuncompress (struct znode*, struct zfile *z, int dodefault, int mask, int *retcode, int index);
extern void zfile_seterror (const TCHAR *format, ...);
extern TCHAR *zfile_geterror (void);
extern int zfile_truncate (struct zfile *z, uae_s64 size);

#define ZFD_NONE 0
#define ZFD_ARCHIVE 1 //zip/lha..
#define ZFD_ADF 2 //adf as a filesystem
#define ZFD_HD 4 //rdb/hdf
#define ZFD_UNPACK 8 //gzip,dms
#define ZFD_RAWDISK 16  //fdi->adf,ipf->adf etc..
#define ZFD_CD 32 //cue/iso, cue has priority over iso
#define ZFD_DISKHISTORY 0x100 //allow diskhistory (if disk image)
#define ZFD_CHECKONLY 0x200 //file exists checkc
#define ZFD_DELAYEDOPEN 0x400 //do not unpack, just get metadata
#define ZFD_NORECURSE 0x10000 // do not recurse archives
#define ZFD_NORMAL (ZFD_ARCHIVE|ZFD_UNPACK)
#define ZFD_ALL 0x0000ffff

#define ZFD_RAWDISK_AMIGA 0x10000
#define ZFD_RAWDISK_PC 0x200000

#define ZFILE_UNKNOWN 0
#define ZFILE_CONFIGURATION 1
#define ZFILE_DISKIMAGE 2
#define ZFILE_ROM 3
#define ZFILE_KEY 4
#define ZFILE_HDF 5
#define ZFILE_STATEFILE 6
#define ZFILE_NVR 7
#define ZFILE_HDFRDB 8
#define ZFILE_CDIMAGE 9

extern const TCHAR *uae_archive_extensions[];
extern const TCHAR *uae_ignoreextensions[];
extern const TCHAR *uae_diskimageextensions[];

extern struct zvolume *zfile_fopen_archive(const TCHAR *filename);
extern struct zvolume *zfile_fopen_archive (const TCHAR *filename, int flags);
extern struct zvolume *zfile_fopen_archive_root (const TCHAR *filename, int flags);
extern void zfile_fclose_archive(struct zvolume *zv);
extern int zfile_fs_usage_archive(const TCHAR *path, const TCHAR *disk, struct fs_usage *fsp);
extern int zfile_stat_archive (const TCHAR *path, struct mystat *statbuf);
extern struct zdirectory *zfile_opendir_archive (const TCHAR *path);
extern struct zdirectory *zfile_opendir_archive (const TCHAR *path, int flags);
extern void zfile_closedir_archive(struct zdirectory*);
extern int zfile_readdir_archive(struct zdirectory *, TCHAR*);
extern int zfile_readdir_archive (struct zdirectory *, TCHAR*, bool fullpath);
extern struct zfile *zfile_readdir_archive_open (struct zdirectory *zd, const TCHAR *mode);
extern void zfile_resetdir_archive (struct zdirectory *);
extern int zfile_fill_file_attrs_archive(const TCHAR *path, int *isdir, int *flags, TCHAR **comment);
extern uae_s64 zfile_lseek_archive (struct zfile *d, uae_s64 offset, int whence);
extern uae_s64 zfile_fsize_archive (struct zfile *d);
extern unsigned int zfile_read_archive (struct zfile *d, void *b, unsigned int size);
extern void zfile_close_archive (struct zfile *d);
extern struct zfile *zfile_open_archive (const TCHAR *path, int flags);
extern int zfile_exists_archive(const TCHAR *path, const TCHAR *rel);
extern bool zfile_needwrite (struct zfile*);

struct mytimeval
{
	uae_s64 tv_sec;
	uae_s32 tv_usec;
};

struct mystat
{
	uae_s64 size;
	uae_u32 mode;
	struct mytimeval mtime;
};
extern void timeval_to_amiga (struct mytimeval *tv, int* days, int* mins, int* ticks, int tickcount);
extern void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks, int tickcount);

#endif /* UAE_ZFILE_H */
