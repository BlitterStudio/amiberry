 /*
  * UAE - The Un*x Amiga Emulator
  *
  * transparent archive handling
  *
  *     2007 Toni Wilen
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "zfile.h"
#include "archivers/zip/unzip.h"
#include "archivers/dms/pfile.h"
#include "crc32.h"
#include "zarchive.h"
#include "disk.h"

#include <zlib.h>

static time_t fromdostime(uae_u32 dd)
{
  struct tm tm;
  time_t t;
  memset(&tm, 0, sizeof tm);
  tm.tm_hour = (dd >> 11) & 0x1f;
  tm.tm_min  = (dd >> 5) & 0x3f;
  tm.tm_sec  = ((dd >> 0) & 0x1f) * 2;
  tm.tm_year = ((dd >> 25) & 0x7f) + 80;
  tm.tm_mon  = ((dd >> 21) & 0x0f) - 1;
  tm.tm_mday = (dd >> 16) & 0x1f;
  t = mktime(&tm);
  _tzset();
  t -= _timezone;
  return t;
}

static struct zvolume *getzvolume (struct znode *parent, struct zfile *zf, unsigned int id)
{
  struct zvolume *zv = NULL;

  switch (id)
  {
	case ArchiveFormatZIP:
  	zv = archive_directory_zip (zf);
  	break;
	case ArchiveFormat7Zip:
  	zv = archive_directory_7z (zf);
  	break;
	case ArchiveFormatLHA:
  	zv = archive_directory_lha (zf);
  	break;
	case ArchiveFormatLZX:
  	zv = archive_directory_lzx (zf);
  	break;
	case ArchiveFormatPLAIN:
  	zv = archive_directory_plain (zf);
  	break;
	case ArchiveFormatADF:
  	zv = archive_directory_adf (parent, zf);
  	break;
  }
  return zv;
}

struct zfile *archive_access_select (struct znode *parent, struct zfile *zf, unsigned int id, int dodefault, int *retcode, int index)
{
  struct zvolume *zv;
  struct znode *zn;
  int zipcnt, first, select;
  TCHAR tmphist[MAX_DPATH];
  struct zfile *z = NULL;
  int we_have_file;
	int diskimg;
	int mask = zf->zfdmask;
	int canhistory = (mask & ZFD_DISKHISTORY) && !(mask & ZFD_CHECKONLY);
	int getflag = (mask &  ZFD_DELAYEDOPEN) ? FILE_DELAYEDOPEN : 0;

  if (retcode)
  	*retcode = 0;
  if (index > 0)
	  return NULL;
	if (zfile_needwrite (zf)) {
		if (retcode)
			*retcode = -1;
		return NULL;
	}
  zv = getzvolume (parent, zf, id);
  if (!zv)
  	return NULL;
  we_have_file = 0;
  tmphist[0] = 0;
  zipcnt = 1;
  first = 1;
  zn = &zv->root;
  while (zn) {
		int isok = 1;
	
		diskimg = -1;
  	if (zn->type != ZNODE_FILE)
	    isok = 0;
  	if (zfile_is_ignore_ext(zn->fullname))
	    isok = 0;
		diskimg = zfile_is_diskimage (zn->fullname);
  	if (isok) {
	    if (tmphist[0]) {
    		if (diskimg >= 0&& canhistory)
	        DISK_history_add (tmphist, -1, diskimg, 1);
        tmphist[0] = 0;
	      first = 0;
	    }
	    if (first) {
				if (diskimg >= 0)
  		    _tcscpy (tmphist, zn->fullname);
	    } else {
       _tcscpy (tmphist, zn->fullname);
    		if (diskimg >= 0&& canhistory)
	        DISK_history_add (tmphist, -1, diskimg, 1);
        tmphist[0] = 0;
	    }
	    select = 0;
	    if (!zf->zipname)
        select = 1;
	    if (zf->zipname && _tcslen (zn->fullname) >= _tcslen (zf->zipname) && !strcasecmp (zf->zipname, zn->fullname + _tcslen (zn->fullname) - _tcslen (zf->zipname)))
        select = -1;
	    if (zf->zipname && zf->zipname[0] == '#' && _tstol (zf->zipname + 1) == zipcnt)
        select = -1;
	    if (select && we_have_file < 10) {
				struct zfile *zt = NULL;
				TCHAR *ext = _tcsrchr (zn->fullname, '.');
				int whf = 1;
				int ft = 0;
				if (mask & ZFD_CD) {
					if (ext && !_tcsicmp (ext, _T(".iso"))) {
						whf = 2;
						ft = ZFILE_CDIMAGE;
					}
					if (ext && !_tcsicmp (ext, _T(".ccd"))) {
						whf = 9;
						ft = ZFILE_CDIMAGE;
					}
					if (ext && !_tcsicmp (ext, _T(".cue"))) {
						whf = 10;
						ft = ZFILE_CDIMAGE;
					}
				} else {
					zt = archive_getzfile (zn, id, getflag);
					ft = zfile_gettype (zt);
				}
				if ((select < 0 || ft) && whf > we_have_file) {
				if (!zt)
					zt = archive_getzfile (zn, id, getflag);
					we_have_file = whf;
					if (z)
						zfile_fclose (z);
					z = zt;
					zt = NULL;
				}
				zfile_fclose (zt);
	    }
  	}
  	zipcnt++;
  	zn = zn->next;
  }
	diskimg = zfile_is_diskimage (zfile_getname (zf));
	if (diskimg >= 0 && first && tmphist[0] && canhistory)
		DISK_history_add (zfile_getname (zf), -1, diskimg, 1);
  zfile_fclose_archive (zv);
  if (z) {
  	zfile_fclose(zf);
  	zf = z;
  } else if (!dodefault && zf->zipname && zf->zipname[0]) {
  	if (retcode)
	    *retcode = -1;
  	zf = NULL;
	} else {
  	zf = NULL;
  }
  return zf;
}

void archive_access_scan (struct zfile *zf, zfile_callback zc, void *user, unsigned int id)
{
  struct zvolume *zv;
  struct znode *zn;

  zv = getzvolume (NULL, zf, id);
  if (!zv)
  	return;
  zn = &zv->root;
  while (zn) {
  	if (zn->type == ZNODE_FILE) {
	    struct zfile *zf2 = archive_getzfile (zn, id, 0);
	    if (zf2) {
    		int ztype = iszip (zf2);
    		if (ztype) {
  		    zfile_fclose (zf2);
    		} else {
    		  int ret = zc (zf2, user);
    		  zfile_fclose(zf2);
    		  if (ret)
  		      break;
    	  }
      }
  	}
  	zn = zn->next;
  }
  zfile_fclose_archive (zv);
}

/* ZIP */

