 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder, Tim Gunn
  *     2002-2007 Toni Wilen
  */

#define RECURSIVE_ARCHIVES 1

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "options.h"
#include "zfile.h"
#include "disk.h"
#include "gui.h"
#include "crc32.h"
#include "fsdb.h"
#include "fsusage.h"
#include "zarchive.h"
#include "diskutil.h"

#include "archivers/zip/unzip.h"
#include "archivers/dms/pfile.h"
#include "archivers/wrp/warp.h"

#ifdef ANDROIDSDL
#include <android/log.h>
#endif

static struct zfile *zlist = 0;

const TCHAR *uae_archive_extensions[] = { _T("zip"), _T("7z"), _T("lha"), _T("lzh"), _T("lzx"), NULL };

static void checkarchiveparent (struct zfile *z)
{
	// unpack completely if opened in PEEK mode
	if (z->archiveparent)
		archive_unpackzfile (z);
}

static struct zfile *zfile_create (struct zfile *prev)
{
  struct zfile *z;

  z = xmalloc (struct zfile, 1);
  if (!z)
  	return 0;
  memset (z, 0, sizeof *z);
  z->next = zlist;
  zlist = z;
  z->opencnt = 1;
  if (prev) {
  	z->zfdmask = prev->zfdmask;
  }
  return z;
}

static void zfile_free (struct zfile *f)
{
  if (f->f)
  	fclose (f->f);
  if (f->deleteafterclose) {
  	_wunlink (f->name);
		write_log (_T("deleted temporary file '%s'\n"), f->name);
  }
  xfree (f->name);
  xfree (f->data);
  xfree (f->mode);
  xfree (f);
}

void zfile_exit (void)
{
  struct zfile *l;

  while ((l = zlist)) {
  	zlist = l->next;
  	zfile_free (l);
  }
}

void zfile_fclose (struct zfile *f)
{
  if (!f)
  	return;
  if (f->opencnt < 0) {
		write_log (_T("zfile: tried to free already closed filehandle!\n"));
  	return;
  }
  f->opencnt--;
  if (f->opencnt > 0)
  	return;
  f->opencnt = -100;
  if (f->parent) {
  	f->parent->opencnt--;
  	if (f->parent->opencnt <= 0)
	    zfile_fclose (f->parent);
  }
	if (f->archiveparent) {
		zfile_fclose (f->archiveparent);
		f->archiveparent = NULL;
	}
	struct zfile *pl = NULL;
	struct zfile *nxt;
	struct zfile *l  = zlist;
  while (l!=f) {
  	if (l == 0) {
			write_log (_T("zfile: tried to free already freed or nonexisting filehandle!\n"));
	    return;
  	}
  	pl = l;
  	l = l->next;
  }
  if (l) 
    nxt = l->next;
  zfile_free (f);
  if (l == 0)
  	return;
  if(!pl)
  	zlist = nxt;
  else
  	pl->next = nxt;
}

static void removeext (TCHAR *s, TCHAR *ext)
{
	if (_tcslen (s) < _tcslen (ext))
		return;
	if (_tcsicmp (s + _tcslen (s) - _tcslen (ext), ext) == 0)
		s[_tcslen (s) - _tcslen (ext)] = 0;
}

static bool checkwrite (struct zfile *zf, int *retcode)
{
	if (zfile_needwrite (zf)) {
		if (retcode)
			*retcode = -1;
		return true;
	}
	return false;
}

static uae_u8 exeheader[]={0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00};
static TCHAR *diskimages[] = { _T("adf"), _T("adz"), _T("ipf"), _T("fdi"), _T("dms"), _T("wrp"), _T("dsq"), 0 };

int zfile_gettype (struct zfile *z)
{
  uae_u8 buf[8];
  TCHAR *ext;
    
  if (!z || !z->name)
  	return ZFILE_UNKNOWN;
  ext = _tcsrchr (z->name, '.');
  if (ext != NULL) {
  	int i;
  	ext++;
  	for (i = 0; diskimages[i]; i++) {
	    if (strcasecmp (ext, diskimages[i]) == 0)
    		return ZFILE_DISKIMAGE;
  	}
		if (strcasecmp (ext, _T("roz")) == 0)
	    return ZFILE_ROM;
		if (strcasecmp (ext, _T("uss")) == 0)
	    return ZFILE_STATEFILE;
		if (strcasecmp (ext, _T("rom")) == 0)
	    return ZFILE_ROM;
		if (strcasecmp (ext, _T("key")) == 0)
	    return ZFILE_KEY;
		if (strcasecmp (ext, _T("nvr")) == 0)
	    return ZFILE_NVR;
		if (strcasecmp (ext, _T("uae")) == 0)
	    return ZFILE_CONFIGURATION;
  }
  memset (buf, 0, sizeof (buf));
  zfile_fread (buf, 8, 1, z);
  zfile_fseek (z, -8, SEEK_CUR);
  if (!memcmp (buf, exeheader, sizeof(buf)))
    return ZFILE_DISKIMAGE;
  if (!memcmp (buf, "RDSK", 4))
  	return ZFILE_HDFRDB;
	if (!memcmp (buf, "DOS", 3)) {
		if (z->size < 4 * 1024 * 1024)
			return ZFILE_DISKIMAGE;
		else
	    return ZFILE_HDF;
	}
  if (ext != NULL) {
		if (strcasecmp (ext, _T("hdf")) == 0)
	    return ZFILE_HDF;
		if (strcasecmp (ext, _T("hdz")) == 0)
	    return ZFILE_HDF;
  }
  return ZFILE_UNKNOWN;
}

static struct zfile *zfile_gunzip (struct zfile *z, int *retcode)
{
  uae_u8 header[2 + 1 + 1 + 4 + 1 + 1];
  z_stream zs;
  int i, size, ret, first;
  uae_u8 flags;
  uae_s64 offset;
  TCHAR name[MAX_DPATH];
  uae_u8 buffer[8192];
  struct zfile *z2;
  uae_u8 b;

  if (checkwrite (z, retcode))
	  return NULL;
  _tcscpy (name, z->name);
  memset (&zs, 0, sizeof (zs));
  memset (header, 0, sizeof (header));
  zfile_fread (header, sizeof (header), 1, z);
  flags = header[3];
  if (header[0] != 0x1f && header[1] != 0x8b)
  	return NULL;
  if (flags & 2) /* multipart not supported */
  	return NULL;
  if (flags & 32) /* encryption not supported */
  	return NULL;
  if (flags & 4) { /* skip extra field */
    zfile_fread (&b, 1, 1, z);
	  size = b;
	  zfile_fread (&b, 1, 1, z);
	  size |= b << 8;
	  zfile_fseek (z, size + 2, SEEK_CUR);
  }

  if (flags & 8) { /* get original file name */
  	i = 0;
  	do {
	    zfile_fread (name + i, 1, 1, z);
  	} while (i < MAX_DPATH - 1 && name[i++]);
  	name[i] = 0;
  }
  if (flags & 16) { /* skip comment */
  	i = 0;
  	do {
	    b = 0;
	    zfile_fread (&b, 1, 1, z);
  	} while (b);
  }
	removeext (name, _T(".gz"));
  offset = zfile_ftell (z);
  zfile_fseek (z, -4, SEEK_END);
  zfile_fread (&b, 1, 1, z);
  size = b;
  zfile_fread (&b, 1, 1, z);
  size |= b << 8;
  zfile_fread (&b, 1, 1, z);
  size |= b << 16;
  zfile_fread (&b, 1, 1, z);
  size |= b << 24;
  if (size < 8 || size > 256 * 1024 * 1024) /* safety check */
  	return NULL;
  zfile_fseek (z, offset, SEEK_SET);
  z2 = zfile_fopen_empty (z, name, size);
  if (!z2)
  	return NULL;
  zs.next_out = z2->data;
  zs.avail_out = size;
  first = 1;
  do {
  	zs.next_in = buffer;
  	zs.avail_in = zfile_fread (buffer, 1, sizeof (buffer), z);
  	if (first) {
	    if (inflateInit2_ (&zs, -MAX_WBITS, ZLIB_VERSION, sizeof(z_stream)) != Z_OK)
    		break;
	    first = 0;
  	}
  	ret = inflate (&zs, 0);
  } while (ret == Z_OK);
  inflateEnd (&zs);
  if (ret != Z_STREAM_END || first != 0) {
  	zfile_fclose (z2);
  	return NULL;
  }
  zfile_fclose (z);
  return z2;
}
struct zfile *zfile_gunzip (struct zfile *z)
{
	return zfile_gunzip (z, NULL);
}

static void truncate880k (struct zfile *z)
{
	int i;
	uae_u8 *b;

	if (z == NULL || z->data == NULL)
		return;
	if (z->size < 880 * 512 * 2) {
		int size = 880 * 512 * 2 - z->size;
		b = xcalloc (uae_u8, size);
		zfile_fwrite (b, size, 1, z);
		xfree (b);
		return;
	}
	for (i = 880 * 512 * 2; i < z->size; i++) {
		if (z->data[i])
			return;
	}
	z->size = 880 * 512 * 2;
}

