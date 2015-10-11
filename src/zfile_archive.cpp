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
    tzset();
    t -= timezone;
    return t;
}

static struct zvolume *getzvolume(struct zfile *zf, unsigned int id)
{
    struct zvolume *zv;

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
    }
#ifdef ARCHIVEACCESS
    if (!zv)
	zv = archive_directory_arcacc (zf, id);
#endif
    return zv;
}

struct zfile *archive_getzfile(struct znode *zn, unsigned int id)
{
    struct zfile *zf = NULL;
    switch (id)
    {
	case ArchiveFormatZIP:
	zf = archive_access_zip (zn);
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
    }
    return zf;
}

struct zfile *archive_access_select (struct zfile *zf, unsigned int id, int dodefault)
{
    struct zvolume *zv;
    struct znode *zn;
    int zipcnt, first, select;
    char tmphist[MAX_DPATH];
    struct zfile *z = NULL;
    int we_have_file;

    zv = getzvolume(zf, id);
    if (!zv)
	return zf;
    we_have_file = 0;
    tmphist[0] = 0;
    zipcnt = 1;
    first = 1;
    zn = &zv->root;
    while (zn) {
	int isok = 1;
	
	if (!zn->isfile)
	    isok = 0;
	if (zfile_is_ignore_ext(zn->fullname))
	    isok = 0;
	if (isok) {
	    if (tmphist[0]) {
	        DISK_history_add (tmphist, -1);
	        tmphist[0] = 0;
	        first = 0;
	    }
	    if (first) {
		if (zfile_isdiskimage (zn->fullname))
		    strcpy (tmphist, zn->fullname);
	    } else {
	        strcpy (tmphist, zn->fullname);
	        DISK_history_add (tmphist, -1);
	        tmphist[0] = 0;
	    }
	    select = 0;
	    if (!zf->zipname)
	        select = 1;
	    if (zf->zipname && strlen (zn->fullname) >= strlen (zf->zipname) && !strcasecmp (zf->zipname, zn->fullname + strlen (zn->fullname) - strlen (zf->zipname)))
	        select = -1;
	    if (zf->zipname && zf->zipname[0] == '#' && atol (zf->zipname + 1) == zipcnt)
	        select = -1;
	    if (select && !we_have_file) {
		z = archive_getzfile(zn, id);
		if (z) {
		    if (select < 0 || zfile_gettype (z))
			we_have_file = 1;
		    if (!we_have_file) {
			zfile_fclose(z);
			z = NULL;
		    }
		}
	    }
	}
	zipcnt++;
	zn = zn->next;
    }
    if (first && tmphist[0])
        DISK_history_add (zfile_getname(zf), -1);
    zfile_fclose_archive (zv);
    if (z) {
	zfile_fclose(zf);
	zf = z;
    } else if (!dodefault && zf->zipname && zf->zipname[0]) {
	zfile_fclose(zf);
	zf = NULL;
    }
    return zf;
}

struct zfile *archive_access_arcacc_select (struct zfile *zf, unsigned int id)
{
    return zf;
}

