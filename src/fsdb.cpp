 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Library of functions to make emulated filesystem as independent as
  * possible of the host filesystem's capabilities.
  *
  * Copyright 1999 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "fsusage.h"
#include "fsdb.h"

/* The on-disk format is as follows:
 * Offset 0, 1 byte, valid
 * Offset 1, 4 bytes, mode
 * Offset 5, 257 bytes, aname
 * Offset 263, 257 bytes, nname
 * Offset 519, 81 bytes, comment
 */

#define TRACING_ENABLED 0
#if TRACING_ENABLED
#define TRACE(x) do { write_log x; } while(0)
#else
#define TRACE(x)
#endif

char *nname_begin (char *nname)
{
    char *p = strrchr (nname, FSDB_DIR_SEPARATOR);
    if (p)
	return p + 1;
    return nname;
}

#ifndef _WIN32
/* Find the name REL in directory DIRNAME.  If we find a file that
 * has exactly the same name, return REL.  If we find a file that
 * has the same name when compared case-insensitively, return a
 * malloced string that contains the name we found.  If no file
 * exists that compares equal to REL, return 0.  */
char *fsdb_search_dir (const char *dirname, char *rel)
{
    char *p = 0;
    int de;
    void *dir;
    char fn[MAX_DPATH];

    dir = my_opendir (dirname);
    /* This really shouldn't happen...  */
    if (! dir)
	return 0;
    
    while (p == 0 && (de = my_readdir (dir, fn)) != 0) {
	if (strcmp (fn, rel) == 0)
	    p = rel;
	else if (strcasecmp (fn, rel) == 0)
	    p = my_strdup (fn);
    }
    my_closedir (dir);
    return p;
}
#endif

static FILE *get_fsdb (a_inode *dir, const char *mode)
{
    char *n;
    FILE *f;
    
    n = build_nname (dir->nname, FSDB_FILE);
    f = fopen (n, mode);
    free (n);
    return f;
}

static void kill_fsdb (a_inode *dir)
{
    char *n = build_nname (dir->nname, FSDB_FILE);
    unlink (n);
    free (n);
}

static void fsdb_fixup (FILE *f, char *buf, int size, a_inode *base)
{
    char *nname;
    int ret;

    if (buf[0] == 0)
	return;
    nname = build_nname (base->nname, buf + 5 + 257);
    ret = fsdb_exists (nname);
    if (ret) {
      free (nname);
	    return;
    }
    TRACE (("uaefsdb '%s' deleted\n", nname));
    /* someone deleted this file/dir outside of emulation.. */
    buf[0] = 0;
    free (nname);
}

/* Prune the db file the first time this directory is opened in a session.  */
void fsdb_clean_dir (a_inode *dir)
{
    char buf[1 + 4 + 257 + 257 + 81];
    char *n;
    FILE *f;
    off_t pos1 = 0, pos2;

    n = build_nname (dir->nname, FSDB_FILE);
    f = fopen (n, "r+b");
    if (f == 0) {
	    free (n);
	    return;
    }
    for (;;) {
      pos2 = ftell (f);
      if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	      break;
	    fsdb_fixup (f, buf, sizeof buf, dir);
	    if (buf[0] == 0)
	      continue;
	    if (pos1 != pos2) {
	      fseek (f, pos1, SEEK_SET);
	      fwrite (buf, 1, sizeof buf, f);
	      fseek (f, pos2 + sizeof buf, SEEK_SET);
	    }
    	pos1 += sizeof buf;
    }
    fclose (f);
    my_truncate (n, pos1);
    free (n);
}

static a_inode *aino_from_buf (a_inode *base, char *buf, long off)
{
    uae_u32 mode;
    a_inode *aino = (a_inode *) xcalloc (sizeof (a_inode), 1);

    mode = do_get_mem_long ((uae_u32 *)(buf + 1));
    buf += 5;
    aino->aname = my_strdup (buf);
    buf += 257;
    aino->nname = build_nname (base->nname, buf);
    buf += 257;
    aino->comment = *buf != '\0' ? my_strdup (buf) : 0;
    fsdb_fill_file_attrs (base, aino);
    aino->amigaos_mode = mode;
    aino->has_dbentry = 1;
    aino->dirty = 0;
    aino->db_offset = off;
    return aino;
}

a_inode *fsdb_lookup_aino_aname (a_inode *base, const char *aname)
{
    FILE *f;

    f = get_fsdb (base, "r+b");
    if (f == 0) {
	    return 0;
    }
    for (;;) {
	char buf[1 + 4 + 257 + 257 + 81];
	if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	    break;
	if (buf[0] != 0 && same_aname (buf + 5, aname)) {
	    long pos = ftell (f) - sizeof buf;
	    fclose (f);
	    return aino_from_buf (base, buf, pos);
	}
    }
    fclose (f);
    return 0;
}