static void archive_close_zip (void *handle)
{
}

struct zvolume *archive_directory_zip (struct zfile *z)
{
  unzFile uz;
  unz_file_info file_info;
  struct zvolume *zv;
  int err;

  uz = unzOpen (z);
  if (!uz)
  	return 0;
  if (unzGoToFirstFile (uz) != UNZ_OK)
  	return 0;
  zv = zvolume_alloc(z, ArchiveFormatZIP, NULL, NULL);
  for (;;) {
    char filename_inzip[MAX_DPATH];
  	TCHAR c;
  	struct zarchive_info zai;
  	time_t t;
  	unsigned int dd;
  	err = unzGetCurrentFileInfo (uz, &file_info, filename_inzip, sizeof (filename_inzip), NULL, 0, NULL, 0);
  	if (err != UNZ_OK)
	    return 0;
	  dd = file_info.dosDate;
  	t = fromdostime(dd);
	  memset(&zai, 0, sizeof zai);
	  zai.name = filename_inzip;
	  zai.t = t;
	  zai.flags = -1;
	  c = filename_inzip[_tcslen(filename_inzip) - 1];
	  if (c != '/' && c != '\\') {
	    int err = unzOpenCurrentFile (uz);
	    if (err == UNZ_OK) {
	    	struct znode *zn;
	    	zai.size = file_info.uncompressed_size;
	    	zn = zvolume_addfile_abs(zv, &zai);
      }
	  } else {
	    filename_inzip[_tcslen (filename_inzip) - 1] = 0;
	    zvolume_adddir_abs(zv, &zai);
	  }
  	err = unzGoToNextFile (uz);
	  if (err != UNZ_OK)
	    break;
  }
  unzClose (uz);
  zv->method = ArchiveFormatZIP;
  return zv;
}


static struct zfile *archive_do_zip (struct znode *zn, struct zfile *z, int flags)
{
	unzFile uz;
	int i;
	TCHAR tmp[MAX_DPATH];
	TCHAR *name = z ? z->archiveparent->name : zn->volume->root.fullname;
	char *s;