static struct zfile *extadf (struct zfile *z, int index, int *retcode)
{
  int i, r;
  struct zfile *zo;
  uae_u16 *mfm;
  uae_u16 *amigamfmbuffer;
  uae_u8 writebuffer_ok[32], *outbuf;
  int tracks, len, offs, pos;
  uae_u8 buffer[2 + 2 + 4 + 4];
  int outsize;
  TCHAR newname[MAX_DPATH];
  TCHAR *ext;
  int cantrunc = 0;
  int done = 0;

  if (index > 1)
	  return NULL;

  mfm = xcalloc (uae_u16, 32000 / 2);
  amigamfmbuffer = xcalloc (uae_u16, 32000 / 2);
  outbuf = xcalloc (uae_u8, 16384);

  zfile_fread (buffer, 1, 8, z);
  zfile_fread (buffer, 1, 4, z);
  tracks = buffer[2] * 256 + buffer[3];
  offs = 8 + 2 + 2 + tracks * (2 + 2 + 4 + 4);

  _tcscpy (newname, zfile_getname (z));
  ext = _tcsrchr (newname, '.');
  if (ext) {
		_tcscpy (newname + _tcslen (newname) - _tcslen (ext), _T(".std.adf"));
  } else {
		_tcscat (newname, _T(".std.adf"));
  }
	if (index > 0)
		_tcscpy (newname + _tcslen (newname) - 4, _T(".ima"));

  zo = zfile_fopen_empty (z, newname, 0);
  if (!zo)
  	goto end;

  if (retcode)
	  *retcode = 1;
  pos = 12;
  outsize = 0;
  for (i = 0; i < tracks; i++) {
  	int type, bitlen;
	
	  zfile_fseek (z, pos, SEEK_SET);
	  zfile_fread (buffer, 2 + 2 + 4 + 4, 1, z);
	  pos = zfile_ftell (z);
	  type = buffer[2] * 256 + buffer[3];
	  len = buffer[5] * 65536 + buffer[6] * 256 + buffer[7];
	  bitlen = buffer[9] * 65536 + buffer[10] * 256 + buffer[11];

	  zfile_fseek (z, offs, SEEK_SET);
	  if (type == 1) {
	    zfile_fread (mfm, len, 1, z);
	    memset (writebuffer_ok, 0, sizeof writebuffer_ok);
	    memset (outbuf, 0, 16384);
			if (index == 0) {
    		r = isamigatrack (amigamfmbuffer, (uae_u8*)mfm, len, outbuf, writebuffer_ok, i, &outsize);
    	  if (r < 0 && i == 0) {
  		    goto end;
    	  }
				if (i == 0)
					done = 1;
	    } else {
    		r = ispctrack (amigamfmbuffer, (uae_u8*)mfm, len, outbuf, writebuffer_ok, i, &outsize);
    		if (r < 0 && i == 0) {
  		    goto end;
    	  }
				if (i == 0)
					done = 1;
	    }
  	} else {
	    outsize = 512 * 11;
			if (bitlen / 8 > 18000)
				outsize *= 2;
	    zfile_fread (outbuf, outsize, 1, z);
			cantrunc = 1;
			if (index == 0)
				done = 1;
  	}
  	zfile_fwrite (outbuf, outsize, 1, zo);

  	offs += len;

  }
	if (done == 0)
		goto end;
  zfile_fclose (z);
  xfree (mfm);
  xfree (amigamfmbuffer);
	if (cantrunc)
		truncate880k (zo);
  return zo;
end:
  zfile_fclose (zo);
  xfree (mfm);
  xfree (amigamfmbuffer);
	return NULL;
}

static struct zfile *dsq (struct zfile *z, int lzx, int *retcode)
{
  struct zfile *zi = NULL;
  struct zvolume *zv = NULL;

  if (checkwrite (z, retcode))
	  return NULL;
  if (lzx) {
  	zv = archive_directory_lzx (z);
  	if (zv) {
	    if (zv->root.child)
    		zi = archive_access_lzx (zv->root.child);
  	}
  } else {
  	zi = z;
  }
  if (zi) {
    uae_u8 *buf = zfile_getdata (zi, 0, -1);
  	if (!memcmp (buf, "PKD\x13", 4) || !memcmp (buf, "PKD\x11", 4)) {
	    TCHAR *fn;
	    int sectors = buf[18];
	    int heads = buf[15];
	    int blocks = (buf[6] << 8) | buf[7];
	    int blocksize = (buf[10] << 8) | buf[11];
	    struct zfile *zo;
	    int size = blocks * blocksize;
			int off;
	    int i;
			uae_u8 *bitmap = NULL;
			uae_u8 *nullsector;

			nullsector = xcalloc (uae_u8, blocksize);
			sectors /= heads;
			if (buf[3] == 0x13) {
				off = 52;
				if (buf[off - 1] == 1) {
					bitmap = &buf[off];
					off += (blocks + 7) / 8;
				} else if (buf[off - 1] > 1) {
					write_log (_T("unknown DSQ extra header type %d\n"), buf[off - 1]);
				}
			} else {
				off = 32;
			}

	    if (size < 1760 * 512)
		    size = 1760 * 512;

	    if (zfile_getfilename (zi) && _tcslen (zfile_getfilename (zi))) {
    		fn = xmalloc (TCHAR, (_tcslen (zfile_getfilename (zi)) + 5));
    		_tcscpy (fn, zfile_getfilename (zi));
				_tcscat (fn, _T(".adf"));
	    } else {
				fn = my_strdup (_T("dsq.adf"));
	    }
	    zo = zfile_fopen_empty (z, fn, size);
	    xfree (fn);
			int seccnt = 0;
			for (i = 0; i < blocks; i++) {
				int bmoff = i - 2;
				int boff = -1;
				uae_u32 mask = 0;
				if (bitmap) {
					boff = (bmoff / 32) * 4;
					mask = (bitmap[boff] << 24) | (bitmap[boff + 1] << 16) | (bitmap[boff + 2] << 8) | (bitmap[boff + 3]);
				}
				if (bmoff >= 0 && boff >= 0 && (mask & (1 << (bmoff & 31)))) {
					zfile_fwrite (nullsector, blocksize, 1, zo);
				} else {
					zfile_fwrite (buf + off, blocksize, 1, zo);
					off += blocksize;
					seccnt++;
				}
				if ((i % sectors) == sectors - 1) {
					off += seccnt * 16;
					seccnt = 0;
				}
	    }
	    zfile_fclose_archive (zv);
	    zfile_fclose (z);
	    xfree (buf);
			xfree (nullsector);
	    return zo;
	  }
	  xfree (buf);
  }
  if (lzx)
  	zfile_fclose (zi);
  return z;
}

static struct zfile *wrp (struct zfile *z, int *retcode)
{
	if (zfile_needwrite (z)) {
		if (retcode)
			*retcode = -1;
		return NULL;
	}
  return unwarp (z);
}

static struct zfile *dms (struct zfile *z, int index, int *retcode)
{
  int ret;
  struct zfile *zo;
  TCHAR *orgname = zfile_getname (z);
  TCHAR *ext = _tcsrchr (orgname, '.');
  TCHAR newname[MAX_DPATH];
	static int recursive;
	int i;
	struct zfile *zextra[DMS_EXTRA_SIZE] = { 0 };

	if (checkwrite (z, retcode))
		return NULL;
	if (recursive)
		return NULL;
  if (ext) {
	  _tcscpy (newname, orgname);
		_tcscpy (newname + _tcslen (newname) - _tcslen (ext), _T(".adf"));
  } else {
		_tcscat (newname, _T(".adf"));
  }

  zo = zfile_fopen_empty (z, newname, 1760 * 512);
  if (!zo) 
    return NULL;
  ret = DMS_Process_File (z, zo, CMD_UNPACK, OPT_VERBOSE, 0, 0, 0, zextra);
  if (ret == NO_PROBLEM || ret == DMS_FILE_END) {
		int off = zfile_ftell (zo);
		if (off >= 1760 * 512 / 3 && off <= 1760 * 512 * 3 / 4) { // possible split dms?
			if (_tcslen (orgname) > 5) {
				TCHAR *s = orgname + _tcslen (orgname) - 5;
				if (!_tcsicmp (s, _T("a.dms"))) {
					TCHAR *fn2 = my_strdup (orgname);
					struct zfile *z2;
					fn2[_tcslen (fn2) - 5]++;
					recursive++;
					z2 = zfile_fopen (fn2, _T("rb"), z->zfdmask);
					recursive--;
					if (z2) {
						ret = DMS_Process_File (z2, zo, CMD_UNPACK, OPT_VERBOSE, 0, 0, 1, NULL);
						zfile_fclose (z2);
					}
					xfree (fn2);
				}
			}
		}
  	zfile_fseek (zo, 0, SEEK_SET);
		if (index > 0) {
			zfile_fclose (zo);
			zo = NULL;
			for (i = 0; i < DMS_EXTRA_SIZE && zextra[i]; i++);
			if (index > i)
				goto end;
			zo = zextra[index - 1];
			zextra[index - 1] = NULL;
    }
		if (retcode)
			*retcode = 1;
		zfile_fclose (z);
		z = NULL;

	} else {
		zfile_fclose (zo);
		zo = NULL;
	}
end:
	for (i = 0; i < DMS_EXTRA_SIZE; i++)
		zfile_fclose (zextra[i]);
	return zo;
}

