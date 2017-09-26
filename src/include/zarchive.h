#ifndef UAE_ZARCHIVE_H
#define UAE_ZARCHIVE_H

#include "uae/types.h"

typedef uae_s64 (*ZFILEREAD)(void*, uae_u64, uae_u64, struct zfile*);
typedef uae_s64 (*ZFILEWRITE)(const void*, uae_u64, uae_u64, struct zfile*);
typedef uae_s64 (*ZFILESEEK)(struct zfile*, uae_s64, int);

struct zfile {
  TCHAR *name;
  TCHAR *zipname;
  TCHAR *mode;
  FILE *f; // real file handle if physical file
  uae_u8 *data; // unpacked data
  int dataseek; // use seek position even if real file
	struct zfile *archiveparent; // set if parent is archive and this has not yet been unpacked (datasize < size)
	int archiveid;
  uae_s64 size; // real size
	uae_s64 datasize; // available size (not yet unpacked completely?)
	uae_s64 allocsize; // memory allocated before realloc() needed again
  uae_s64 seek; // seek position
  int deleteafterclose;
  int textmode;
  struct zfile *next;
  int zfdmask;
  struct zfile *parent;
  uae_u64 offset; // byte offset from parent file
  int opencnt;
  ZFILEREAD zfileread;
  ZFILEWRITE zfilewrite;
  ZFILESEEK zfileseek;
  void *userdata;
  int useparent;
};

#define ZNODE_FILE 0
#define ZNODE_DIR 1
#define ZNODE_VDIR -1
struct znode {
  int type;
  struct znode *sibling;
  struct znode *child;
  struct zvolume *vchild;
  struct znode *parent;
  struct zvolume *volume;
  struct znode *next;
  struct znode *prev;
  struct znode *vfile; // points to real file when this node is virtual directory
  TCHAR *name;
  TCHAR *fullname;
  uae_s64 size;
  struct zfile *f;
  TCHAR *comment;
  int flags;
  struct mytimeval mtime;
  /* decompressor specific */
  unsigned int offset;
  unsigned int offset2;
  unsigned int method;
  unsigned int packedsize;
};

struct zvolume
{
    struct zfile *archive;
    void *handle;
    struct znode root;
    struct zvolume *next;
    struct znode *last;
    struct znode *parentz;
    struct zvolume *parent;
    uae_s64 size;
    unsigned int id;
    uae_s64 archivesize;
    unsigned int method;
    TCHAR *volumename;
    int zfdmask;
};

struct zarchive_info
{
    TCHAR *name;
    uae_s64 size;
    int flags;
    TCHAR *comment;
	struct mytimeval tv;
};

#define MCC(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

#define ArchiveFormat7Zip  MCC('7', 'z', ' ', ' ')
#define ArchiveFormatRAR   MCC('r', 'a', 'r', ' ')
#define ArchiveFormatZIP   MCC('z', 'i', 'p', ' ')
#define ArchiveFormatLHA   MCC('l', 'h', 'a', ' ')
#define ArchiveFormatLZX   MCC('l', 'z', 'x', ' ')
#define ArchiveFormatPLAIN MCC('-', '-', '-', '-')
#define ArchiveFormatDIR   MCC('D', 'I', 'R', ' ')
#define ArchiveFormatAA    MCC('a', 'a', ' ', ' ') // method only
#define ArchiveFormatADF   MCC('D', 'O', 'S', ' ')
#define ArchiveFormatRDB   MCC('R', 'D', 'S', 'K')
#define ArchiveFormatMBR   MCC('M', 'B', 'R', ' ')
#define ArchiveFormatFAT   MCC('F', 'A', 'T', ' ')
#define ArchiveFormatTAR   MCC('t', 'a', 'r', ' ')

#define PEEK_BYTES 1024
#define FILE_PEEK 1
#define FILE_DELAYEDOPEN 2

extern int zfile_is_ignore_ext (const TCHAR *name);

extern struct zvolume *zvolume_alloc (struct zfile *z, unsigned int id, void *handle, const TCHAR*);
extern struct zvolume *zvolume_alloc_empty (struct zvolume *zv, const TCHAR *name);

extern struct znode *zvolume_addfile_abs (struct zvolume *zv, struct zarchive_info*);
extern struct znode *zvolume_adddir_abs (struct zvolume *zv, struct zarchive_info *zai);
extern struct znode *znode_adddir (struct znode *parent, const TCHAR *name, struct zarchive_info*);

extern struct zvolume *archive_directory_plain (struct zfile *zf);
extern struct zvolume *archive_directory_lha(struct zfile *zf);
extern struct zfile *archive_access_lha (struct znode *zn);
extern struct zvolume *archive_directory_zip(struct zfile *zf);
extern struct zvolume *archive_directory_7z (struct zfile *z);
extern struct zvolume *archive_directory_rar (struct zfile *z);
extern struct zfile *archive_access_rar (struct znode *zn);
extern struct zvolume *archive_directory_lzx (struct zfile *in_file);
extern struct zfile *archive_access_lzx (struct znode *zn);
extern struct zvolume *archive_directory_arcacc (struct zfile *z, unsigned int id);
extern struct zfile *archive_access_arcacc (struct znode *zn);
extern struct zvolume *archive_directory_adf (struct znode *zn, struct zfile *z);
extern struct zvolume *archive_directory_rdb (struct zfile *z);
extern struct zvolume *archive_directory_fat (struct zfile *z);
extern struct zvolume *archive_directory_tar (struct zfile *zf);
extern struct zfile *archive_access_tar (struct znode *zn);

extern struct zfile *archive_access_select (struct znode *parent, struct zfile *zf, unsigned int id, int doselect, int *retcode, int index);
extern struct zfile *archive_access_arcacc_select (struct zfile *zf, unsigned int id, int *retcode);
extern int isfat (uae_u8*);

extern void archive_access_scan (struct zfile *zf, zfile_callback zc, void *user, unsigned int id);

extern void archive_access_close (void *handle, unsigned int id);

extern struct zfile *archive_getzfile (struct znode *zn, unsigned int id, int flags);
extern struct zfile *archive_unpackzfile (struct zfile *zf);

extern struct zfile *decompress_zfd (struct zfile*);

#endif /* UAE_ZARCHIVE_H */