	uz = unzOpen (z ? z->archiveparent : zn->volume->archive);
	if (!uz)
		return 0;
	if (z)
		_tcscpy (tmp, z->archiveparent->name);
	else
		_tcscpy (tmp, zn->fullname + _tcslen (zn->volume->root.fullname) + 1);
	if (unzGoToFirstFile (uz) != UNZ_OK)
		goto error;
	for (i = 0; tmp[i]; i++) {
		if (tmp[i] == '\\')
			tmp[i] = '/';
	}
	s = my_strdup (tmp);
	if (unzLocateFile (uz, s, 1) != UNZ_OK) {
		xfree (s);
		for (i = 0; tmp[i]; i++) {
			if (tmp[i] == '/')
				tmp[i] = '\\';
		}
		s = my_strdup (tmp);
		if (unzLocateFile (uz, s, 1) != UNZ_OK) {
			xfree (s);
			goto error;
		}
	}
	xfree (s);
	s = NULL;
	if (unzOpenCurrentFile (uz) != UNZ_OK)
		goto error;
	if (!z)
		z = zfile_fopen_empty (NULL, zn->fullname, zn->size);
	if (z) {
		int err = -1;
		if (!(flags & FILE_DELAYEDOPEN) || z->size <= PEEK_BYTES) {
			err = unzReadCurrentFile (uz, z->data, z->datasize);
		} else {
			z->archiveparent = zfile_dup (zn->volume->archive);
			if (z->archiveparent) {
				xfree (z->archiveparent->name);
				z->archiveparent->name = my_strdup (tmp);
				z->datasize = PEEK_BYTES;
				err = unzReadCurrentFile (uz, z->data, z->datasize);
			} else {
				err = unzReadCurrentFile (uz, z->data, z->datasize);
			}
		}
	}
	unzCloseCurrentFile (uz);
	unzClose (uz);
	return z;
error:
	unzClose (uz);
	return NULL;
}

static struct zfile *archive_access_zip (struct znode *zn, int flags)
{
	return archive_do_zip (zn, NULL, flags);
}
static struct zfile *archive_unpack_zip (struct zfile *zf)
{
	return archive_do_zip (NULL, zf, 0);
}
    
/* 7Z */

#include "archivers/7z/Types.h"
#include "archivers/7z/7zCrc.h"
#include "archivers/7z/Archive/7z/7zIn.h"
#include "archivers/7z/Archive/7z/7zExtract.h"
#include "archivers/7z/Archive/7z/7zAlloc.h"

static ISzAlloc allocImp;
static ISzAlloc allocTempImp;

typedef struct
{
  ISeekInStream s;
  struct zfile *zf;
} CFileInStream;

static SRes SzFileReadImp (void *object, void *buffer, size_t *size)
{
  CFileInStream *s = (CFileInStream *)object;
	struct zfile *zf = s->zf;
  *size = zfile_fread (buffer, 1, *size, zf);
  return SZ_OK;
}

static SRes SzFileSeekImp(void *object, Int64 *pos, ESzSeek origin)
{
  CFileInStream *s = (CFileInStream *)object;
	struct zfile *zf = s->zf;
  int org = 0;
  switch (origin)
  {
    case SZ_SEEK_SET: org = SEEK_SET; break;
    case SZ_SEEK_CUR: org = SEEK_CUR; break;
    case SZ_SEEK_END: org = SEEK_END; break;
  }
  zfile_fseek (zf, *pos, org);
  *pos = zfile_ftell (zf);
  return SZ_OK;
}

static void init_7z(void)
{
  static int initialized;
 
  if (initialized)
  	return;
  initialized = 1;
  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;
  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;
  CrcGenerateTable ();
  _tzset ();
}

struct SevenZContext
{
  CSzArEx db;
  CFileInStream archiveStream;
  CLookToRead lookStream;
  Byte *outBuffer;
  size_t outBufferSize;
  UInt32 blockIndex;
};

static void archive_close_7z (struct SevenZContext *ctx)
{
  SzArEx_Free (&ctx->db, &allocImp);
  allocImp.Free (&allocImp, ctx->outBuffer);
  xfree(ctx);
}

#define EPOCH_DIFF 0x019DB1DED53E8000LL /* 116444736000000000 nsecs */
#define RATE_DIFF 10000000 /* 100 nsecs */

struct zvolume *archive_directory_7z (struct zfile *z)
{
  SRes res;
  struct zvolume *zv;
  int i;
  struct SevenZContext *ctx;

  init_7z();
  ctx = xcalloc (struct SevenZContext, 1);
  ctx->blockIndex = 0xffffffff;
  ctx->archiveStream.s.Read = SzFileReadImp;
  ctx->archiveStream.s.Seek = SzFileSeekImp;
  ctx->archiveStream.zf = z;
  LookToRead_CreateVTable (&ctx->lookStream, False);
  ctx->lookStream.realStream = &ctx->archiveStream.s;
  LookToRead_Init (&ctx->lookStream);

  SzArEx_Init (&ctx->db);
  res = SzArEx_Open (&ctx->db, &ctx->lookStream.s, &allocImp, &allocTempImp);
  if (res != SZ_OK) {
  	write_log(_T("7Z: SzArchiveOpen %s returned %d\n"), zfile_getname(z), res);
  	xfree (ctx);
  	return NULL;
  }
  zv = zvolume_alloc (z, ArchiveFormat7Zip, ctx, NULL);
  for (i = 0; i < ctx->db.db.NumFiles; i++) {
  	CSzFileItem *f = ctx->db.db.Files + i;
  	TCHAR *name = f->Name;
  	struct zarchive_info zai;

  	memset(&zai, 0, sizeof zai);
  	zai.name = name;
  	zai.flags = -1;
  	zai.size = f->Size;
  	zai.t = 0;
  	if (f->MTimeDefined) {
	    uae_u64 t = (((uae_u64)f->MTime.High) << 32) | f->MTime.Low;
	    if (t >= EPOCH_DIFF) {
    		zai.t = (t - EPOCH_DIFF) / RATE_DIFF;
    		zai.t -= _timezone;
    		if (_daylight)
  		    zai.t += 1 * 60 * 60;
	    }
  	}
  	if (!f->IsDir) {
	    struct znode *zn = zvolume_addfile_abs(zv, &zai);
	    zn->offset = i;
  	}
  }
  zv->method = ArchiveFormat7Zip;
  return zv;
}

