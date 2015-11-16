 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder
  */
struct zfile;
struct zvolume;

typedef int (*zfile_callback)(struct zfile*, void*);

extern struct zfile *zfile_fopen (const char *, const char *);
extern struct zfile *zfile_fopen_nozip (const char *, const char *);
extern struct zfile *zfile_fopen_empty (const char *name, int size);
extern struct zfile *zfile_fopen_data (const char *name, int size, uae_u8 *data);
extern int zfile_exists (const char *name);
extern void zfile_fclose (struct zfile *);
extern int zfile_fseek (struct zfile *z, long offset, int mode);
extern long zfile_ftell (struct zfile *z);
extern size_t zfile_fread (void *b, size_t l1, size_t l2, struct zfile *z);
extern size_t zfile_fwrite (void *b, size_t l1, size_t l2, struct zfile *z);
extern char *zfile_fgets(char *s, int size, struct zfile *z);
extern size_t zfile_fputs (struct zfile *z, char *s);
extern int zfile_getc (struct zfile *z);
extern int zfile_putc (int c, struct zfile *z);
extern int zfile_ferror (struct zfile *z);
extern char *zfile_getdata (struct zfile *z, int offset, int len);
extern void zfile_exit (void);

extern int execute_command (char *);
extern int zfile_iscompressed (struct zfile *z);
extern int zfile_zcompress (struct zfile *dst, void *src, int size);
extern int zfile_zuncompress (void *dst, int dstsize, struct zfile *src, int srcsize);
extern int zfile_gettype (struct zfile *z);
extern int zfile_zopen (const char *name, zfile_callback zc, void *user);
extern char *zfile_getname (struct zfile *f);
extern uae_u32 zfile_crc32 (struct zfile *f);
extern struct zfile *zfile_dup (struct zfile *f);
extern struct zfile *zfile_gunzip (struct zfile *z);
extern int zfile_isdiskimage (const char *name);
extern int iszip (struct zfile *z);

#define ZFILE_UNKNOWN 0
#define ZFILE_CONFIGURATION 1
#define ZFILE_DISKIMAGE 2
#define ZFILE_ROM 3
#define ZFILE_KEY 4
#define ZFILE_HDF 5
#define ZFILE_STATEFILE 6
#define ZFILE_NVR 7
#define ZFILE_HDFRDB 8

extern const char *uae_archive_extensions[];
extern const char *uae_ignoreextensions[];
extern const char *uae_diskimageextensions[];

extern struct zvolume *zfile_fopen_archive(const char *filename);
extern void zfile_fclose_archive(struct zvolume *zv);
extern int zfile_fs_usage_archive(const char *path, const char *disk, struct fs_usage *fsp);
extern int zfile_stat_archive(const char *path, struct stat *statbuf);
extern void *zfile_opendir_archive(const char *path);
extern void zfile_closedir_archive(struct zdirectory*);
extern int zfile_readdir_archive(struct zdirectory *, char*);
extern int zfile_fill_file_attrs_archive(const char *path, int *isdir, int *flags, char **comment);
extern unsigned int zfile_lseek_archive (void *d, unsigned int offset, int whence);
extern unsigned int zfile_read_archive (void *d, void *b, unsigned int size);
extern void zfile_close_archive (void *d);
extern void *zfile_open_archive (const char *path, int flags);
extern int zfile_exists_archive(const char *path, const char *rel);
