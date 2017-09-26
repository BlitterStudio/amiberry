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
#include "traps.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "fsusage.h"
#include "fsdb.h"
#include "uae/io.h"

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

TCHAR *nname_begin (TCHAR *nname)
{
  TCHAR *p = _tcsrchr (nname, FSDB_DIR_SEPARATOR);
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
TCHAR *fsdb_search_dir (const TCHAR *dirname, TCHAR *rel)
{
  TCHAR *p = 0;
	int de;
	my_opendir_s *dir;
  TCHAR fn[MAX_DPATH];

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

static FILE *get_fsdb (a_inode *dir, const TCHAR *mode)
{
  TCHAR *n;
  FILE *f;
    
	if (!dir->nname)
		return NULL;
  n = build_nname (dir->nname, FSDB_FILE);
	f = uae_tfopen (n, mode);
  xfree (n);
  return f;
}

static void kill_fsdb (a_inode *dir)
{
	if (!dir->nname)
		return;
  TCHAR *n = build_nname (dir->nname, FSDB_FILE);
  _wunlink (n);
  xfree (n);
}

static void fsdb_fixup (FILE *f, uae_u8 *buf, int size, a_inode *base)
{
  TCHAR *nname;
  int ret;

  if (buf[0] == 0)
  	return;
	TCHAR *fnname = au ((char*)buf + 5 + 257);
	nname = build_nname (base->nname, fnname);
	xfree (fnname);
  ret = fsdb_exists (nname);
  if (ret) {
  	xfree (nname);
    return;
  }
	TRACE ((_T("uaefsdb '%s' deleted\n"), nname));
  /* someone deleted this file/dir outside of emulation.. */
  buf[0] = 0;
  xfree (nname);
}

/* Prune the db file the first time this directory is opened in a session.  */
void fsdb_clean_dir (a_inode *dir)
{
	uae_u8 buf[1 + 4 + 257 + 257 + 81];
  TCHAR *n;
  FILE *f;
  off_t pos1 = 0, pos2;

	if (!dir->nname)
		return;
  n = build_nname (dir->nname, FSDB_FILE);
	f = uae_tfopen (n, _T("r+b"));
  if (f == 0) {
  	xfree (n);
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
	if (pos1 == 0) {
		kill_fsdb (dir);
	} else {
    my_truncate (n, pos1);
	}
  xfree (n);
}

static a_inode *aino_from_buf (a_inode *base, uae_u8 *buf, long off)
{
  uae_u32 mode;
  a_inode *aino = xcalloc (a_inode, 1);
	TCHAR *s;

  mode = do_get_mem_long ((uae_u32 *)(buf + 1));
  buf += 5;
	aino->aname = au ((char*)buf);
  buf += 257;
	s = au ((char*)buf);
	aino->nname = build_nname (base->nname, s);
	xfree (s);
  buf += 257;
	aino->comment = *buf != '\0' ? au ((char*)buf) : 0;
  fsdb_fill_file_attrs (base, aino);
  aino->amigaos_mode = mode;
  aino->has_dbentry = 1;
  aino->dirty = 0;
  aino->db_offset = off;
  return aino;
}

a_inode *fsdb_lookup_aino_aname (a_inode *base, const TCHAR *aname)
{
  FILE *f;

  f = get_fsdb (base, _T("r+b"));
  if (f == 0) {
    return 0;
  }
  for (;;) {
  	uae_u8 buf[1 + 4 + 257 + 257 + 81];
		TCHAR *s;
  	if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	    break;
		s = au ((char*)buf + 5);
		if (buf[0] != 0 && same_aname (s, aname)) {
	    long pos = ftell (f) - sizeof buf;
	    fclose (f);
			xfree (s);
	    return aino_from_buf (base, buf, pos);
  	}
		xfree (s);
  }
  fclose (f);
  return 0;
}

a_inode *fsdb_lookup_aino_nname (a_inode *base, const TCHAR *nname)
{
  FILE *f;
	char *s;

  f = get_fsdb (base, _T("r+b"));
  if (f == 0) {
  	return 0;
  }
	s = ua (nname);
  for (;;) {
  	uae_u8 buf[1 + 4 + 257 + 257 + 81];
  	if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	    break;
		if (buf[0] != 0 && strcmp ((char*)buf + 5 + 257, s) == 0) {
	    long pos = ftell (f) - sizeof buf;
	    fclose (f);
			xfree (s);
	    return aino_from_buf (base, buf, pos);
  	}
  }
	xfree (s);
  fclose (f);
  return 0;
}

int fsdb_used_as_nname (a_inode *base, const TCHAR *nname)
{
  FILE *f;
  uae_u8 buf[1 + 4 + 257 + 257 + 81];

	f = get_fsdb (base, _T("r+b"));
  if (f == 0) {
    return 0;
  }
  for (;;) {
		TCHAR *s;
  	if (fread (buf, 1, sizeof buf, f) < sizeof buf)
	    break;
  	if (buf[0] == 0)
	    continue;
		s = au ((char*)buf + 5 + 257);
		if (_tcscmp (s, nname) == 0) {
			xfree (s);
	    fclose (f);
	    return 1;
  	}
		xfree (s);
  }
  fclose (f);
  return 0;
}

static int needs_dbentry (a_inode *aino)
{
  const TCHAR *nn_begin;

  if (aino->deleted)
  	return 0;
    
  if (! fsdb_mode_representable_p (aino, aino->amigaos_mode) || aino->comment != 0)
  	return 1;

  nn_begin = nname_begin (aino->nname);
  return _tcscmp (nn_begin, aino->aname) != 0;
}

static void write_aino (FILE *f, a_inode *aino)
{
  uae_u8 buf[1 + 4 + 257 + 257 + 81] = { 0 };

	buf[0] = aino->needs_dbentry ? 1 : 0;
  do_put_mem_long ((uae_u32 *)(buf + 1), aino->amigaos_mode);
	ua_copy ((char*)buf + 5, 256, aino->aname);
  buf[5 + 256] = '\0';
	ua_copy ((char*)buf + 5 + 257, 256, nname_begin (aino->nname));
  buf[5 + 257 + 256] = '\0';
	ua_copy ((char*)buf + 5 + 2 * 257, 80, aino->comment ? aino->comment : _T(""));
  buf[5 + 2 * 257 + 80] = '\0';
  aino->db_offset = ftell (f);
  fwrite (buf, 1, sizeof buf, f);
  aino->has_dbentry = aino->needs_dbentry;
	TRACE ((_T("%d '%s' '%s' written\n"), aino->db_offset, aino->aname, aino->nname));
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

	TRACE ((_T("fsdb writeback %s\n"), dir->aname));
  /* First pass: clear dirty bits where unnecessary, and see if any work
   * needs to be done.  */
  for (aino = dir->child; aino; aino = aino->sibling) {
		/*
		int old_needs_dbentry = aino->needs_dbentry || aino->has_dbentry;
		aino->needs_dbentry = needs_dbentry (aino);
		entries_needed |= aino->has_dbentry | aino->needs_dbentry;
		*/
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
		TRACE ((_T("fsdb removed\n")));
  	return;
  }

  if (! changes_needed) {
		TRACE ((_T("not modified\n")));
    return;
  }

	f = get_fsdb (dir, _T("r+b"));
  if (f == 0) {
  	f = get_fsdb (dir, _T("w+b"));
  	if (f == 0) {
	    TRACE ((_T("failed\n")));
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
	TRACE ((_T("**** updating '%s' %d\n"), dir->aname, size));

  for (aino = dir->child; aino; aino = aino->sibling) {
  	if (! aino->dirty)
	    continue;
  	aino->dirty = 0;

  	i = 0;
  	while (!aino->has_dbentry && i < size) {
			TCHAR *s = au ((char*)tmpbuf + i +  5);
			if (!_tcscmp (s, aino->aname)) {
    		aino->has_dbentry = 1;
    		aino->db_offset = i;
	    }
			xfree (s);
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
	TRACE ((_T("end\n")));
  fclose (f);
  xfree (tmpbuf);
}