static struct zfile *archive_access_7z (struct znode *zn)
{
  SRes res;
  struct zvolume *zv = zn->volume;
  struct zfile *z = NULL;
  size_t offset;
  size_t outSizeProcessed;
  struct SevenZContext *ctx;

  ctx = (struct SevenZContext *) zv->handle;
  res = SzAr_Extract (&ctx->db, &ctx->lookStream.s, zn->offset,
    &ctx->blockIndex, &ctx->outBuffer, &ctx->outBufferSize, 
    &offset, &outSizeProcessed, 
    &allocImp, &allocTempImp);
  if (res == SZ_OK) {
    z = zfile_fopen_empty (NULL, zn->fullname, zn->size);
  	zfile_fwrite (ctx->outBuffer + offset, zn->size, 1, z);
  } else {
  	write_log(_T("7Z: SzExtract %s returned %d\n"), zn->fullname, res);
  }
  return z;
}

/* plain single file */

static struct znode *addfile (struct zvolume *zv, struct zfile *zf, const TCHAR *path, uae_u8 *data, int size)
{
  struct zarchive_info zai;
  struct znode *zn;
  struct zfile *z;

  z = zfile_fopen_empty (zf, path, size);
  zfile_fwrite(data, size, 1, z);
  memset(&zai, 0, sizeof zai);
	zai.name = my_strdup (path);
  zai.flags = -1;
  zai.size = size;
  zn = zvolume_addfile_abs(zv, &zai);
  if (zn)
    zn->f = z;
  else
    zfile_fclose(z);
  xfree (zai.name);
  return zn;
}

static uae_u8 exeheader[]={0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00};
struct zvolume *archive_directory_plain (struct zfile *z)
{
  struct zfile *zf, *zf2;
  struct zvolume *zv;
  struct znode *zn;
  struct zarchive_info zai;
  uae_u8 id[8];
	int rc, index;

  memset(&zai, 0, sizeof zai);
  zv = zvolume_alloc(z, ArchiveFormatPLAIN, NULL, NULL);
  memset(id, 0, sizeof id);
  zai.name = zfile_getfilename (z);
  zai.flags = -1;
  zfile_fseek(z, 0, SEEK_END);
  zai.size = zfile_ftell(z);
  zfile_fseek(z, 0, SEEK_SET);
  zfile_fread(id, sizeof id, 1, z);
  zfile_fseek(z, 0, SEEK_SET);
  zn = zvolume_addfile_abs(zv, &zai);
  if (!memcmp (id, exeheader, sizeof id)) {
  	char *data = xmalloc(char, 1 + _tcslen(zai.name) + 1 + 1 + 1);
  	sprintf (data, "\"%s\"\n", zai.name);
  	zn = addfile (zv, z, _T("s/startup-sequence"), (uae_u8*)data, strlen (data));
  	xfree(data);
  }
	index = 0;
	for (;;) {
    zf = zfile_dup (z);
		if (!zf)
			break;
		zf2 = zuncompress (NULL, zf, 0, ZFD_ALL & ~ZFD_ADF, &rc, index);
    if (zf2) {
    	zf = NULL;
    	zai.name = zfile_getfilename (zf2);
    	zai.flags = -1;
    	zfile_fseek (zf2, 0, SEEK_END);
    	zai.size = zfile_ftell (zf2);
    	zfile_fseek (zf2, 0, SEEK_SET);
    	zn = zvolume_addfile_abs (zv, &zai);
			zn->f = zf2;
//			if (zn)
//				zn->offset = index + 1;
//			zfile_fclose (zf2);
		} else {
			if (rc == 0) {
				zfile_fclose (zf);
				break;
			}
    }
		index++;
    zfile_fclose (zf);
	}
  return zv;
}

static struct zfile *archive_access_plain (struct znode *zn)
{
  struct zfile *z;

  if (zn->offset) {
  	struct zfile *zf;
    z = zfile_fopen_empty (zn->volume->archive, zn->fullname, zn->size);
  	zf = zfile_fopen (zfile_getname (zn->volume->archive), _T("rb"), zn->volume->archive->zfdmask & ~ZFD_ADF, zn->offset - 1);
  	if (zf) {
    	zfile_fread (z->data, zn->size, 1, zf);
    	zfile_fclose (zf);
  	}
  } else {
  	z = zfile_fopen_empty (zn->volume->archive, zn->fullname, zn->size);
  	if (z) {
	    zfile_fseek (zn->volume->archive, 0, SEEK_SET);
      zfile_fread(z->data, zn->size, 1, zn->volume->archive);
  	}
  }
  return z;
}

