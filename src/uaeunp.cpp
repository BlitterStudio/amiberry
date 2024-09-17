#include <stdio.h>
#include <tchar.h>

#include <windows.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "zfile.h"
#include "fsdb.h"
#include "zarchive.h"

TCHAR start_path_exe[MAX_DPATH];
TCHAR start_path_data[MAX_DPATH];
TCHAR sep[] = { FSDB_DIR_SEPARATOR, 0 };

struct uae_prefs currprefs;
static int debug = 0;
static int amigatest;

#define WRITE_LOG_BUF_SIZE 4096
void write_log (const TCHAR *format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start (parms, format);
	if (debug) {
		count = _vsntprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
		_tprintf (buffer);
	}
	va_end (parms);
}

void gui_message (const TCHAR *format, ...)
{
}

uae_u32 uaerand (void)
{
	return rand ();
}

/* convert time_t to/from AmigaDOS time */
static const uae_s64 msecs_per_day = 24 * 60 * 60 * 1000;
static const uae_s64 diff = ((8 * 365 + 2) * (24 * 60 * 60)) * (uae_u64)1000;

void timeval_to_amiga (struct mytimeval *tv, int *days, int *mins, int *ticks)
{
	/* tv.tv_sec is secs since 1-1-1970 */
	/* days since 1-1-1978 */
	/* mins since midnight */
	/* ticks past minute @ 50Hz */

	uae_s64 t = tv->tv_sec * 1000 + tv->tv_usec / 1000;
	t -= diff;
	if (t < 0)
		t = 0;
	*days = t / msecs_per_day;
	t -= *days * msecs_per_day;
	*mins = t / (60 * 1000);
	t -= *mins * (60 * 1000);
	*ticks = t / (1000 / 50);
}

void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks)
{
	uae_s64 t;

	if (days < 0)
		days = 0;
	if (days > 9900 * 365)
		days = 9900 * 365; // in future far enough?
	if (mins < 0 || mins >= 24 * 60)
		mins = 0;
	if (ticks < 0 || ticks >= 60 * 50)
		ticks = 0;

	t = ticks * 20;
	t += mins * (60 * 1000);
	t += ((uae_u64)days) * msecs_per_day;
	t += diff;

	tv->tv_sec = t / 1000;
	tv->tv_usec = (t % 1000) * 1000;
}


static int pattern_match (const TCHAR *str, const TCHAR *pattern)
{
	enum State {
		Exact,        // exact match
		Any,        // ?
		AnyRepeat    // *
	};

	const TCHAR *s = str;
	const TCHAR *p = pattern;
	const TCHAR *q = 0;
	int state = 0;

	int match = TRUE;
	while (match && *p) {
		if (*p == '*') {
			state = AnyRepeat;
			q = p+1;
		} else if (*p == '?') state = Any;
		else state = Exact;

		if (*s == 0) break;

		switch (state) {
		case Exact:
			match = *s == *p;
			s++;
			p++;
			break;

		case Any:
			match = TRUE;
			s++;
			p++;
			break;

		case AnyRepeat:
			match = TRUE;
			s++;

			if (*s == *q){
				// make a recursive call so we don't match on just a single character
				if (pattern_match(s,q) == TRUE) {
					p++;
				}
			}
			break;
		}
	}

	if (state == AnyRepeat) return (*s == *q);
	else if (state == Any) return (*s == *p);
	else return match && (*s == *p);
} 




static void geterror (void)
{
	TCHAR *err = zfile_geterror ();
	if (!err)
		return;
	_tprintf (_T("%s\n"), err);
}

static const TCHAR *prots = _T("HSPARWED");

struct arcdir {
	TCHAR *name;
	int isdir;
	uae_u32 flags;
	uae_u64 size;
	TCHAR *comment;
	uae_u32 crc32;
	int iscrc;
	__time64_t dt;
	int parent, nextlevel;
};

static struct arcdir **filelist;

