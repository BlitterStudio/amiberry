 /*
  * UAE - The Un*x Amiga Emulator
  *
  * transparent archive handling
  *
  *     2007 Toni Wilen
  */

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef _WIN32
#include <windows.h>
#include "win32.h"
#endif

#include "options.h"
#include "zfile.h"
#include "archivers/zip/unzip.h"
#include "archivers/dms/pfile.h"
#include "crc32.h"
#include "zarchive.h"
#include "disk.h"

#include <zlib.h>

#define unpack_log write_log
#undef unpack_log
#define unpack_log

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
  t -= _timezone;
  return t;
}

static struct zvolume *getzvolume (struct znode *parent, struct zfile *zf, unsigned int id)
{
  struct zvolume *zv = NULL;

  switch (id)
  {
#ifdef A_ZIP
	case ArchiveFormatZIP:
  	zv = archive_directory_zip (zf);
  	break;
#endif
#ifdef A_7Z
	case ArchiveFormat7Zip:
  	zv = archive_directory_7z (zf);
  	break;
#endif
#ifdef A_RAR
	case ArchiveFormatRAR:
		zv = archive_directory_rar (zf);
		break;
#endif
#ifdef A_LHA
	case ArchiveFormatLHA:
  	zv = archive_directory_lha (zf);
  	break;
#endif
#ifdef A_LZX
	case ArchiveFormatLZX:
  	zv = archive_directory_lzx (zf);
  	break;
#endif
	case ArchiveFormatPLAIN:
  	zv = archive_directory_plain (zf);
  	break;
	case ArchiveFormatADF:
  	zv = archive_directory_adf (parent, zf);
  	break;
	case ArchiveFormatRDB:
		zv = archive_directory_rdb (zf);
		break;
	case ArchiveFormatTAR:
		zv = archive_directory_tar (zf);
		break;
	case ArchiveFormatFAT:
		zv = archive_directory_fat (zf);
		break;
  }
#ifdef ARCHIVEACCESS
	if (!zv)
		zv = archive_directory_arcacc (zf, id);
#endif
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
#ifndef _CONSOLE
    		if (diskimg >= 0&& canhistory)
	        DISK_history_add (tmphist, -1, diskimg, 1);
#endif
        tmphist[0] = 0;
	      first = 0;
	    }
	    if (first) {
				if (diskimg >= 0)
  		    _tcscpy (tmphist, zn->fullname);
	    } else {
       _tcscpy (tmphist, zn->fullname);
#ifndef _CONSOLE
    		if (diskimg >= 0&& canhistory)
	        DISK_history_add (tmphist, -1, diskimg, 1);
#endif
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
#ifdef WITH_CHD
					if (ext && !_tcsicmp (ext, _T(".chd"))) {
						whf = 2;
						ft = ZFILE_CDIMAGE;
					}
#endif
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
#ifndef _CONSOLE
	diskimg = zfile_is_diskimage (zfile_getname (zf));
	if (diskimg >= 0 && first && tmphist[0] && canhistory)
		DISK_history_add (zfile_getname (zf), -1, diskimg, 1);
#endif
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

struct zfile *archive_access_arcacc_select (struct zfile *zf, unsigned int id, int *retcode)
{
	if (zfile_needwrite (zf)) {
		if (retcode)
			*retcode = -1;
		return NULL;
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

/* TAR */

static void archive_close_tar (void *handle)
{
}

struct zvolume *archive_directory_tar (struct zfile *z)
{
	struct zvolume *zv;
	struct znode *zn;

	_tzset ();
	zv = zvolume_alloc (z, ArchiveFormatTAR, NULL, NULL);
	for (;;) {
		uae_u8 block[512];
		char name[MAX_DPATH];
		int ustar = 0;
		struct zarchive_info zai;
		int valid = 1;
		uae_u64 size;

		if (zfile_fread (block, 512, 1, z) != 1)
			break;
		if (block[0] == 0)
			break;
			
		if (!memcmp (block + 257, "ustar  ", 8))
			ustar = 1;
		name[0] = 0;
		if (ustar)
			strcpy (name, (char*)block + 345);
		strcat (name, (char*)block);

		if (name[0] == 0)
			valid = 0;
		if (block[156] != '0')
			valid = 0;
		if (ustar && (block[256] != 0 && block[256] != '0'))
			valid = 0;

		size = _strtoui64 ((char*)block + 124, NULL, 8);

		if (valid) {
			memset (&zai, 0, sizeof zai);
			zai.name = au (name);
			zai.size = size;
			zai.tv.tv_sec = _strtoui64 ((char*)block + 136, NULL, 8);
			zai.tv.tv_sec += _timezone;
			if (_daylight)
				zai.tv.tv_sec -= 1 * 60 * 60;
			if (zai.name[_tcslen (zai.name) - 1] == '/') {
				zn = zvolume_adddir_abs (zv, &zai);
			} else {
				zn = zvolume_addfile_abs (zv, &zai);
				if (zn)
					zn->offset = zfile_ftell (z);
			}
			xfree (zai.name);
		}
		zfile_fseek (z, (size + 511) & ~511, SEEK_CUR);
	}
	zv->method = ArchiveFormatTAR;
	return zv;
}

struct zfile *archive_access_tar (struct znode *zn)
{
#if 0
	struct zfile *zf = zfile_fopen_empty (zn->volume->archive, zn->fullname, zn->size);
	zfile_fseek (zn->volume->archive, zn->offset, SEEK_SET);
	zfile_fwrite (zf->data, zn->size, 1, zn->volume->archive);
	return zf;
#else
	return zfile_fopen_parent (zn->volume->archive, zn->fullname, zn->offset, zn->size);
#endif
}

/* ZIP */
#ifdef A_ZIP

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
		char filename_inzip2[MAX_DPATH];
  	TCHAR c;
  	struct zarchive_info zai;
  	time_t t;
  	unsigned int dd;
		TCHAR *filename_inzip;

		err = unzGetCurrentFileInfo (uz, &file_info, filename_inzip2, sizeof (filename_inzip2), NULL, 0, NULL, 0);
  	if (err != UNZ_OK)
	    return 0;
		if (file_info.flag & (1 << 11)) { // UTF-8 encoded
			filename_inzip = utf8u (filename_inzip2);
		} else {
			filename_inzip = au (filename_inzip2);
		}
	  dd = file_info.dosDate;
  	t = fromdostime(dd);
	  memset(&zai, 0, sizeof zai);
	  zai.name = filename_inzip;
		zai.tv.tv_sec = t;
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
		xfree (filename_inzip);
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
	s = ua (tmp);
	if (unzLocateFile (uz, s, 1) != UNZ_OK) {
		xfree (s);
		for (i = 0; tmp[i]; i++) {
			if (tmp[i] == '/')
				tmp[i] = '\\';
		}
		s = ua (tmp);
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
			unpack_log (_T("ZIP: unpacking %s, flags=%d\n"), name, flags);
			err = unzReadCurrentFile (uz, z->data, z->datasize);
			unpack_log (_T("ZIP: unpacked, code=%d\n"), err);
		} else {
			z->archiveparent = zfile_dup (zn->volume->archive);
			if (z->archiveparent) {
				unpack_log (_T("ZIP: delayed open '%s'\n"), name);
				xfree (z->archiveparent->name);
				z->archiveparent->name = my_strdup (tmp);
				z->datasize = PEEK_BYTES;
				err = unzReadCurrentFile (uz, z->data, z->datasize);
				unpack_log (_T("ZIP: unpacked, code=%d\n"), err);
			} else {
				unpack_log (_T("ZIP: unpacking %s (failed DELAYEDOPEN)\n"), name);
				err = unzReadCurrentFile (uz, z->data, z->datasize);
				unpack_log (_T("ZIP: unpacked, code=%d\n"), err);
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
#endif
    
#ifdef A_7Z
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

static void archive_close_7z (void *ctx)
{
	struct SevenZContext *ctx7 = (struct SevenZContext*)ctx;
	SzArEx_Free (&ctx7->db, &allocImp);
	allocImp.Free (&allocImp, ctx7->outBuffer);
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
  	if (f->MTimeDefined) {
	    uae_u64 t = (((uae_u64)f->MTime.High) << 32) | f->MTime.Low;
	    if (t >= EPOCH_DIFF) {
				zai.tv.tv_sec = (t - EPOCH_DIFF) / RATE_DIFF;
				zai.tv.tv_sec -= _timezone;
    		if (_daylight)
					zai.tv.tv_sec += 1 * 60 * 60;
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

	z = zfile_fopen_empty (NULL, zn->fullname, zn->size);
	if (!z)
		return NULL;
  ctx = (struct SevenZContext *) zv->handle;
  res = SzAr_Extract (&ctx->db, &ctx->lookStream.s, zn->offset,
    &ctx->blockIndex, &ctx->outBuffer, &ctx->outBufferSize, 
    &offset, &outSizeProcessed, 
    &allocImp, &allocTempImp);
  if (res == SZ_OK) {
  	zfile_fwrite (ctx->outBuffer + offset, zn->size, 1, z);
  } else {
  	write_log(_T("7Z: SzExtract %s returned %d\n"), zn->fullname, res);
		zfile_fclose (z);
		z = NULL;
  }
  return z;
}
#endif

/* RAR */
#ifdef A_RAR

/* copy and paste job? you are only imagining it! */
static struct zfile *rarunpackzf; /* stupid unrar.dll */
#include <unrar.h>
typedef HANDLE (_stdcall* RAROPENARCHIVEEX)(struct RAROpenArchiveDataEx*);
static RAROPENARCHIVEEX pRAROpenArchiveEx;
typedef int (_stdcall* RARREADHEADEREX)(HANDLE,struct RARHeaderDataEx*);
static RARREADHEADEREX pRARReadHeaderEx;
typedef int (_stdcall* RARPROCESSFILE)(HANDLE,int,char*,char*);
static RARPROCESSFILE pRARProcessFile;
typedef int (_stdcall* RARCLOSEARCHIVE)(HANDLE);
static RARCLOSEARCHIVE pRARCloseArchive;
typedef void (_stdcall* RARSETCALLBACK)(HANDLE,UNRARCALLBACK,LONG);
static RARSETCALLBACK pRARSetCallback;
typedef int (_stdcall* RARGETDLLVERSION)(void);
static RARGETDLLVERSION pRARGetDllVersion;

static int canrar (void)
{
	static int israr;

	if (israr == 0) {
		israr = -1;
#ifdef _WIN32
		{
			HMODULE rarlib;

			rarlib = WIN32_LoadLibrary (_T("unrar.dll"));
			if (rarlib) {
				TCHAR tmp[MAX_DPATH];
				tmp[0] = 0;
				GetModuleFileName (rarlib, tmp, sizeof tmp / sizeof (TCHAR));
				pRAROpenArchiveEx = (RAROPENARCHIVEEX)GetProcAddress (rarlib, "RAROpenArchiveEx");
				pRARReadHeaderEx = (RARREADHEADEREX)GetProcAddress (rarlib, "RARReadHeaderEx");
				pRARProcessFile = (RARPROCESSFILE)GetProcAddress (rarlib, "RARProcessFile");
				pRARCloseArchive = (RARCLOSEARCHIVE)GetProcAddress (rarlib, "RARCloseArchive");
				pRARSetCallback = (RARSETCALLBACK)GetProcAddress (rarlib, "RARSetCallback");
				pRARGetDllVersion = (RARGETDLLVERSION)GetProcAddress (rarlib, "RARGetDllVersion");
				if (pRAROpenArchiveEx && pRARReadHeaderEx && pRARProcessFile && pRARCloseArchive && pRARSetCallback) {
					int version = -1;
					israr = 1;
					if (pRARGetDllVersion)
						version = pRARGetDllVersion ();
					write_log (_T("%s version %08X detected\n"), tmp, version);
					if (version < 4) {
						write_log (_T("Too old unrar.dll, must be at least version 4\n"));
						israr = -1;
					}

				}
			}
		}
#endif
	}
	return israr < 0 ? 0 : 1;
}

static int CALLBACK RARCallbackProc (UINT msg,LONG UserData,LONG P1,LONG P2)
{
	if (msg == UCM_PROCESSDATA) {
		zfile_fwrite ((uae_u8*)P1, 1, P2, rarunpackzf);
		return 0;
	}
	return -1;
}

struct RARContext
{
	struct RAROpenArchiveDataEx OpenArchiveData;
	struct RARHeaderDataEx HeaderData;
	HANDLE hArcData;
};

static void archive_close_rar (void *ctx)
{
	struct RARContext* rc = (struct RARContext*)ctx;
	xfree (rc);
}

struct zvolume *archive_directory_rar (struct zfile *z)
{
	struct zvolume *zv;
	struct RARContext *rc;
	struct zfile *zftmp;
	int cnt;

	if (!canrar ())
		return archive_directory_arcacc (z, ArchiveFormatRAR);
	if (z->data)
		/* wtf? stupid unrar.dll only accept filename as an input.. */
		return archive_directory_arcacc (z, ArchiveFormatRAR);
	rc = xcalloc (struct RARContext, 1);
	zv = zvolume_alloc (z, ArchiveFormatRAR, rc, NULL);
	rc->OpenArchiveData.ArcNameW = z->name;
	rc->OpenArchiveData.OpenMode = RAR_OM_LIST;
	rc->hArcData = pRAROpenArchiveEx (&rc->OpenArchiveData);
	if (rc->OpenArchiveData.OpenResult != 0) {
		zfile_fclose_archive (zv);
		return archive_directory_arcacc (z, ArchiveFormatRAR);
	}
	pRARSetCallback (rc->hArcData, RARCallbackProc, 0);
	cnt = 0;
	while (pRARReadHeaderEx (rc->hArcData, &rc->HeaderData) == 0) {
		struct zarchive_info zai;
		struct znode *zn;
		memset (&zai, 0, sizeof zai);
		zai.name = rc->HeaderData.FileNameW;
		zai.size = rc->HeaderData.UnpSize;
		zai.flags = -1;
		zai.tv.tv_sec = fromdostime (rc->HeaderData.FileTime);
		zn = zvolume_addfile_abs (zv, &zai);
		zn->offset = cnt++;
		pRARProcessFile (rc->hArcData, RAR_SKIP, NULL, NULL);
	}
	pRARCloseArchive (rc->hArcData);
	zftmp = zfile_fopen_empty (z, z->name, 0);
	zv->archive = zftmp;
	zv->method = ArchiveFormatRAR;
	return zv;
}

static struct zfile *archive_access_rar (struct znode *zn)
{
	struct RARContext *rc = (struct RARContext*)zn->volume->handle;
	int i;
	struct zfile *zf = NULL;

	if (zn->volume->method != ArchiveFormatRAR)
		return archive_access_arcacc (zn);
	rc->OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
	rc->hArcData = pRAROpenArchiveEx (&rc->OpenArchiveData);
	if (rc->OpenArchiveData.OpenResult != 0)
		return NULL;
	pRARSetCallback (rc->hArcData, RARCallbackProc, 0);
	for (i = 0; i <= zn->offset; i++) {
		if (pRARReadHeaderEx (rc->hArcData, &rc->HeaderData))
			return NULL;
		if (i < zn->offset) {
			if (pRARProcessFile (rc->hArcData, RAR_SKIP, NULL, NULL))
				goto end;
		}
	}
	zf = zfile_fopen_empty (zn->volume->archive, zn->fullname, zn->size);
	if (zf) {
		rarunpackzf = zf;
		if (pRARProcessFile (rc->hArcData, RAR_TEST, NULL, NULL)) {
			zfile_fclose (zf);
			zf = NULL;
		}
	}
end:
	pRARCloseArchive(rc->hArcData);
	return zf;
}
#endif


/* ArchiveAccess */


#if defined(ARCHIVEACCESS)

struct aaFILETIME
{
	uae_u32 dwLowDateTime;
	uae_u32 dwHighDateTime;
};
typedef void* aaHandle;
// This struct contains file information from an archive. The caller may store
// this information for accessing this file after calls to findFirst, findNext
#define FileInArchiveInfoStringSize 1024
struct aaFileInArchiveInfo {
	int ArchiveHandle; // handle for Archive/class pointer
	uae_u64 CompressedFileSize;
	uae_u64 UncompressedFileSize;
	uae_u32 attributes;
	int IsDir;
	struct aaFILETIME LastWriteTime;
	char path[FileInArchiveInfoStringSize];
};

typedef HRESULT (__stdcall *aaReadCallback)(int StreamID, uae_u64 offset, uae_u32 count, void* buf, uae_u32 *processedSize);
typedef HRESULT (__stdcall *aaWriteCallback)(int StreamID, uae_u64 offset, uae_u32 count, const void *buf, uae_u32 *processedSize);
typedef aaHandle (__stdcall *aapOpenArchive)(aaReadCallback function, int StreamID, uae_u64 FileSize, int ArchiveType, int *result, TCHAR *password);
typedef int (__stdcall *aapGetFileCount)(aaHandle ArchiveHandle);
typedef int (__stdcall *aapGetFileInfo)(aaHandle ArchiveHandle, int FileNum, struct aaFileInArchiveInfo *FileInfo);
typedef int (__stdcall *aapExtract)(aaHandle ArchiveHandle, int FileNum, int StreamID, aaWriteCallback WriteFunc, uae_u64 *written);
typedef int (__stdcall *aapCloseArchive)(aaHandle ArchiveHandle);

static aapOpenArchive aaOpenArchive;
static aapGetFileCount aaGetFileCount;
static aapGetFileInfo aaGetFileInfo;
static aapExtract aaExtract;
static aapCloseArchive aaCloseArchive;

#ifdef _WIN32
static HMODULE arcacc_mod;

static void arcacc_free (void)
{
	if (arcacc_mod)
		FreeLibrary (arcacc_mod);
	arcacc_mod = NULL;
}

static int arcacc_init (struct zfile *zf)
{
	if (arcacc_mod)
		return 1;
	arcacc_mod = WIN32_LoadLibrary (_T("archiveaccess.dll"));
	if (!arcacc_mod) {
		write_log (_T("failed to open archiveaccess.dll ('%s')\n"), zfile_getname (zf));
		return 0;
	}
	aaOpenArchive = (aapOpenArchive) GetProcAddress (arcacc_mod, "aaOpenArchive");
	aaGetFileCount = (aapGetFileCount) GetProcAddress (arcacc_mod, "aaGetFileCount");
	aaGetFileInfo = (aapGetFileInfo) GetProcAddress (arcacc_mod, "aaGetFileInfo");
	aaExtract = (aapExtract) GetProcAddress (arcacc_mod, "aaExtract");
	aaCloseArchive = (aapCloseArchive) GetProcAddress (arcacc_mod, "aaCloseArchive");
	if (!aaOpenArchive || !aaGetFileCount || !aaGetFileInfo || !aaExtract || !aaCloseArchive) {
		write_log (_T("Missing functions in archiveaccess.dll. Old version?\n"));
		arcacc_free ();
		return 0;
	}
	return 1;
}
#endif

#define ARCACC_STACKSIZE 10
static struct zfile *arcacc_stack[ARCACC_STACKSIZE];
static int arcacc_stackptr = -1;

static int arcacc_push (struct zfile *f)
{
	if (arcacc_stackptr == ARCACC_STACKSIZE - 1)
		return -1;
	arcacc_stackptr++;
	arcacc_stack[arcacc_stackptr] = f;
	return arcacc_stackptr;
}
static void arcacc_pop (void)
{
	arcacc_stackptr--;
}

static HRESULT __stdcall readCallback (int StreamID, uae_u64 offset, uae_u32 count, void *buf, uae_u32 *processedSize)
{
	struct zfile *f = arcacc_stack[StreamID];
	int ret;

	zfile_fseek (f, (long)offset, SEEK_SET);
	ret = zfile_fread (buf, 1, count, f);
	if (processedSize)
		*processedSize = ret;
	return 0;
}
static HRESULT __stdcall writeCallback (int StreamID, uae_u64 offset, uae_u32 count, const void *buf, uae_u32 *processedSize)
{
	struct zfile *f = arcacc_stack[StreamID];
	int ret;

	ret = zfile_fwrite ((void*)buf, 1, count, f);
	if (processedSize)
		*processedSize = ret;
	if (ret != count)
		return -1;
	return 0;
}

struct zvolume *archive_directory_arcacc (struct zfile *z, unsigned int id)
{
	aaHandle ah;
	int id_r, status;
	int fc, f;
	struct zvolume *zv;
	int skipsize = 0;

	if (!arcacc_init (z))
		return NULL;
	zv = zvolume_alloc (z, ArchiveFormatAA, NULL, NULL);
	id_r = arcacc_push (z);
	ah = aaOpenArchive (readCallback, id_r, zv->archivesize, id, &status, NULL);
	if (!status) {
		zv->handle = ah;
		fc = aaGetFileCount (ah);
		for (f = 0; f < fc; f++) {
			struct aaFileInArchiveInfo fi;
			TCHAR *name;
			struct znode *zn;
			struct zarchive_info zai;

			memset (&fi, 0, sizeof (fi));
			aaGetFileInfo (ah, f, &fi);
			if (fi.IsDir)
				continue;

			name = au (fi.path);
			memset (&zai, 0, sizeof zai);
			zai.name = name;
			zai.flags = -1;
			zai.size = (unsigned int)fi.UncompressedFileSize;
			zn = zvolume_addfile_abs (zv, &zai);
			xfree (name);
			zn->offset = f;
			zn->method = id;

			if (id == ArchiveFormat7Zip) {
				if (fi.CompressedFileSize)
					skipsize = 0;
				skipsize += (int)fi.UncompressedFileSize;
			}
		}
		aaCloseArchive (ah);
	}
	arcacc_pop ();
	zv->method = ArchiveFormatAA;
	return zv;
}


static struct zfile *archive_access_arcacc (struct znode *zn)
{
	struct zfile *zf;
	struct zfile *z = zn->volume->archive;
	int status, id_r, id_w;
	aaHandle ah;
	int ok = 0;

	id_r = arcacc_push (z);
	ah = aaOpenArchive (readCallback, id_r, zn->volume->archivesize, zn->method, &status, NULL);
	if (!status) {
		int err;
		uae_u64 written = 0;
		struct aaFileInArchiveInfo fi;
		memset (&fi, 0, sizeof (fi));
		aaGetFileInfo (ah, zn->offset, &fi);
		zf = zfile_fopen_empty (z, zn->fullname, zn->size);
		id_w = arcacc_push (zf);
		err = aaExtract(ah, zn->offset, id_w, writeCallback, &written);
		if (zf->seek == fi.UncompressedFileSize)
			ok = 1;
		arcacc_pop();
	}
	aaCloseArchive(ah);
	arcacc_pop();
	if (ok)
		return zf;
	zfile_fclose(zf);
	return NULL;
}
#endif

/* plain single file */

static struct znode *addfile (struct zvolume *zv, struct zfile *zf, const TCHAR *path, uae_u8 *data, int size)
{
  struct zarchive_info zai;
  struct znode *zn;
  struct zfile *z;

  z = zfile_fopen_empty (zf, path, size);
	if (!z)
		return NULL;
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
		char *an = ua (zai.name);
		char *data = xmalloc (char, 1 + strlen (an) + 1 + 1 + 1);
		sprintf (data, "\"%s\"\n", an);
  	zn = addfile (zv, z, _T("s/startup-sequence"), (uae_u8*)data, strlen (data));
  	xfree(data);
		xfree (an);
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
	return au (buf);
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
			amiga_to_timeval (&zai.tv, gl (adf, bs - 23 * 4), gl (adf, bs - 22 * 4),gl (adf, bs - 21 * 4));
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
			fname = au ((char*)s);
	    i = 0;
	    while (*s) {
    		s++;
    		i++;
	    }
	    s++;
	    i++;
	    if (*s)
				zai.comment = au ((char*)s);
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
				zai.tv.tv_sec = glx (p + 22) - diff2;
	    else
				zai.tv.tv_sec = glx (p + 20) - diff;
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

static int rl (uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}
static TCHAR *tochar (uae_u8 *s, int len)
{
	int i, j;
	uae_char tmp[256];
	j = 0;
	for (i = 0; i < len; i++) {
		uae_char c = *s++;
		if (c >= 0 && c <= 9) {
			tmp[j++] = FSDB_DIR_SEPARATOR;
			tmp[j++] = '0' + c;
		} else if (c < ' ' || c > 'z') {
			tmp[j++] = '.';
		} else {
			tmp[j++] = c;
		}
		tmp[j] = 0;
	}
	return au (tmp);
}

struct zvolume *archive_directory_rdb (struct zfile *z)
{
	uae_u8 buf[512] = { 0 };
	int partnum, bs;
	TCHAR *devname;
	struct zvolume *zv;
	struct zarchive_info zai;
	uae_u8 *p;
	struct znode *zn;

	zv = zvolume_alloc (z, ArchiveFormatRDB, NULL, NULL);

	zfile_fseek (z, 0, SEEK_SET);
	zfile_fread (buf, 1, 512, z);

	partnum = 0;
	for (;;) {
		int partblock;
		TCHAR tmp[MAX_DPATH];
		int surf, spt, spb, lowcyl, highcyl, reserved;
		int size, block, blocksize, rootblock;
		TCHAR comment[81], *dos;

		if (partnum == 0)
			partblock = rl (buf + 28);
		else
			partblock = rl (buf + 4 * 4);
		partnum++;
		if (partblock <= 0)
			break;
		zfile_fseek (z, partblock * 512, SEEK_SET);
		zfile_fread (buf, 1, 512, z);
		if (memcmp (buf, "PART", 4))
			break;

		p = buf + 128 - 16;
		surf = rl (p + 28);
		spb = rl (p + 32);
		spt = rl (p + 36);
		reserved = rl (p + 40);
		lowcyl = rl (p + 52);
		highcyl = rl (p + 56);
		blocksize = rl (p + 20) * 4 * spb;
		block = lowcyl * surf * spt;

		size = (highcyl - lowcyl + 1) * surf * spt;
		size *= blocksize;

		dos = tochar (buf + 192, 4);

		if (!memcmp (dos, _T("DOS"), 3))
			rootblock = ((size / blocksize) - 1 + 2) / 2;
		else
			rootblock = 0;

		devname = getBSTR (buf + 36);
		_stprintf (tmp, _T("%s.hdf"), devname);
		memset (&zai, 0, sizeof zai);
		_stprintf (comment, _T("FS=%s LO=%d HI=%d HEADS=%d SPT=%d RES=%d BLOCK=%d ROOT=%d"),
			dos, lowcyl, highcyl, surf, spt, reserved, blocksize, rootblock);
		zai.comment = comment;
		xfree (dos);
		zai.name = tmp;
		zai.size = size;
		zai.flags = -1;
		zn = zvolume_addfile_abs (zv, &zai);
		zn->offset = partblock;
		zn->offset2 = blocksize; // abuse of offset2..
	}

	zfile_fseek (z, 0, SEEK_SET);
	p = buf;
	zfile_fread (buf, 1, 512, z);
	zai.name = _T("rdb_dump.dat");
	bs = rl (p + 16);
	zai.size = rl (p + 140) * bs;
	zai.comment = NULL;
	zn = zvolume_addfile_abs (zv, &zai);
	zn->offset = 0;

	zv->method = ArchiveFormatRDB;
	return zv;
}

static struct zfile *archive_access_rdb (struct znode *zn)
{
	struct zfile *z = zn->volume->archive;
	struct zfile *zf;
	uae_u8 buf[512] = { 0 };
	int surf, spb, spt, lowcyl, highcyl;
	int size, block, blocksize;
	uae_u8 *p;

	if (zn->offset) {
		zfile_fseek (z, zn->offset * 512, SEEK_SET);
		zfile_fread (buf, 1, 512, z);

		p = buf + 128 - 16;
		surf = rl (p + 28);
		spb = rl (p + 32);
		spt = rl (p + 36);
		lowcyl = rl (p + 52);
		highcyl = rl (p + 56);
		blocksize = rl (p + 20) * 4;
		block = lowcyl * surf * spt;

		size = (highcyl - lowcyl + 1) * surf * spt;
		size *= blocksize;
	} else {
		zfile_fseek (z, 0, SEEK_SET);
		zfile_fread (buf, 1, 512, z);
		p = buf;
		blocksize = rl (p + 16);
		block = 0;
		size = zn->size;
	}

	zf = zfile_fopen_parent (z, zn->fullname, block * blocksize, size);
	return zf;
}

int isfat (uae_u8 *p)
{
	int i, b;

	if ((p[0x15] & 0xf0) != 0xf0)
		return 0;
	if (p[0x0b] != 0x00 || p[0x0c] != 0x02)
		return 0;
	b = 0;
	for (i = 0; i < 8; i++) {
		if (p[0x0d] & (1 << i))
			b++;
	}
	if (b != 1)
		return 0;
	if (p[0x0f] != 0)
		return 0;
	if (p[0x0e] > 8 || p[0x0e] == 0)
		return 0;
	if (p[0x10] == 0 || p[0x10] > 8)
		return 0;
	b = (p[0x12] << 8) | p[0x11];
	if (b > 8192 || b <= 0)
		return 0;
	b = p[0x16] | (p[0x17] << 8);
	if (b == 0 || b > 8192)
		return 0;
	return 1;
}

/*
* The epoch of FAT timestamp is 1980.
*     :  bits :     value
* date:  0 -  4: day   (1 -  31)
* date:  5 -  8: month (1 -  12)
* date:  9 - 15: year  (0 - 127) from 1980
* time:  0 -  4: sec   (0 -  29) 2sec counts
* time:  5 - 10: min   (0 -  59)
* time: 11 - 15: hour  (0 -  23)
*/
#define SECS_PER_MIN    60
#define SECS_PER_HOUR   (60 * 60)
#define SECS_PER_DAY    (SECS_PER_HOUR * 24)
#define UNIX_SECS_1980  315532800L   
#if BITS_PER_LONG == 64
#define UNIX_SECS_2108  4354819200L
#endif
/* days between 1.1.70 and 1.1.80 (2 leap days) */
#define DAYS_DELTA      (365 * 10 + 2)         
/* 120 (2100 - 1980) isn't leap year */
#define YEAR_2100       120 
#define IS_LEAP_YEAR(y) (!((y) & 3) && (y) != YEAR_2100)

/* Linear day numbers of the respective 1sts in non-leap years. */
static time_t days_in_year[] = {
	/* Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec */
	0,   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 0, 0, 0,
};

/* Convert a FAT time/date pair to a UNIX date (seconds since 1 1 70). */
static time_t fat_time_fat2unix (uae_u16 time, uae_u16 date, int fat12)
{
	time_t second, day, leap_day, month, year;

	if (0 && fat12) {
		year = date & 0x7f;
		month = (date >> 7) & 0x0f;
		day = (date >> 11);
	} else {
		year  = date >> 9;  
		month = max(1, (date >> 5) & 0xf);
		day   = max(1, date & 0x1f) - 1;
	}

	leap_day = (year + 3) / 4;
	if (year > YEAR_2100)           /* 2100 isn't leap year */
		leap_day--;
	if (IS_LEAP_YEAR(year) && month > 2)
		leap_day++;

	second =  (time & 0x1f) << 1;
	second += ((time >> 5) & 0x3f) * SECS_PER_MIN;
	second += (time >> 11) * SECS_PER_HOUR;
	second += (year * 365 + leap_day
		+ days_in_year[month] + day
		+ DAYS_DELTA) * SECS_PER_DAY;
	return second;
}

static int getcluster (struct zfile *z, int cluster, int fatstart, int fatbits)
{
	uae_u32 fat = 0;
	uae_u8 p[4];
	int offset = cluster * fatbits;
	zfile_fseek (z, fatstart * 512 + offset / 8, SEEK_SET);
	if (fatbits == 12) {
		zfile_fread (p, 2, 1, z);
		if ((offset & 4))
			fat = ((p[0] & 0xf0) >> 4) | (p[1] << 4);
		else
			fat = (p[0]) | ((p[1] & 0x0f) << 8);
		if (fat >= 0xff0)
			return -1;
	} else if (fatbits == 16) {
		zfile_fread (p, 2, 1, z);
		fat = p[0] | (p[1] << 8);
		if (fat >= 0xfff0)
			return -1;
	} else if (fatbits == 32) {
		zfile_fread (p, 4, 1, z);
		fat = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
		fat &= ~0x0fffffff;
		if (fat >= 0x0ffffff0)
			return -1;
	}
	return fat;
}

static void fatdirectory (struct zfile *z, struct zvolume *zv, TCHAR *name, int startblock, int entries, int sectorspercluster, int fatstart, int dataregion, int fatbits)
{
	struct zarchive_info zai;
	struct znode *znnew;
	int i, j;

	for (i = 0; i < entries; i++) {
		TCHAR name2[MAX_DPATH], *fname;
		uae_s64 size;
		uae_u8 fatname[16];
		uae_u8 buf[32];
		int attr, cnt, ext;
		int startcluster;

		memset (buf, 0, sizeof buf);
		memset (&zai, 0, sizeof zai);
		zfile_fseek (z, startblock * 512 + i * 32, SEEK_SET);
		zfile_fread (buf, 32, 1, z);
		if (buf[0] == 0)
			break;
		if (buf[0] == 0xe5)
			continue;
		if (buf[0] == 0x05)
			buf[0] = 0xe5;
		size = buf[0x1c] | (buf[0x1d] << 8) | (buf[0x1e] << 16) | (buf[0x1f] << 24);
		attr = buf[0x0b];
		startcluster = buf[0x1a] | (buf[0x1b] << 8);
		if ((attr & (0x4 | 0x2)) == 0x06) // system+hidden
			continue;
		if (attr & 8) // disk name
			continue;
		if (attr & 1) // read-only
			zai.flags |= 1 << 3;
		if (!(attr & 32)) // archive
			zai.flags |= 1 << 4;

		cnt = 0;
		ext = 0;
		for (j = 0; j < 8 && buf[j] != 0x20 && buf[j] != 0; j++)
			fatname[cnt++] = buf[j];
		for (j = 0; j < 3 && buf[8 + j] != 0x20 && buf[8 + j] != 0; j++) {
			if (ext == 0)
				fatname[cnt++] = '.';
			ext = 1;
			fatname[cnt++] = buf[8 + j];
		}
		fatname[cnt] = 0;

		fname = au ((char*)fatname);
		name2[0] = 0;
		if (name[0]) {
			TCHAR sep[] = { FSDB_DIR_SEPARATOR, 0 };
			_tcscpy (name2, name);
			_tcscat (name2, sep);
		}
		_tcscat (name2, fname);

		zai.name = name2;
		zai.tv.tv_sec = fat_time_fat2unix (buf[0x16] | (buf[0x17] << 8), buf[0x18] | (buf[0x19] << 8), 1);
		if (attr & (16 | 8)) {
			int nextblock, cluster;
			nextblock = dataregion + (startcluster - 2) * sectorspercluster;
			cluster = getcluster (z, startcluster, fatstart, fatbits);
			if ((cluster < 0 || cluster >= 3) && nextblock != startblock) {
				znnew = zvolume_adddir_abs (zv, &zai);
				fatdirectory (z, zv, name2, nextblock, sectorspercluster * 512 / 32, sectorspercluster, fatstart, dataregion, fatbits);
				while (cluster >= 3) {
					nextblock = dataregion + (cluster - 2) * sectorspercluster;
					fatdirectory (z, zv, name2, nextblock, sectorspercluster * 512 / 32, sectorspercluster, fatstart, dataregion, fatbits);
					cluster = getcluster (z, cluster, fatstart, fatbits);
				}
			}
		} else {
			zai.size = size;
			znnew = zvolume_addfile_abs (zv, &zai);
			znnew->offset = startcluster;
		}

		xfree (fname);
	}
}

struct zvolume *archive_directory_fat (struct zfile *z)
{
	uae_u8 buf[512] = { 0 };
	int fatbits = 12;
	struct zvolume *zv;
	int rootdir, reserved, sectorspercluster;
	int numfats, sectorsperfat, rootentries;
	int dataregion;

	zfile_fseek (z, 0, SEEK_SET);
	zfile_fread (buf, 1, 512, z);

	if (!isfat (buf))
		return NULL;
	reserved = buf[0x0e] | (buf[0x0f] << 8);
	numfats = buf[0x10];
	sectorsperfat = buf[0x16] | (buf[0x17] << 8);
	rootentries = buf[0x11] | (buf[0x12] << 8);
	sectorspercluster = buf[0x0d];
	rootdir = reserved + numfats * sectorsperfat;
	dataregion = rootdir + rootentries * 32 / 512;

	zv = zvolume_alloc (z, ArchiveFormatFAT, NULL, NULL);
	fatdirectory (z, zv, _T(""), rootdir, rootentries, sectorspercluster, reserved, dataregion, fatbits);
	zv->method = ArchiveFormatFAT;
	return zv;
}

static struct zfile *archive_access_fat (struct znode *zn)
{
	uae_u8 buf[512] = { 0 };
	int fatbits = 12;
	int size = zn->size;
	struct zfile *sz, *dz;
	int rootdir, reserved, sectorspercluster;
	int numfats, sectorsperfat, rootentries;
	int dataregion;
	int offset, cluster;

	sz = zn->volume->archive;

	zfile_fseek (sz, 0, SEEK_SET);
	zfile_fread (buf, 1, 512, sz);

	if (!isfat (buf))
		return NULL;
	reserved = buf[0x0e] | (buf[0x0f] << 8);
	numfats = buf[0x10];
	sectorsperfat = buf[0x16] | (buf[0x17] << 8);
	rootentries = buf[0x11] | (buf[0x12] << 8);
	sectorspercluster = buf[0x0d];
	rootdir = reserved + numfats * sectorsperfat;
	dataregion = rootdir + rootentries * 32 / 512;

	dz = zfile_fopen_empty (sz, zn->fullname, size);
	if (!dz)
		return NULL;

	offset = 0;
	cluster = zn->offset;
	while (size && cluster >= 2) {
		int left = size > sectorspercluster * 512 ? sectorspercluster * 512 : size;
		int sector = dataregion + (cluster - 2) * sectorspercluster;
		zfile_fseek (sz, sector * 512, SEEK_SET);
		zfile_fread (dz->data + offset, 1, left, sz);
		size -= left;
		offset += left;
		cluster = getcluster (sz, cluster, reserved, fatbits);
	}

	return dz;
}

void archive_access_close (void *handle, unsigned int id)
{
  switch (id)
  {
#ifdef A_ZIP
	case ArchiveFormatZIP:
  	archive_close_zip(handle);
  	break;
#endif
#ifdef A_7Z
	case ArchiveFormat7Zip:
  	archive_close_7z(handle);
  	break;
#endif
#ifdef A_RAR
	case ArchiveFormatRAR:
		archive_close_rar (handle);
		break;
#endif
#ifdef A_LHA
	case ArchiveFormatLHA:
  	break;
#endif
	case ArchiveFormatADF:
  	archive_close_adf (handle);
  	break;
	case ArchiveFormatTAR:
		archive_close_tar (handle);
		break;
  }
}

static struct zfile *archive_access_dir (struct znode *zn)
{
	return zfile_fopen (zn->fullname, _T("rb"), 0);
}

struct zfile *archive_unpackzfile (struct zfile *zf)
{
	struct zfile *zout = NULL;
	if (!zf->archiveparent)
		return NULL;
	unpack_log (_T("delayed unpack '%s'\n"), zf->name);
	zf->datasize = zf->size;
	switch (zf->archiveid)
	{
#ifdef A_ZIP
	case ArchiveFormatZIP:
		zout = archive_unpack_zip (zf);
		break;
#endif
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
#ifdef A_ZIP
	case ArchiveFormatZIP:
  	zf = archive_access_zip (zn, flags);
  	break;
#endif
#ifdef A_7Z
	case ArchiveFormat7Zip:
  	zf = archive_access_7z (zn);
  	break;
#endif
#ifdef A_RAR
	case ArchiveFormatRAR:
		zf = archive_access_rar (zn);
		break;
#endif
#ifdef A_LHA
	case ArchiveFormatLHA:
  	zf = archive_access_lha (zn);
  	break;
#endif
#ifdef A_LZX
	case ArchiveFormatLZX:
  	zf = archive_access_lzx (zn);
  	break;
#endif
	case ArchiveFormatPLAIN:
  	zf = archive_access_plain (zn);
  	break;
	case ArchiveFormatADF:
  	zf = archive_access_adf (zn);
		break;
	case ArchiveFormatRDB:
		zf = archive_access_rdb (zn);
  	break;
	case ArchiveFormatFAT:
		zf = archive_access_fat (zn);
		break;
	case ArchiveFormatDIR:
		zf = archive_access_dir (zn);
		break;
	case ArchiveFormatTAR:
		zf = archive_access_tar (zn);
		break;
  }
	if (zf) {
		zf->archiveid = id;
		zfile_fseek (zf, 0, SEEK_SET);
	}
  return zf;
}