struct adfhandle {
  int size;
  int highblock;
  int blocksize;
  int rootblock;
  struct zfile *z;
  uae_u8 block[65536];
  uae_u32 dostype;
};


static int dos_checksum (uae_u8 *p, int blocksize)
{
  uae_u32 cs = 0;
  int i;
  for (i = 0; i < blocksize; i += 4)
  	cs += (p[i] << 24) | (p[i + 1] << 16) | (p[i + 2] << 8) | (p[i + 3] << 0);
  return cs;
}
static int sfs_checksum (uae_u8 *p, int blocksize, int sfs2)
{
  uae_u32 cs = sfs2 ? 2 : 1;
  int i;
  for (i = 0; i < blocksize; i += 4)
  	cs += (p[i] << 24) | (p[i + 1] << 16) | (p[i + 2] << 8) | (p[i + 3] << 0);
  return cs;
}

static TCHAR *getBSTR (uae_u8 *bstr)
{
  int n = *bstr++;
  uae_char buf[257];
  int i;

  for (i = 0; i < n; i++)
  	buf[i] = *bstr++;
  buf[i] = 0;
  return my_strdup((char *)buf);
}

static uae_u32 gl (struct adfhandle *adf, int off)
{
  uae_u8 *p = adf->block + off;
  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}
static uae_u32 glx (uae_u8 *p)
{
  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}
static uae_u32 gwx (uae_u8 *p)
{
  return (p[0] << 8) | (p[1] << 0);
}

static const int secs_per_day = 24 * 60 * 60;
static const int diff = (8 * 365 + 2) * (24 * 60 * 60);
static const int diff2 = (-8 * 365 - 2) * (24 * 60 * 60);
static time_t put_time (long days, long mins, long ticks)
{
  time_t t;

  if (days < 0)
  	days = 0;
  if (days > 9900 * 365)
  	days = 9900 * 365; // in future far enough?
  if (mins < 0 || mins >= 24 * 60)
  	mins = 0;
  if (ticks < 0 || ticks >= 60 * 50)
  	ticks = 0;

  t = ticks / 50;
  t += mins * 60;
  t += ((uae_u64)days) * secs_per_day;
  t += diff;

  return t;
}

static int adf_read_block (struct adfhandle *adf, int block)
{
  memset (adf->block, 0, adf->blocksize);
  if (block >= adf->highblock || block < 0)
  	return 0;
  zfile_fseek (adf->z, block * adf->blocksize, SEEK_SET);
  zfile_fread (adf->block, adf->blocksize, 1, adf->z);
  return 1;
}

static void recurseadf (struct znode *zn, int root, TCHAR *name)
{
  int i;
  struct zvolume *zv = zn->volume;
  struct adfhandle *adf = (struct adfhandle *)zv->handle;
  TCHAR name2[MAX_DPATH];
  int bs = adf->blocksize;

  for (i = 0; i < bs / 4 - 56; i++) {
  	int block;
  	if (!adf_read_block (adf, root))
	    return;
  	block = gl (adf, (i + 6) * 4);
  	while (block > 0 && block < adf->size / bs) {
	    struct zarchive_info zai;
	    TCHAR *fname;
	    uae_u32 size, secondary;

	    if (!adf_read_block (adf, block))
    		return;
	    if (gl (adf, 0) != 2)
        break;
	    if (gl (adf, 1 * 4) != block)
    		break;
	    secondary = gl (adf, bs - 1 * 4);
	    if (secondary != -3 && secondary != 2)
        break;
	    memset (&zai, 0, sizeof zai);
	    fname = getBSTR (adf->block + bs - 20 * 4);
	    size = gl (adf, bs - 47 * 4);
	    name2[0] = 0;
	    if (name[0]) {
    		TCHAR sep[] = { FSDB_DIR_SEPARATOR, 0 };
    		_tcscpy (name2, name);
    		_tcscat (name2, sep);
	    }
	    _tcscat (name2, fname);
	    zai.name = name2;
	    if (size < 0 || size > 0x7fffffff)
    		size = 0;
	    zai.size = size;
	    zai.flags = gl (adf, bs - 48 * 4);
	    zai.t = put_time (gl (adf, bs - 23 * 4), gl (adf, bs - 22 * 4),gl (adf, bs - 21 * 4));
	    if (secondary == -3) {
        struct znode *znnew = zvolume_addfile_abs (zv, &zai);
        znnew->offset = block;
	    } else {
    		struct znode *znnew = zvolume_adddir_abs (zv, &zai);
    		znnew->offset = block;
    		recurseadf (znnew, block, name2);
    		if (!adf_read_block (adf, block))
  		    return;
	    }
	    xfree (fname);
	    block = gl (adf, bs - 4 * 4);
  	}
  }
}