static void dolist (struct arcdir **filelist, struct arcdir *adp, int entries, int parent, int level)
{
	int ii, i;

	for (ii = 0; ii < 2; ii++) {
		for (i = 0; i < entries; i++) {
			struct arcdir *ad = filelist[i];
			int j;
			TCHAR protflags[9];
			TCHAR dates[32];
			TCHAR crcs[16];
			int flags;
			struct tm *dt;

			if (ad->parent != parent)
				continue;

			if ((ii == 0 && ad->isdir) || (ii == 1 && !ad->isdir)) {

				flags = ad->flags;

				if (flags >= 0) {
					for (j = 0; j < 8; j++) {
						protflags[j] = '-';
						if (flags & (1 << (7 - j)))
							protflags[j] = prots[j];
					}
					protflags[j] = 0;
				} else {
					_tcscpy (protflags, _T("--------"));
				}

				if (ad->dt > 0) {
					dt = _gmtime64 (&ad->dt);
					_tcsftime (dates, sizeof (dates) / sizeof (TCHAR), _T("%Y/%m/%d %H:%M:%S"), dt);
				} else {
					_tcscpy (dates, _T("-------------------"));
				}

				for (j = 0; j < level; j++)
					_tprintf (_T(" "));
				if (ad->iscrc > 0)
					_stprintf (crcs, _T("%08X"), ad->crc32);
				else if (ad->iscrc < 0)
					_tcscpy (crcs, _T("????????"));
				else
					_tcscpy (crcs, _T("--------"));
				if (ad->isdir > 0)
					_tprintf (_T("     [DIR] %s %s          %s\n"), protflags, dates, ad->name);
				else if (ad->isdir < 0)
					_tprintf (_T("    [VDIR] %s %s          %s\n"), protflags, dates, ad->name);
				else
					_tprintf (_T("%10I64d %s %s %s %s\n"), ad->size, protflags, dates, crcs, ad->name);
				if (ad->comment)
					_tprintf (_T(" \"%s\"\n"), ad->comment);
				if (ad->nextlevel >= 0) {
					level++;
					dolist (filelist, adp, entries, ad - adp, level);
					level--;
				}

			}
		}
	}
}

static int parentid = -1, subdirid;
static int maxentries = 10000, entries;

static void resetlist (void)
{
	parentid = -1;
	subdirid = 0;
	maxentries = 10000;
	entries = 0;
}

