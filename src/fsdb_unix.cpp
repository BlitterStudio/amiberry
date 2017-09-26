 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Library of functions to make emulated filesystem as independent as
  * possible of the host filesystem's capabilities.
  * This is the Unix version.
  *
  * Copyright 1999 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"

#include "fsdb.h"
#include "uae.h"

/* these are deadly (but I think allowed on the Amiga): */
#define NUM_EVILCHARS 7
static TCHAR evilchars[NUM_EVILCHARS] = { '\\', '*', '?', '\"', '<', '>', '|' };

#define UAEFSDB_BEGINS _T("__uae___")

/* Return nonzero for any name we can't create on the native filesystem.  */
static int fsdb_name_invalid_2 (const TCHAR *n, int dir)
{
  int i;
	int l = _tcslen (n);

  /* the reserved fsdb filename */
  if (_tcscmp (n, FSDB_FILE) == 0)
  	return -1;

	if (dir) {
		if (n[0] == '.' && l == 1)
			return -1;
		if (n[0] == '.' && n[1] == '.' && l == 2)
			return -1;
	}

  /* these characters are *never* allowed */
  for (i = 0; i < NUM_EVILCHARS; i++) {
    if (_tcschr (n, evilchars[i]) != 0)
      return 1;
  }

	return 0; /* the filename passed all checks, now it should be ok */
}

int fsdb_name_invalid (const TCHAR *n)
{
  int v = fsdb_name_invalid_2 (n, 0);
  if (v <= 0)
    return v;
  write_log (_T("FILESYS: '%s' illegal filename\n"), n);
  return v;
}

int fsdb_name_invalid_dir (const TCHAR *n)
{
	int v = fsdb_name_invalid_2 (n, 1);
	if (v <= 0)
		return v;
	write_log (_T("FILESYS: '%s' illegal filename\n"), n);
	return v;
}

int fsdb_exists (const TCHAR *nname)
{
  struct stat statbuf;
  return (stat (nname, &statbuf) != -1);
}

/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
int fsdb_fill_file_attrs (a_inode *base, a_inode *aino)
{
    struct stat statbuf;
    /* This really shouldn't happen...  */
    if (stat (aino->nname, &statbuf) == -1)
	return 0;
    aino->dir = S_ISDIR (statbuf.st_mode) ? 1 : 0;
    
    aino->amigaos_mode = ((S_IXUSR & statbuf.st_mode ? 0 : A_FIBF_EXECUTE)
    			  | (S_IWUSR & statbuf.st_mode ? 0 : A_FIBF_WRITE)
    			  | (S_IRUSR & statbuf.st_mode ? 0 : A_FIBF_READ));

#if defined(WIN32) || defined(ANDROIDSDL) || defined(RASPBERRY)
    // Always give execute & read permission
    // Temporary do this for raspberry...
    aino->amigaos_mode &= ~A_FIBF_EXECUTE;
    aino->amigaos_mode &= ~A_FIBF_READ;
#endif
    return 1;
}

int fsdb_set_file_attrs (a_inode *aino)
{
  struct stat statbuf;
  int mode;
  uae_u32 mask = aino->amigaos_mode;

	if (aino->vfso)
		return 1;

    if (stat (aino->nname, &statbuf) == -1)
	return ERROR_OBJECT_NOT_AROUND;
	
    mode = statbuf.st_mode;

	if (mask & A_FIBF_READ)
	    mode &= ~S_IRUSR;
	else
	    mode |= S_IRUSR;

	if (mask & A_FIBF_WRITE)
	    mode &= ~S_IWUSR;
	else
	    mode |= S_IWUSR;

	if (mask & A_FIBF_EXECUTE)
	    mode &= ~S_IXUSR;
	else
	    mode |= S_IXUSR;

	chmod (aino->nname, mode);

    aino->dirty = 1;
    return 0;
}

/* return supported combination */
int fsdb_mode_supported (const a_inode *aino)
{
  int mask = aino->amigaos_mode;
	if (aino->vfso)
		return mask;
  if (0 && aino->dir)
    return 0;
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  mask &= ~(A_FIBF_SCRIPT | A_FIBF_READ | A_FIBF_EXECUTE);
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  mask &= ~A_FIBF_WRITE;
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  mask &= ~A_FIBF_DELETE;
  if (fsdb_mode_representable_p (aino, mask))
    return mask;
  return 0;
}

/* Return nonzero if we can represent the amigaos_mode of AINO within the
 * native FS.  Return zero if that is not possible.  */
int fsdb_mode_representable_p (const a_inode *aino, int amigaos_mode)
{
  int mask = amigaos_mode ^ 15;

  if (0 && aino->dir)
  	return amigaos_mode == 0;
    
	if (aino->vfso)
		return 1;
  if (mask & A_FIBF_SCRIPT) /* script */
    return 0;
  if ((mask & 15) == 15) /* xxxxRWED == OK */
    return 1;
  if (!(mask & A_FIBF_EXECUTE)) /* not executable */
    return 0;
  if (!(mask & A_FIBF_READ)) /* not readable */
    return 0;
  if ((mask & 15) == (A_FIBF_READ | A_FIBF_EXECUTE)) /* ----RxEx == ReadOnly */
    return 1;
  return 0;
}

TCHAR *fsdb_create_unique_nname (a_inode *base, const TCHAR *suggestion)
{
	TCHAR *c;
  TCHAR tmp[256] = UAEFSDB_BEGINS;
 	int i;

  _tcsncat (tmp, suggestion, 240);

	/* replace the evil ones... */
	for (i = 0; i < NUM_EVILCHARS; i++)
		while ((c = _tcschr (tmp, evilchars[i])) != 0)
			*c = '_';

	while ((c = _tcschr (tmp, '.')) != 0)
		*c = '_';
	while ((c = _tcschr (tmp, ' ')) != 0)
		*c = '_';

  for (;;) {
  	TCHAR *p = build_nname (base->nname, tmp);
  	if (access (p, R_OK) < 0 && errno == ENOENT) {
  	    write_log (_T("unique name: %s\n"), p);
  	    return p;
  	}
  	xfree (p);

	  /* tmpnam isn't reentrant and I don't really want to hack configure
	   * right now to see whether tmpnam_r is available...  */
	  for (i = 0; i < 8; i++) {
      tmp[i+8] = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[uaerand () % 63];
	  }
  }
}