static void recursesfs (struct znode *zn, int root, TCHAR *name, int sfs2)
{
  struct zvolume *zv = zn->volume;
  struct adfhandle *adf = (struct adfhandle *)zv->handle;
  TCHAR name2[MAX_DPATH];
  int bs = adf->blocksize;
  int block;
  uae_u8 *p, *s;
  struct zarchive_info zai;

  block = root;
  while (block) {
    if (!adf_read_block (adf, block))
	    return;
  	p = adf->block + 12 + 3 * 4;
  	while (glx (p + 4) && p < adf->block + adf->blocksize - 27) {
	    TCHAR *fname;
	    int i;
	    int align;

	    memset (&zai, 0, sizeof zai);
	    zai.flags = glx (p + 8) ^ 0x0f;
	    s = p + (sfs2 ? 27 : 25);
	    fname = my_strdup ((char *)s);
	    i = 0;
	    while (*s) {
    		s++;
    		i++;
	    }
	    s++;
	    i++;
	    if (*s)
    		zai.comment = my_strdup ((char *)s);
	    while (*s) {
    		s++;
    		i++;
	    }
	    s++;
	    i++;
	    i += sfs2 ? 27 : 25;
	    align = i & 1;

	    name2[0] = 0;
	    if (name[0]) {
    		TCHAR sep[] = { FSDB_DIR_SEPARATOR, 0 };
    		_tcscpy (name2, name);
    		_tcscat (name2, sep);
	    }
	    _tcscat (name2, fname);
	    zai.name = name2;
	    if (sfs2)
    		zai.t = glx (p + 22) - diff2;
	    else
    		zai.t = glx (p + 20) - diff;
	    if (p[sfs2 ? 26 : 24] & 0x80) { // dir
    		struct znode *znnew = zvolume_adddir_abs (zv, &zai);
    		int newblock = glx (p + 16);
    		if (newblock) {
  		    znnew->offset = block;
  		    recursesfs (znnew, newblock, name2, sfs2);
    		}
    		if (!adf_read_block (adf, block))
  		    return;
	    } else {
    		struct znode *znnew;
    		if (sfs2) {
  		    uae_u64 b1 = p[16];
  		    uae_u64 b2 = p[17];
  		    zai.size = (b1 << 40) | (b2 << 32) | glx (p + 18) ;
    		} else {
  		    zai.size = glx (p + 16);
    		}
    		znnew = zvolume_addfile_abs (zv, &zai);
    		znnew->offset = block;
    		znnew->offset2 = p - adf->block;
	    }
	    xfree (zai.comment);
	    xfree (fname);
	    p += i + align;
  	}
  	block = gl (adf, 12 + 4);
  }

}

struct zvolume *archive_directory_adf (struct znode *parent, struct zfile *z)
{
  struct zvolume *zv;
  struct adfhandle *adf;
  TCHAR *volname = NULL;
  TCHAR name[MAX_DPATH];
  int gotroot = 0;

  adf = xcalloc (struct adfhandle, 1);
  zfile_fseek (z, 0, SEEK_END);
  adf->size = zfile_ftell (z);
  zfile_fseek (z, 0, SEEK_SET);

  adf->blocksize = 512;
  if (parent && parent->offset2) {
  	if (parent->offset2 == 1024 || parent->offset2 == 2048 || parent->offset2 == 4096 || parent->offset2 == 8192 ||
	    parent->offset2 == 16384 || parent->offset2 == 32768 || parent->offset2 == 65536) {
	    adf->blocksize = parent->offset2;
	    gotroot = 1;
  	}
  }

  adf->highblock = adf->size / adf->blocksize;
  adf->z = z;

  if (!adf_read_block (adf, 0))
  	goto fail;
  adf->dostype = gl (adf, 0);

