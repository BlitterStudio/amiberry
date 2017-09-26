 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Library of functions to make emulated filesystem as independent as
  * possible of the host filesystem's capabilities.
  *
  * Copyright 1999 Bernd Schmidt
  */

#ifndef UAE_FSDB_H
#define UAE_FSDB_H

#include "uae/types.h"

#ifndef FSDB_FILE
#define FSDB_FILE _T("_UAEFSDB.___")
#endif

#ifndef FSDB_DIR_SEPARATOR
#define FSDB_DIR_SEPARATOR '/'
#endif

/* AmigaOS errors */
#define ERROR_NO_FREE_STORE		103
#define ERROR_BAD_NUMBER			115
#define ERROR_LINE_TOO_LONG			120
#define ERROR_OBJECT_IN_USE		202
#define ERROR_OBJECT_EXISTS		203
#define ERROR_DIR_NOT_FOUND		204
#define ERROR_OBJECT_NOT_AROUND		205
#define ERROR_ACTION_NOT_KNOWN		209
#define ERROR_INVALID_LOCK		211
#define ERROR_OBJECT_WRONG_TYPE		212
#define ERROR_DISK_WRITE_PROTECTED	214
#define ERROR_DIRECTORY_NOT_EMPTY	216
#define ERROR_DEVICE_NOT_MOUNTED	218
#define ERROR_SEEK_ERROR		219
#define ERROR_COMMENT_TOO_BIG		220
#define ERROR_DISK_IS_FULL		221
#define ERROR_DELETE_PROTECTED		222
#define ERROR_WRITE_PROTECTED		223
#define ERROR_READ_PROTECTED		224
#define ERROR_NOT_A_DOS_DISK		225
#define ERROR_NO_DISK			226
#define ERROR_NO_MORE_ENTRIES		232
#define ERROR_IS_SOFT_LINK			233
#define ERROR_NOT_IMPLEMENTED		236
#define ERROR_RECORD_NOT_LOCKED		240
#define ERROR_LOCK_COLLISION		241
#define ERROR_LOCK_TIMEOUT			242
#define ERROR_UNLOCK_ERROR			243

#define A_FIBF_HIDDEN  (1<<7)
#define A_FIBF_SCRIPT  (1<<6)
#define A_FIBF_PURE    (1<<5)
#define A_FIBF_ARCHIVE (1<<4)
#define A_FIBF_READ    (1<<3)
#define A_FIBF_WRITE   (1<<2)
#define A_FIBF_EXECUTE (1<<1)
#define A_FIBF_DELETE  (1<<0)

struct virtualfilesysobject
{
	int dir;
	TCHAR *comment;
	uae_u32 amigaos_mode;
	uae_u8 *data;
	int size;
};

/* AmigaOS "keys" */
typedef struct a_inode_struct {
  /* Circular list of recycleable a_inodes.  */
  struct a_inode_struct *next, *prev;
  /* This a_inode's relatives in the directory structure.  */
  struct a_inode_struct *parent;
  struct a_inode_struct *child, *sibling;
  /* AmigaOS name, and host OS name.  The host OS name is a full path, the
   * AmigaOS name is relative to the parent.  */
  TCHAR *aname;
  TCHAR *nname;
  /* AmigaOS file comment, or NULL if file has none.  */
  TCHAR *comment;
  /* AmigaOS protection bits.  */
  int amigaos_mode;
  /* Unique number for identification.  */
  uae_u32 uniq;
  /* For a directory that is being ExNext()ed, the number of child ainos
     which must be kept locked in core.  */
    unsigned int locked_children;
  /* How many ExNext()s are going on in this directory?  */
    unsigned int exnext_count;
  /* AmigaOS locking bits.  */
  int shlock;
  long db_offset;
  unsigned int dir:1;
  unsigned int softlink:2;
  unsigned int elock:1;
  /* Nonzero if this came from an entry in our database.  */
  unsigned int has_dbentry:1;
  /* Nonzero if this will need an entry in our database.  */
  unsigned int needs_dbentry:1;
  /* This a_inode possibly needs writing back to the database.  */
  unsigned int dirty:1;
  /* If nonzero, this represents a deleted file; the corresponding
   * entry in the database must be cleared.  */
  unsigned int deleted:1;
  /* target volume flag */
  unsigned int volflags;
  /* not equaling unit.mountcount -> not in this volume */
  unsigned int mountcount;
	uae_u64 uniq_external;
	struct virtualfilesysobject *vfso;
} a_inode;