static int unlist2 (struct arcdir *adp, const TCHAR *src, int all)
{
	struct zvolume *zv;
	struct zdirectory *h;
	int i;
	TCHAR p[MAX_DPATH];
	TCHAR fn[MAX_DPATH];
	struct arcdir *ad;

	zv = zfile_fopen_archive_root (src, ZFD_ALL);
	if (zv == NULL) {
		geterror();
		_tprintf (_T("Couldn't open archive '%s'\n"), src);
		return 0;
	}
	h = zfile_opendir_archive (src);
	if (!h) {
		_tcscpy (p, src);
		_tcscat (p, _T(".DIR"));
		h = zfile_opendir_archive (src);
		if (!h) {
			geterror();
			_tprintf (_T("Couldn't open directory '%s'\n"), src);
			return 0;
		}
	}

	while (zfile_readdir_archive (h, fn)) {
		struct mystat st; 
		int isdir;
		int flags;
		TCHAR *comment;
		struct zfile *zf;
		uae_u32 crc32 = 0;
		int iscrc = 0;
		int nextdir = -1;

		_tcscpy (p, src);
		_tcscat (p, sep);
		_tcscat (p, fn);
		if (!zfile_stat_archive (p, &st)) {
			st.size = -1;
			st.mtime.tv_sec = st.mtime.tv_usec = 0;
		}
		isdir = 0;
		flags = 0;
		comment = 0;
		zfile_fill_file_attrs_archive (p, &isdir, &flags, &comment);
		flags ^= 15;
		if (!isdir) {
			if (0 && st.size >= 2 * 1024 * 1024) {
				iscrc = -1;
			} else if (st.size > 0) {
				zf = zfile_open_archive (p, 0);
				if (zf) {
					crc32 = zfile_crc32 (zf);
					iscrc = 1;
				}
			}
		}

		ad = &adp[entries++];
		ad->isdir = isdir;
		ad->comment = comment;
		ad->flags = flags;
		ad->name = my_strdup (fn);
		ad->size = st.size;
		ad->dt = st.mtime.tv_sec;
		ad->parent = parentid;
		ad->crc32 = crc32;
		ad->iscrc = iscrc;

		if (isdir && all) {
			int oldparent = parentid;
			parentid = ad - adp;
			nextdir = parentid + 1;
			unlist2 (adp, p, all);
			parentid = oldparent;
		}

		ad->nextlevel = nextdir;

		if (entries >= maxentries)
			break;
	}
	if (parentid >= 0)
		return 1;

	filelist = xmalloc (struct arcdir*, entries);
	for (i = 0; i < entries; i++) {
		filelist[i] = &adp[i];
	}

	// bubblesort is the winner!
	for (i = 0; i < entries; i++) {
		int j;
		for (j = i + 1; j < entries; j++) {
			int diff = _tcsicmp (filelist[i]->name, filelist[j]->name);
			if (diff > 0) {
				struct arcdir *tmp;
				tmp = filelist[i];
				filelist[i] = filelist[j];
				filelist[j] = tmp;
			}
		}
	}

	dolist (filelist, adp, entries, -1, 0);
	zfile_closedir_archive (h);
	zfile_fclose_archive (zv);
	return 1;
}

static int unlist (const TCHAR *src, int all)
{
	struct arcdir *adp;
	adp = xcalloc (struct arcdir, maxentries);
	unlist2 (adp, src, all);
	return 1;
}

static int docrclist (const TCHAR *src)
{
	WIN32_FIND_DATA ffd;
	HANDLE h;
	TCHAR path[MAX_DPATH];

	_tcscpy (path, src);
	_tcscat (path, _T("\\*.*"));
	h = FindFirstFile (path, &ffd);
	while (h) {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!_tcscmp (ffd.cFileName, _T(".")) || !_tcscmp (ffd.cFileName, _T("..")))
				goto next;
			_tcscpy (path, src);
			_tcscat (path, _T("\\"));
			_tcscat (path, ffd.cFileName);
			docrclist (path);
		} else {
			TCHAR path2[MAX_DPATH];
			_tcscpy (path, src);
			_tcscat (path, _T("\\"));
			_tcscat (path, ffd.cFileName);
			GetFullPathName (path, MAX_DPATH, path2, NULL);
			resetlist ();
			unlist (path2, 1);
		}
next:
		if (!FindNextFile (h, &ffd)) {
			FindClose (h);
			break;
		}
	}
	return 1;
}

static void setdate (const TCHAR *src, __time64_t tm)
{
	struct utimbuf ut;
	if (tm) {
		ut.actime = ut.modtime = tm;
		_wutime64 (src, &ut);
	}
}

static int found;