  if ((adf->dostype & 0xffffff00) == 'DOS\0') {
  	int bs = adf->blocksize;
    int res;

    adf->rootblock = ((adf->size / bs) - 1 + 2) / 2;
  	if (!gotroot) {
	    for (res = 2; res >= 1; res--) {
    		for (bs = 512; bs < 65536; bs <<= 1) {
  		    adf->blocksize = bs;
  		    adf->rootblock = ((adf->size / bs) - 1 + res) / 2;
  		    if (!adf_read_block (adf, adf->rootblock))
      			continue;
  		    if (gl (adf, 0) != 2 || gl (adf, bs - 1 * 4) != 1)
      			continue;
  		    if (dos_checksum (adf->block, bs) != 0)
      			continue;
  		    gotroot = 1;
  		    break;
  		  }
		    if (gotroot)
		      break;
	    }
	  }
  	if (!gotroot) {
	    bs = adf->blocksize = 512;
	    if (adf->size < 2000000 && adf->rootblock != 880) {
        adf->rootblock = 880;
	    	if (!adf_read_block (adf, adf->rootblock))
	  	    goto fail;
	    	if (gl (adf, 0) != 2 || gl (adf, bs - 1 * 4) != 1)
	  	    goto fail;
	    	if (dos_checksum (adf->block, bs) != 0)
	  	    goto fail;
	    }
	  }

    if (!adf_read_block (adf, adf->rootblock))
	    goto fail;
    if (gl (adf, 0) != 2 || gl (adf, bs - 1 * 4) != 1)
	    goto fail;
  	if (dos_checksum (adf->block, adf->blocksize) != 0)
	    goto fail;
  	adf->blocksize = bs;
    adf->highblock = adf->size / adf->blocksize;
  	volname = getBSTR (adf->block + adf->blocksize - 20 * 4);
  	zv = zvolume_alloc (z, ArchiveFormatADF, NULL, NULL);
  	zv->method = ArchiveFormatADF;
  	zv->handle = adf;
    
  	name[0] = 0;
  	recurseadf (&zv->root, adf->rootblock, name);

  } else if ((adf->dostype & 0xffffff00) == 'SFS\0') {

  	uae_u16 version, sfs2;

  	for (;;) {
	    for (;;) {
    		version = gl (adf, 12) >> 16;
    		sfs2 = version > 3;
    		if (version > 4)
  		    break;
    		adf->rootblock = gl (adf, 104);
    		if (!adf_read_block (adf, adf->rootblock))
  		    break;
    		if (gl (adf, 0) != 'OBJC')
  		    break;
    		if (sfs_checksum (adf->block, adf->blocksize, sfs2))
  		    break;
    		adf->rootblock = gl (adf, 40);
    		if (!adf_read_block (adf, adf->rootblock))
  		    break;
    		if (gl (adf, 0) != 'OBJC')
  		    break;
    		if (sfs_checksum (adf->block, adf->blocksize, sfs2))
  		    break;
    		gotroot = 1;
    		break;
	    }
	    if (gotroot)
    		break;
	    adf->blocksize <<= 1;
	    if (adf->blocksize == 65536)
    		break;
  	}
  	if (!gotroot)
	    goto fail;

  	zv = zvolume_alloc (z, ArchiveFormatADF, NULL, NULL);
  	zv->method = ArchiveFormatADF;
  	zv->handle = adf;

   	name[0] = 0;
  	recursesfs (&zv->root, adf->rootblock, name, version > 3);

  } else {
  	goto fail;
  }


  xfree (volname);
  return zv;
fail:
  xfree (adf);
  return NULL;
}

struct sfsblock
{
  int block;
  int length;
};

static int sfsfindblock (struct adfhandle *adf, int btree, int theblock, struct sfsblock **sfsb, int *sfsblockcnt, int *sfsmaxblockcnt, int sfs2)
{
  int nodecount, isleaf, nodesize;
  int i;
  uae_u8 *p;

  if (!btree)
  	return 0;
  if (!adf_read_block (adf, btree))
    return 0;
  if (memcmp (adf->block, "BNDC", 4))
    return 0;
  nodecount = gwx (adf->block + 12);
  isleaf = adf->block[14];
  nodesize = adf->block[15];
  p = adf->block + 16;
  for (i = 0; i < nodecount; i++) {
  	if (isleaf) {
	    uae_u32 key = glx (p);
	    uae_u32 next = glx (p + 4);
	    uae_u32 prev = glx (p + 8);
	    uae_u32 blocks;
	    if (sfs2)
    		blocks = glx (p + 12);
	    else
    		blocks = gwx (p + 12);
	    if (key == theblock) {
    		struct sfsblock *sb;
    		if (*sfsblockcnt >= *sfsmaxblockcnt) {
  		    *sfsmaxblockcnt += 100;
  				*sfsb = xrealloc (struct sfsblock, *sfsb, *sfsmaxblockcnt);
    		}
    		sb = *sfsb + (*sfsblockcnt);
    		sb->block = key;
    		sb->length = blocks;
    		(*sfsblockcnt)++;
    		return next;
	    }
  	} else {
	    uae_u32 key = glx (p);
	    uae_u32 data = glx (p + 4);
	    int newblock = sfsfindblock (adf, data, theblock, sfsb, sfsblockcnt, sfsmaxblockcnt, sfs2);
	    if (newblock)
    		return newblock;
	    if (!adf_read_block (adf, btree))
    		return 0;
	    if (memcmp (adf->block, "BNDC", 4))
    		return 0;
  	}
    p += nodesize;
  }
  return 0;
}


static struct zfile *archive_access_adf (struct znode *zn)
{
  struct zfile *z = NULL;
  int root, ffs;
  struct adfhandle *adf = (struct adfhandle *)zn->volume->handle;
  int size, bs;
  int i;
  uae_u8 *dst;

