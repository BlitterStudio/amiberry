 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Library of functions to make emulated filesystem as independent as
  * possible of the host filesystem's capabilities.
  *
  * Copyright 1999 Bernd Schmidt
  */

#ifndef FSDB_FILE
#define FSDB_FILE "_UAEFSDB.___"
#endif

#ifndef FSDB_DIR_SEPARATOR
#define FSDB_DIR_SEPARATOR '/'
#endif

/* AmigaOS errors */
#define ERROR_BAD_NUMBER		  6
#define ERROR_NO_FREE_STORE		103
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
#define ERROR_DISK_IS_FULL		221
#define ERROR_DELETE_PROTECTED		222
#define ERROR_WRITE_PROTECTED		223
#define ERROR_READ_PROTECTED		224
#define ERROR_NOT_A_DOS_DISK		225
#define ERROR_NO_DISK			226
#define ERROR_NO_MORE_ENTRIES		232
#define ERROR_NOT_IMPLEMENTED		236

#define A_FIBF_HIDDEN  (1<<7)
#define A_FIBF_SCRIPT  (1<<6)
#define A_FIBF_PURE    (1<<5)
#define A_FIBF_ARCHIVE (1<<4)
#define A_FIBF_READ    (1<<3)
#define A_FIBF_WRITE   (1<<2)
#define A_FIBF_EXECUTE (1<<1)
#define A_FIBF_DELETE  (1<<0)

/* AmigaOS "keys" */
typedef struct a_inode_struct {
    /* Circular list of recycleable a_inodes.  */
    struct a_inode_struct *next, *prev;
    /* This a_inode's relatives in the directory structure.  */
    struct a_inode_struct *parent;
    struct a_inode_struct *child, *sibling;
    /* AmigaOS name, and host OS name.  The host OS name is a full path, the
     * AmigaOS name is relative to the parent.  */
    char *aname;
    char *nname;
    /* AmigaOS file comment, or NULL if file has none.  */
    char *comment;
    /* AmigaOS protection bits.  */
    int amigaos_mode;
    /* Unique number for identification.  */
    uae_u32 uniq;
    /* For a directory that is being ExNext()ed, the number of child ainos
       which must be kept locked in core.  */
    unsigned long locked_children;
    /* How many ExNext()s are going on in this directory?  */
    unsigned long exnext_count;
    /* AmigaOS locking bits.  */
    int shlock;
    long db_offset;
    unsigned int dir:1;
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
} a_inode;

extern char *nname_begin (char *);

extern char *build_nname (const char *d, const char *n);
extern char *build_aname (const char *d, const char *n);

/* Filesystem-independent functions.  */
extern void fsdb_clean_dir (a_inode *);
extern char *fsdb_search_dir (const char *dirname, char *rel);
extern void fsdb_dir_writeback (a_inode *);
extern int fsdb_used_as_nname (a_inode *base, const char *);
extern a_inode *fsdb_lookup_aino_aname (a_inode *base, const char *);
extern a_inode *fsdb_lookup_aino_nname (a_inode *base, const char *);
extern int fsdb_exists (const char *nname);

STATIC_INLINE int same_aname (const char *an1, const char *an2)
{
    return strcasecmp (an1, an2) == 0;
}

/* Filesystem-dependent functions.  */
extern int fsdb_name_invalid (const char *n);
extern int fsdb_fill_file_attrs (a_inode *, a_inode *);
extern int fsdb_set_file_attrs (a_inode *);
extern int fsdb_mode_representable_p (const a_inode *, int);
extern int fsdb_mode_supported (const a_inode *);
extern char *fsdb_create_unique_nname (a_inode *base, const char *);

extern void *my_opendir (const char*);
extern void my_closedir (void*);
extern int my_readdir (void*, char*);

extern int my_rmdir (const char*);
extern int my_mkdir (const char*);
extern int my_unlink (const char*);
extern int my_rename (const char*, const char*);

extern void *my_open (const char*, int);
extern void my_close (void*);
extern unsigned int my_lseek (void*, unsigned int, int);
extern unsigned int my_read (void*, void*, unsigned int);
extern unsigned int my_write (void*, void*, unsigned int);
extern int my_truncate (const char *name, long int len);
extern int dos_errno (void);
extern int my_existsfile (const char *name);
extern int my_existsdir (const char *name);

#define MYVOLUMEINFO_READONLY 1
#define MYVOLUMEINFO_STREAMS 2
#define MYVOLUMEINFO_ARCHIVE 4
#define MYVOLUMEINFO_REUSABLE 8

extern int my_getvolumeinfo (const char *root);