static int unpack (const TCHAR *src, const TCHAR *filename, const TCHAR *dst, int out, int all, int level)
{
	struct zdirectory *h;
	struct zvolume *zv;
	int ret;
	uae_u8 *b;
	int size;
	TCHAR fn[MAX_DPATH];

	ret = 0;
	zv = zfile_fopen_archive_root (src, ZFD_ALL);
	if (zv == NULL) {
		geterror();
		_tprintf (_T("Couldn't open archive '%s'\n"), src);
		return 0;
	}
	h = zfile_opendir_archive (src);
	if (!h) {
		geterror();
		_tprintf (_T("Couldn't open directory '%s'\n"), src);
		return 0;
	}
	while (zfile_readdir_archive (h, fn)) {
		if (all || !_tcsicmp (filename, fn)) {
			TCHAR tmp[MAX_DPATH];
			struct zfile *s, *d;
			struct mystat st;

			found = 1;
			_tcscpy (tmp, src);
			_tcscat (tmp, sep);
			_tcscat (tmp, fn);
			if (!zfile_stat_archive (tmp, &st)) {
				_tprintf (_T("Couldn't stat '%s'\n"), tmp);
				continue;
			}
			if (dst == NULL || all)
				dst = fn;
			if (st.mode) {
				if (all > 0)
					continue;
				if (all < 0) {
					TCHAR oldcur[MAX_DPATH];
					my_mkdir (fn);
					my_setcurrentdir (fn, oldcur);
					unpack (tmp, fn, dst, out, all, 1);
					my_setcurrentdir (oldcur, NULL);
					setdate (dst, st.mtime.tv_sec);
					continue;
				}
				_tprintf (_T("Directory extraction not yet supported\n"));
				return 0;
			}

			s = zfile_open_archive (tmp, ZFD_ARCHIVE | ZFD_NORECURSE);
			if (!s) {
				geterror();
				_tprintf (_T("Couldn't open '%s' for reading\n"), src);
				continue;
			}
			zfile_fseek (s, 0, SEEK_END);
			size = zfile_ftell (s);
			zfile_fseek (s, 0, SEEK_SET);
			b = xcalloc (uae_u8, size);
			if (b) {
				if (zfile_fread (b, size, 1, s) == 1) {
					if (out) {
						_tprintf (_T("\n"));
						fwrite (b, size, 1, stdout);
					} else {
						d = zfile_fopen (dst, _T("wb"), 0);
						if (d) {
							if (zfile_fwrite (b, size, 1, d) == 1) {
								ret = 1;
								_tprintf (_T("%s extracted, %d bytes\n"), dst, size);
							}
							zfile_fclose (d);
							setdate (dst, st.mtime.tv_sec);
						}
					}
				}
				xfree (b);
			}
			zfile_fclose (s);
			if (!all)
				break;
		}
	}
	geterror ();
	if (!found && !level) {
		if (filename[0])
			_tprintf (_T("'%s' not found\n"), filename);
		else
			_tprintf (_T("nothing extracted\n"));
	}
	return ret;
}

static int unpack2 (const TCHAR *src, const TCHAR *match, int level)
{
	struct zdirectory *h;
	struct zvolume *zv;
	int ret;
	uae_u8 *b;
	int size;
	TCHAR fn[MAX_DPATH];

	ret = 0;
	zv = zfile_fopen_archive_root (src, ZFD_ALL);
	if (zv == NULL) {
		geterror();
		_tprintf (_T("Couldn't open archive '%s'\n"), src);
		return 0;
	}
	h = zfile_opendir_archive (src);
	if (!h) {
		geterror();
		_tprintf (_T("Couldn't open directory '%s'\n"), src);
		return 0;
	}
	while (zfile_readdir_archive (h, fn)) {
		TCHAR tmp[MAX_DPATH];
		TCHAR *dst;
		struct zfile *s, *d;
		int isdir, flags;

		_tcscpy (tmp, src);
		_tcscat (tmp, sep);
		_tcscat (tmp, fn);
		zfile_fill_file_attrs_archive (tmp, &isdir, &flags, NULL);
		if (isdir) {
			TCHAR *p = _tcsstr (fn, _T(".DIR"));
			if (isdir == ZNODE_VDIR && p && _tcslen (p) == 4) {
				p[0] = 0;
				if (pattern_match (fn, match))
					continue;
				p[0] = '.';
			}
			unpack2 (tmp, match, 1);
			continue;
		}

		if (pattern_match (fn, match)) {
			struct mystat st;

			if (!zfile_stat_archive (tmp, &st)) {
				st.mtime.tv_sec = st.mtime.tv_usec = -1;
			}
			found = 1;
			dst = fn;
			s = zfile_open_archive (tmp, ZFD_NORECURSE);
			if (!s) {
				geterror();
				_tprintf (_T("Couldn't open '%s' for reading\n"), tmp);
				continue;
			}
			zfile_fseek (s, 0, SEEK_END); 
			size = zfile_ftell (s);
			zfile_fseek (s, 0, SEEK_SET);
			b = xcalloc (uae_u8, size);
			if (b) {
				if (zfile_fread (b, size, 1, s) == 1) {
					d = zfile_fopen (dst, _T("wb"), 0);
					if (d) {
						if (zfile_fwrite (b, size, 1, d) == 1) {
							ret = 1;
							_tprintf (_T("%s extracted, %d bytes\n"), dst, size);
						}
						zfile_fclose (d);
						setdate (dst, st.mtime.tv_sec);
					}
				}
				xfree (b);
			}
			zfile_fclose (s);
		}
	}
	geterror ();
	if (!found && !level) {
		_tprintf (_T("'%s' not matched\n"), match);
	}
	return ret;
}