const TCHAR *uae_ignoreextensions[] = 
  { _T(".gif"), _T(".jpg"), _T(".png"), _T(".xml"), _T(".pdf"), _T(".txt"), 0 };
const TCHAR *uae_diskimageextensions[] =
  { _T(".adf"), _T(".adz"), _T(".ipf"), _T(".fdi"), _T(".exe"), _T(".dms"), _T(".wrp"), _T(".dsq"), 0 };

int zfile_is_ignore_ext(const TCHAR *name)
{
  int i;
	const TCHAR *ext;
    
	ext = _tcsrchr (name, '.');
	if (!ext)
		return 0;
  for (i = 0; uae_ignoreextensions[i]; i++) {
		if (!strcasecmp (uae_ignoreextensions[i], ext))
      return 1;
  }
  return 0;
}

int zfile_is_diskimage (const TCHAR *name)
{
  int i;

	const TCHAR *ext = _tcsrchr (name, '.');
	if (!ext)
		return 0;
  i = 0;
  while (uae_diskimageextensions[i]) {
		if (!strcasecmp (ext, uae_diskimageextensions[i]))
			return HISTORY_FLOPPY;
	    i++;
  }
	if (!_tcsicmp (ext, _T(".cue")))
		return HISTORY_CD;
	return -1;
}