a_inode *fsdb_lookup_aino_nname (a_inode *base, const char *nname)
{
    FILE *f;

    f = get_fsdb (base, "r+b");
    if (f == 0) {
    	return 0;
    }
    for (;;) {
	char buf[1 + 4 + 257 + 257 + 81];
	if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	    break;
	if (buf[0] != 0 && strcmp (buf + 5 + 257, nname) == 0) {
	    long pos = ftell (f) - sizeof buf;
	    fclose (f);
	    return aino_from_buf (base, buf, pos);
	}
    }
    fclose (f);
    return 0;
}

int fsdb_used_as_nname (a_inode *base, const char *nname)
{
    FILE *f;
    char buf[1 + 4 + 257 + 257 + 81];

    f = get_fsdb (base, "r+b");
    if (f == 0) {
	    return 0;
    }
    for (;;) {
	if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	    break;
	if (buf[0] == 0)
	    continue;
	if (strcmp (buf + 5 + 257, nname) == 0) {
	    fclose (f);
	    return 1;
	}
    }
    fclose (f);
    return 0;
}

static int needs_dbentry (a_inode *aino)
{
    const char *nn_begin;

    if (aino->deleted)
	return 0;
    
    if (! fsdb_mode_representable_p (aino, aino->amigaos_mode) || aino->comment != 0)
	return 1;

    nn_begin = nname_begin (aino->nname);
    return strcmp (nn_begin, aino->aname) != 0;
}

static void write_aino (FILE *f, a_inode *aino)
{
    char buf[1 + 4 + 257 + 257 + 81];
    buf[0] = aino->needs_dbentry;
    do_put_mem_long ((uae_u32 *)(buf + 1), aino->amigaos_mode);
    strncpy (buf + 5, aino->aname, 256);
    buf[5 + 256] = '\0';
    strncpy (buf + 5 + 257, nname_begin (aino->nname), 256);
    buf[5 + 257 + 256] = '\0';
    strncpy (buf + 5 + 2*257, aino->comment ? aino->comment : "", 80);
    buf[5 + 2 * 257 + 80] = '\0';
    aino->db_offset = ftell (f);
    fwrite (buf, 1, sizeof buf, f);
    aino->has_dbentry = aino->needs_dbentry;
    TRACE (("%d '%s' '%s' written\n", aino->db_offset, aino->aname, aino->nname));
}

/* Write back the db file for a directory.  */

void fsdb_dir_writeback (a_inode *dir)
{
    FILE *f;
    int changes_needed = 0;
    int entries_needed = 0;
    a_inode *aino;
    uae_u8 *tmpbuf;
    int size, i;

    TRACE (("fsdb writeback %s\n", dir->aname));
    /* First pass: clear dirty bits where unnecessary, and see if any work
     * needs to be done.  */
    for (aino = dir->child; aino; aino = aino->sibling) {
	int old_needs_dbentry = aino->has_dbentry;
	int need = needs_dbentry (aino);
	aino->needs_dbentry = need;
	entries_needed |= need;
	if (! aino->dirty)
	    continue;
	if (! aino->needs_dbentry && ! old_needs_dbentry)
	    aino->dirty = 0;
	else
	    changes_needed = 1;
    }
    if (! entries_needed) {
	kill_fsdb (dir);
	TRACE (("fsdb removed\n"));
	return;
    }

    if (! changes_needed) {
	    TRACE (("not modified\n"));
	    return;
    }

    f = get_fsdb (dir, "r+b");
    if (f == 0) {
	f = get_fsdb (dir, "w+b");
	if (f == 0) {
	    TRACE (("failed\n"));
	    /* This shouldn't happen... */
	    return;
  }
    }
    fseek (f, 0, SEEK_END);
    size = ftell (f);
    fseek (f, 0, SEEK_SET);
    tmpbuf = 0;
    if (size > 0) {
	tmpbuf = (uae_u8 *)malloc (size);
	fread (tmpbuf, 1, size, f);
    }
    TRACE (("**** updating '%s' %d\n", dir->aname, size));

    for (aino = dir->child; aino; aino = aino->sibling) {

	if (! aino->dirty)
	    continue;
	aino->dirty = 0;

	i = 0;
	while (!aino->has_dbentry && i < size) {
	    if (!strcmp ((const char *)(tmpbuf + i + 5), aino->aname)) {
		aino->has_dbentry = 1;
		aino->db_offset = i;
	    }
	    i += 1 + 4 + 257 + 257 + 81;
	}
	if (! aino->has_dbentry) {
	    fseek (f, 0, SEEK_END);
	    aino->has_dbentry = 1;
	} else {
	    fseek (f, aino->db_offset, SEEK_SET);
  }
	write_aino (f, aino);
    }
    TRACE (("end\n"));
    fclose (f);
    free (tmpbuf);
}