extern TCHAR *nname_begin (TCHAR *);

extern TCHAR *build_nname (const TCHAR *d, const TCHAR *n);
extern TCHAR *build_aname (const TCHAR *d, const TCHAR *n);

/* Filesystem-independent functions.  */
extern void fsdb_clean_dir (a_inode *);
extern TCHAR *fsdb_search_dir (const TCHAR *dirname, TCHAR *rel);
extern void fsdb_dir_writeback (a_inode *);
extern int fsdb_used_as_nname (a_inode *base, const TCHAR *);
extern a_inode *fsdb_lookup_aino_aname (a_inode *base, const TCHAR *);
extern a_inode *fsdb_lookup_aino_nname (a_inode *base, const TCHAR *);
extern int fsdb_exists (const TCHAR *nname);

STATIC_INLINE int same_aname (const TCHAR *an1, const TCHAR *an2)
{
  return strcasecmp (an1, an2) == 0;
}

/* Filesystem-dependent functions.  */
extern int fsdb_name_invalid (const TCHAR *n);
extern int fsdb_name_invalid_dir (const TCHAR *n);
extern int fsdb_fill_file_attrs (a_inode *, a_inode *);
extern int fsdb_set_file_attrs (a_inode *);
extern int fsdb_mode_representable_p (const a_inode *, int);
extern int fsdb_mode_supported (const a_inode *);
extern TCHAR *fsdb_create_unique_nname (a_inode *base, const TCHAR *);

struct my_opendir_s;
struct my_openfile_s;

extern struct my_opendir_s *my_opendir (const TCHAR*, const TCHAR*);
extern struct my_opendir_s *my_opendir (const TCHAR*);
extern void my_closedir (struct my_opendir_s *);
extern int my_readdir (struct my_opendir_s *, TCHAR*);

extern int my_rmdir (const TCHAR*);
extern int my_mkdir (const TCHAR*);
extern int my_unlink (const TCHAR*);
extern int my_rename (const TCHAR*, const TCHAR*);
extern int my_setcurrentdir (const TCHAR *curdir, TCHAR *oldcur);

extern struct my_openfile_s *my_open (const TCHAR*, int);
extern void my_close (struct my_openfile_s*);
extern uae_s64 my_lseek (struct my_openfile_s*, uae_s64, int);
extern uae_s64 my_fsize (struct my_openfile_s*);
extern unsigned int my_read (struct my_openfile_s*, void*, unsigned int);
extern unsigned int my_write (struct my_openfile_s*, void*, unsigned int);
extern int my_truncate (const TCHAR *name, uae_u64 len);
extern int dos_errno (void);
extern int my_existsfile (const TCHAR *name);
extern int my_existsdir (const TCHAR *name);
extern FILE *my_opentext (const TCHAR*);

extern bool my_stat (const TCHAR *name, struct mystat *ms);
extern bool my_utime (const TCHAR *name, struct mytimeval *tv);
extern bool my_chmod (const TCHAR *name, uae_u32 mode);
extern const TCHAR *my_getfilepart(const TCHAR *filename);
extern bool my_issamepath(const TCHAR *path1, const TCHAR *path2);

#define MYVOLUMEINFO_READONLY 1
#define MYVOLUMEINFO_STREAMS 2
#define MYVOLUMEINFO_ARCHIVE 4
#define MYVOLUMEINFO_REUSABLE 8
#define MYVOLUMEINFO_CDFS 16

extern int my_getvolumeinfo (const TCHAR *root);

#endif /* UAE_FSDB_H */