static const TCHAR *archive_extensions[] = {
	_T("7z"), _T("zip"), _T("lha"), _T("lzh"), _T("lzx"),
	_T("adf"), _T("adz"), _T("dsq"), _T("dms"), _T("ipf"), _T("fdi"), _T("wrp"), _T("ima"),
	_T("hdf"),
	NULL
};
static const TCHAR *plugins_7z[] = { _T("7z"), _T("zip"), _T("lha"), _T("lzh"), _T("lzx"), _T("adf"), _T("dsq"), _T("hdf"), NULL };
static const uae_char *plugins_7z_x[] = { "7z", "MK", NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static const int plugins_7z_t[] = { 
    ArchiveFormat7Zip, ArchiveFormatZIP, ArchiveFormatLHA, ArchiveFormatLHA, ArchiveFormatLZX,
    ArchiveFormatADF, ArchiveFormatADF, ArchiveFormatADF
};
static const int plugins_7z_m[] = {
    ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE,
    ZFD_ADF, ZFD_ADF, ZFD_ADF 
};

int iszip (struct zfile *z, int mask)
{
  TCHAR *name = z->name;
  TCHAR *ext = _tcsrchr (name, '.');
  uae_u8 header[32];
  int i;

  if (!ext)
  	return 0;
  memset (header, 0, sizeof (header));
  zfile_fseek (z, 0, SEEK_SET);
  zfile_fread (header, sizeof (header), 1, z);
  zfile_fseek (z, 0, SEEK_SET);

  if (mask & ZFD_ARCHIVE) {
		if (!strcasecmp (ext, _T(".zip")) || !strcasecmp (ext, _T(".rp9"))) {
      if(header[0] == 'P' && header[1] == 'K')
    	  return ArchiveFormatZIP;
      return 0;
    }
  }
  if (mask & ZFD_ARCHIVE) {
    if (!strcasecmp (ext, _T(".7z"))) {
      if(header[0] == '7' && header[1] == 'z')
    	  return ArchiveFormat7Zip;
      return 0;
    }
    if (!strcasecmp (ext, _T(".lha")) || !strcasecmp (ext, _T(".lzh"))) {
      if(header[2] == '-' && header[3] == 'l' && header[4] == 'h' && header[6] == '-')
        return ArchiveFormatLHA;
      return 0;
    }
    if (!strcasecmp (ext, _T(".lzx"))) {
      if(header[0] == 'L' && header[1] == 'Z' && header[2] == 'X')
        return ArchiveFormatLZX;
      return 0;
    }
  }
  if (mask & ZFD_ADF) {
  	if (!strcasecmp (ext, _T(".adf"))) {
	    if (header[0] == 'D' && header[1] == 'O' && header[2] == 'S' && (header[3] >= 0 && header[3] <= 7))
		    return ArchiveFormatADF;
	    return 0;
	  }
  }
  if (mask & ZFD_HD) {
  	if (!strcasecmp (ext, _T(".hdf"))) {
	    if (header[0] == 'D' && header[1] == 'O' && header[2] == 'S' && (header[3] >= 0 && header[3] <= 7))
    		return ArchiveFormatADF;
	    if (header[0] == 'S' && header[1] == 'F' && header[2] == 'S')
    		return ArchiveFormatADF;
	    return 0;
  	}
  }
  return 0;
}
int iszip (struct zfile *z)
{
	return iszip (z, ZFD_NORMAL);
}

struct zfile *zuncompress (struct znode *parent, struct zfile *z, int dodefault, int mask, int *retcode, int index)
{
  TCHAR *name = z->name;
  TCHAR *ext = NULL;
  uae_u8 header[32];
  int i;

  if (retcode)
  	*retcode = 0;
  if (!mask)
  	return NULL;
  if (name) {
  	ext = _tcsrchr (name, '.');
  	if (ext)
	    ext++;
  }

  if (ext != NULL) {
  	if (mask & ZFD_ARCHIVE) {
	    if (strcasecmp (ext, _T("7z")) == 0)
        return archive_access_select (parent, z, ArchiveFormat7Zip, dodefault, retcode, index);
      if (strcasecmp (ext, _T("zip")) == 0)
        return archive_access_select (parent, z, ArchiveFormatZIP, dodefault, retcode, index);
	    if (strcasecmp (ext, _T("lha")) == 0 || strcasecmp (ext, _T("lzh")) == 0)
        return archive_access_select (parent, z, ArchiveFormatLHA, dodefault, retcode, index);
	    if (strcasecmp (ext, _T("lzx")) == 0)
	      return archive_access_select (parent, z, ArchiveFormatLZX, dodefault, retcode, index);
    }
	  if (mask & ZFD_UNPACK) {
		  if (index == 0) {
				if (strcasecmp (ext, _T("gz")) == 0)
	        return zfile_gunzip (z, retcode);
				if (strcasecmp (ext, _T("adz")) == 0)
	        return zfile_gunzip (z, retcode);
				if (strcasecmp (ext, _T("roz")) == 0)
	        return zfile_gunzip (z, retcode);
				if (strcasecmp (ext, _T("hdz")) == 0)
	        return zfile_gunzip (z, retcode);
				if (strcasecmp (ext, _T("wrp")) == 0)
	        return wrp (z, retcode);
		  }
      if (strcasecmp (ext, _T("dms")) == 0)
        return dms (z, index, retcode);
    }
  }
  memset (header, 0, sizeof (header));
  zfile_fseek (z, 0, SEEK_SET);
  zfile_fread (header, sizeof (header), 1, z);
  zfile_fseek (z, 0, SEEK_SET);
  if (mask & ZFD_UNPACK) {
	  if (index == 0) {
  	  if (header[0] == 0x1f && header[1] == 0x8b)
	      return zfile_gunzip (z, retcode);
  	  if (header[0] == 'P' && header[1] == 'K' && header[2] == 'D')
	      return dsq (z, 0, retcode);
    }
	  if (header[0] == 'D' && header[1] == 'M' && header[2] == 'S' && header[3] == '!')
      return dms (z, index, retcode);
	}
  if (mask & ZFD_RAWDISK) {
	  if (!memcmp (header, "UAE-1ADF", 8))
		  return extadf (z, index, retcode);
  }
  if (index > 0)
	  return NULL;
  if (mask & ZFD_ARCHIVE) {
    if (header[0] == 'P' && header[1] == 'K')
      return archive_access_select (parent, z, ArchiveFormatZIP, dodefault, retcode, index);
  	if (header[0] == 'L' && header[1] == 'Z' && header[2] == 'X')
      return archive_access_select (parent, z, ArchiveFormatLZX, dodefault, retcode, index);
	  if (header[2] == '-' && header[3] == 'l' && header[4] == 'h' && header[6] == '-')
	    return archive_access_select (parent, z, ArchiveFormatLHA, dodefault, retcode, index);
  }
  if (mask & ZFD_ADF) {
   	if (header[0] == 'D' && header[1] == 'O' && header[2] == 'S' && (header[3] >= 0 && header[3] <= 7))
	    return archive_access_select (parent, z, ArchiveFormatADF, dodefault, retcode, index);
   	if (header[0] == 'S' && header[1] == 'F' && header[2] == 'S')
	    return archive_access_select (parent, z, ArchiveFormatADF, dodefault, retcode, index);
  }

  if (ext) {
  	if (mask & ZFD_UNPACK) {
			if (strcasecmp (ext, _T("dsq")) == 0)
    		return dsq (z, 1, retcode);
  	}
  	if (mask & ZFD_ADF) {
			if (strcasecmp (ext, _T("adf")) == 0 && !memcmp (header, "DOS", 3))
    		return archive_access_select (parent, z, ArchiveFormatADF, dodefault, retcode, index);
  	}
  }
  return NULL;
}

static struct zfile *zfile_fopen_nozip (const TCHAR *name, const TCHAR *mode)
{
  struct zfile *l;
  FILE *f;

  if(*name == '\0')
  	return NULL;
  l = zfile_create (NULL);
  l->name = my_strdup (name);
  l->mode = my_strdup (mode);
  f = _tfopen (name, mode);
  if (!f) {
    zfile_fclose (l);
    return 0;
  }
  l->f = f;
  return l;
}


static struct zfile *openzip (const TCHAR *pname)
{
  int i, j;
  TCHAR v;
  TCHAR name[MAX_DPATH];
  TCHAR zippath[MAX_DPATH];

  zippath[0] = 0;
  _tcscpy (name, pname);
  i = _tcslen (name) - 2;
  while (i > 0) {
  	if (name[i] == '/' || name[i] == '\\' && i > 4) {
	    v = name[i];
	    name[i] = 0;
	    for (j = 0; plugins_7z[j]; j++) {
    		int len = _tcslen (plugins_7z[j]);
    		if (name[i - len - 1] == '.' && !strcasecmp (name + i - len, plugins_7z[j])) {
  		    struct zfile *f = zfile_fopen_nozip (name, _T("rb"));
		      if (f) {
    			  f->zipname = my_strdup(name + i + 1);
    			  return f;
		      }
		      break;
  		  }
	    }
	    name[i] = v;
  	}
  	i--;
  }
  return 0;
}

static bool writeneeded (const TCHAR *mode)
{
	return _tcschr (mode, 'w') || _tcschr (mode, 'a') || _tcschr (mode, '+') || _tcschr (mode, 't');
}
bool zfile_needwrite (struct zfile *zf)
{
	if (!zf->mode)
		return false;
	return writeneeded (zf->mode);
}

static struct zfile *zfile_fopen_2 (const TCHAR *name, const TCHAR *mode, int mask)
{
  struct zfile *l;
  FILE *f;

  if( *name == '\0' )
    return NULL;
  l = openzip (name);
  if (l) {
  	if (writeneeded (mode)) {
	    zfile_fclose (l);
	    return 0;
  	}
  	l->zfdmask = mask;
  } else {
  	struct stat st;
  	l = zfile_create (NULL);
  	l->mode = my_strdup (mode);
  	l->name = my_strdup (name);
  	l->zfdmask = mask;
  	if (!_tcsicmp (mode, _T("r"))) {
	    f = my_opentext (l->name);
	    l->textmode = 1;
  	} else {
	    f = _tfopen (l->name, mode);
  	}
    if (!f) {
    	zfile_fclose (l);
    	return 0;
    }
		if (stat (l->name, &st) != -1)
			l->size = st.st_size;
  	l->f = f;
  }
  return l;
}

#define AF _T("%AMIGAFOREVERDATA%")

static void manglefilename(TCHAR *out, const TCHAR *in)
{
	int i;

  out[0] = 0;
  if (!strncasecmp(in, AF, _tcslen(AF)))
  	_tcscpy (out, start_path_data);
  if ((in[0] == '/' || in[0] == '\\') || (_tcslen(in) > 3 && in[1] == ':' && in[2] == '\\'))
  	out[0] = 0;
  _tcscat(out, in);
	for (i = 0; i < _tcslen (out); i++) {
		// remove \\ or // in the middle of path
		if ((out[i] == '/' || out[i] == '\\') && (out[i + 1] == '/' || out[i + 1] == '\\') && i > 0) {
			memmove (out + i, out + i + 1, (_tcslen (out + i) + 1) * sizeof (TCHAR));
			i--;
			continue;
    }
  }
}

int zfile_zopen (const TCHAR *name, zfile_callback zc, void *user)
{
  struct zfile *l;
  int ztype;
  TCHAR path[MAX_DPATH];
    
  manglefilename(path, name);
  l = zfile_fopen_2 (path, _T("rb"), ZFD_NORMAL);
  if (!l)
  	return 0;
  ztype = iszip (l);
  if (ztype == 0)
    zc (l, user);
  else
  	archive_access_scan (l, zc, user, ztype);
  zfile_fclose (l);
  return 1;
}    

/*
 * fopen() for a compressed file
 */
static struct zfile *zfile_fopen_x (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
  int cnt = 10;
  struct zfile *l, *l2;
  TCHAR path[MAX_DPATH];

	if (_tcslen (name) == 0)
		return NULL;
  manglefilename(path, name);
  l = zfile_fopen_2 (path, mode, mask);
  if (!l)
  	return 0;
  l2 = NULL;
  while (cnt-- > 0) {
  	int rc;
    zfile_fseek (l, 0, SEEK_SET);
  	l2 = zuncompress (NULL, l, 0, mask, &rc, index);
  	if (!l2) {
	    if (rc < 0) {
    		zfile_fclose (l);
    		return NULL;
	    }
	    zfile_fseek (l, 0, SEEK_SET);
      break;
		} else {
			if (l2->parent == l)
				l->opencnt--;
	  }
	  l = l2;
  }
  return l;
}

static struct zfile *zfile_fopenx2 (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	struct zfile *f;
	TCHAR tmp[MAX_DPATH];

	f = zfile_fopen_x (name, mode, mask, index);
	if (f)
		return f;
	if (_tcslen (name) <= 2)
		return NULL;
	if (name[1] != ':') {
		_tcscpy (tmp, start_path_data);
		_tcscat (tmp, name);
		f = zfile_fopen_x (tmp, mode, mask, index);
		if (f)
			return f;
	}
	return NULL;
}

static struct zfile *zfile_fopenx (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	struct zfile *zf;
	zf = zfile_fopenx2 (name, mode, mask, index);
	return zf;
}

struct zfile *zfile_fopen (const TCHAR *name, const TCHAR *mode, int mask)
{
	return zfile_fopenx (name, mode, mask, 0);
}
struct zfile *zfile_fopen (const TCHAR *name, const TCHAR *mode)
{
	return zfile_fopenx (name, mode, 0, 0);
}
struct zfile *zfile_fopen (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	return zfile_fopenx (name, mode, mask, index);
}

struct zfile *zfile_dup (struct zfile *zf)
{
  struct zfile *nzf;
  if (!zf)
  	return NULL;
	if (zf->archiveparent)
		checkarchiveparent (zf);
  if (zf->data) {
	  nzf = zfile_create (zf);
    nzf->data = xmalloc (uae_u8, zf->size);
    memcpy (nzf->data, zf->data, zf->size);
    nzf->size = zf->size;
	  nzf->datasize = zf->datasize;
  } else {
    if (zf->zipname) {
  	  nzf = openzip (zf->name);
	  	if (nzf)
    	  return nzf;
    }
		FILE *ff = _tfopen (zf->name, zf->mode);
		if (!ff)
			return NULL;
	  nzf = zfile_create (zf);
    nzf->f = ff;
  }
  zfile_fseek (nzf, zf->seek, SEEK_SET);
  if (zf->name)
      nzf->name = my_strdup (zf->name);
  if (nzf->zipname)
      nzf->zipname = my_strdup (zf->zipname);
  nzf->zfdmask = zf->zfdmask;
  nzf->mode = my_strdup (zf->mode);
	nzf->size = zf->size;
  return nzf;
}

int zfile_exists (const TCHAR *name)
{
  struct zfile *z;

  if (my_existsfile (name))
  	return 1;
  z = zfile_fopen (name, _T("rb"), ZFD_NORMAL | ZFD_CHECKONLY);
  if (!z)
  	return 0;
  zfile_fclose (z);
  return 1;
}

int zfile_iscompressed (struct zfile *z)
{
  return z->data ? 1 : 0;
}

struct zfile *zfile_fopen_empty (struct zfile *prev, const TCHAR *name, uae_u64 size)
{
  struct zfile *l;
  l = zfile_create (prev);
  l->name = my_strdup (name ? name : _T(""));
  if (size) {
	  l->data = xcalloc (uae_u8, size);
    if (!l->data)  {
      xfree (l);
      return NULL;
    }
    l->size = size;
	  l->datasize = size;
  	l->allocsize = size;
  } else {
	  l->data = xcalloc (uae_u8, 1000);
  	l->size = 0;
		l->allocsize = 1000;
  }
  return l;
}

struct zfile *zfile_fopen_empty (struct zfile *prev, const TCHAR *name)
{
	return zfile_fopen_empty (prev, name, 0);
}

struct zfile *zfile_fopen_parent (struct zfile *z, const TCHAR *name, uae_u64 offset, uae_u64 size)
{
  struct zfile *l;

  if (z == NULL)
	  return NULL;
  l = zfile_create (z);
  if (name)
  	l->name = my_strdup (name);
  else if (z->name)
  	l->name = my_strdup (z->name);
  l->size = size;
  l->datasize = size;
  l->offset = offset;
  for (;;) {
  	l->parent = z;
		l->useparent = 1;
  	if (!z->parent)
      break;
    l->offset += z->offset;
  	z = z->parent;
  }
  z->opencnt++;
  return l;
}

struct zfile *zfile_fopen_data (const TCHAR *name, uae_u64 size, const uae_u8 *data)
{
  struct zfile *l;
  l = zfile_create (NULL);
  l->name = my_strdup (name ? name  : _T(""));
  l->data = xmalloc (uae_u8, size);
  l->size = size;
  l->datasize = size;
  memcpy (l->data, data, size);
  return l;
}

uae_u8 *zfile_load_data (const TCHAR *name, const uae_u8 *data,int datalen, int *outlen)
{
	struct zfile *zf, *f;
	int size;
	uae_u8 *out;
	
	zf = zfile_fopen_data (name, datalen, data);
	f = zfile_gunzip (zf);
	size = f->datasize;
	zfile_fseek (f, 0, SEEK_SET);
	out = xmalloc (uae_u8, size);
	zfile_fread (out, 1, size, f);
	zfile_fclose (f);
	*outlen = size;
	return out;
}

int zfile_truncate (struct zfile *z, uae_s64 size)
{
	if (z->data) {
		if (z->size > size) {
			z->size = size;
			if (z->datasize > z->size)
				z->datasize = z->size;
			if (z->seek > z->size)
				z->seek = z->size;
			return 1;
		}
		return 0;
	} else {
		/* !!! */
		return 0;
	}
}

uae_s64 zfile_size (struct zfile *z)
{
	return z->size;
}

uae_s64 zfile_ftell (struct zfile *z)
{
	if (z->data || z->parent)
	  return z->seek;
	return _ftelli64 (z->f);
}

uae_s64 zfile_fseek (struct zfile *z, uae_s64 offset, int mode)
{
  if (z->data || (z->parent && z->useparent)) {
  	int ret = 0;
  	switch (mode)
  	{
	    case SEEK_SET:
  	    z->seek = offset;
  	    break;
	    case SEEK_CUR:
  	    z->seek += offset;
  	    break;
 	    case SEEK_END:
	      z->seek = z->size + offset;
	      break;
	  }
	  if (z->seek < 0) {
	    z->seek = 0;
	    ret = 1;
	  }
	  if (z->seek > z->size) {
	    z->seek = z->size;
	    ret = 1;
	  }
	  return ret;
  } else {
    return _fseeki64 (z->f, offset, mode);
  }
  return 1;
}

size_t zfile_fread (void *b, size_t l1, size_t l2, struct zfile *z)
{
  if (z->data) {
  	if (z->datasize < z->size && z->seek + l1 * l2 > z->datasize) {
			if (z->archiveparent) {
				archive_unpackzfile (z);
				return zfile_fread (b, l1, l2, z);
			}
  		return 0;
  	}
  	if (z->seek + l1 * l2 > z->size) {
	    if (l1)
	    	l2 = (z->size - z->seek) / l1;
	    else
	    	l2 = 0;
	    if (l2 < 0)
	    	l2 = 0;
	  }
	  memcpy (b, z->data + z->offset + z->seek, l1 * l2);
	  z->seek += l1 * l2;
	  return l2;
  }
  if (z->parent && z->useparent) {
  	size_t ret;
  	uae_s64 v;
  	uae_s64 size = z->size;
  	v = z->seek;
  	if (v + l1 * l2 > size) {
	    if (l1)
    		l2 = (size - v) / l1;
	    else
    		l2 = 0;
	    if (l2 < 0)
    		l2 = 0;
  	}
  	zfile_fseek (z->parent, z->seek + z->offset, SEEK_SET);
  	v = z->seek;
  	ret = zfile_fread (b, l1, l2, z->parent);
  	z->seek = v + l1 * ret;
  	return ret;
  }
  return fread (b, l1, l2, z->f);
}

size_t zfile_fwrite (void *b, size_t l1, size_t l2, struct zfile *z)
{
	if (z->archiveparent)
		return 0;
  if (z->parent && z->useparent)
	  return 0;
  if (z->data) {
	  int off = z->seek + l1 * l2;
		if (z->allocsize == 0) {
			write_log (_T("zfile_fwrite(data,%s) but allocsize=0!\n"), z->name);
			return 0;
		}
		if (off > z->allocsize) {
			if (z->allocsize < off)
				z->allocsize = off;
			z->allocsize += z->size / 2;
			if (z->allocsize < 10000)
				z->allocsize = 10000;
			z->data = xrealloc (uae_u8, z->data, z->allocsize);
	    z->datasize = z->size = off;
	  }
	  memcpy (z->data + z->seek, b, l1 * l2);
	  z->seek += l1 * l2;
		if (z->seek > z->size)
			z->size = z->seek;
		if (z->size > z->datasize)
			z->datasize = z->size;
	  return l2;
  }
  return fwrite (b, l1, l2, z->f);
}

size_t zfile_fputs (struct zfile *z, TCHAR *s)
{
  return zfile_fwrite (s, strlen (s), 1, z);
}

char *zfile_fgetsa(char *s, int size, struct zfile *z)
{
	checkarchiveparent (z);
  if (z->data) {
  	char *os = s;
  	int i;
  	for (i = 0; i < size - 1; i++) {
	    if (z->seek == z->size) {
    		if (i == 0)
  		    return NULL;
    		break;
	    }
	    *s = z->data[z->seek++];
	    if (*s == '\n') {
    		s++;
    		break;
	    }
	    s++;
  	}
  	*s = 0;
  	return os;
  } else {
  	return fgets(s, size, z->f);
  }
}

TCHAR *zfile_fgets (TCHAR *s, int size, struct zfile *z)
{
  return zfile_fgetsa(s, size, z);
}

int zfile_putc (int c, struct zfile *z)
{
  uae_u8 b = (uae_u8)c;
  return zfile_fwrite (&b, 1, 1, z) ? 1 : -1;
}

int zfile_getc (struct zfile *z)
{
	checkarchiveparent (z);
  int out = -1;
  if (z->data) {
  	if (z->seek < z->size) {
	    out = z->data[z->seek++];
  	}
  } else {
  	out = fgetc (z->f);
  }
  return out;
}

int zfile_ferror (struct zfile *z)
{
  return 0;
}

uae_u8 *zfile_getdata (struct zfile *z, uae_s64 offset, int len)
{
  uae_s64 pos = zfile_ftell (z);
  uae_u8 *b;
  if (len < 0) {
  	zfile_fseek (z, 0, SEEK_END);
  	len = zfile_ftell (z);
  	zfile_fseek (z, 0, SEEK_SET);
  }
  b = xmalloc (uae_u8, len);
	zfile_fseek (z, offset, SEEK_SET);
	zfile_fread (b, len, 1, z);
	zfile_fseek (z, pos, SEEK_SET);
  return b;
}

int zfile_zuncompress (void *dst, int dstsize, struct zfile *src, int srcsize)
{
  z_stream zs;
  int v;
  uae_u8 inbuf[4096];
  int incnt;

  memset (&zs, 0, sizeof(zs));
  if (inflateInit_ (&zs, ZLIB_VERSION, sizeof(z_stream)) != Z_OK)
    return 0;
  zs.next_out = (Bytef*)dst;
  zs.avail_out = dstsize;
  incnt = 0;
  v = Z_OK;
  while (v == Z_OK && zs.avail_out > 0) {
    if (zs.avail_in == 0) {
      int left = srcsize - incnt;
      if (left == 0)
	      break;
      if (left > sizeof (inbuf)) 
        left = sizeof (inbuf);
      zs.next_in = inbuf;
      zs.avail_in = zfile_fread (inbuf, 1, left, src);
      incnt += left;
    }
    v = inflate (&zs, 0);
  }
  inflateEnd (&zs);
  return 0;
}

int zfile_zcompress (struct zfile *f, void *src, int size)
{
  int v;
  z_stream zs;
  uae_u8 outbuf[4096];

  memset (&zs, 0, sizeof (zs));
  if (deflateInit_ (&zs, Z_DEFAULT_COMPRESSION, ZLIB_VERSION, sizeof(z_stream)) != Z_OK)
    return 0;
  zs.next_in = (Bytef*)src;
  zs.avail_in = size;
  v = Z_OK;
  while (v == Z_OK) {
    zs.next_out = outbuf;
    zs.avail_out = sizeof (outbuf);
    v = deflate(&zs, Z_NO_FLUSH | Z_FINISH);
    if (sizeof(outbuf) - zs.avail_out > 0)
      zfile_fwrite (outbuf, 1, sizeof (outbuf) - zs.avail_out, f);
  }
  deflateEnd(&zs);
  return zs.total_out;
}

TCHAR *zfile_getname (struct zfile *f)
{
  return f ? f->name : NULL;
}

TCHAR *zfile_getfilename (struct zfile *f)
{
  int i;
  if (f->name == NULL)
  	return NULL;
  for (i = _tcslen (f->name) - 1; i >= 0; i--) {
    if (f->name[i] == '\\' || f->name[i] == '/' || f->name[i] == ':') {
	    i++;
	    return &f->name[i];
  	}
  }
  return f->name;
}

uae_u32 zfile_crc32 (struct zfile *f)
{
  uae_u8 *p;
  int pos, size;
  uae_u32 crc;

  if (!f)
  	return 0;
  if (f->data)
  	return get_crc32 (f->data, f->size);
  pos = zfile_ftell (f);
  zfile_fseek (f, 0, SEEK_END);
  size = zfile_ftell (f);
  p = xmalloc (uae_u8, size);
  if (!p)
  	return 0;
  memset (p, 0, size);
  zfile_fseek (f, 0, SEEK_SET);
  zfile_fread (p, 1, size, f);
  zfile_fseek (f, pos, SEEK_SET);        
  crc = get_crc32 (p, size);
  xfree (p);
  return crc;
}

static struct zvolume *zvolume_list;

static void recurparent (TCHAR *newpath, struct znode *zn, int recurse)
{
  if (zn->parent && (&zn->volume->root != zn->parent || zn->volume->parentz == NULL)) {
  	if (&zn->volume->root == zn->parent && zn->volume->parentz == NULL && !_tcscmp (zn->name, zn->parent->name))
	    goto end;
  	recurparent (newpath, zn->parent, recurse);
  } else {
  	struct zvolume *zv = zn->volume;
  	if (zv->parentz && recurse)
	    recurparent (newpath, zv->parentz, recurse);
  }
end:
  if (newpath[0])
  	_tcscat (newpath, FSDB_DIR_SEPARATOR_S);
  _tcscat (newpath, zn->name);
}

static struct znode *znode_alloc(struct znode *parent, const TCHAR *name)
{
  TCHAR fullpath[MAX_DPATH];
  TCHAR tmpname[MAX_DPATH];
  struct znode *zn = xcalloc (struct znode, 1);
  struct znode *zn2;

  _tcscpy (tmpname, name);
  zn2 = parent->child;
  while (zn2) {
  	if (!_tcscmp (zn2->name, tmpname)) {
	    TCHAR *ext = _tcsrchr (tmpname, '.');
	    if (ext && ext > tmpname + 2 && ext[-2] == '.') {
    		ext[-1]++;
	    } else if (ext) {
		    memmove (ext + 2, ext, (_tcslen (ext) + 1) * sizeof (TCHAR));
		    ext[0] = '.';
		    ext[1] = '1';
	    } else {
		    int len = _tcslen (tmpname);
		    tmpname[len] = '.';
		    tmpname[len + 1] = '1';
		    tmpname[len + 2] = 0;
	    }
	    zn2 = parent->child;
	    continue;
  	}
  	zn2 = zn2->sibling;
  }

  fullpath[0] = 0;
  recurparent (fullpath, parent, FALSE);
  _tcscat (fullpath, FSDB_DIR_SEPARATOR_S);
  _tcscat (fullpath, tmpname);
  zn->fullname = my_strdup(fullpath);
  zn->name = my_strdup(tmpname);
  zn->volume = parent->volume;
  zn->volume->last->next = zn;
  zn->prev = zn->volume->last;
  zn->volume->last = zn;
  return zn;
}

static struct znode *znode_alloc_child(struct znode *parent, const TCHAR *name)
{
  struct znode *zn = znode_alloc(parent, name);

  if (!parent->child) {
  	parent->child = zn;
  } else {
    struct znode *pn = parent->child;
    while (pn->sibling)
	    pn = pn->sibling;
  	pn->sibling = zn;
  }
  zn->parent = parent;
  return zn;
}
static struct znode *znode_alloc_sibling(struct znode *sibling, const TCHAR *name)
{
  struct znode *zn = znode_alloc(sibling->parent, name);

  if (!sibling->sibling) {
  	sibling->sibling = zn;
  } else {
    struct znode *pn = sibling->sibling;
    while (pn->sibling)
	    pn = pn->sibling;
  	pn->sibling = zn;
  }
  zn->parent = sibling->parent;
  return zn;
}

static void zvolume_addtolist(struct zvolume *zv)
{
  if (!zv)
  	return;
  if (!zvolume_list) {
    zvolume_list = zv;
  } else {
    struct zvolume *v = zvolume_list;
    while (v->next)
	    v = v->next;
  	v->next = zv;
  }
}

static struct zvolume *zvolume_alloc_2 (const TCHAR *name, struct zfile *z, unsigned int id, void *handle, const TCHAR *volname)
{
  struct zvolume *zv = xcalloc (struct zvolume, 1);
  struct znode *root;
  uae_s64 pos;
  int i;

  root = &zv->root;
  zv->last = root;
  zv->archive = z;
  zv->handle = handle;
  zv->id = id;
  zv->blocks = 4;
  if (z)
  	zv->zfdmask = z->zfdmask;
  root->volume = zv;
  root->type = ZNODE_DIR;
  i = 0;
  if (name[0] != '/' && name[0] != '\\' && _tcsncmp (name, _T(".\\"), 2) != 0) {
  	if (_tcschr (name, ':') == 0) {
	    for (i = _tcslen (name) - 1; i > 0; i--) {
    		if (name[i] == FSDB_DIR_SEPARATOR) {
  		    i++;
  		    break;
    		}
	    }
  	}
  }
  root->name = my_strdup (name + i);
  root->fullname = my_strdup(name);
  if (volname)
  	zv->volumename = my_strdup (volname);
  if (z) {
  	pos = zfile_ftell(z);
  	zfile_fseek(z, 0, SEEK_END);
  	zv->archivesize = zfile_ftell(z);
  	zfile_fseek(z, pos, SEEK_SET);
  }
  return zv;
}
struct zvolume *zvolume_alloc (struct zfile *z, unsigned int id, void *handle, const TCHAR *volumename)
{
  return zvolume_alloc_2 (zfile_getname (z), z, id, handle, volumename);
}
struct zvolume *zvolume_alloc_nofile (const TCHAR *name, unsigned int id, void *handle, const TCHAR *volumename)
{
  return zvolume_alloc_2 (name, NULL, id, handle, volumename);
}
struct zvolume *zvolume_alloc_empty (struct zvolume *prev, const TCHAR *name)
{
  struct zvolume *zv = zvolume_alloc_2(name, 0, 0, 0, NULL);
  if (!zv)
  	return NULL;
  if (prev)
  	zv->zfdmask = prev->zfdmask;
  return zv;
}

static struct zvolume *get_zvolume(const TCHAR *path)
{
  struct zvolume *zv = zvolume_list;
  while (zv) {
  	TCHAR *s = zfile_getname (zv->archive);
  	if (!s)
	    s = zv->root.name;
  	if (_tcslen (path) >= _tcslen (s) && !memcmp (path, s, _tcslen (s) * sizeof (TCHAR)))
	    return zv;
   	zv = zv->next;
  }
  return NULL;
}

static struct zvolume *zfile_fopen_archive_ext (struct znode *parent, struct zfile *zf, int flags)
{
  struct zvolume *zv = NULL;
  TCHAR *name = zfile_getname (zf);
  TCHAR *ext;
  uae_u8 header[7];

  if (!name)
  	return NULL;

  memset (header, 0, sizeof (header));
  zfile_fseek (zf, 0, SEEK_SET);
  zfile_fread (header, sizeof (header), 1, zf);
  zfile_fseek (zf, 0, SEEK_SET);

  ext = _tcsrchr (name, '.');
  if (ext != NULL) {
  	ext++;
  	if (flags & ZFD_ARCHIVE) {
			if (strcasecmp (ext, _T("lha")) == 0 || strcasecmp (ext, _T("lzh")) == 0)
        zv = archive_directory_lha (zf);
			if (strcasecmp (ext, _T("zip")) == 0)
	      zv = archive_directory_zip (zf);
			if (strcasecmp (ext, _T("7z")) == 0)
	      zv = archive_directory_7z (zf);
			if (strcasecmp (ext, _T("lzx")) == 0)
	      zv = archive_directory_lzx (zf);
    }
    if (flags & ZFD_ADF) {
			if (strcasecmp (ext, _T("adf")) == 0 && !memcmp (header, "DOS", 3))
	      zv = archive_directory_adf (parent, zf);
    }
		if (flags & ZFD_HD) {
	    if (strcasecmp (ext, _T("hdf")) == 0)  {
  		  zv = archive_directory_adf (parent, zf);
	    }
    }
  }
  return zv;
}


static struct zvolume *zfile_fopen_archive_data (struct znode *parent, struct zfile *zf, int flags)
{
  struct zvolume *zv = NULL;
  uae_u8 header[32];

  memset (header, 0, sizeof (header));
  zfile_fread (header, sizeof (header), 1, zf);
  zfile_fseek (zf, 0, SEEK_SET);
	if (flags & ZFD_ARCHIVE) {
    if (header[0] == 'P' && header[1] == 'K')
      zv = archive_directory_zip (zf);
    if (header[0] == 'L' && header[1] == 'Z' && header[2] == 'X')
      zv = archive_directory_lzx (zf);
    if (header[2] == '-' && header[3] == 'l' && header[4] == 'h' && header[6] == '-')
      zv = archive_directory_lha (zf);
	}
	if (flags & ZFD_ADF) {
    if (header[0] == 'D' && header[1] == 'O' && header[2] == 'S' && (header[3] >= 0 && header[3] <= 7))
     zv = archive_directory_adf (parent, zf);
  }
  return zv;
}

static struct znode *get_znode (struct zvolume *zv, const TCHAR *ppath, int);

static void zfile_fopen_archive_recurse2 (struct zvolume *zv, struct znode *zn, int flags)
{
  struct zvolume *zvnew;
  struct znode *zndir;
  TCHAR tmp[MAX_DPATH];

	_stprintf (tmp, _T("%s.DIR"), zn->fullname + _tcslen (zv->root.name) + 1);
  zndir = get_znode(zv, tmp, TRUE);
  if (!zndir) {
  	struct zarchive_info zai = { 0 };
  	zvnew = zvolume_alloc_empty (zv, tmp); 
    zvnew->parentz = zn;
  	zai.name = tmp;
  	zai.t = zn->mtime;
  	zai.comment = zv->volumename;
  	if (zn->flags < 0)
	    zai.flags = zn->flags;
  	zndir = zvolume_adddir_abs(zv, &zai);
    zndir->type = ZNODE_VDIR;
  	zndir->vfile = zn;
  	zndir->vchild = zvnew;
  	zvnew->parent = zv;
  	zndir->offset = zn->offset;
  	zndir->offset2 = zn->offset2;
  }
}

static int zfile_fopen_archive_recurse (struct zvolume *zv, int flags)
{
  struct znode *zn;
  int i, added;

  added = 0;
  zn = zv->root.child;
  while (zn) {
	  int done = 0;
	  struct zfile *z;
	  TCHAR *ext = _tcsrchr (zn->name, '.');
	  if (ext && !zn->vchild && zn->type == ZNODE_FILE) {
			for (i = 0; !done && archive_extensions[i]; i++) {
				if (!strcasecmp (ext + 1, archive_extensions[i])) {
		      zfile_fopen_archive_recurse2 (zv, zn, flags);
					done = 1;
    		}
      }
	  }
		if (!done) {
	    z = archive_getzfile (zn, zv->method, 0);
	    if (z && iszip (z))
	      zfile_fopen_archive_recurse2 (zv, zn, flags);
		}
  	zn = zn->next;
  }
  return 0;
}


static struct zvolume *prepare_recursive_volume (struct zvolume *zv, const TCHAR *path, int flags)
{
  struct zfile *zf = NULL;
  struct zvolume *zvnew = NULL;
	int done = 0;

  zf = (struct zfile *) zfile_open_archive (path, 0);
  if (!zf)
  	goto end;
  zvnew = zfile_fopen_archive_ext (zv->parentz, zf, flags);
  if (!zvnew && !(flags & ZFD_NORECURSE)) {
		zvnew = archive_directory_plain (zf);
		if (zvnew) {
			zfile_fopen_archive_recurse (zvnew, flags);
		  done = 1;
		}
	} else if (zvnew) {
    zvnew->parent = zv->parent;
		zfile_fopen_archive_recurse (zvnew, flags);
		done = 1;
	}
	if (!done)
		goto end;
  zfile_fclose_archive(zv);
  return zvnew;
end:
	write_log (_T("unpack '%s' failed\n"), path);
  zfile_fclose_archive (zvnew);
  zfile_fclose(zf);
  return NULL;
}

static struct znode *get_znode (struct zvolume *zv, const TCHAR *ppath, int recurse)
{
  struct znode *zn;
  TCHAR path[MAX_DPATH], zpath[MAX_DPATH];

  if (!zv)
  	return NULL;
  _tcscpy (path, ppath);
  zn = &zv->root;
  while (zn) {
  	zpath[0] = 0;
  	recurparent (zpath, zn, recurse);
  	if (zn->type == ZNODE_FILE) {
	    if (!_tcsicmp (zpath, path))
    		return zn;
  	} else {
	    int len = _tcslen (zpath);
	    if (_tcslen (path) >= len && (path[len] == 0 || path[len] == FSDB_DIR_SEPARATOR) && !_tcsnicmp (zpath, path, len)) {
    		if (path[len] == 0)
  		    return zn;
    		if (zn->vchild) {
  		    /* jump to separate tree, recursive archives */
  		    struct zvolume *zvdeep = zn->vchild;
  		    if (zvdeep->archive == NULL) {
      			TCHAR newpath[MAX_DPATH];
      			newpath[0] = 0;
      			recurparent (newpath, zn, recurse);
      			zvdeep = prepare_recursive_volume (zvdeep, newpath, ZFD_ALL);
      			if (!zvdeep) {
							write_log (_T("failed to unpack '%s'\n"), newpath);
    			    return NULL;
      			}
      			/* replace dummy empty volume with real volume */
      			zn->vchild = zvdeep;
      			zvdeep->parentz = zn;
  		    }
  		    zn = zvdeep->root.child;
	    	} else {
	  	    zn = zn->child;
	    	}
	    	continue;
	    }
	  }
	  zn = zn->sibling;
  }
  return NULL;
}

static void addvolumesize (struct zvolume *zv, uae_s64 size)
{
  unsigned int blocks = (size + 511) / 512;

  if (blocks == 0)
  	blocks++;
  while (zv) {
  	zv->blocks += blocks;
  	zv->size += size;
  	zv = zv->parent;
  }
}

struct znode *znode_adddir(struct znode *parent, const TCHAR *name, struct zarchive_info *zai)
{
  struct znode *zn;
  TCHAR path[MAX_DPATH];
  
  path[0] = 0;
  recurparent (path, parent, FALSE);
	_tcscat (path, FSDB_DIR_SEPARATOR_S);
  _tcscat (path, name);
  zn = get_znode (parent->volume, path, FALSE);
  if (zn)
  	return zn;
  zn = znode_alloc_child(parent, name);
  zn->mtime = zai->t;
  zn->type = ZNODE_DIR;
  if (zai->comment)
  	zn->comment = my_strdup (zai->comment);
  if (zai->flags < 0)
  	zn->flags = zai->flags;
  addvolumesize(parent->volume, 0);
  return zn;
}

struct znode *zvolume_adddir_abs(struct zvolume *zv, struct zarchive_info *zai)
{
  struct znode *zn2;
  TCHAR *path = my_strdup(zai->name);
  TCHAR *p, *p2;
  int i;

  if (_tcslen (path) > 0) {
  	/* remove possible trailing / or \ */
  	TCHAR last;
  	last = path[_tcslen (path) - 1];
  	if (last == '/' || last == '\\')
	    path[_tcslen (path) - 1] = 0;
  }
  zn2 = &zv->root;
  p = p2 = path;
  for (i = 0; path[i]; i++) {
  	if (path[i] == '/' || path[i] == '\\') {
	    path[i] = 0;
	    zn2 = znode_adddir(zn2, p, zai);
	    path[i] = FSDB_DIR_SEPARATOR;
	    p = p2 = &path[i + 1];
  	}
  }
  return znode_adddir(zn2, p, zai);
}

struct znode *zvolume_addfile_abs(struct zvolume *zv, struct zarchive_info *zai)
{
  struct znode *zn, *zn2;
  int i;
  TCHAR *path = my_strdup (zai->name);
  TCHAR *p, *p2;

  zn2 = &zv->root;
  p = p2 = path;
  for (i = 0; path[i]; i++) {
  	if (path[i] == '/' || path[i] == '\\') {
	    path[i] = 0;
	    zn2 = znode_adddir(zn2, p, zai);
	    path[i] = FSDB_DIR_SEPARATOR;
	    p = p2 = &path[i + 1];
  	}
  }
  if (p2) {
  	zn = znode_alloc_child(zn2, p2);
  	zn->size = zai->size;
  	zn->type = ZNODE_FILE;
  	zn->mtime = zai->t;
  	if (zai->comment)
	    zn->comment = my_strdup(zai->comment);
  	zn->flags = zai->flags;
  	addvolumesize(zn->volume, zai->size);
  }
  xfree(path);
  return zn;
}

struct zvolume *zfile_fopen_archive (const TCHAR *filename, int flags)
{
  struct zvolume *zv = NULL;
  struct zfile *zf = zfile_fopen_nozip (filename, _T("rb"));

  if (!zf)
  	return NULL;
  zf->zfdmask = flags;
  zv = zfile_fopen_archive_ext (NULL, zf, flags);
  if (!zv)
  	zv = zfile_fopen_archive_data (NULL, zf, flags);
#if 0
  if (!zv) {
  	struct zfile *zf2 = zuncompress (zf, 0, 0);
  	if (zf2 != zf) {
	    zf = zf2;
      zv = zfile_fopen_archive_ext(zf, flags);
      if (!zv)
    	  zv = zfile_fopen_archive_data(zf, flags);
  	}
  }
#endif
  /* pointless but who cares? */
  if (!zv && !(flags & ZFD_NORECURSE))
  	zv = archive_directory_plain (zf);

#if RECURSIVE_ARCHIVES
  if (zv && !(flags & ZFD_NORECURSE))
		zfile_fopen_archive_recurse (zv, flags);
#endif

  if (zv)
  	zvolume_addtolist (zv);
  else
  	zfile_fclose(zf);
  return zv;
}
struct zvolume *zfile_fopen_archive (const TCHAR *filename)
{
	return zfile_fopen_archive (filename, ZFD_ALL);
}

void zfile_fclose_archive(struct zvolume *zv)
{
  struct znode *zn;
  struct zvolume *v;

  if (!zv)
  	return;
  zn = &zv->root;
  while (zn) {
  	struct znode *zn2 = zn->next;
  	if (zn->vchild)
	    zfile_fclose_archive(zn->vchild);
  	xfree(zn->comment);
  	xfree(zn->fullname);
  	xfree(zn->name);
  	zfile_fclose(zn->f);
  	memset (zn, 0, sizeof (struct znode));
  	if (zn != &zv->root)
	    xfree(zn);
  	zn = zn2;
  }
  archive_access_close (zv->handle, zv->id);
  if (zvolume_list == zv) {
  	zvolume_list = zvolume_list->next;
  } else {
  	v = zvolume_list;
  	while (v) {
	    if (v->next == zv) {
    		v->next = zv->next;
    		break;
	    }
	    v = v->next;
  	}
  }
  xfree(zv);
}

struct zdirectory {
	TCHAR *parentpath;
  struct znode *first;
  struct znode *n;
	bool doclose;
	struct zvolume *zv;
	int cnt;
	int offset;
	TCHAR **filenames;
};

struct zdirectory *zfile_opendir_archive (const TCHAR *path, int flags)
{
  struct zvolume *zv = get_zvolume(path);
	bool created = false;
	if (zv == NULL) {
		zv = zfile_fopen_archive (path, flags);
		created = true;
	}
  struct znode *zn = get_znode(zv, path, TRUE);
  struct zdirectory *zd;

	if (!zn || (!zn->child && !zn->vchild)) {
		if (created)
			zfile_fclose_archive (zv);
  	return NULL;
	}
	zd = xcalloc (struct zdirectory, 1);
	if (created)
		zd->zv = zv;
  if (zn->child) {
  	zd->n = zn->child;
  } else {
  	if (zn->vchild->archive == NULL) {
			struct zvolume *zvnew = prepare_recursive_volume (zn->vchild, path, flags);
	    if (zvnew) {
    		zn->vchild = zvnew;
    		zvnew->parentz = zn;
	    }
  	}
  	zd->n = zn->vchild->root.next;
  }
	zd->parentpath = my_strdup (path);
  zd->first = zd->n;
  return zd;
}
struct zdirectory *zfile_opendir_archive (const TCHAR *path)
{
	return zfile_opendir_archive (path, ZFD_ALL | ZFD_NORECURSE);
}
void zfile_closedir_archive(struct zdirectory *zd)
{
	if (!zd)
		return;
	zfile_fclose_archive (zd->zv);
	xfree (zd->parentpath);
	xfree (zd->filenames);
    xfree(zd);
}
int zfile_readdir_archive (struct zdirectory *zd, TCHAR *out, bool fullpath)
{
	if (out)
		out[0] = 0;
	if (!zd->n || (zd->filenames != NULL && zd->offset >= zd->cnt))
	return 0;
	if (zd->filenames == NULL) {
		struct znode *n = zd->first;
		int cnt = 0, len = 0;
		while (n) {
			cnt++;
			n = n->sibling;
		}
		n = zd->first;
		uae_u8 *buf = xmalloc (uae_u8, cnt * sizeof (TCHAR*));
		zd->filenames = (TCHAR**)buf;
		buf += cnt * sizeof (TCHAR*);
		for (int i = 0; i < cnt; i++) {
			zd->filenames[i] = n->name;
			n = n->sibling;
		}
		for (int i = 0; i < cnt; i++) {
			for (int j = i + 1; j < cnt; j++) {
				if (_tcscmp (zd->filenames[i], zd->filenames[j]) > 0) {
					TCHAR *tmp = zd->filenames[i];
					zd->filenames[i] = zd->filenames[j];
					zd->filenames[j] = tmp;
				}
			}
		}
		zd->cnt = cnt;
	}
	if (out == NULL)
		return zd->cnt;
	if (fullpath) {
		_tcscpy (out, zd->parentpath);
		_tcscat (out, _T("\\"));
	}
	_tcscat (out, zd->filenames[zd->offset]);
	zd->offset++;
    return 1;
}
int zfile_readdir_archive (struct zdirectory *zd, TCHAR *out)
{
	return zfile_readdir_archive (zd, out, false);
}
void zfile_resetdir_archive (struct zdirectory *zd)
{
	zd->offset = 0;
    zd->n = zd->first;
}

int zfile_fill_file_attrs_archive(const TCHAR *path, int *isdir, int *flags, TCHAR **comment)
{
  struct zvolume *zv = get_zvolume(path);
  struct znode *zn = get_znode (zv, path, TRUE);

  *isdir = 0;
  *flags = 0;
  if (comment)
    *comment = 0;
  if (!zn)
  	return 0;
  if (zn->type == ZNODE_DIR)
  	*isdir = 1;
  else if (zn->type == ZNODE_VDIR)
  	*isdir = -1;
  *flags = zn->flags;
  if (zn->comment && comment)
  	*comment = my_strdup(zn->comment);
  return 1;
}

int zfile_fs_usage_archive(const TCHAR *path, const TCHAR *disk, struct fs_usage *fsp)
{
  struct zvolume *zv = get_zvolume(path);

  if (!zv)
  	return -1;
  fsp->fsu_blocks = zv->blocks;
  fsp->fsu_bavail = 0;
  return 0;
}

int zfile_stat_archive (const TCHAR *path, struct _stat64 *s)
{
  struct zvolume *zv = get_zvolume(path);
  struct znode *zn = get_znode (zv, path, TRUE);

  memset (s, 0, sizeof (struct _stat64));
  if (!zn)
  	return 0;
  s->st_mode = zn->type == ZNODE_FILE ? 0 : FILEFLAG_DIR;
  s->st_size = zn->size;
  s->st_mtime = zn->mtime;
  return 1;
}

uae_s64 zfile_lseek_archive (struct zfile *d, uae_s64 offset, int whence)
{
	uae_s64 old = zfile_ftell (d);
  if (old < 0 || zfile_fseek (d, offset, whence))
  	return -1;
  return old;
}
uae_s64 zfile_fsize_archive (struct zfile *d)
{
	return zfile_size (d);
}

unsigned int zfile_read_archive (struct zfile *d, void *b, unsigned int size)
{
  return zfile_fread (b, 1, size, d);
}

void zfile_close_archive (struct zfile *d)
{
  /* do nothing, keep file cached */
}

struct zfile *zfile_open_archive (const TCHAR *path, int flags)
{
  struct zvolume *zv = get_zvolume(path);
  struct znode *zn = get_znode (zv, path, TRUE);
  struct zfile *z;

  if (!zn)
  	return 0;
  if (zn->f) {
  	zfile_fseek(zn->f, 0, SEEK_SET);
  	return zn->f;
  }
  if (zn->vfile)
  	zn = zn->vfile;
  z = archive_getzfile (zn, zn->volume->id, 0);
  if (z)
  	zfile_fseek(z, 0, SEEK_SET);
  zn->f = z;
  return zn->f;
}

int zfile_exists_archive (const TCHAR *path, const TCHAR *rel)
{
  TCHAR tmp[MAX_DPATH];
  struct zvolume *zv;
  struct znode *zn;
    
	_stprintf (tmp, _T("%s%c%s"), path, FSDB_DIR_SEPARATOR, rel);
  zv = get_zvolume(tmp);
  zn = get_znode (zv, tmp, TRUE);
  return zn ? 1 : 0;
}

int zfile_convertimage (const TCHAR *src, const TCHAR *dst)
{
  struct zfile *s, *d;
  int ret = 0;

	s = zfile_fopen (src, _T("rb"), ZFD_NORMAL);
  if (s) {
  	uae_u8 *b;
  	int size;
  	zfile_fseek (s, 0, SEEK_END);
  	size = zfile_ftell (s);
  	zfile_fseek (s, 0, SEEK_SET);
  	b = xcalloc (uae_u8, size);
  	if (b) {
	    if (zfile_fread (b, size, 1, s) == 1) {
				d = zfile_fopen (dst, _T("wb"), 0);
    		if (d) {
  		    if (zfile_fwrite (b, size, 1, d) == 1)
      			ret = 1;
  		    zfile_fclose (d);
    		}
	    }
	    xfree (b);
  	}
  	zfile_fclose (s);
  }
  return ret;
}