void archive_access_scan (struct zfile *zf, zfile_callback zc, void *user, unsigned int id)
{
    struct zvolume *zv;
    struct znode *zn;

    zv = getzvolume(zf, id);
    if (!zv)
	return;
    zn = &zv->root;
    while (zn) {
	if (zn->isfile) {
	    struct zfile *zf2 = archive_getzfile (zn, id);
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
    unzClose(handle);
}

struct zvolume *archive_directory_zip (struct zfile *z)
{
    unzFile uz;
    unz_file_info file_info;
    char filename_inzip[MAX_DPATH];
    struct zvolume *zv;
    int err;

    uz = unzOpen (z);
    if (!uz)
	return 0;
    if (unzGoToFirstFile (uz) != UNZ_OK)
	return 0;
    zv = zvolume_alloc(z, ArchiveFormatZIP, uz);
    for (;;) {
	char c;
	struct zarchive_info zai;
	time_t t;
	unsigned int dd;
	err = unzGetCurrentFileInfo(uz, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
	if (err != UNZ_OK)
	    return 0;
	dd = file_info.dosDate;
	t = fromdostime(dd);
	memset(&zai, 0, sizeof zai);
	zai.name = filename_inzip;
	zai.t = t;
	c = filename_inzip[strlen(filename_inzip) - 1];
	if (c != '/' && c != '\\') {
	    int err = unzOpenCurrentFile (uz);
	    if (err == UNZ_OK) {
		struct znode *zn;
		zai.size = file_info.uncompressed_size;
		zn = zvolume_addfile_abs(zv, &zai);
	    }
	} else {
	    filename_inzip[strlen(filename_inzip) - 1] = 0;
	    zvolume_adddir_abs(zv, &zai);
	}
	err = unzGoToNextFile (uz);
	if (err != UNZ_OK)
	    break;
    }
    zv->method = ArchiveFormatZIP;
    return zv;
}


struct zfile *archive_access_zip (struct znode *zn)
{
    struct zfile *z = NULL;
    unzFile uz = zn->volume->handle;
    int i, err;
    char tmp[MAX_DPATH];

    strcpy (tmp, zn->fullname + strlen(zn->volume->root.fullname) + 1);
    if (unzGoToFirstFile (uz) != UNZ_OK)
	return 0;
    for (i = 0; tmp[i]; i++) {
	if (tmp[i] == '\\')
	    tmp[i] = '/';
    }
    if (unzLocateFile (uz, tmp, 1) != UNZ_OK) {
	for (i = 0; tmp[i]; i++) {
	    if (tmp[i] == '/')
		tmp[i] = '\\';
	}
	if (unzLocateFile (uz, tmp, 1) != UNZ_OK)
	    return 0;
    }
    if (unzOpenCurrentFile (uz) != UNZ_OK)
	return 0;
    z = zfile_fopen_empty (zn->fullname, zn->size);
    if (z) {
	err = unzReadCurrentFile (uz, z->data, zn->size);
    }
    unzCloseCurrentFile (uz);
    return z;
}
    
/* 7Z */

#include "archivers/7z/7zCrc.h"
#include "archivers/7z/7zIn.h"
#include "archivers/7z/7zExtract.h"

typedef struct _CFileInStream
{
  ISzInStream InStream;
  struct zfile *zf;
} CFileInStream;

static ISzAlloc allocImp;
static ISzAlloc allocTempImp;

static SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = zfile_fread(buffer, 1, size, s->zf);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

static SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;
  int res = zfile_fseek(s->zf, pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
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
    InitCrcTable();
}

struct SevenZContext
{
    CArchiveDatabaseEx db;
    CFileInStream archiveStream;
    Byte *outBuffer;
    size_t outBufferSize;
    UInt32 blockIndex;
};

static void archive_close_7z (struct SevenZContext *ctx)
{
    SzArDbExFree(&ctx->db, allocImp.Free);
    allocImp.Free(ctx->outBuffer);
    xfree(ctx);
}

struct zvolume *archive_directory_7z (struct zfile *z)
{
    SZ_RESULT res;
    struct zvolume *zv;
    int i;
    struct SevenZContext *ctx;

    init_7z();
    ctx = (struct SevenZContext *) xcalloc (sizeof (struct SevenZContext), 1);
    ctx->blockIndex = 0xffffffff;
    ctx->archiveStream.InStream.Read = SzFileReadImp;
    ctx->archiveStream.InStream.Seek = SzFileSeekImp;
    SzArDbExInit(&ctx->db);
    ctx->archiveStream.zf = z;
    res = SzArchiveOpen(&ctx->archiveStream.InStream, &ctx->db, &allocImp, &allocTempImp);
    if (res != SZ_OK) {
	write_log("7Z: SzArchiveOpen %s returned %d\n", zfile_getname(z), res);
	return NULL;
    }
    zv = zvolume_alloc(z, ArchiveFormat7Zip, ctx);
    for (i = 0; i < ctx->db.Database.NumFiles; i++) {
	CFileItem *f = ctx->db.Database.Files + i;
	char *name = f->Name;
	struct zarchive_info zai;

	memset(&zai, 0, sizeof zai);
	zai.name = name;
	zai.size = f->Size;
	if (!f->IsDirectory) {
	    struct znode *zn = zvolume_addfile_abs(zv, &zai);
	    zn->offset = i;
	}
    }
    zv->method = ArchiveFormat7Zip;
    return zv;
}

struct zfile *archive_access_7z (struct znode *zn)
{
    SZ_RESULT res;
    struct zvolume *zv = zn->volume;
    struct zfile *z = NULL;
    size_t offset;
    size_t outSizeProcessed;
    struct SevenZContext *ctx;

    ctx = (struct SevenZContext *) zv->handle;
    res = SzExtract(&ctx->archiveStream.InStream, &ctx->db, zn->offset, 
		    &ctx->blockIndex, &ctx->outBuffer, &ctx->outBufferSize, 
		    &offset, &outSizeProcessed, 
		    &allocImp, &allocTempImp);
    if (res == SZ_OK) {
        z = zfile_fopen_empty (zn->fullname, zn->size);
	zfile_fwrite (ctx->outBuffer + offset, zn->size, 1, z);
    } else {
	write_log("7Z: SzExtract %s returned %d\n", zn->fullname, res);
    }
    return z;
}

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
	int IsDir, IsEncrypted;
	struct aaFILETIME LastWriteTime;
	char path[FileInArchiveInfoStringSize];
};

typedef HRESULT (__stdcall *aaReadCallback)(int StreamID, uae_u64 offset, uae_u32 count, void* buf, uae_u32 *processedSize);
typedef HRESULT (__stdcall *aaWriteCallback)(int StreamID, uae_u64 offset, uae_u32 count, const void *buf, uae_u32 *processedSize);
typedef aaHandle (__stdcall *aapOpenArchive)(aaReadCallback function, int StreamID, uae_u64 FileSize, int ArchiveType, int *result, char *password);
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
    arcacc_mod = WIN32_LoadLibrary ("archiveaccess.dll");
    if (!arcacc_mod) {
	write_log ("failed to open archiveaccess.dll ('%s')\n", zfile_getname (zf));
	return 0;
    }
    aaOpenArchive = (aapOpenArchive) GetProcAddress (arcacc_mod, "aaOpenArchive");
    aaGetFileCount = (aapGetFileCount) GetProcAddress (arcacc_mod, "aaGetFileCount");
    aaGetFileInfo = (aapGetFileInfo) GetProcAddress (arcacc_mod, "aaGetFileInfo");
    aaExtract = (aapExtract) GetProcAddress (arcacc_mod, "aaExtract");
    aaCloseArchive = (aapCloseArchive) GetProcAddress (arcacc_mod, "aaCloseArchive");
    if (!aaOpenArchive || !aaGetFileCount || !aaGetFileInfo || !aaExtract || !aaCloseArchive) {
	write_log ("Missing functions in archiveaccess.dll. Old version?\n");
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
    zv = zvolume_alloc(z, id, NULL);
    id_r = arcacc_push (z);
    ah = aaOpenArchive (readCallback, id_r, zv->archivesize, id, &status, NULL);
    if (!status) {
	zv->handle = ah;
	fc = aaGetFileCount (ah);
	for (f = 0; f < fc; f++) {
	    struct aaFileInArchiveInfo fi;
	    char *name;
	    struct znode *zn;
	    struct zarchive_info zai;

	    memset (&fi, 0, sizeof (fi));
	    aaGetFileInfo (ah, f, &fi);
	    if (fi.IsDir)
		continue;

	    name = fi.path;
	    memset(&zai, 0, sizeof zai);
	    zai.name = name;
	    zai.size = (unsigned int)fi.UncompressedFileSize;
	    zn = zvolume_addfile_abs(zv, &zai);
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


struct zfile *archive_access_arcacc (struct znode *zn)
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
	zf = zfile_fopen_empty (zn->fullname, zn->size);
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
    }
}

/* plain single file */

static void addfile(struct zvolume *zv, const char *path, uae_u8 *data, int size)
{
    struct zarchive_info zai;
    struct znode *zn;
    struct zfile *z = zfile_fopen_empty (path, size);

    zfile_fwrite(data, size, 1, z);
    memset(&zai, 0, sizeof zai);
    zai.name = path;
    zai.size = size;
    zn = zvolume_addfile_abs(zv, &zai);
    if (zn)
        zn->f = z;
    else
        zfile_fclose(z);
}

static uae_u8 exeheader[]={0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00};
struct zvolume *archive_directory_plain (struct zfile *z)
{
    struct zvolume *zv;
    struct znode *zn;
    struct zarchive_info zai;
    int i;
    char *filename;
    uae_u8 id[8];

    memset(&zai, 0, sizeof zai);
    zv = zvolume_alloc(z, ArchiveFormatPLAIN, NULL);
    for (i = strlen(z->name) - 1; i >= 0; i--) {
	if (z->name[i] == '\\' || z->name[i] == '/' || z->name[i] == ':') {
	    i++;
	    break;
	}
    }
    filename = &z->name[i];
    memset(id, 0, sizeof id);
    zai.name = filename;
    zfile_fseek(z, 0, SEEK_END);
    zai.size = zfile_ftell(z);
    zfile_fseek(z, 0, SEEK_SET);
    zfile_fread(id, sizeof id, 1, z);
    zfile_fseek(z, 0, SEEK_SET);
    zn = zvolume_addfile_abs(zv, &zai);
    if (!memcmp (id, exeheader, sizeof id)) {
	char *data = (char *)xmalloc(1 + strlen(filename) + 1 + 2);
	sprintf(data,"\"%s\"\n", filename);
	addfile(zv, "s/startup-sequence", (uae_u8 *)data, strlen(data));
	xfree(data);
    }
    return zv;
}

struct zfile *archive_access_plain (struct znode *zn)
{
    struct zfile *z;

    z = zfile_fopen_empty (zn->fullname, zn->size);
    zfile_fread(z->data, zn->size, 1, zn->volume->archive);
    return z;
}