  size = zn->size;
  bs = adf->blocksize;
  z = zfile_fopen_empty (zn->volume->archive, zn->fullname, size);
  if (!z)
  	return NULL;

  if ((adf->dostype & 0xffffff00) == 'DOS\0') {

  	ffs = adf->dostype & 1;
  	root = zn->offset;
  	dst = z->data;
  	for (;;) {
	    adf_read_block (adf, root);
	    for (i = bs / 4 - 51; i >= 6; i--) {
    		int bsize = ffs ? bs : bs - 24;
    		int block = gl (adf, i * 4);
    		if (size < bsize)
  		    bsize = size;
    		if (ffs)
  		    zfile_fseek (adf->z, block * adf->blocksize, SEEK_SET);
    		else
  		    zfile_fseek (adf->z, block * adf->blocksize + 24, SEEK_SET);
    		zfile_fread (dst, bsize, 1, adf->z);
    		size -= bsize;
    		dst += bsize;
    		if (size <= 0)
  		    break;
	    }
	    if (size <= 0)
    		break;
	    root = gl (adf, bs - 2 * 4);
  	}
  } else if ((adf->dostype & 0xffffff00) == 'SFS\0') {

  	struct sfsblock *sfsblocks;
  	int sfsblockcnt, sfsmaxblockcnt, i;
  	int bsize;
  	int block = zn->offset;
  	int dblock;
  	int btree, version, sfs2;
  	uae_u8 *p;

  	if (!adf_read_block (adf, 0))
	    goto end;
  	btree = glx (adf->block + 108);
  	version = gwx (adf->block + 12);
  	sfs2 = version > 3;

  	if (!adf_read_block (adf, block))
	    goto end;
  	p = adf->block + zn->offset2;
  	dblock = glx (p + 12);

  	sfsblockcnt = 0;
  	sfsmaxblockcnt = 0;
  	sfsblocks = NULL;
  	if (size > 0) {
	    int nextblock = dblock;
	    while (nextblock) {
    		nextblock = sfsfindblock (adf, btree, nextblock, &sfsblocks, &sfsblockcnt, &sfsmaxblockcnt, sfs2);
	    }
  	}

  	bsize = 0;
  	for (i = 0; i < sfsblockcnt; i++)
	    bsize += sfsblocks[i].length * adf->blocksize;
  	if (bsize < size)
	    write_log (_T("SFS extracting error, %s size mismatch %d<%d\n"), z->name, bsize, size);

  	dst = z->data;
    block = zn->offset;
  	for (i = 0; i < sfsblockcnt; i++) {
	    block = sfsblocks[i].block;
	    bsize = sfsblocks[i].length * adf->blocksize;
	    zfile_fseek (adf->z, block * adf->blocksize, SEEK_SET);
	    if (bsize > size)
    		bsize = size;
	    zfile_fread (dst, bsize, 1, adf->z);
	    dst += bsize;
	    size -= bsize; 
  	}

  	xfree (sfsblocks);
  }
  return z;
end:
  zfile_fclose (z);
  return NULL;
}

static void archive_close_adf (void *v)
{
  struct adfhandle *adf = (struct adfhandle *)v;
  xfree (adf);
}

void archive_access_close (void *handle, unsigned int id)
{
  switch (id)
  {
	case ArchiveFormatZIP:
  	archive_close_zip(handle);
  	break;
	case ArchiveFormat7Zip:
  	archive_close_7z((struct SevenZContext *)handle);
  	break;
	case ArchiveFormatLHA:
  	break;
	case ArchiveFormatADF:
  	archive_close_adf (handle);
  	break;
  }
}

struct zfile *archive_unpackzfile (struct zfile *zf)
{
	struct zfile *zout = NULL;
	if (!zf->archiveparent)
		return NULL;
	zf->datasize = zf->size;
	switch (zf->archiveid)
	{
	case ArchiveFormatZIP:
		zout = archive_unpack_zip (zf);
		break;
	}
	zfile_fclose (zf->archiveparent);
	zf->archiveparent = NULL;
	zf->archiveid = 0;
	return NULL;
}

struct zfile *archive_getzfile (struct znode *zn, unsigned int id, int flags)
{
  struct zfile *zf = NULL;
  switch (id)
  {
	case ArchiveFormatZIP:
  	zf = archive_access_zip (zn, flags);
  	break;
	case ArchiveFormat7Zip:
  	zf = archive_access_7z (zn);
  	break;
	case ArchiveFormatLHA:
  	zf = archive_access_lha (zn);
  	break;
	case ArchiveFormatLZX:
  	zf = archive_access_lzx (zn);
  	break;
	case ArchiveFormatPLAIN:
  	zf = archive_access_plain (zn);
  	break;
	case ArchiveFormatADF:
  	zf = archive_access_adf (zn);
  	break;
  }
	if (zf)
		zf->archiveid = id;
  return zf;
}