static int scanpath (TCHAR *src, TCHAR *outpath)
{
	struct zvolume *zv;
	struct zdirectory *h;
	TCHAR fn[MAX_DPATH];

	zv = zfile_fopen_archive_root (src, ZFD_ALL | ZFD_NORECURSE);
	if (zv == NULL) {
		geterror();
		_tprintf (_T("Couldn't open archive '%s'\n"), src);
		return 0;
	}
	h = zfile_opendir_archive (src);
	if (!h) {
		geterror();
		_tprintf (_T("Couldn't open directory '%s'\n"), src);
		return 0;
	}
	while (zfile_readdir_archive (h, fn)) {
		TCHAR tmp[MAX_DPATH];
		int isdir, flags;
		_tcscpy (tmp, src);
		_tcscat (tmp, sep);
		_tcscat (tmp, fn);
		zfile_fill_file_attrs_archive (tmp, &isdir, &flags, NULL);
		if (isdir == ZNODE_VDIR) {
			_tcscpy (outpath, tmp);
			scanpath (tmp, outpath);
			break;
		}
	}
	return 1;
}

int __cdecl wmain (int argc, wchar_t *argv[], wchar_t *envp[])
{
	int ok = 0, i;
	int list = 0, xtract = 0, extract = 0;
	int out = 0, all = 0, crclist = 0;
	TCHAR path[MAX_DPATH] = { 0 }, pathx[MAX_DPATH] = { 0 };
#if 0
	TCHAR tmppath[MAX_DPATH];
#endif
	int used[32] = { 0 };
	TCHAR *parm2 = NULL;
	TCHAR *parm3 = NULL;
	TCHAR *match = NULL;

	resetlist ();

	for (i = 0; i < argc && i < 32; i++) {
		if (!_tcsicmp (argv[i], _T("-crclist"))) {
			crclist = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("o"))) {
			out = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("-o"))) {
			out = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("l"))) {
			list = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("-l"))) {
			list = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("x"))) {
			xtract = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("-x"))) {
			xtract = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("e"))) {
			extract = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("-e"))) {
			extract = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("*"))) {
			all = 1;
			used[i] = 1;
		}
		if (!_tcsicmp (argv[i], _T("**"))) {
			all = -1;
			used[i] = 1;
		}
		if (!used[i] && (_tcschr (argv[i], '*') || _tcschr (argv[i], '?'))) {
			extract = 1;
			match = argv[i];
			used[i] = 1;
		}
	}
	for (i = 1; i < argc && i < 32; i++) {
		if (!used[i]) {
			_tcscpy (pathx, argv[i]);
			GetFullPathName (argv[i], MAX_DPATH, path, NULL);
			used[i] = 1;
			break;
		}
	}
	for (i = 1; i < argc && i < 32; i++) {
		if (!used[i]) {
			parm2 = argv[i];
			used[i] = 1;
			break;
		}
	}
	for (i = 1; i < argc && i < 32; i++) {
		if (!used[i]) {
			parm3 = argv[i];
			used[i] = 1;
			break;
		}
	}

	//    _tcscpy (tmppath, path);
	//    scanpath (tmppath, path);

	if (crclist) {
		docrclist (_T("."));
		ok = 1;
	} else if (!list && match) {
		unpack2 (path, match, 0);
		ok = 1;
	} else if (!list && !parm2 && all > 0) {
		unpack2 (path, _T("*"), 0);
		ok = 1;
	} else if (!list && extract && parm2) {
		unpack2 (path, parm2, 0);
		ok = 1;
	} else if (argc == 2 || (argc > 2 && list)) {
		unlist (path, all);
		ok = 1;
	} else if (((xtract && parm2) || all || (argc >= 3 && parm2)) && !out) {
		unpack (path, parm2, parm3, 0, all, 0);
		ok = 1;
	} else if (parm2 && (argc >= 4 && out)) {
		unpack (path, parm2, parm3, 1, all, 0);
		ok = 1;
	}
	if (!ok) {
		_tprintf (_T("UAE unpacker uaeunp 0.8f by Toni Wilen (c)2012\n"));
		_tprintf (_T("\n"));
		_tprintf (_T("List: \"uaeunp (-l) <path>\"\n"));
		_tprintf (_T("List all recursively: \"uaeunp -l <path> **\"\n"));
		_tprintf (_T("Extract to file: \"uaeunp (-x) <path> <filename> [<dst name>]\"\n"));
		_tprintf (_T("Extract all (single directory): \"uaeunp (-x) <path> *\"\n"));
		_tprintf (_T("Extract all (recursively): \"uaeunp (-x) <path> **\"\n"));
		_tprintf (_T("Extract all (recursively, current dir): \"uaeunp -e <path> <match string>\"\n"));
		_tprintf (_T("Output to console: \"uaeunp (-x) -o <path> <filename>\"\n"));
		_tprintf (_T("\n"));
		_tprintf (_T("Supported disk image formats:\n"));
		_tprintf (_T(" ADF, HDF (VHD), DMS, encrypted DMS, IPF, FDI, DSQ, WRP\n"));
		_tprintf (_T("Supported filesystems:\n"));
		_tprintf (_T(" OFS, FFS, SFS, SFS2 and FAT12\n"));
		_tprintf (_T("Supported archive formats:\n"));
		_tprintf (_T(" 7ZIP, LHA, LZX, RAR (unrar.dll), TAR, ZIP, ArchiveAccess.DLL\n"));
		_tprintf (_T("Miscellaneous formats:\n"));
		_tprintf (_T(" RDB partition table, GZIP, XZ\n"));
	}
	return 0;
}

/*

0.8f:

- PCDOS raw MFM decoding improved (multiformat disks)

0.8e:

- DSQ extra header supported

0.8c:

- do not extract archives inside archive in recursive extraction mode

0.8b:

- DMS full cylinder 0 BBS ADs skipped when extracting
- DMS BBS AD extraction support

0.8:

- tar archive support
- some fixes and improvements

0.7:

- vhd read support
- dms, ipf (and possible other) disk image formats didn't unpack inside archives
- fixed duplicate vdir entries with some image types
- all raw disk image formats support Amiga, FAT and extended adf extraction
- cache relatively slow ipf and fdi extracted data

0.6:

- rdb handling optimization (no more huge memory usage)
- fat12 supported

0.5:

- adf protection flags fixed
- sfs support added
- >512 block sizes supported (rdb hardfiles only)

0.5b:

- SFS file extraction fixed
- SFS2 supported
- block size autodetection implemented (if non-rdb hardfile)

0.5c:

- rdb_dump.dat added to rdb hardfiles, can be used to dump/backup rdb blocks

*/
